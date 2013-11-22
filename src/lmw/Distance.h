/** 
 * This file contains DISTANCE functions. They take two objects, and return a
 * similarity or disimilarity.
 * 
 * For example, to bit vectors are compared using the hamming distance.
 * 
 * A DISTANCE function is called with pointers to two objects of type T.
 * 
 * The required operations are
 *      // Returns the distance.
 *      double operator()(T* , T*)
 * 
 *      // Returns the distance squared. This is required for calculating values
 *      // based on the squared error such as RMSE.
 *      double squared(T*, T*)      
 * 
 * For example,
 *      SVector<bool> a, b;
 *      hammingDistance hamming;
 *      double distance = hamming(&a, &b);
 *      double squaredDistance = hamming.squared(&a, &b);
 */

#ifndef DISTANCE_H
#define	DISTANCE_H

#include "SVector.h"

namespace lmw {

struct hammingDistance {
    double operator()(SVector<bool> *v1, SVector<bool> *v2) const {
        return SVector<bool>::hammingDistance(*v1, *v2);
    }
    
    double squared(SVector<bool> *v1, SVector<bool> *v2) const {
        double distance = operator()(v1, v2);
        return distance * distance;
    }    
};

template <typename T>
struct euclideanDistanceSq {    
    double operator()(T *t1, T *t2) const {
        typename T::iterator it1 = t1->begin();
        typename T::iterator it2 = t2->begin();
        double d, sum = 0.0f;
        for (it1 = t1->begin(), it2 = t2->begin(); it1 != t1->end(), it2 != t2->end(); it1++, it2++) {
            d = *it1 - *it2;
            sum = sum + (d * d);
        }
        return sum;
    }
    
    double squared(T *t1, T *t2) const {
        return operator()(t1, t2);
    }    
};

template <typename T>
struct euclideanDistance {
    double operator()(T *t1, T *t2) const {
        euclideanDistanceSq<T> squared;
        return sqrt(squared(t1, t2));
    }
    
    double squared(T *t1, T *t2) const {
        euclideanDistanceSq<T> squared;
        return squared(t1, t2);
    }    
};

} // namespace lmw

#endif	/* DISTANCE_H */

