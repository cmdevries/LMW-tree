/** 
 * This file contains DISTANCE functions. They take two objects, and return a
 * similarity or disimilarity.
 * 
 * For example, to bit vectors are compared using the hamming distance.
 * 
 * A DISTANCE function is called with pointers to two objects of type T.
 * 
 * The only required operation is,
 *      float operator()(T* , T*)
 * 
 * For example,
 *      SVector<bool> a, b;
 *      hammingDistance hamming;
 *      float distance = hamming(&a, &b);
 */

#ifndef DISTANCE_H
#define	DISTANCE_H

#include "SVector.h"

struct hammingDistance {
    float operator()(SVector<bool> *v1, SVector<bool> *v2) const {
        return SVector<bool>::hammingDistance(*v1, *v2);
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
            sum = sum + (d * d);
        }
        return sum;
    }
};

template <typename T>
struct euclideanDistance {
    float operator()(T *t1, T *t2) const {
        euclideanDistanceSq<T> squared;
        return sqrt(squared(t1, t2));
    }
};

#endif	/* DISTANCE_H */

