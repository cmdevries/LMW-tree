#ifndef STREAMINGEMTREEEXPERIMENTS_H
#define	STREAMINGEMTREEEXPERIMENTS_H

#include "StdIncludes.h"

#include "CreateSignatures.h"

typedef SVector<bool> vecType;
typedef hammingDistance distanceType;
typedef meanBitPrototype2 protoType;
typedef Node<vecType> nodeType;
typedef RandomSeeder<vecType> seederType;
typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;
typedef SVector<int> ACCUMULATOR;

typedef StreamingEMTree<vecType, distanceType, protoType, ACCUMULATOR> StreamingEMTree_t;

StreamingEMTree_t* streamingEMTreeInit() {
    // load data
    vector < SVector<bool>*> vectors;
    int veccount = -1;
    {
        boost::timer::auto_cpu_timer load("loading signatures: %w seconds\n");
        loadWikiSignatures(vectors, veccount);
    }

    // filter data to XML Mining subset
    vector < SVector<bool>*> subset;
    {
        boost::timer::auto_cpu_timer load("filtering subset: %w seconds\n");
        loadSubset(vectors, subset, "data/inex_xml_mining_subset_2010.txt");
    }

    // run TSVQ to build tree on sample
    constexpr int m = 30;
    constexpr int depth = 3;
    constexpr int maxiter = 0;
    TSVQ<vecType, clustererType, distanceType, protoType> tsvq(m, depth, maxiter);

    {
        boost::timer::auto_cpu_timer load("cluster subset using TSVQ: %w seconds\n");
        tsvq.cluster(subset);
    }

    cout << "initializing streaming EM-tree based on TSVQ subset sample" << endl;
    cout << "TSVQ iterations = " << maxiter << endl;
    auto tree = new StreamingEMTree_t(tsvq.getMWayTree());

    for (auto v : vectors) {
        delete v;
    }

    return tree;
}

void streamingEMTreeInsertPruneReport(StreamingEMTree_t* emtree) {
    constexpr char docidFile[] = "data/wiki.4096.docids";
    constexpr char signatureFile[] = "data/wiki.4096.sig";
    constexpr size_t signatureLength = 4096;
    constexpr size_t readSize = 1000;

    BitVectorStream bvs(docidFile, signatureFile, signatureLength);

    // insert from stream
    size_t read = 0;
    boost::timer::auto_cpu_timer insert("inserting into streaming EM-tree: %w seconds\n");
    insert.start();
    for (;;) {
        vector < SVector<bool>*> data = bvs.read(readSize);
        if (data.empty()) {
            break;
        }
        read += data.size();
        emtree->insert(data);
        bvs.free(data);
    }
    insert.stop();
    cout << read << " vectors streamed from disk" << endl;
    insert.report();

    // prune
    int pruned = emtree->prune();
    cout << pruned << " nodes pruned" << endl;

    // report
    int maxDepth = emtree->getMaxLevelCount();
    cout << "max depth = " << maxDepth << endl;
    for (int i = 0; i < maxDepth; i++) {
        cout << "cluster count level " << i + 1 << " = "
                << emtree->getClusterCount(i + 1) << endl;
    }
    cout << "streaming EM-tree had " << emtree->getObjCount() << " vectors inserted" << endl;
    cout << "RMSE = " << emtree->getRMSE() << endl;    
}

void streamingEMTree() {
    // streaming EMTree
    constexpr int maxIters = 10;
    StreamingEMTree_t* emtree = streamingEMTreeInit();
    cout << endl << "Streaming EM-tree:" << endl;
    for (int i = 0; i < maxIters - 1; i++) {
        cout << "ITERATION " << i << endl;
        streamingEMTreeInsertPruneReport(emtree);
        {
            boost::timer::auto_cpu_timer update("update streaming EM-tree: %w seconds\n");
            emtree->update();
        }
        cout << "-----" << endl << endl;
    }
    
    // last iteration requires no update
    streamingEMTreeInsertPruneReport(emtree);
}

#endif	/* STREAMINGEMTREEEXPERIMENTS_H */

