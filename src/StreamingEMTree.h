#ifndef STREAMINGEMTREE_H
#define	STREAMINGEMTREE_H

#include "StdIncludes.h"
#include "SVectorStream.h"
#include "tbb/mutex.h"
#include "tbb/pipeline.h"

/**
 * Visits all the clusters in a Streaming EM-tree along the insertion path.
 */
template <typename T>
class InsertVisitor {
public:
    InsertVisitor() { }
    virtual ~InsertVisitor() { }
    
    /**
     * Must be thread safe. It can be called from multiple threads.
     */
    virtual void accept(T* clusterCenter, int level, double RMSE,
            uint64_t objectCount) = 0;
};

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
            _root->setOwnsKeys(true);
            deepCopy(root, _root);
    }
        
    ~StreamingEMTree() {
        delete _root;
    }

    size_t insert(SVectorStream<T>& vs) {
        const int readsize = 1000;
        const int maxtokens = 1024; // maximum number of readsize vector chunks that can be loaded at once
        size_t totalRead = 0;
        
        // setup parallel processing pipeline
        tbb::parallel_pipeline(maxtokens,
                // Input filter reads readsize chunks of vectors in serial
                tbb::make_filter<void, vector < SVector<bool>*>*>(
                tbb::filter::serial_out_of_order,
                [&] (tbb::flow_control & fc) -> vector < SVector<bool>*>* {
                    auto data = new vector<T*>;
                    size_t read = vs.read(readsize, data);
                    if (read == 0) {
                        delete data;
                        fc.stop();
                        return NULL;
                    }
                    totalRead += data->size();
                    return data;
                }
        ) &
        // Insert filter inserts readsize chunks of vectors into streaming EM-tree in parallel
        tbb::make_filter < vector < SVector<bool>*>*, void>(
                tbb::filter::parallel,
                [&] (vector < SVector<bool>*>* data) -> void {
                    insert(*data);
                    vs.free(data);
                            delete data;
                }
        )
        );
        
        return totalRead;
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
        update(_root);
        clearAccumulators(_root);
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
        uint64_t size = getObjCount();
        RMSE /= size;
        RMSE = sqrt(RMSE);
        return RMSE;
    }

private:
    typedef tbb::mutex Mutex;
    
    struct AccumulatorKey {
        AccumulatorKey() : key(NULL), sumSquaredError(0), accumulator(NULL),
                count(0), mutex(NULL) { }
        
        ~AccumulatorKey() {
            if (key) {
                delete key;
            }
            if (accumulator) {
                delete accumulator;
            }
            if (mutex) {
                delete mutex;
            }
        }
                
        T* key;
        double sumSquaredError;
        ACCUMULATOR* accumulator; // accumulator for partially updated key
        uint64_t count; // how many vectors have been added to accumulator
        Mutex* mutex;
    };
   
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
    
    void gatherAccumulators(Node<AccumulatorKey>* node, ACCUMULATOR* total,
            uint64_t* totalCount) {
        if (node->isLeaf()) {
            for (auto accumulatorKey : node->getKeys()) {
                auto accumulator = accumulatorKey->accumulator;
                for (size_t i = 0; i < accumulator->size(); i++) {
                    (*total)[i] += (*accumulator)[i];
                }
                *totalCount += accumulatorKey->count;
            }
        } else {
            for (auto child : node->getChildren()) {
                gatherAccumulators(child, total, totalCount);
            }
        }
    }
    
    /**
     * TODO(cdevries): Make it work for something other than bitvectors. It needs
     * to be parameterized, for example, with float vectors, a mean is taken.
     */
    static void updatePrototypeFromAccumulator(T* key, ACCUMULATOR* accumulator,
            uint64_t count) {
        // calculate new key based on accumulator
        key->setAllBlocks(0);
        for (size_t i = 0; i < key->size(); i++) {
            if ((*accumulator)[i] > (count / 2)) {
                key->set(i);
            }
        }
    }
    
    void update(Node<AccumulatorKey>* node) {
        if (node->isLeaf()) {
            // leaves flatten accumulators in node
            for (auto accumulatorKey : node->getKeys()) {
                updatePrototypeFromAccumulator(accumulatorKey->key, 
                        accumulatorKey->accumulator, accumulatorKey->count);
            }
        } else {
            // internal nodes must gather accumulators from leaves
            size_t dimensions = node->getKey(0)->key->size();
            for (size_t i = 0; i < node->size(); i++) {
                auto accumulatorKey = node->getKey(i);
                T* key = accumulatorKey->key;
                auto child = node->getChild(i);
                ACCUMULATOR total(dimensions);
                total.setAll(0);
                uint64_t totalCount = 0;
                gatherAccumulators(child, &total, &totalCount);
                updatePrototypeFromAccumulator(key, &total, totalCount);
            }
            for (auto child : node->getChildren()) {
                update(child);
            }
        }
    }

    void clearAccumulators(Node<AccumulatorKey>* node) {
        if (node->isLeaf()) {
            for (auto accumulatorKey : node->getKeys()) {
                accumulatorKey->sumSquaredError = 0;
                accumulatorKey->accumulator->setAll(0);
                accumulatorKey->count = 0;
            }
        } else {
            for (auto child : node->getChildren()) {
                clearAccumulators(child);
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
                if (child->isLeaf()) {
                    // Do not copy leaves of original tree and setup
                    // accumulators for the lowest level cluster means.
                    accumulatorKey->accumulator = new ACCUMULATOR(dimensions);
                    accumulatorKey->accumulator->setAll(0);
                    accumulatorKey->mutex = new Mutex();
                    dst->add(accumulatorKey);
                } else {
                    auto newChild = new Node<AccumulatorKey>();
                    newChild->setOwnsKeys(true);
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

    Node<AccumulatorKey>* _root;
    DISTANCE _dist;
    PROTOTYPE _prototype;  
};
 
#endif	/* STREAMINGEMTREE_H */

