#ifndef FUNCS_H
#define FUNCS_H

#include "Clusterer.h"
#include "SVector.h"
#include "Node.h"
#include "BitMapList8.h"
#include "BitMapList16.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

template <typename T>
struct euclideanDistance {

	float operator()(T *t1, T *t2) const {
	
		typename T::iterator it1 = t1->begin();
		typename T::iterator it2 = t2->begin();

		float d, sum = 0.0f;

		for (it1 = t1->begin(), it2 = t2->begin(); it1 != t1->end(), it2 != t2->end(); 
			it1++, it2++) {

			d = *it1 - *it2;			
			sum = sum + (d*d);
		}

		return sqrt(sum);
	}
};

template <typename T>
struct euclideanDistanceSq {

	float operator()(T *t1, T *t2) const {
	
		typename T::iterator it1 = t1->begin();
		typename T::iterator it2 = t2->begin();

		float d, sum = 0.0f;

		for (it1 = t1->begin(), it2 = t2->begin(); it1 != t1->end(), it2 != t2->end(); 
			it1++, it2++) {

			d = *it1 - *it2;			
			sum = sum + (d*d);
		}

		return sum;
	}
};


template <typename T>
struct meanPrototype {

	void operator()(T *t1, vector<T*> &objs, vector<int> &weights) const {
	
		float total = 0.0f;

		t1->setAll(0);

		if (weights.size()!=0) {
			for (size_t t = 0; t < objs.size(); t++) {
				t1->addMult(*objs[t], weights[t]);
				total += (float)weights[t];
			}
		}
		else {
			for (size_t t = 0; t < objs.size(); t++) {
				t1->add(*objs[t]);				
			}
			total = (float)objs.size();
		}

		t1->scale(1.0f/total);
	}
};


struct meanBitPrototype {

	// We define this variable as a member variable instead of
	// creating it new each time operator() is called.
	// 65536 is an arbitrary size.
	int *bitCountPerDimension;

	meanBitPrototype() {
		bitCountPerDimension = new int[65536];
	}

	~meanBitPrototype() {
		delete[] bitCountPerDimension;
	}

	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

		block_type *data = t1->getData();;
		int vecSize = t1->size();
		int dataSize = sizeof(data[0]) * 8;
		int numBlocks = t1->getNumBlocks();

		t1->setAllBlocks(0);

		//int *bitCountPerDimension = new int[t1->size()];
		memset(bitCountPerDimension, 0, vecSize*4);

		int halfCount = 0;

		//std::cout << "\nObjects size: " << objs.size();
		//std::cout << "\nVec size: " << vecSize;

		int incr;

		if (weights.size()!=0) {
			for (size_t t = 0; t < objs.size(); t++) {
				for (size_t s=0; s<vecSize; s++) {
					incr = objs[t]->at(s);
					bitCountPerDimension[s] += (objs[t]->at(s)*weights[t]); 
					//cout << " " << incr;
				}
			}
			for (int w : weights) {
				halfCount += w;
			}
			halfCount /= 2;
		}
		else {
			for (size_t t = 0; t < objs.size(); t++) {
				for (size_t s=0; s<vecSize; s++) {
					incr = objs[t]->at(s);
					bitCountPerDimension[s] += incr; 
					//cout << " " << incr;
				}			
			}
			halfCount = objs.size()/2;
		}
		for (size_t s=0; s<vecSize; s++) {
			//std::cout << bitCountPerDimension[s];
			if (bitCountPerDimension[s]>halfCount) t1->set(s); 
			//cout << " " << bitCountPerDimension[s];
		}	
	}
};

// This version uses a look up table to optimise
// the averaging of bit vectors.
struct meanBitPrototype2 {
	
	BitMapList16 bMap;
		
	block_type *data;
	//block_type block;
	int vecSize;
	int dataSize;
	int numBlocks;
	//int numSteps;
	unsigned short val;

	// We define this variable as a member variable instead of
	// creating it new each time operator() is called.
	// 65536 is an arbitrary size.
	int *bitCountPerDimension;

	meanBitPrototype2() {
		bitCountPerDimension = new int[65536];
		bMap.initialise();
	}

	~meanBitPrototype2() {
		delete[] bitCountPerDimension;
	}
	
	// We assume that the length of bit vectors is less than 65536 and greater than 0.
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

		unsigned short val;

		block_type *data = t1->getData();
		int vecSize = t1->size();
		int dataSize = sizeof(data[0]) * 8;
		int numBlocks = t1->getNumBlocks();

		t1->setAllBlocks(0);
				
		//memset(bitCountPerDimension, 0, t1->size());
		memset(bitCountPerDimension, 0, vecSize*4);

		int halfCount = 0;
		int *pos;
		
		if (weights.size()!=0) {
			for (size_t t = 0; t < objs.size(); t++) {				
					
				data = objs[t]->getData();
				pos = bitCountPerDimension;

				//std::cout << "\n----------\n";

				for (int i=0; i<numBlocks; i++) {	
					//std::cout << "\n .. " << data[i];
					//std::cout << "\n .. " << (data[i] >> 48);
					val = data[i];
					for (int j=0; j<dataSize; j+=16) {	
						//std::cout << "\n ''" << ((data[i] >> j) & 65535) << "\n"; 
						val = (data[i] >> j) & 65535;
						//std::cout << "\n" << j << "   " << val;
						//std::cout << "\n" << val;
						bMap.add(val, pos, weights[t]);
						//bMap.add1(val, pos);
						pos+=16;
					}
				}
				
			}
			for (int w : weights) {
				halfCount += w;
			}
			halfCount /= 2;
		}
		else {
			for (size_t t = 0; t < objs.size(); t++) {
				
				data = objs[t]->getData();
				pos = bitCountPerDimension;

				for (int i=0; i<numBlocks; i++) {
					//val = data[i];
					
					val = data[i] & 65535LL;
					bMap.add1(val, pos);
					pos+=16;

					val = (data[i] >> 16) & 65535LL;
					bMap.add1(val, pos);
					pos+=16;

					val = (data[i] >> 32) & 65535LL;
					bMap.add1(val, pos);
					pos+=16;

					val = (data[i] >> 48) & 65535LL;
					bMap.add1(val, pos);
					pos+=16;

				}	
			}


			halfCount = objs.size()/2;
		}
		//cout << "\n";
		for (size_t s=0; s<t1->size(); s++) {
			if (bitCountPerDimension[s]>halfCount) t1->set(s); 
			//cout << " " << bitCountPerDimension[s];
		}
	}
	
};

// This version uses a look up table to optimise
// the averaging of bit vectors.
struct meanBitPrototype8 {
	
	BitMapList8 bMap;

	// We define this variable as a member variable instead of
	// creating it new each time operator() is called.
	// 65536 is an arbitrary size.
	int *bitCountPerDimension;

	meanBitPrototype8() {
		bitCountPerDimension = new int[65536];
		bMap.initialise();
	}

	~meanBitPrototype8() {
		delete[] bitCountPerDimension;
	}
	
	// We assume that the length of bit vectors is less than 65536 and greater than 0.
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

		unsigned short val;

		block_type *data = t1->getData();
		int vecSize = t1->size();
		int dataSize = sizeof(data[0]) * 8;
		int numBlocks = t1->getNumBlocks();

		t1->setAllBlocks(0);
				
		memset(bitCountPerDimension, 0, vecSize*sizeof(int));

		int halfCount = 0;
		int *pos;		

		if (weights.size()!=0) {
			for (size_t t = 0; t < objs.size(); t++) {				
					
				data = objs[t]->getData();
				pos = bitCountPerDimension;

				//std::cout << "\n----------\n";

				for (int i=0; i<numBlocks; i++) {	
					//std::cout << "\n .. " << data[i];
					//std::cout << "\n .. " << (data[i] >> 48);
					val = data[i];
					for (int j=0; j<dataSize; j+=8) {	
						//std::cout << "\n ''" << ((data[i] >> j) & 65535) << "\n"; 
						val = (data[i] >> j) & 255LL;
						//std::cout << "\n" << j << "   " << val;
						//std::cout << "\n" << val;
						bMap.add(val, pos, weights[t]);
						//bMap.add1(val, pos);
						pos+=8;
					}
				}
				
			}
			for (int w : weights) {
				halfCount += w;
			}
			halfCount /= 2;
		}
		else {
			for (size_t t = 0; t < objs.size(); t++) {
				
				data = objs[t]->getData();
				pos = bitCountPerDimension;

				for (int i=0; i<numBlocks; i++) {
					//val = data[i];
					
					val = data[i] & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 8) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 16) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 24) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 32) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 40) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 48) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

					val = (data[i] >> 56) & 255LL;
					bMap.add1(val, pos);
					pos+=8;

				}	
			}


			halfCount = objs.size()/2;
		}
		//cout << "\n";
		for (size_t s=0; s<t1->size(); s++) {
			if (bitCountPerDimension[s]>halfCount) t1->set(s); 
			//cout << " " << bitCountPerDimension[s];
		}
	}
	
};


struct hammingDistance {

	float operator()(SVector<bool> *v1, SVector<bool> *v2) const {
	
		return SVector<bool>::hammingDistance( *v1,  *v2);

	}
};


//---------------------------------
// Function object for use with TBB

template <typename T, typename DTYPE>
struct VecToCentroid {

	vector<T*>& _data;
	vector<T*>& _centroids;
	vector<size_t>& _nearestCentroid;

	DTYPE _distF;
	bool *_converged;

public:

	void operator()(const tbb::blocked_range<size_t>& r) const {

		int nearest;

		for (size_t i = r.begin(); i != r.end(); i++) {
			nearest = nearestObj(_data[i]);
			if (nearest != _nearestCentroid[i]) {
				*_converged = false;
			}
			_nearestCentroid[i] = nearest;
		}
	}

	size_t nearestObj(T *obj) const {

		size_t nearest = 0;
		float dist;
		float nearestDistance = _distF(obj, _centroids[0]);

		for (size_t i = 1; i < _centroids.size(); ++i) {
			dist = _distF(obj, _centroids[i]);
			if (dist < nearestDistance) {
				nearestDistance = dist;
				nearest = i;
			}
		}

		return nearest;
	}

	VecToCentroid(vector<T*>& centroids, vector<T*>& data, vector<size_t>& nearestCentroid, bool *converged) :
		_centroids(centroids),
		_data(data),
		_nearestCentroid(nearestCentroid),
		_converged(converged)
	{

	}
};


template <typename T, typename PTYPE>
struct UpdateCentroid {

	vector<int>& _weights;
	// Should this be a reference?
	vector<Cluster<T>*> &_clusters;

	PTYPE _protoF;

public:

	void operator()(const tbb::blocked_range<size_t>& r) const {

		Cluster<T>* c;
		int count;

		for (size_t i = r.begin(); i != r.end(); i++) {
			c = _clusters[i];
			count = (int)(c->size());

			if (count > 0) {
				_protoF(c->getCentroid(), c->getNearestList(), _weights);
			}
		}
	}

	UpdateCentroid(vector<Cluster<T>*> &clusters, vector<int>& weights) : 
		_clusters(clusters), 
		_weights(weights)
	{

	}

};

/*
template <typename VEC_TYPE, typename CLUSTERER_TYPE>
class ClustererTask : public tbb::task {

public:
	
	const Node<VEC_TYPE> *_current;
	const int _m;
	const int _depth;
	const int _maxIters;

	ClustererTask(Node<VEC_TYPE>* current, int order, int depth, int maxiters) : 
		_current(current),
		_m(order), 
		_depth(depth),
		_maxIters(maxiters)
	{
	
	}


	task* execute() {      // Overrides virtual function task::execute

		ClustererType _clusterer;
		_clusterer.setNumClusters(_m);
		_clusterer.setMaxIters(_maxIters);
		
		vector<Cluster<T>*> clusters = _clusterer.cluster(_current->getKeys());
		_current->clearKeysAndChildren();
		
		for (Cluster<T>* c : clusters) {
			Node<T>* child = new Node<T>();
			child->addAll(c->getNearestList());
			current->add(c->getCentroid(), child);
		}

		// All the compute tasks in this part of the tree
		vector<ClustererTask*> tasks;

		if (depth > 1) {
			
			for (Node<T>* n : current->getChildren()) {
				
				tasks.push_back(new(allocate_child()) FibTask(n - 1, &x);)
			}

			cluster(n, depth - 1);
		}


		if (depth == 1) {
			return;
		}
		else {
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

		long x, y;
		FibTask& a = *new(allocate_child()) FibTask(n - 1, &x);
		FibTask& b = *new(allocate_child()) FibTask(n - 2, &y);
		
		// Set ref_count to 'two children plus one for the wait".
		set_ref_count(3);
		
		// Start b running.
		spawn(b);
		
		// Start a running and wait for all children (a and b).
		spawn_and_wait_for_all(a);
		
		// Do the sum
			*sum = x + y;
		
		return NULL;
	}
};
*/



#endif




