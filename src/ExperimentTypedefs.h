#ifndef EXPERIMENTTYPEDEFS_H
#define	EXPERIMENTTYPEDEFS_H

typedef SVector<bool> vecType;
typedef hammingDistance distanceType;
typedef meanBitPrototype2 protoType;
typedef Node<vecType> nodeType;
typedef RandomSeeder<vecType> seederType;
typedef Optimizer<vecType, distanceType, Minimize, protoType> OPTIMIZER;
typedef KMeans<vecType, seederType, OPTIMIZER> clustererType;
typedef SVector<uint32_t> ACCUMULATOR;
typedef StreamingEMTree<vecType, ACCUMULATOR, OPTIMIZER> StreamingEMTree_t;

#endif	/* EXPERIMENTTYPEDEFS_H */

