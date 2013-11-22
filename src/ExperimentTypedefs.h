#ifndef EXPERIMENTTYPEDEFS_H
#define	EXPERIMENTTYPEDEFS_H

#include "lmw/Distance.h"
#include "lmw/Prototype.h"
#include "lmw/SVector.h"
#include "lmw/Cluster.h"
#include "lmw/Clusterer.h"
#include "lmw/Seeder.h"
#include "lmw/DSquaredSeeder.h"
#include "lmw/RandomSeeder.h"
#include "lmw/VectorGenerator.h"
#include "lmw/StdIncludes.h"
#include "lmw/SVectorStream.h"
#include "lmw/Optimizer.h"

#include "lmw/KMeans.h"
#include "lmw/TSVQ.h"
#include "lmw/KTree.h"
#include "lmw/EMTree.h"
#include "lmw/StreamingEMTree.h"

using namespace lmw;

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

