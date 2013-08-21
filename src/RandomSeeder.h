#ifndef RANDOM_SEEDER_H
#define RANDOM_SEEDER_H


#include "Seeder.h"
#include "StdIncludes.h"


template <typename T>
class RandomSeeder : public Seeder<T> {

private:


public:

	RandomSeeder() {
		
	}

	// Pre: The centroids vector is empty
	void seed(vector<T*> &data, vector<T*> &centroids, int numCentres) {

		centroids.clear();

		vector<int> indices;
		indices.reserve(data.size());
		for (int i=0; i<data.size(); i++) indices.push_back(i);

		std::random_shuffle ( indices.begin(), indices.end() );

		T *vec;

		for (int i=0; i<numCentres && i<data.size(); i++) {
			vec = new T(*data[indices[i]]);
			centroids.push_back(vec); 
		}

		//std::cout << ".";
	}
	
};


#endif