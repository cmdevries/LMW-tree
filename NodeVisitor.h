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







#endif	/* NODE_VISITOR_H */


