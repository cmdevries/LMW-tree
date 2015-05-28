#ifndef STREAMINGEMTREE_H
#define	STREAMINGEMTREE_H

#include "StdIncludes.h"
#include "SVectorStream.h"
#include "ClusterVisitor.h"
#include "InsertVisitor.h"
#include "tbb/mutex.h"
#include "tbb/pipeline.h"

namespace lmw {

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
 * ACCUMULATOR is the the type used for the accumulator vectors. For example,
 * with bit vectors, integer accumulators are used.
 *
 * ACCUMULATORs must support being constructed with the number of dimensions,
 * auto a = ACCUMULATOR(dimensions);
 * They must also support the add operation at a given dimension,
 * a[i] += 1;
 *
 * OPTIMIZER provides the functions necessary for optimization.
 */
template <typename T, typename ACCUMULATOR, typename OPTIMIZER>
class StreamingEMTree {
public:
    explicit StreamingEMTree(const Node<T>* root) :
        _root(new Node<AccumulatorKey>()) {
            _root->setOwnsKeys(true);
            deepCopy(root, _root);
    }

    ~StreamingEMTree() {
        delete _root;
    }

    size_t visit(SVectorStream<T>& vs, InsertVisitor<T>& visitor) {
        size_t totalRead = 0;

        // setup parallel processing pipeline
        tbb::parallel_pipeline(_maxtokens,
                // Input filter reads readsize chunks of vectors in serial
                tbb::make_filter<void, vector < SVector<bool>*>*>(
                tbb::filter::serial_out_of_order,
                inputFilter(vs, totalRead)
                ) &
                // Visit filter visits readsize chunks of vectors into streaming EM-tree in parallel
                tbb::make_filter < vector < SVector<bool>*>*, void>(
                tbb::filter::parallel,
                [&] (vector < SVector<bool>*>* data) -> void {
                    visit(*data, visitor);
                    vs.free(data);
                            delete data;
                }
        )
        );

        return totalRead;
    }

    void visit(ClusterVisitor<T>& visitor) const {
        visit(NULL, _root, visitor);
    }

    void visit(vector<T*>& data, InsertVisitor<T>& visitor) const {
        for (T* object : data) {
            visit(_root, object, visitor);
        }
    }

    size_t insert(SVectorStream<T>& vs) {
        return insert(vs, -1);
    }

    /** Returns the total number of vectors read from the stream.
     *  Returns 0 if the end of the stream has been reached.
     */
    size_t insert(SVectorStream<T>& vs, const size_t maxToRead) {
        size_t totalRead = 0;

        // setup parallel processing pipeline
        tbb::parallel_pipeline(_maxtokens,
                // Input filter reads readsize chunks of vectors in serial
                tbb::make_filter<void, vector < SVector<bool>*>*>(
                tbb::filter::serial_out_of_order,
                inputFilter(vs, totalRead, maxToRead)
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
    }

    void clearAccumulators() {
        clearAccumulators(_root);
    }

    void clearCountLastPassAndSSE() {
        clearCountLastPassAndSSE(_root);
    }

    int getMaxLevelCount() const {
        return maxLevelCount(_root);
    }

    int getClusterCount(int depth) const {
        return clusterCount(_root, depth);
    }

    uint64_t getObjCount() const {
        return objCount(_root);
    }

    uint64_t getObjCountLastPass() const {
        return objCountLastPass(_root);
    }

    double getRMSE() const {
        double RMSE = sumSquaredError(_root);
        uint64_t size = getObjCountLastPass();
        RMSE /= size;
        RMSE = sqrt(RMSE);
        return RMSE;
    }

private:
    typedef tbb::mutex Mutex;

    struct AccumulatorKey {
        AccumulatorKey() : key(NULL), sumSquaredError(0), accumulator(NULL),
                count(0),  countLastPass(0), mutex(NULL) { }

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
        uint64_t countLastPass; // how many vectors added in last pass, needed for RMSE
        Mutex* mutex;
    };

    struct Accessor {
        T* operator()(AccumulatorKey* accumulatorKey) const {
            return accumulatorKey->key;
        }
    };

    void visit(const T* parentKey, const Node<AccumulatorKey>* node,
            ClusterVisitor<T>& visitor, const int level = 1) const {
        for (size_t i = 0; i < node->size(); i++) {
            auto accumulatorKey = node->getKey(i);
            uint64_t count = objCountLastPass(node, i);
            double SSE = sumSquaredError(node, i);
            double RMSE = sqrt(SSE / count);
            visitor.accept(level, parentKey, accumulatorKey->key, RMSE, count);
            if (!node->isLeaf()) {
                visit(accumulatorKey->key, node->getChild(i), visitor, level + 1);
            }
        }
    }

    Nearest<AccumulatorKey> nearestKey(const T* object,
            const Node<AccumulatorKey>* node) const {
        return _optimizer.nearest(object, node->getKeys(), _accessor);
    }

    void visit(const Node<AccumulatorKey>* node, const T* object,
            InsertVisitor<T>& visitor, const int level = 1) const {
        auto nearest = nearestKey(object, node);
        auto accumulatorKey = nearest.key;
        visitor.accept(level, object, accumulatorKey->key, nearest.distance);
        if (node->isLeaf()) {
            // update stats but not accumulators
            Mutex::scoped_lock lock(*accumulatorKey->mutex);
            accumulatorKey->sumSquaredError +=
                    _optimizer.squaredDistance(object, accumulatorKey->key);
            accumulatorKey->count++;
            accumulatorKey->countLastPass++;
        } else {
            visit(node->getChild(nearest.index), object, visitor, level + 1);
        }
    }

    void insert(Node<AccumulatorKey>* node, T* object) {
        auto nearest = nearestKey(object, node);
        if (node->isLeaf()) {
            // update stats and accumulators
            auto accumulatorKey = nearest.key;
            Mutex::scoped_lock lock(*accumulatorKey->mutex);
            T* key = accumulatorKey->key;
            accumulatorKey->sumSquaredError += _optimizer.squaredDistance(object, key);
            ACCUMULATOR* accumulator = accumulatorKey->accumulator;
            for (size_t i = 0; i < accumulator->size(); i++) {
                (*accumulator)[i] += (*object)[i];
            }
            accumulatorKey->count++;
            accumulatorKey->countLastPass++;
        } else {
            insert(node->getChild(nearest.index), object);
        }
    }

    int prune(Node<AccumulatorKey>* node) {
        int pruned = 0;
        for (int i = 0; i < node->size(); i++) {
            if (objCount(node, i) == 0) {
                node->remove(i);
                pruned++;
            } else if (!node->isLeaf()) {
                pruned += prune(node->getChild(i));
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
        if (count == 0) return;

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
                accumulatorKey->countLastPass = 0;
            }
        } else {
            for (auto child : node->getChildren()) {
                clearAccumulators(child);
            }
        }
    }

    void clearCountLastPassAndSSE(Node<AccumulatorKey>* node) {
        if (node->isLeaf()) {
            for (auto accumulatorKey : node->getKeys()) {
                accumulatorKey->sumSquaredError = 0;
                accumulatorKey->countLastPass = 0;
            }
        } else {
            for (auto child : node->getChildren()) {
                clearCountLastPassAndSSE(child);
            }
        }
    }

    void deepCopy(const Node<T>* src, Node<AccumulatorKey>* dst) {
        if (!src->isEmpty()) {
            size_t dimensions = src->getKey(0)->size();
            for (size_t i = 0; i < src->size(); i++) {
                auto key = src->getKey(i);
                auto child = src->getChild(i);
                auto accumulatorKey = new AccumulatorKey();
                accumulatorKey->key = new T(*key);
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

    std::function<vector<SVector<bool>*>*(tbb::flow_control&)> inputFilter(
            SVectorStream<T>& vs, size_t& totalRead, const size_t maxToRead = -1) {
        return ([&vs, &totalRead, this, maxToRead]
                (tbb::flow_control & fc) -> vector < SVector<bool>*>* {
            if (maxToRead > 0 && totalRead >= maxToRead) {
                fc.stop();
                return NULL;
            }
            auto data = new vector<T*>;
            size_t read = vs.read(_readsize, data);
            if (read == 0) {
                delete data;
                fc.stop();
                return NULL;
            }
            totalRead += data->size();
            return data;
        });
    }

    double sumSquaredError(const Node<AccumulatorKey>* node, const size_t i) const {
        if (node->isLeaf()) {
            return node->getKey(i)->sumSquaredError;
        } else {
            return sumSquaredError(node->getChild(i));
        }
    }

    double sumSquaredError(const Node<AccumulatorKey>* node) const {
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

    /**
     * Object count for cluster i in node.
     */
    uint64_t objCount(const Node<AccumulatorKey>* node, const size_t i) const {
        if (node->isLeaf()) {
            return node->getKey(i)->count;
        } else {
            return objCount(node->getChild(i));
        }
    }

    uint64_t objCount(const Node<AccumulatorKey>* node) const {
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

    uint64_t objCountLastPass(const Node<AccumulatorKey>* node, const size_t i) const {
        if (node->isLeaf()) {
            return node->getKey(i)->countLastPass;
        } else {
            return objCountLastPass(node->getChild(i));
        }
    }

    uint64_t objCountLastPass(const Node<AccumulatorKey>* node) const {
        if (node->isLeaf()) {
            uint64_t localCount = 0;
            for (auto key : node->getKeys()) {
                localCount += key->countLastPass;
            }
            return localCount;
        } else {
            uint64_t localCount = 0;
            for (auto child : node->getChildren()) {
                localCount += objCountLastPass(child);
            }
            return localCount;
        }
    }

    int maxLevelCount(const Node<AccumulatorKey>* current) const {
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

    int clusterCount(const Node<AccumulatorKey>* current, const int depth) const {
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
    OPTIMIZER _optimizer;
    Accessor _accessor;

    // How mamny vectors to read at once when processing a stream.
    int _readsize = 1000;

    // The maximum number of readsize vector chunks that can be loaded at once.
    int _maxtokens = 1024;
};

} // namespace lmw

#endif	/* STREAMINGEMTREE_H */
