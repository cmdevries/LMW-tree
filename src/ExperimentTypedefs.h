#ifndef EXPERIMENTTYPEDEFS_H
#define	EXPERIMENTTYPEDEFS_H

typedef SVector<bool> vecType;
typedef RandomSeeder<vecType> RandomSeeder_t;
typedef Optimizer<vecType, hammingDistance, Minimize, meanBitPrototype2> OPTIMIZER;
typedef KMeans<vecType, RandomSeeder_t, OPTIMIZER> KMeans_t;
typedef TSVQ<vecType, KMeans_t, hammingDistance> TSVQ_t;
typedef KTree<vecType, KMeans_t, OPTIMIZER> KTree_t;
typedef EMTree<vecType, KMeans_t, OPTIMIZER> EMTree_t;
typedef SVector<uint32_t> ACCUMULATOR;
typedef StreamingEMTree<vecType, ACCUMULATOR, OPTIMIZER> StreamingEMTree_t;

#endif	/* EXPERIMENTTYPEDEFS_H */

