#ifndef STREAMINGEMTREE_H
#define	STREAMINGEMTREE_H

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
    
    void insert(vector<T*> data) {
    }
    
    void prune() {
    }
    
    void update() {
    }
    
    int getMaxLevelCount() {
        return maxLevelCount(_root);
    }
    
    int getClusterCount(int depth) {
        return clusterCount(_root, depth);
    }
    
private:
    struct AccumulatorKey {
        T* key;
        ACCUMULATOR* accumulator; // accumulator for partially updated key
        uint32_t count; // how many vectors have been added to accumulator
    };
    
    void deepCopy(Node<T>* src, Node<AccumulatorKey>* dst) {
        if (!src->isEmpty()) {
            size_t dimensions = src->getKey(0)->size();
            for (size_t i = 0; i < src->size(); i++) {
                auto key = src->getKey(i);
                auto child = src->getChild(i);
                auto accumulatorKey = new AccumulatorKey();
                accumulatorKey->key = new T(key);
                accumulatorKey->accumulator = NULL;
                accumulatorKey->count = 0;
                if (child->isLeaf()) {
                    // Do not copy leaves of original tree and setup
                    // accumulators for the lowest level cluster means.
                    accumulatorKey->accumulator = new ACCUMULATOR(dimensions);
                    dst->add(accumulatorKey);
                } else {
                    auto newChild = new Node<AccumulatorKey>();
                    deepCopy(child, newChild);
                    dst->add(accumulatorKey, newChild);
                }
            }
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

