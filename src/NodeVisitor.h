#ifndef NODE_VISITOR_H
#define NODE_VISITOR_H

#include "Node.h"

template <typename T>
class NodeVisitor {
	
public:

	virtual void accept(T *node) = 0;

	virtual ~NodeVisitor<T>() {  }

};


template <typename T>
class ClusterCounter : public NodeVisitor<T> {
	
	int _count;

public:

	ClusterCounter<T>() {
		_count = 0;
	}

	~ClusterCounter<T>() {  
	
	}

	void accept(T *node) {
		if (node->isLeaf()) _count++;
	}

	int getCount() {
		return _count;
	}
};

template <typename T>
class ClusterHistogramCounter : public NodeVisitor<T> {
	
	int _count;
	int _size;
	vector<int> _buckets;

public:

	ClusterHistogramCounter<T>(int maxClusterSize) {
		_count = 0;
		_buckets.resize(maxClusterSize+1);
		for (int i =0; i<_buckets.size(); i++) _buckets[i] = 0;  
	}

	~ClusterHistogramCounter<T>() {  
	
	}

	void accept(T *node) {
		if (node->isLeaf()) {
			 _size = node->size();
			_buckets[_size]++;
		}
	}

	void report() {
		std::cout << std::endl;
		for (int i=0; i<_buckets.size(); i++) {
			std::cout << _buckets[i] << std::endl;
		}
	}
};






#endif	/* NODE_VISITOR_H */


