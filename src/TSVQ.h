#ifndef TSVQ_H
#define	TSVQ_H

#include "StdIncludes.h"

#include "Node.h"

#include "tbb/task.h"

template <typename T, typename ClustererType, typename DistanceType, typename ProtoType>
class TSVQ {
public:

    TSVQ(int order, int depth, int maxiters) : _m(order), _depth(depth),
    _root(new Node<T>()), _maxIters(maxiters) {
        _clusterer.setNumClusters(_m);
        _clusterer.setMaxIters(_maxIters);
    }
    
    ~TSVQ() {
        delete _root;
    }
    
    Node<T>* getMWayTree() {
        return _root;
    }

    int getClusterCount() {
        return clusterCount(_root);
    }

    uint64_t getObjCount() {
        return objCount(_root);
    }

    int getLevelCount() {
        return levelCount(_root);
    }

    int getMaxLevelCount() {
        return maxLevelCount(_root);
    }

    int getMinLevelCount() {
        return minLevelCount(_root);
    }
    
    void printStats() {
        std::cout << "\nNumber of objects: " << getObjCount();
        std::cout << "\nCluster count: " << getClusterCount();
        std::cout << "\nLevel count (node 0): " << getLevelCount();
        std::cout << "\nMax depth: " << getMaxLevelCount();
        std::cout << "\nRMSE: " << getRMSE();
    }

    void cluster(vector<T*> &data) {
        // make the root a leaf containing all data
        _root->addAll(data);
        
        // spawn parallel tasks for recursion when building the tree
        TSVQTask *t = new(tbb::task::allocate_root()) TSVQTask(_root, _m, _depth, _maxIters);
        tbb::task::spawn_root_and_wait(*t);
    }

    double getRMSE() {
        return RMSE();
    }

private:
    
    class TSVQTask : public tbb::task {
    public:

        TSVQTask(Node<T>* current, int order, int depth, int maxiters) : _current(current), _m(order),
        _treeDepth(depth),
        _maxIters(maxiters) {
        }

        ~TSVQTask() {
        }

        void cluster() {
            // split using clustering algorithm
            ClustererType* clusterer = new ClustererType();
            clusterer->setNumClusters(_m);
            clusterer->setMaxIters(_maxIters);
            vector<Cluster<T>*> clusters = clusterer->cluster(_current->getKeys());            
            
            // assign clusters to tree
            _current->clearKeysAndChildren();
            for (Cluster<T>* c : clusters) {
                Node<T>* child = new Node<T>();
                child->addAll(c->getNearestList());
                _current->add(c->getCentroid(), child);
            }
            _current->setOwnsKeys(true);
            delete clusterer;
        }

        /**
         * Separate tasks are created for each child node in the tree. Simply
         * parallelizing k-means does not enable full usage of CPUs on a 16 CPU
         * system. By allocating tasks, multiple copies of parallel k-means can
         * run at once.
         */
        void createChildTasks() {
            vector<TSVQTask*> childTasks;
            // Create TBB tasks
            for (Node<T>* n : _current->getChildren()) {
                TSVQTask *t = new(allocate_child()) TSVQTask(n, _m, _treeDepth - 1, _maxIters);
                childTasks.push_back(t);
            }
            // Set ref count to number of tasks + 1
            set_ref_count(childTasks.size() + 1);
            // Spawn tasks, except 1st task
            for (size_t i = 1; i < childTasks.size(); i++) {
                tbb::task::spawn(*childTasks[i]);
            }
            // Start 1st task running and wait for all
            tbb::task::spawn_and_wait_for_all(*childTasks[0]);
        }

        tbb::task* execute() {
            if (_treeDepth == 1) {
                return NULL;
            } else {
                cluster();
                createChildTasks();
            }
            return NULL;
        }

    private:
        // Num clusters
        int _m;

        // The current tree depth
        int _treeDepth;

        // The maximum number of iterations
        int _maxIters;

        // The root of the tree.
        Node<T> *_current;

        DistanceType _distF;
        ProtoType _protoF;
    };

    double RMSE() {
        double RMSE = sumSquaredError(NULL, _root);
        uint64_t size = getObjCount();
        RMSE /= size;
        RMSE = sqrt(RMSE);
        return RMSE;
    }

    double sumSquaredError(T* parentKey, Node<T> *child) {

        double distance = 0.0;
        double dis;

        if (child->isLeaf()) {
            vector<T*> &keys = child->getKeys();
            for (T* key : keys) {
                dis = _distF(key, parentKey);
                distance += dis * dis;
            }
        } else {
            int numEntries = child->size();

            vector<T*> &keys = child->getKeys();
            vector<Node<T>*> &children = child->getChildren();

            for (int i = 0; i < numEntries; i++) {
                distance += sumSquaredError(keys[i], children[i]);
            }
        }

        return distance;
    }

    uint64_t objCount(Node<T>* current) {
        if (current->isLeaf()) {
            return current->size();
        } else {
            uint64_t localCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                localCount += objCount(child);

            }
            return localCount;
        }
    }

    int clusterCount(Node<T>* current) {
        if (current->isLeaf()) {
            if (current->isEmpty()) {
                return 0;
            } else {
                return 1;
            }
        } else {
            int localCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                localCount += clusterCount(child);
            }
            return localCount;
        }
    }

    int levelCount(Node<T>* current) {
        if (current->isLeaf()) {
            return 1;
        } else {
            return levelCount(current->getChild(0)) + 1;
        }
    }

    int maxLevelCount(Node<T>* current) {
        if (current->isLeaf()) {
            return 1;
        }
        else {
            int count = 0;
            int maxCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                count = maxLevelCount(child);
                if (count > maxCount) maxCount = count;
            }
            return maxCount + 1;
        }
    }
    
    int minLevelCount(Node<T>* current) {
        if (current->isLeaf()) {
            return 1;
        }
        else {
            int count = 0;
            int minCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                count = maxLevelCount(child);
                if (count < minCount) minCount = count;
            }
            return minCount + 1;
        }
    }

private:
    // The order of this tree
    int _m;

    // The order of this tree
    int _depth;

    // The root of the tree.
    Node<T> *_root;

	// The maximum number of iterations
	int _maxIters;

    ClustererType _clusterer;
    DistanceType _distF;
    ProtoType _protoF;
};

#endif	/* TSVQ_H */