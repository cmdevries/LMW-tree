#ifndef KMEANS_H
#define KMEANS_H

#include "Cluster.h"
#include "Clusterer.h"
#include "Seeder.h"
#include "StdIncludes.h"

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
    
    // The dimension of the vectors.
    //int _dim;

    // present number of iterations
    int _iterCount;

    // maximum number of iterations
    int _maxIters;

    // How many clusters should be found? i.e. k
    int _numClusters;

    // The centroids of the clusters.
    //vector<T*> _centroids;

    vector<T*> _centroids;

    vector<Cluster<T>*> _clusters;

    vector<Cluster<T>*> _finalClusters;

    // The centroid index for each vector. Aligned with vectors member variable.
    vector<size_t> _nearestCentroid;

    // Weights for prototype function (we don't have to use these)
    vector<int> weights;

public:

    //KMeans(Seeder *seeder, int numClusters) {

    KMeans() {
        _seeder = new SeederType();
        _numClusters = 0;
        _maxIters = 100;
    }

    KMeans(int numClusters) {
        _seeder = new SeederType();
        _numClusters = numClusters;
        _maxIters = 100;
    }

    ~KMeans() {
        // Need to clean up any created cluster objects
        Utils::purge(_clusters);
    }

    vector<size_t>& getNearestCentroids() {
        return _nearestCentroid;
    }

    /*
    void setSeeder(Seeder *seeder) {
            _seeder = seeder;
    }*/

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

        //std::cout << "\nClustering... ";

        Utils::purge(_clusters);
        _clusters.clear();
        _finalClusters.clear();

        cluster(data, _numClusters);

        finalizeClusters(data);

        return _finalClusters;
    }


    // Private methods here:

private:

    void finalizeClusters(vector<T*> &data) {      
        // Create list of final clusters to return;
        bool emptyCluster = assignClusters(data);
        if (emptyCluster && _enforceNumClusters) {
            // randomly shuffle if k cluster were not created to enforce the number of clusters if required
            std::cout << std::endl << "k-means is splitting randomly";
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

        _iterCount = 0;
        //_dim = vectors[0]->length;
        _numClusters = clusters;

        int dataCount = data.size();

        _nearestCentroid.resize(dataCount);
        //_vectorsPerCentroid.resize(_numClusters);

        float lastRMSE = std::numeric_limits<float>::max();
        float currentRMSE;
        float DIFF_RMSE = (float) 1e-3;

        // Setup initial state.
        _seeder->seed(data, _centroids, _numClusters);

        // Create as many cluster objects as there are centroids
        for (T* c : _centroids) {
            _clusters.push_back(new Cluster<T>(c));
        }

        //cout << "\nAfter seeding there are " << _clusters.size() << " clusters.";

        // First iteration
        vectorsToNearestCentroid(data);
        recalculateCentroids(data);
        if (_maxIters == -1) {
            return;
        }

        //if (_maxIters<=1) return;

        // Repeat until convergence.
        bool converged = false;
        _iterCount = 1;
        currentRMSE = getRMSE(data);
        //std::cout << "\nRMSE: " << currentRMSE;

        while (!converged) {
            lastRMSE = currentRMSE;

            //converged = vectorsToNearestCentroid(data);
            vectorsToNearestCentroid(data);
            recalculateCentroids(data);

            currentRMSE = getRMSE(data);
            if (abs(lastRMSE - currentRMSE) < DIFF_RMSE) converged = true;

            _iterCount++;

            if (_iterCount >= _maxIters) break;

            //std::cout << "\n-------\nRMSE: " << currentRMSE;
            //std::cout << "\n-------";
        }

        //std::cout << "\n";
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

    /**
     * Assign vectors to nearest centroid.
     * Pre: seedCentroids() OR recalculateCentroids() has been called
     * Post: nearestCentroid contains all the indexes into centroids for the
     *       nearest centroid for the vector (nearestCentroid and vectors are
     *       aligned).
     * @return boolean indicating if there were any changes, i.e. was there
     *                 convergence
     */
    bool vectorsToNearestCentroid(vector<T*> &data) {
        bool converged = true;
        size_t nearest;
        size_t dataCount = data.size();

        //std::cout << "\nVectors to nearest centroid ...";
        //std::cout << "\n";

        for (size_t i = 0; i < dataCount; ++i) {
            // Find centroid closest to current vector.
            nearest = nearestObj(data[i], _centroids);

            // If there was a change then there is no convergence.
            if (nearest != _nearestCentroid[i]) {
                converged = false;
            }
            _nearestCentroid[i] = nearest;

            //std::cout << nearest << " ";
        }

        return converged;
    }

    /**
     * Recalculate centroids after vectors have been moved to there nearest
     * centroid.
     * Pre: vectorsToNearestCentroid() has been called
     * Post: centroids has been updated with new vector data
     */
    void recalculateCentroids(vector<T*> &data) {

        size_t nearest;
        size_t dataCount = data.size();

        // Clear the nearest vectors in each cluster
        for (Cluster<T> *c : _clusters) {
            //c->getCentroid()->setToZero();
            c->clearNearest();
        }

        // Accumlate into clusters
        for (size_t i = 0; i < dataCount; i++) {
            nearest = _nearestCentroid[i];
            _clusters[nearest]->addNearest(data[i]);
        }

        //std::cout << std::endl;

        // Apply prototype function
        for (Cluster<T> *c : _clusters) {
            int count = (int) (c->size());
            if (count > 0) {
                _protoF(c->getCentroid(), c->getNearestList(), weights);
            }
            //std::cout << " " << count;
        }
        //std::cout << "\nRMSE: " << getRMSE(data);
    }

    float getRMSE(vector<T*> &data) {
        float rmse = 0;
        float e = 0;

        size_t dataCount = data.size();
        for (size_t i = 0; i < dataCount; ++i) {
            // Need to change this in future to use an error function
            e = _distF(data[i], _centroids[_nearestCentroid[i]]);
            rmse += e*e;
        }
        rmse /= dataCount;
        return (float) sqrt(rmse);
    }

};



#endif	/* KMEANS_H */



