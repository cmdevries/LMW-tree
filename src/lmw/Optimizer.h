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

namespace lmw {

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
        size_t nearestIndex = 0;
        return nearest(object, others, &nearestIndex);
    } 

    size_t nearestIndex(T* object, vector<T*>& others) {
        size_t nearestIndex = 0;
        nearest(object, others, &nearestIndex);
        return nearestIndex;
    }

    /**
     * This function provides access to a more complex KEY via used of an
     * ACCESSOR functor that implements T* operator()(KEY* key).
     * 
     * For example, this is used in StreamingEMTree where ACCUMULATORs are also
     * stored in the key. It avoids having to unpack all they T* from the 
     * more complex key type.
     */
    template <typename KEY, typename ACCESSOR>
    size_t nearestIndex(T* object, vector<KEY*>& others, ACCESSOR& accessor) {
        size_t nearestIndex = 0;
        nearest(object, others, &nearestIndex, accessor);
        return nearestIndex;
    }

    double squaredDistance(T* object1, T* object2) {
        return _distance.squared(object1, object2);
    }
    
    double sumSquaredError(T* object, vector<T*>& others) {
        double SSE = 0;
        for (auto otherObject : others) {
            SSE += _distance.squared(object, otherObject);
        }
        return SSE;
    }

private:
    /**
     * The default accessor is used for simple key types where the vector of
     * other objects match the object.
     */
    struct DefaultAccessor {
        T* operator()(T* key) {
            return key;
        }
    };
    
    T* nearest(T* object, vector<T*>& others, size_t* nearestIndex) {
        return nearest(object, others, nearestIndex, _defaultAccessor);
    }
    
    template <typename KEY, typename ACCESSOR>
    KEY* nearest(T* object, vector<KEY*>& others, size_t* nearestIndex, ACCESSOR& accessor) {
        *nearestIndex = 0;
        double nearestDistance = _distance(object, accessor(others[0]));
        for (size_t i = 1; i < others.size(); ++i) {
            double currentDistance = _distance(object, accessor(others[i]));
            if (_comp(currentDistance, nearestDistance)) {
                nearestDistance = currentDistance;
                *nearestIndex = i;
            }
        }
        return others[*nearestIndex];
    }  
    
    COMPARATOR _comp;
    DISTANCE _distance;
    PROTOTYPE _prototype;
    DefaultAccessor _defaultAccessor;
};

} // namespace lmw

#endif	/* OPTIMIZER_H */