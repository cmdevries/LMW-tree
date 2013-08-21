#ifndef FUNCS_H
#define FUNCS_H

#include "Clusterer.h"
#include "SVector.h"
#include "Node.h"
#include "BitMapList8.h"
#include "BitMapList16.h"

#include "threadpool.hpp"


template <typename T>
struct euclideanDistance {

	float operator()(T *t1, T *t2) {
	
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

	float operator()(T *t1, T *t2) {
	
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

	void operator()(T *t1, vector<T*> &objs, vector<int> &weights) {
	
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
	
	block_type *data;
	int vecSize;
	int dataSize;
	int numBlocks;

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

	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, vector<int> &weights) {

		vecSize = t1->size(); 
		data = t1->getData();
		dataSize = sizeof(data[0]) * 8;
		numBlocks = t1->getNumBlocks();

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
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, vector<int> &weights) {

		//std::cout << "\nComputing mean ...";

		//float total = 0.0f;
		
		vecSize = t1->size();
		data = t1->getData();
		dataSize = sizeof(data[0]) * 8;
		numBlocks = t1->getNumBlocks();
		//numSteps = dataSize/16; 

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
		else {/*
			for (size_t t = 0; t < objs.size(); t++) {
				
				data = objs[t]->getData();
				pos = bitCountPerDimension;

				for (int i=0; i<numBlocks; i++) {	
					val = data[i];
					for (int j=0; j<dataSize; j+=16) {							
						val = (data[i] >> j) & 65535LL;
						//bMap.add(val, pos, 1);
						bMap.add1(val, pos);
						pos+=16;
					}
				}	
			}*/
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

	meanBitPrototype8() {
		bitCountPerDimension = new int[65536];
		bMap.initialise();
	}

	~meanBitPrototype8() {
		delete[] bitCountPerDimension;
	}
	
	// We assume that the length of bit vectors is less than 65536 and greater than 0.
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, vector<int> &weights) {

		//std::cout << "\nComputing mean ...";

		//float total = 0.0f;
		
		vecSize = t1->size();
		data = t1->getData();
		dataSize = sizeof(data[0]) * 8;
		numBlocks = t1->getNumBlocks();
		//numSteps = dataSize/16; 

		t1->setAllBlocks(0);
				
		//memset(bitCountPerDimension, 0, t1->size());
		// 
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

	float operator()(SVector<bool> *v1, SVector<bool> *v2) {
	
		return SVector<bool>::hammingDistance( *v1,  *v2);

	}
};


boost::mutex mio;

void logMsg(char* reason, int i)
{
    boost::lock_guard<boost::mutex> lock(mio); // 2.
    std::cout << reason << " " << i << std::endl;
}


template <typename T, typename ClustererType>
struct seedTask {

	static void seed (Node<T>* current, int m, int depth) {

		
		ClustererType clusterer;
		clusterer.setNumClusters(m);
		clusterer.setMaxIters(1);
				
		//cout << "\nHello";
		//logMsg("Seeding ...");

		Node<T> *child;

		if (depth == 1) {
			//cout << "\n*";
			return;
		} else {

			//logMsg("Seeding ...");
			// Copy clusters - if we don't copy we will have problems
			// with recursion.
			vector<Cluster<T>*> clusters = clusterer.cluster(current->getKeys());

//			if (clusters.size() < m) { // split didn't work, so stop
//				cout << "\nBad split ...";
//				return;
//			}

			// make keys centroids from k-means and children the nearest neigbhours
			current->clearKeysAndChildren();

			int count=0;
			for (Cluster<T>* c : clusters) {
				//cout << "\nCluster ..." << count;
				count++;

				child = new Node<T>();
                
				child->addAll(c->getNearestList());
				current->add(c->getCentroid(), child);

				// continue to split until base case is reached
				//seed(child);
			}

			vector<Node<T>*> &children = current->getChildren();

			for (Node<T>* n : children) {

				// Seed each of the children
				seed2(n, clusterer, m, depth - 1);

			}

		}

	}


	static void seed2 (Node<T>* current, ClustererType &clusterer, int m, int depth) {

				
		//cout << "\nHello";
		//logMsg("Seeding ...");

		Node<T> *child;

		if (depth == 1) {
			//cout << "\n*";
			return;
		} else {

			//logMsg("Seeding ...");
			// Copy clusters - if we don't copy we will have problems
			// with recursion.
			vector<Cluster<T>*> clusters = clusterer.cluster(current->getKeys());

//			if (clusters.size() < m) { // split didn't work, so stop
//				cout << "\nBad split ...";
//				return;
//			}

			// make keys centroids from k-means and children the nearest neigbhours
			current->clearKeysAndChildren();

			int count=0;
			for (Cluster<T>* c : clusters) {
				//cout << "\nCluster ..." << count;
				count++;

				child = new Node<T>();
                
				child->addAll(c->getNearestList());
				current->add(c->getCentroid(), child);

				// continue to split until base case is reached
				//seed(child);
			}

			vector<Node<T>*> &children = current->getChildren();

			for (Node<T>* n : children) {

				// Seed each of the children
				seed2(n, clusterer, m, depth - 1);

			}

		}

	}

};


template <typename T, typename R>
struct taskT {

	static int run(T a, R b) {
		logMsg("Start job4", a+b);
		return (int)(a + b);
	}

};





#endif




