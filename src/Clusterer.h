#ifndef CLUSTERER_H
#define CLUSTERER_H

#include "SVector.h"
#include "Cluster.h"

// Implementations of this class will generate cluster
// objects and will also create the centroids contained within
// the cluster objects. The ownership of the cluster centroids
// passes to the object which contains the clusterer,
// and the clusterer will not destroy any created centroids.

// Future: perhaps used shared pointers instead.

// Lance

template <typename T>
class Clusterer {
public:

    virtual ~Clusterer<T>() {
    }

    virtual int numClusters() = 0;

    virtual vector<Cluster<T>*>& cluster(vector<T*> &data) = 0;

    virtual void setNumClusters(size_t n) = 0;
};

#endif	/* CLUSTERER_H */



