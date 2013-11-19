#ifndef KMEANS_H
#define KMEANS_H

#include "Cluster.h"
#include "Clusterer.h"
#include "Seeder.h"
#include "StdIncludes.h"
#include "tbb/atomic.h"

template <typename T, typename SeederType, typename DistanceType, typename ProtoType>
class KMeans : public Clusterer<T> {
private:

    // Seeder
    SeederType *_seeder;

    DistanceType _distF;
    ProtoType _protoF;
    
    // enforce the number of clusters required
    // if less than k clusters are produced, shuffle vectors randomly and split into k cluster
    bool _enforceNumClusters = false;

    // present number of iterations
    int _iterCount = 0;

    // maximum number of iterations
    // -1 - run until complete convergence
    // 0 - only assign nearest neighbors after seeding
    // >= 1 - perform this many iterations
    int _maxIters = 100;

    // How many clusters should be found? i.e. k
    int _numClusters = 0;

    vector<T*> _centroids;
    vector<Cluster<T>*> _clusters;
    vector<Cluster<T>*> _finalClusters;

    // The centroid index for each vector. Aligned with vectors member variable.
    vector<size_t> _nearestCentroid;

    // Weights for prototype function (we don't have to use these)
    vector<int> _weights;

    // Residual for convergence
    float _eps = 0.00001f;
    
    // has the clustering converged
    bool _converged;

public:

    //KMeans(Seeder *seeder, int numClusters) {

    KMeans() {
        _seeder = new SeederType();
    }

	KMeans(int numClusters) : _numClusters(numClusters) {
        _seeder = new SeederType();
    }

	KMeans(int numClusters, float eps) : 
		_numClusters(numClusters), 
		_eps(eps) 
	{
		_seeder = new SeederType();
		_maxIters = 100;
	}

    ~KMeans() {
        // Need to clean up any created cluster objects
        Utils::purge(_clusters);
    }

    vector<size_t>& getNearestCentroids() {
        return _nearestCentroid;
    }

    void setNumClusters(size_t numClusters) {
        _numClusters = numClusters;
    }

    void setMaxIters(int maxIters) {
        _maxIters = maxIters;
    }
    
    void setEnforceNumClusters(bool enforceNumClusters) {
        _enforceNumClusters = enforceNumClusters;
    }

    int numClusters() {
        return _numClusters;
    }

    vector<Cluster<T>*>& cluster(vector<T*> &data) {
        Utils::purge(_clusters);
        _clusters.clear();
        _finalClusters.clear();
        cluster(data, _numClusters);
        finalizeClusters(data);
        return _finalClusters;
    }
    
    float getRMSE(vector<T*> &data) {
        float rmse = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            // Need to change this in future to use an error function
            float e = _distF(data[i], _centroids[_nearestCentroid[i]]);
            rmse += e*e;
        }
        rmse /= data.size();
        return (float) sqrt(rmse);
    }

private:

    void finalizeClusters(vector<T*> &data) {      
        // Create list of final clusters to return;
        bool emptyCluster = assignClusters(data);
        if (emptyCluster && _enforceNumClusters) {
            // randomly shuffle if k cluster were not created to enforce the number of clusters if required
            //std::cout << std::endl << "k-means is splitting randomly";
            vector<T*> shuffled(data);
            std::random_shuffle(shuffled.begin(), shuffled.end());
            {
                size_t step = shuffled.size(), i = 0, clusterIndex = 0;
                for (; i < shuffled.size(); i += step, ++clusterIndex) {
                    for (size_t j = i; j < i + step && j < shuffled.size(); ++j) {
                        _nearestCentroid[j] = clusterIndex;
                    }
                }
            }
            vectorsToNearestCentroid(data);
            recalculateCentroids(data);
            assignClusters(data);
        }
    }
    
    bool assignClusters(vector<T*> &data) {      
        // Create list of final clusters to return;
        bool emptyCluster = false;
        for (Cluster<T>* c : _clusters) {
            if (!c->getNearestList().empty()) {
                _finalClusters.push_back(c);
            } else {
                emptyCluster = true;
            }
        }
        return emptyCluster;
    }
    

    /**
     * @param vectors       the vectors to form clusters for
     * @param clusters      the number of clusters to find (i.e. k)
     */
    void cluster(vector<T*> &data, size_t clusters) {
        // Setup initial state.
        _iterCount = 0;
        _numClusters = clusters;
        _nearestCentroid.resize(data.size());
        _seeder->seed(data, _centroids, _numClusters);

        // Create as many cluster objects as there are centroids
        for (T* c : _centroids) {
            _clusters.push_back(new Cluster<T>(c));
        }

        //cout << "\nAfter seeding there are " << _clusters.size() << " clusters.";

        // First iteration
        vectorsToNearestCentroid(data);
        if (_maxIters == 0) {
            return;
        }
        recalculateCentroids(data);
        if (_maxIters == 1) {
            return;
        }

        // Repeat until convergence.
        _converged = false;
        _iterCount = 1;
        while (!_converged) {
            vectorsToNearestCentroid(data);
            recalculateCentroids(data);
            _iterCount++;
            if (_maxIters != -1 && _iterCount >= _maxIters) {
                break;
            }
        }
    }

    size_t nearestObj(T *obj, vector<T*> &others) {
        size_t nearest = 0;
        float dist = 0;
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

    void vectorToNearestCentroid(size_t i, vector<T*>& data) {
        size_t nearest = nearestObj(data[i], _centroids);
        if (nearest != _nearestCentroid[i]) {
            _converged = false;
        }
        _nearestCentroid[i] = nearest;
    }
    
    class VecToCentroid {
    public:
        VecToCentroid(KMeans* kmeans, vector<T*>& data) : _kmeans(kmeans), _data(data) { }

        void operator()(const tbb::blocked_range<size_t>& r) const {
            for (size_t i = r.begin(); i != r.end(); i++) {
                _kmeans->vectorToNearestCentroid(i, _data);
            }
        }
        
    private:
        KMeans* _kmeans;
        vector<T*>& _data;
    };
    
    /**
     * Assign vectors to nearest centroid.
     * Pre: seedCentroids() OR recalculateCentroids() has been called
     * Post: nearestCentroid contains all the indexes into centroids for the
     *       nearest centroid for the vector (nearestCentroid and vectors are
     *       aligned).
     * @return boolean indicating if there were any changes, i.e. was there
     *                 convergence
     */
    void vectorsToNearestCentroid(vector<T*> &data) {
        // Clear the nearest vectors in each cluster
        for (Cluster<T> *c : _clusters) {
            c->clearNearest();
        }
        _converged = true;

        // Parallel
        VecToCentroid vc(this, data);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, data.size(), 1000), vc);
        tbb::atomic_fence(); // make sure all data is visible

        // Serial
        // Clear the nearest vectors in each cluster
        for (Cluster<T> *c : _clusters) {
            c->clearNearest();
        }
        // Accumlate into clusters
        for (size_t i = 0; i < data.size(); i++) {
            size_t nearest = _nearestCentroid[i];
            _clusters[nearest]->addNearest(data[i]);
        }
    }

    void recalculateCentroid(size_t i) {
        Cluster<T>* c = _clusters[i];
        if (c->size() > 0) {
            _protoF(c->getCentroid(), c->getNearestList(), _weights);
        }
    }

    class RecalculateCentroid {
    public:
        RecalculateCentroid(KMeans* kmeans) : _kmeans(kmeans) {
        }

        void operator()(const tbb::blocked_range<size_t>& r) const {
            for (size_t i = r.begin(); i != r.end(); i++) {
                _kmeans->recalculateCentroid(i);
            }
        }

    private:
        KMeans* _kmeans;
    };
    
    /**
     * Recalculate centroids after vectors have been moved to there nearest
     * centroid.
     * Pre: vectorsToNearestCentroid() has been called
     * Post: centroids has been updated with new vector data
     */
    void recalculateCentroids(vector<T*> &data) {
        RecalculateCentroid recalculate(this);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, _clusters.size(), 2), recalculate);
        tbb::atomic_fence(); // make sure all data is visible
    }
};



#endif	/* KMEANS_H */



