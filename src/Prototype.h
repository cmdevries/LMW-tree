/**
 * This file contains PROTOTYPE functions. It summarizes a list of objects into
 * a single object. It also takes weights for each object into account.
 * 
 * It works on objects of type T.
 * 
 * The PROTOTYPE function is called with a pointer to the result, a list of
 * object and a list of weights.
 * 
 * The only required operation is,
 *      void operator()(T* result, vector<T*> objects, vector<int> weights)
 * 
 * Note that weights can be empty if no weights are present, otherwise the 
 * number of objects must match the number of weights,
 *      assert(objects.size() == weight.size())
 * 
 * For example, floating point vectors can use the mean or median, and bit
 * vectors use a specialized prototype optimized for speed. 
 */

#ifndef PROTOTYPE_H
#define	PROTOTYPE_H

#include <vector>

#include "BitMapList8.h"
#include "BitMapList16.h"
#include "SVector.h"

using namespace std;

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
	meanBitPrototype() {
	}

	~meanBitPrototype() {
	}

	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

            int bitCountPerDimension[65536];
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

	meanBitPrototype2() {
		bMap.initialise();
	}

	~meanBitPrototype2() {
	}
	
	// We assume that the length of bit vectors is less than 65536 and greater than 0.
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

            int bitCountPerDimension[65536];
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

	meanBitPrototype8() {
		bMap.initialise();
	}

	~meanBitPrototype8() {
	}
	
	// We assume that the length of bit vectors is less than 65536 and greater than 0.
	void operator()(SVector<bool> *t1, vector<SVector<bool>*> &objs, 
		vector<int> &weights) const {

            int bitCountPerDimension[65536];
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

#endif	/* PROTOTYPE_H */

