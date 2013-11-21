/**
 * The OPTIMIZER concept chooses how to optimize a given distance function.
 * Typically this is minimizing or maximizing the function. However, other
 * types of optimizers may be used.
 * 
 * TODO(cdevries): finish description of concept
 * 
 * For example,
 *      Optimizer<SVector<bool>, hammingDistance, Minimize> optimizer;
 *      SVector<bool>* centroid = ...;
 *      vector<SVector<bool>*> vectors = ...;
 *      T* nearest = optimizer.nearest(centroid, vectors);
 */

#ifndef OPTIMIZER_H
#define	OPTIMIZER_H

#include "StdIncludes.h"

struct Minimize {
    bool operator()(double currentDistance, double nearestDistance) {
        return currentDistance < nearestDistance;
    }
};

struct Maximize {
    bool operator()(double currentDistance, double nearestDistance) {
        return currentDistance > nearestDistance;
    }
};

template <typename T, typename DISTANCE, typename COMPARATOR, typename PROTOTYPE>
class Optimizer {
public:
        
    void updatePrototype(T* prototype, vector<T*>& neighbours, vector<int>& weights) {
        _prototype(prototype, neighbours, weights);
    }
    
    T* nearest(T* object, vector<T*>& others) {
        double nearestDistance = 0;
        size_t nearestIndex = 0;
        return nearest(object, others, &nearestDistance, &nearestIndex);
    }
    
    T* nearest(T* object, vector<T*>& others, double* nearestDistance) {
        size_t nearestIndex;
        return nearest(object, others, nearestDistance, &nearestIndex);
    }    

    size_t nearestIndex(T* object, vector<T*>& others) {
        double nearestDistance = 0;
        size_t nearestIndex = 0;
        nearest(object, others, &nearestDistance, &nearestIndex);
        return nearestIndex;
    }
    
    double sumSquaredError(T* object, vector<T*>& others) {
        double SSE = 0;
        for (auto otherObject : others) {
            SSE += _distance.squared(object, otherObject);
        }
        return SSE;
    }    

private:
    T* nearest(T* object, vector<T*>& others, double* nearestDistance, size_t* nearestIndex) {
        *nearestIndex = 0;
        *nearestDistance = _distance(object, others[0]);
        for (size_t i = 1; i < others.size(); ++i) {
            double currentDistance = _distance(object, others[i]);
            if (_comp(currentDistance, *nearestDistance)) {
                *nearestDistance = currentDistance;
                *nearestIndex = i;
            }
        }
        return others[*nearestIndex];
    }  
    
    COMPARATOR _comp;
    DISTANCE _distance;
    PROTOTYPE _prototype;
};

#endif	/* OPTIMIZER_H */

