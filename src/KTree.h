#ifndef KTREE_H
#define KTREE_H

#include "StdIncludes.h"

#include "Node.h"
#include "KMeans.h"
#include "NodeVisitor.h"

template <typename T>
struct SplitResult {
    bool isSplit;

    T *_key1;
    T *_key2;

    Node<T> *_child1;
    Node<T> *_child2;

    SplitResult() {
        isSplit = false;
    }

};

// KTree class
template <typename T, typename ClustererType, typename DistanceType, typename ProtoType>
class KTree {
private:

    // The order of this tree
    int _m;

    // The root of the tree.
    Node<T> *_root;

    ClustererType _clusterer;

    DistanceType _distF;
    ProtoType _protoF;

    // Use this for splits instead of creating
    // a new one every time a split is performed.
    SplitResult<T> _splitR;

    // Use these containers so as not to create new ones
    // every time we split
    vector<T*> tempKeys;
    vector<Node<T>*> tempChildren;
    vector<size_t> tempNearCentroids;

    vector<T*> removed;

    // Weights for prototype function (we don't have to use these)
    vector<int> weights;

    // How many vectors have been inserted into the tree.
    size_t _added;

    // Use delayed updates?
    bool _delayedUpdates;

    // Update along insertion path every _updateDelay insertions.
    int _updateDelay;

public:

    KTree(int order, int clustererMaxiters) {
        _m = order;
        _root = new Node<T>(); // initial root is a leaf
        _clusterer.setNumClusters(2);
        _clusterer.setMaxIters(clustererMaxiters);
        _clusterer.setEnforceNumClusters(true);
        _added = 0;
        _delayedUpdates = false;
        _updateDelay = 1000;
    }

    void setUpdateDelay(int updateDelay) {
        _updateDelay = updateDelay;
    }

    void setDelayedUpdates(bool delayedUpdates) {
        _delayedUpdates = delayedUpdates;
    }

    int getClusterCount() {
        return clusterCount(_root);
    }

    int getClusterCount(int depth) {
        return clusterCount(_root, depth);
    }

    int getEmptyClusterCount() {
        return emptyClusterCount(_root);
    }

    int getObjCount() {
        return objCount(_root);
    }

    int getLevelCount() {
        return levelCount(_root);
    }

    void printStats() {
        cout << "Number of objects: " << getObjCount() << endl;
        int levelCount = getLevelCount();
        cout << "Level count: " << getLevelCount() << endl;
        for (int level = 1; level < levelCount; ++level) {
                cout << "Cluster count level " << level << ": "
                        << getClusterCount(level) << endl;
        }
        int empty = getEmptyClusterCount();
        // Cluster counts no longer include empty clusters
        //cout << "Empty Cluster count: " << empty << endl;
        //cout << "Non-empty Cluster count: " << count - empty << endl;

        cout << "RMSE: " << getRMSE() << endl;
    }

    void EMStep() {

        rearrange();
        int pruned = 1;
        while (pruned > 0) {
            pruned = prune();
        }
        rebuildInternal();
    }

    void rearrange() {

        removeData(_root, removed);

        for (int i = 0; i < removed.size(); i++) {
            pushDownNoUpdate(_root, removed[i]);
        }

        removed.clear();
    }

    int prune() {
        return prune(_root);
    }

    void rebuildInternal() {
        // rebuild starting with above leaf level (bottom up)
        for (int depth = getLevelCount() - 1; depth >= 1; --depth) {
            rebuildInternal(_root, depth);
        }
    }

    void add(T *obj) {
        SplitResult<T> result = pushDown(_root, obj);
        if (result.isSplit) {
            _root = new Node<T>();
            _root->add(result._key1, result._child1);
            _root->add(result._key2, result._child2);
        }
        ++_added;
    }

    double getRMSE() {
        return RMSE();
    }

    void visit(NodeVisitor<Node<T> > &visitor) {
        visit(visitor, _root);
    }

    void visit(NodeVisitor<Node<T> > &visitor, int depth) {
        visit(visitor, _root, depth);
    }



private:

    double RMSE() {
        double RMSE = sumSquaredError(NULL, _root);
        int size = getObjCount();
        RMSE /= size;
        RMSE = sqrt(RMSE);
        return RMSE;
    }

    void visit(NodeVisitor<Node<T> > &visitor, Node<T> *node) {

        visitor.accept(node);

        if (!node->isLeaf()) {
            vector<Node<T>*> &children = node->getChildren();
            for (Node<T> *child : children) {
                visit(visitor, child);
            }
        }
    }

    void visit(NodeVisitor<Node<T> > &visitor, Node<T> *node, int depth) {

        if (depth == 1) visitor.accept(node);
        else if (!node->isLeaf()) {
            vector<Node<T>*> &children = node->getChildren();
            for (Node<T> *child : children) {
                visit(visitor, child, depth - 1);
            }
        }
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

    int clusterCount(Node<T>* current, int depth) {
        if (depth == 1) {
            int localCount = 0;
            for (Node<T>* child : current->getChildren()) {
                if (!child->isEmpty()) {
                    ++localCount;
                }
            }
            return localCount;
        } else {
            int localCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                localCount += clusterCount(child, depth - 1);
            }
            return localCount;
        }
    }


    int emptyClusterCount(Node<T>* current) {
        if (current->isLeaf()) {
            if (current->size() == 0) return 1;
            else return 0;
        } else {
            int localCount = 0;
            vector<Node<T>*>& children = current->getChildren();
            for (Node<T> *child : children) {
                localCount += emptyClusterCount(child);
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

    size_t nearestChild(T *obj, vector<T*> &others) {

        size_t nearest = 0;
        float dist;
        float nearestDistance = _distF(obj, others[0]);

        for (size_t i = 1; i < others.size(); ++i) {
            dist = _distF(obj, others[i]);
            if (dist < nearestDistance) {
                nearestDistance = dist;
                nearest = i;
            }
        }

        return nearest;
    }

    int prune(Node<T>* n) {
        if (n->isLeaf()) {
            return 0; // non-empty leaf node
        } else {
            int pruned = 0;
            vector<Node<T>*> &children = n->getChildren();
            for (int i = 0; i < children.size(); i++) {
                if (children[i]->isEmpty()) {
                    n->remove(i);
                    pruned++;
                } else {
                    pruned += prune(children[i]);
                }
            }
            n->finalizeRemovals();
            return pruned;
        }
    }

    void rebuildInternal(Node<T> *n, int depth) {

        if (n->isLeaf()) return;

        vector<Node<T>*> &children = n->getChildren();

        if (depth == 1) {
            vector<T*> &keys = n->getKeys();
            for (int i = 0; i < children.size(); ++i) {
                Node<T>* child = children[i];
                T* key = keys[i];
                updatePrototype(child, key);
            }
        } else {
            for (int i = 0; i < children.size(); ++i) {
                rebuildInternal(children[i], depth - 1);
            }
        }
    }

    void pushDownNoUpdate(Node<T> *n, T *vec) {

        //std::cout << "\n\tPushing down (no update) ...";

        if (n->isLeaf()) {
            n->add(vec); // Finished
        } else { // It is an internal node.
            // recurse via nearest neighbour cluster

            vector<T*>& keys = n->getKeys();
            vector<Node<T>*>& children = n->getChildren();

            size_t nearest = nearestChild(vec, keys);
            Node<T> *nearestChild = children[nearest];

            pushDownNoUpdate(nearestChild, vec);
        }
    }

    SplitResult<T> pushDown(Node<T> *n, T *vec) {

        //std::cout << "\n\tPushing down ...";

        SplitResult<T> result;

        if (n->isLeaf()) {
            if (n->size() >= _m) {
                // split this node and pass new node to parent to insert
                result = splitLeafNode(n, vec);
            } else {
                n->add(vec); // Finished
            }
        } else { // It is an internal node.
            // recurse via nearest neighbour cluster

            vector<T*>& keys = n->getKeys();
            vector<Node<T>*>& children = n->getChildren();

            size_t nearest = nearestChild(vec, keys);

            T *nearestKey = keys[nearest];
            Node<T> *nearestChild = children[nearest];

            result = pushDown(nearestChild, vec);

            if (result.isSplit) { // if there was a split

                updatePrototype(result._child1, result._key1);
                updatePrototype(result._child2, result._key2);

                // add new node
                if (n->size() >= _m) {
                    // split this node and pass new node to parent to insert
                    result = splitInternalNode(n, result._child2, result._key2);
                } else {
                    // insert new entry
                    n->add(result._key2, result._child2);
                    result.isSplit = false;
                }
            } else {
                if (!_delayedUpdates || (_delayedUpdates && _added % _updateDelay == 0)) {
                    updatePrototype(nearestChild, nearestKey);
                }
            }
        }

        return result;
    }


    // Perform a binary split of the child node

    SplitResult<T> splitInternalNode(Node<T>* parent, Node<T>* child, T* obj) {

        //cout << "\nSplitting internal node ...";

        SplitResult<T> result;

        // Create a 2nd node
        Node<T>* node2 = new Node<T>();

        // Copy child nodes into a temp storage vector

        tempKeys = parent->getKeys();
        tempKeys.push_back(obj);

        tempChildren = parent->getChildren();
        tempChildren.push_back(child);

        // DetachRemove children from child node
        parent->clearKeysAndChildren();

        // At this point we have 2 clear nodes: "parent" node (node 1) and "node2"

        // Cluster keys into 2 groups
        _clusterer.setNumClusters(2);
        //_clusterer.cluster(tempKeys);
        vector<Cluster<T>*>& clusters = _clusterer.cluster(tempKeys);
        //std::cout << "clusters found = " << clusters.size() << std::flush;

        // Get nearest centroids after clustering
        tempNearCentroids = _clusterer.getNearestCentroids();


        for (int i = 0; i < tempNearCentroids.size(); i++) {
            if (tempNearCentroids[i] == 0) parent->add(tempKeys[i], tempChildren[i]);
            else node2->add(tempKeys[i], tempChildren[i]);
        }

        // Now make our split result
        result.isSplit = true;
        result._child1 = parent;
        result._child2 = node2;
        result._key1 = clusters[0]->getCentroid();
        result._key2 = clusters[1]->getCentroid();

        return result;
    }


    // Perform a binary split of the child node

    SplitResult<T> splitLeafNode(Node<T>* child, T* obj) {

        //cout << "\nSplitting leaf node ...";

        SplitResult<T> result;

        // Create a 2nd node
        Node<T>* node2 = new Node<T>();

        // Copy child nodes into a temp storage vector

        tempKeys = child->getKeys();
        tempKeys.push_back(obj);

        // DetachRemove children from child node
        child->clearKeysAndChildren();

        // At this point we have 2 clear nodes: "child" node (node 1) and "node2"

        // Cluster keys into 2 groups
        _clusterer.setNumClusters(2);
        //_clusterer.cluster(tempKeys);
        vector<Cluster<T>*>& clusters = _clusterer.cluster(tempKeys);
        //std::cout << "clusters found = " << clusters.size() << std::flush;

        // Get nearest centroids after clustering
        tempNearCentroids = _clusterer.getNearestCentroids();

        for (int i = 0; i < tempNearCentroids.size(); i++) {
            if (tempNearCentroids[i] == 0) child->add(tempKeys[i]);
            else node2->add(tempKeys[i]);
        }

        // Now make our split result
        result.isSplit = true;
        result._child1 = child;
        result._child2 = node2;
        result._key1 = clusters[0]->getCentroid();
        result._key2 = clusters[1]->getCentroid();

        return result;
    }

    // Update the protype parentKey

    void updatePrototype(Node<T> *child, T* parentKey) {

        //cout << "\nUpdating mean ...";

        //int[] weights = new int[count];
        weights.clear();

        if (!child->isLeaf()) {
            vector<Node<T>*>& children = child->getChildren();

            for (size_t i = 0; i < children.size(); i++) {
                weights.push_back(objCount(children[i]));
            }
        }

        _protoF(parentKey, child->getKeys(), weights);
    }

    void removeData(Node<T> *n, vector<T*> &data) {

        if (n->isLeaf()) {
            n->removeData(data);
        } else {
            vector<Node<T>*>& children = n->getChildren();

            for (int i = 0; i < children.size(); i++) {
                removeData(children[i], data);
            }
        }
    }

};


#endif






