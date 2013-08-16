#ifndef EM_TREE_H
#define EM_TREE_H

#include "StdIncludes.h"

#include "Node.h"
#include "threadpool.hpp"
#include "threadpool/size_policies.hpp"

using namespace boost::threadpool;




template <typename T, typename ClustererType, typename DistanceType, typename ProtoType>
class EMTree {


private:

    // The order of this tree
    int _m;

    // The order of this tree
    int _maxLeaf;

    // The root of the tree.
    Node<T> *_root;

	ClustererType _clusterer;

	DistanceType _distF;
	ProtoType _protoF;

	vector<T*> removed;

	// Weights for prototype function (we don't have to use these)
    vector<int> weights;

	//pool _tPool;

	//boost::threadpool::pool pool;

public:

    EMTree( int order, int maxLeaf ) {        
        _m = order;
		_maxLeaf = maxLeaf;
        _root = new Node<T>(); // initial root is a leaf
		_clusterer.setNumClusters(_m);
		_clusterer.setMaxIters(0);	
		//_tp = pool::::create_pool(4); 
		//pool::size_policy_type::
		//_tPool.

		//_tPool.size_controller().resize(4);
		
		//size_controller_type sc = pool.size_controller();
				
		//size_controller_type sc = _tp.
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

	void printStats() {
		std::cout << "\nNumber of objects: " << getObjCount();
		std::cout << "\nCluster count: " << getClusterCount();
		std::cout << "\nLevel count (node 0): " << getLevelCount();
		std::cout << "\nMax depth: " << getMaxLevelCount();
		std::cout << "\nRMSE: " << getRMSE();
	}

    void seed(vector<T*> &data) {
        // make the root a leaf containing all data
		_root->addAll(data);
		seed(_root);
    }

    void seed(Node<T>* current) {

		//cout << "\n\nSeeding ... " << current->size();

		pool _tPool(2);

		Node<T> *child;
		
        //assert(current.isLeaf());
        if (current->size() <= _maxLeaf) {
			//cout << "\n*";
            return;
        } else {

			// Copy clusters - if we don't copy we will have problems
			// with recursion.
			vector<Cluster<T>*> clusters = _clusterer.cluster(current->getKeys());

			//cout << "\n" << clusters.size();

            // split the leaf into m parts
            //km.cluster(current.getKeyList());
            //List<Cluster> clusters = km.getClusters();

            if (clusters.size() < _m) { // split didn't work, so stop
				cout << "\nBad split ...";
                return;
            }

            // make keys centroids from k-means and children the nearest neigbhours
            current->clearKeysAndChildren();

			//int count=0;
            for (Cluster<T>* c : clusters) {
				//cout << "\nCluster ..." << count;
				//count++;

                child = new Node<T>();
                
				child->addAll(c->getNearestList());
                current->add(c->getCentroid(), child);

                // continue to split until base case is reached
                //seed(child);
            }

			vector<Node<T>*> &children = current->getChildren();

			int count = 0;
			for (Node<T>* n : children) {

				// Seed each of the children

				//
				//EMTree<T, ClustererType, DistanceType, ProtoType>::seed

				//cout << "\nScheduling ...  " << _m << "  " << _maxLeaf;
				
				_tPool.schedule(std::bind(seedTask<T, ClustererType>::seed, n, _m, _maxLeaf));
                //_tPool.schedule(std::bind(taskT<int,float>::run, count, 10.5f));

				//seed(n);

				count++;

            }

        }
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

        for (int i=0; i<removed.size(); i++) {
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

	/*
    void add(T *obj) {
		//std::cout << "\nAdding object ... ";

		SplitResult<T> result = pushDown(_root, obj);
        if (result.isSplit) {
			_root = new Node<T>();
			_root->add(result._key1, result._child1);
			_root->add(result._key2, result._child2);
        }
    }*/

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
		}
		else {
			int numEntries = child->size();	
			
			vector<T*> &keys = child->getKeys();
			vector<Node<T>*> &children = child->getChildren();

			for (int i=0; i<numEntries; i++) {
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
            return 1;
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
				if (count>maxCount) maxCount = count;
            }
            return maxCount + 1;
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
				rebuildInternal( children[i], depth - 1);
            }
        }
    }


    void pushDownNoUpdate(Node<T> *n, T *vec) {

		//std::cout << "\n\tPushing down (no update) ...";
		
		if (n->isLeaf()) {
			n->add(vec);  // Finished
        } else { // It is an internal node.
            // recurse via nearest neighbour cluster

			vector<T*>& keys = n->getKeys();
			vector<Node<T>*>& children = n->getChildren();

			size_t nearest = nearestChild(vec, keys);
            Node<T> *nearestChild = children[nearest];

			pushDownNoUpdate(nearestChild, vec);
        }
    }

	/*
    SplitResult<T> pushDown(Node<T> *n, T *vec) {

		//std::cout << "\n\tPushing down ...";

		SplitResult<T> result;
		
		if (n->isLeaf()) {
			if (n->size() >= _m)  {
                // split this node and pass new node to parent to insert
				result = splitLeafNode(n, vec);
            } else {
				n->add(vec);  // Finished
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
            }
            else {
				updatePrototype(nearestChild, nearestKey);
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
				
		// Get nearest centroids after clustering
		tempNearCentroids = _clusterer.getNearestCentroids();

		for (int i=0; i<tempNearCentroids.size(); i++) {
			if (tempNearCentroids[i]==0) parent->add(tempKeys[i], tempChildren[i]);			
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
				
		// Get nearest centroids after clustering
		tempNearCentroids = _clusterer.getNearestCentroids();

		for (int i=0; i<tempNearCentroids.size(); i++) {
			if (tempNearCentroids[i]==0) child->add(tempKeys[i]);
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
	*/

	// Update the protype parentKey
    void updatePrototype(Node<T> *child, T* parentKey) {
        
		//cout << "\nUpdating mean ...";

        //int[] weights = new int[count];
		weights.clear();

		if (!child->isLeaf()) {
			vector<Node<T>*>& children = child->getChildren();

			for (size_t i = 0; i < children.size(); i++) {
				weights.push_back(children[i]->size());
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

