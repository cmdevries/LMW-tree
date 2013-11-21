#ifndef EM_TREE_H
#define EM_TREE_H

#include "StdIncludes.h"

#include "Node.h"


template <typename T, typename CLUSTERER, typename DISTANCE, typename PROTOTYPE>
class EMTree {
public:

    EMTree(int order) : _m(order), _root(new Node<T>()) {
    }
    
    EMTree(Node<T>* root) : _m(-1), _root(root) {
    }    
    
    ~EMTree() {
        delete _root;
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

    void printStats() {
        std::cout << "\nNumber of objects: " << getObjCount();
        std::cout << "\nCluster count: " << getClusterCount();
        std::cout << "\nLevel count (node 0): " << getLevelCount();
        std::cout << "\nMax depth: " << getMaxLevelCount();
        std::cout << "\nRMSE: " << getRMSE();
    }

    void seed(vector<T*> &data, int depth) {
        deque<int> splits;
        for (int i = 0; i < depth; i++) {
            splits.push_back(_m);
        }
        seed(data, splits);
    }

    void seed(vector<T*> &data, deque<int> splits, bool updateMeans = true) {
        CLUSTERER clusterer(_m);
        _root->addAll(data);
        if (updateMeans) {
            clusterer.setMaxIters(1);
        } else {
            clusterer.setMaxIters(0);
        }
        seed(_root, splits, clusterer);
    }
    
    void seed(Node<T>* current, deque<int> splits, CLUSTERER& clusterer) {
        if (splits.empty()) {
            return;
        } else {
            clusterer.setNumClusters(splits[0]);
            vector<Cluster<T>*> clusters = clusterer.cluster(current->getKeys());
            current->clearKeysAndChildren();
            for (Cluster<T>* c : clusters) {
                Node<T>* child = new Node<T>();
                child->addAll(c->getNearestList());
                current->add(c->getCentroid(), child);
            }
            current->setOwnsKeys(true);
            splits.pop_front();
            for (Node<T>* n : current->getChildren()) {
                seed(n, splits, clusterer);
            }
        }   
    }

    void EMStep() {

        {
            boost::timer::auto_cpu_timer t("insert %w secs\n");
            rearrange();
        }
        {
            boost::timer::auto_cpu_timer t("prune %w secs\n");
            int pruned = 1;
            while (pruned > 0) {
                pruned = prune();
                //std::cout << "pruned " << pruned << " nodes" << std::endl;
            }
        }
        {
            boost::timer::auto_cpu_timer t("update %w secs\n");
            rebuildInternal();
        }
    }
    
    // perform EM-step replacing data in the tree
    void EMStep(vector<T*> &data) {

        {
            //boost::timer::auto_cpu_timer t("insert %w secs\n");
            replace(data);
        }
        {
            //boost::timer::auto_cpu_timer t("prune %w secs\n");
            int pruned = 1;
            while (pruned > 0) {
                pruned = prune();
                //std::cout << "pruned " << pruned << " nodes" << std::endl;
            }
        }
        {
            //boost::timer::auto_cpu_timer t("update %w secs\n");
            rebuildInternal();
        }
    }    

    void replace(vector<T*> &data) {
        removeData(_root, removed);
        removed.clear();
        for (T* vector : data) {
            pushDownNoUpdate(_root, vector);
        }
    }

    
    void rearrange() {

        removeData(_root, removed);

        for (int i = 0; i < removed.size(); i++) {
            pushDownNoUpdate(_root, removed[i]);
        }

        removed.clear();
    }

    void rearrangeInternal() {
        for (int depth = 2; depth < getMaxLevelCount(); ++depth) {
            removeDataInternal(_root, removed, removedChildren, depth);
            for (int i = 0; i < removed.size(); i++) {
                pushDownNoUpdateInternal(_root, removed[i], removedChildren[i], depth);
            }
            prune();
            removed.clear();
            removedChildren.clear();
        }
    }    

    int prune() {
        return prune(_root);
    }

    void rebuildInternal() {
        // rebuild starting with above leaf level (bottom up)
        // we are rebuilding means in internal nodes only, this is why we start 
        // with the above leaf level
        for (int depth = getLevelCount() - 1; depth >= 1; --depth) {
            rebuildInternal(_root, depth);
        }
    }

    double getRMSE() {
        return RMSE();
    }


private:

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
        if (n->isLeaf()) {
            return;
        }
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
    
    void pushDownNoUpdateInternal(Node<T> *n, T* key, Node<T>* child, int depth) {

        //std::cout << "\n\tPushing down (no update) ...";

        if (depth == 1) {
            n->add(key, child); // Finished
        } else { // It is an internal node.
            // recurse via nearest neighbour cluster

            vector<T*>& keys = n->getKeys();
            vector<Node<T>*>& children = n->getChildren();

            size_t nearest = nearestChild(key, keys);
            Node<T> *nearestChild = children[nearest];

            pushDownNoUpdateInternal(nearestChild, key, child, depth - 1);
        }
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
    
    void removeDataInternal(Node<T>* n, vector<T*>& keys, vector<Node<T>*>& children, int depth) {
        if (depth == 1) {
            n->removeData(keys, children);
        } else {
            for (Node<T>* child : n->getChildren()) {
                removeDataInternal(child, keys, children, depth - 1);
            }
        }
    }
    
    // The order of this tree
    int _m;

    // The root of the tree.
    Node<T> *_root;

    DISTANCE _distF;
    PROTOTYPE _protoF;

    vector<T*> removed;
    vector<Node<T>*> removedChildren;

    // Weights for prototype function (we don't have to use these)
    vector<int> weights;    
};


#endif

