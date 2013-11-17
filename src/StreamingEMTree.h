#ifndef STREAMINGEMTREE_H
#define	STREAMINGEMTREE_H

#include "tbb/mutex.h"

/**
 * The streaming version of the EM-tree algorithm does not store the
 * data vectors in the tree. Therefore, the leaf level in the tree contain
 * cluster representatives.
 * 
 * It has accumulator vectors for centroids at the leaf level. The accumulators
 * are used to calculate a mean in the streaming setting. Note that accumulators
 * are only needed in the leaf level as means at all higher levels in the tree
 * can be calculated from the leaves.
 * 
 * T is the type of vector stored in the node.
 * 
 * DISTANCE is the distance function to use when comparing pairs of T.
 * 
 * PROTOTYPE is the function that summarizes a list of T to a single T.
 * 
 * ACCUMULATOR is the the type used for the accumulator vectors. For example,
 * with bit vectors, integer accumulators are used.
 * 
 * ACCUMULATORs must support being constructed with the number of dimensions,
 * auto a = ACCUMULATOR(dimensions);
 * They must also support the add operation at a given dimension,
 * a[i] += 1;
 */
template <typename T, typename DISTANCE, typename PROTOTYPE, typename ACCUMULATOR>
class StreamingEMTree {
public:
    explicit StreamingEMTree(Node<T>* root) :
        _root(new Node<AccumulatorKey>()) {
            deepCopy(root, _root);
    }
        
    ~StreamingEMTree() {
        delete _root;
    }
    
    /**
     * Insert is thread safe. Shared accumulators are locked.
     */
    void insert(vector<T*>& data) {
        for (T* object : data) {
            insert(_root, object);
        }
    }
    
    int prune() {
        return prune(_root);
    }
    
    void update() {
        for (int depth = getMaxLevelCount(); depth >= 1; --depth) {
            update(_root, depth);
        }        
    }
    
    int getMaxLevelCount() {
        return maxLevelCount(_root);
    }
    
    int getClusterCount(int depth) {
        return clusterCount(_root, depth);
    }
    
    uint64_t getObjCount() {
        return objCount(_root);
    }
    
    double getRMSE() {
        double RMSE = sumSquaredError(_root);
        int size = getObjCount();
        RMSE /= size;
        RMSE = sqrt(RMSE);
        return RMSE;
    }

private:
    typedef tbb::mutex Mutex;
    
    struct AccumulatorKey {
        T* key;
        double sumSquaredError;
        ACCUMULATOR* accumulator; // accumulator for partially updated key
        uint64_t count; // how many vectors have been added to accumulator
        Mutex* mutex;
    };
    
    Node<AccumulatorKey>* _root;
    DISTANCE _dist;
    PROTOTYPE _prototype;    
    
    size_t nearest(Node<AccumulatorKey>* node, T* object, float* nearestDistance) {
        size_t nearest = 0;
        *nearestDistance = _dist(object, node->getKey(0)->key);
        for (size_t i = 1; i < node->size(); i++) {
            float dist = _dist(object, node->getKey(i)->key);
            if (dist < *nearestDistance) {
                *nearestDistance = dist;
                nearest = i;
            }
        }
        return nearest;
    }   
    
    void insert(Node<AccumulatorKey>* node, T* object) {
        float nearestDistance = 0;
        size_t nearestIndex = nearest(node, object, &nearestDistance);
        if (node->isLeaf()) {
            auto accumulatorKey = node->getKey(nearestIndex);
            accumulatorKey->mutex->lock();
            T* key = accumulatorKey->key;
            accumulatorKey->sumSquaredError += nearestDistance * nearestDistance;
            ACCUMULATOR* accumulator = accumulatorKey->accumulator;
            for (size_t i = 0; i < accumulator->size(); i++) {
                (*accumulator)[i] += (*object)[i];
            }
            accumulatorKey->count++;
            accumulatorKey->mutex->unlock();
        } else {
            insert(node->getChild(nearestIndex), object);
        }
    }

    int prune(Node<AccumulatorKey>* node) {
        int pruned = 0;
        vector<Node<AccumulatorKey>*>& children = node->getChildren();
        for (int i = 0; i < children.size(); i++) {
            if (objCount(children[i]) == 0) {
                node->remove(i);
                pruned++;
            } else {
                pruned += prune(children[i]);
            }
        }
        node->finalizeRemovals();
        return pruned;
    }
    
    /**
     * TODO(cdevries): Make it work for something other than bitvectors. It needs
     * to be parameterized, for example, with float vectors, a mean is taken.
     */
    void updatePrototypeFromAccumulators(AccumulatorKey* accumulatorKey) {
        // calculate new key based on accumulator
        T* key = accumulatorKey->key;
        ACCUMULATOR* accumulator = accumulatorKey->accumulator;
        int halfCount = accumulatorKey->count / 2;
        key->setAllBlocks(0);
        for (size_t i = 0; i < key->size(); i++) {
            if ((*accumulator)[i] > halfCount) {
                key->set(i);
            }
        }

        // reset accumulators for next insert
        accumulatorKey->sumSquaredError = 0;
        accumulatorKey->accumulator->setAll(0);
        accumulatorKey->count = 0;
    }
    
    void updatePrototype(Node<AccumulatorKey>* childNode, T* parentKey) {
        vector<int> weights;
        if (!childNode->isLeaf()) {
            for (auto child : childNode->getChildren()) {
                weights.push_back(objCount(child));
            }
        }
        vector<T*> childKeys;
        for (auto accumulatorKey : childNode->getKeys()) {
            childKeys.push_back(accumulatorKey->key);
        }
        _prototype(parentKey, childKeys, weights);
    }    
    
    void update(Node<AccumulatorKey>* node, int depth) {
        if (depth == 1) {
            if (node->isLeaf()) {
                for (auto accumulatorKey : node->getKeys()) {
                    updatePrototypeFromAccumulators(accumulatorKey);
                }
            } else {
                auto children = node->getChildren();
                auto keys = node->getKeys();
                for (int i = 0; i < children.size(); i++) {
                    updatePrototype(children[i], keys[i]->key);
                }
            }
        } else {
            for (auto child : node->getChildren()) {
                update(child, depth - 1);
            }
        }
    }
        
    void deepCopy(Node<T>* src, Node<AccumulatorKey>* dst) {
        if (!src->isEmpty()) {
            size_t dimensions = src->getKey(0)->size();
            for (size_t i = 0; i < src->size(); i++) {
                auto key = src->getKey(i);
                auto child = src->getChild(i);
                auto accumulatorKey = new AccumulatorKey();
                accumulatorKey->key = new T(key);
                accumulatorKey->sumSquaredError = 0;
                accumulatorKey->accumulator = NULL;
                accumulatorKey->count = 0;
                accumulatorKey->mutex = NULL;
                if (child->isLeaf()) {
                    // Do not copy leaves of original tree and setup
                    // accumulators for the lowest level cluster means.
                    accumulatorKey->accumulator = new ACCUMULATOR(dimensions);
                    accumulatorKey->accumulator->setAll(0);
                    accumulatorKey->mutex = new Mutex();
                    dst->add(accumulatorKey);
                } else {
                    auto newChild = new Node<AccumulatorKey>();
                    deepCopy(child, newChild);
                    dst->add(accumulatorKey, newChild);
                }
            }
        }
    }
    
    double sumSquaredError(Node<AccumulatorKey>* node) {
        if (node->isLeaf()) {
            double localSum = 0;
            for (auto key : node->getKeys()) {
                localSum += key->sumSquaredError;
            }
            return localSum;
        } else {
            double localSum = 0;
            for (auto child : node->getChildren()) {
                localSum += sumSquaredError(child);
            }
            return localSum;
        }        
    }
    
    uint64_t objCount(Node<AccumulatorKey>* node) {
        if (node->isLeaf()) {
            uint64_t localCount = 0;
            for (auto key : node->getKeys()) {
                localCount += key->count;
            }
            return localCount;
        } else {
            uint64_t localCount = 0;
            for (auto child : node->getChildren()) {
                localCount += objCount(child);
            }
            return localCount;
        }
    }    

    int maxLevelCount(Node<AccumulatorKey>* current) {
        if (current->isLeaf()) {
            return 1;
        } else {
            int maxCount = 0;
            for (auto child : current->getChildren()) {
                maxCount = max(maxCount, maxLevelCount(child));
            }
            return maxCount + 1;
        }
    }
    
    int clusterCount(Node<AccumulatorKey>* current, int depth) {
        if (depth == 1) {
            return current->size();
        } else {
            int localCount = 0;
            for (auto child : current->getChildren()) {
                localCount += clusterCount(child, depth - 1);
            }
            return localCount;
        }
    }    
};
 
#endif	/* STREAMINGEMTREE_H */

