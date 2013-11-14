#ifndef TSVQ_H
#define	TSVQ_H

#include "StdIncludes.h"

#include "Node.h"
#include "threadpool.hpp"
#include "threadpool/size_policies.hpp"

using namespace boost::threadpool;

template <typename T, typename ClustererType, typename DistanceType, typename ProtoType>
class TSVQ {
private:
    // The order of this tree
    int _m;

    // The order of this tree
    int _depth;

    // The root of the tree.
    Node<T> *_root;

    ClustererType _clusterer;
    DistanceType _distF;
    ProtoType _protoF;

public:
    TSVQ(int order, int depth, int maxiters) : _m(order), _depth(depth),
            _root(new Node<T>()) {
        _clusterer.setNumClusters(_m);
        _clusterer.setMaxIters(maxiters);
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

    int getObjCount() {
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
        cluster(_root, _depth);
    }

    void cluster(Node<T>* current, int depth) {
        if (depth == 1) {
            return;
        } else {
            vector<Cluster<T>*> clusters = _clusterer.cluster(current->getKeys());
            current->clearKeysAndChildren();
            for (Cluster<T>* c : clusters) {
                Node<T>* child = new Node<T>();
                child->addAll(c->getNearestList());
                current->add(c->getCentroid(), child);
            }
            for (Node<T>* n : current->getChildren()) {
                cluster(n, depth - 1);
            }
        }
    }

    double getRMSE() {
        return RMSE();
    }


private:
    double RMSE() {
        double RMSE = sumSquaredError(NULL, _root);
        int size = getObjCount();
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

    int objCount(Node<T>* current) {

        if (current->isLeaf()) {
            return current->size();
        } else {
            int localCount = 0;
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


};


#endif	/* TSVQ_H */

