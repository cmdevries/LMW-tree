#ifndef STREAMINGEMTREEEXPERIMENTS_H
#define	STREAMINGEMTREEEXPERIMENTS_H

#include "CreateSignatures.h"
#include "StdIncludes.h"
#include "tbb/task_scheduler_init.h"


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
    const int m = 30;
    const int depth = 3;
    const int maxiter = 0;
    TSVQ_t tsvq(m, depth, maxiter);

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

const char wikiDocidFile[] = "data/wiki.4096.docids";
const char wikiSignatureFile[] = "data/wiki.4096.sig";
const size_t wikiSignatureLength = 4096;
    
void writeClusters(const string& filenamePrefix, StreamingEMTree_t* emtree) {
    // open files
    SVectorStream<SVector<bool>> vs(wikiDocidFile, wikiSignatureFile, wikiSignatureLength);

    // setup output streams for all levels in the tree
    struct ClusterStreams {
        ofstream* stats;
        ofstream* clusters;
    };
    vector<ClusterStreams> outputStreams;
    for (int level = 1; level <= emtree->getMaxLevelCount(); level++) {
        stringstream ss;
        ss << filenamePrefix << "_level" << level;
        ClusterStreams streams;
        streams.stats = new ofstream(ss.str() + "_cluster_stats.txt");
        streams.clusters = new ofstream(ss.str() + "_clusters.txt");
        outputStreams.push_back(streams);
    }
}

void streamingEMTreeInsertPruneReport(StreamingEMTree_t* emtree) {
    // open files
    SVectorStream<SVector<bool>> vs(wikiDocidFile, wikiSignatureFile, wikiSignatureLength);
    
    // insert from stream
    boost::timer::auto_cpu_timer insert("inserting into streaming EM-tree: %w seconds\n");
    insert.start();
    size_t read = emtree->insert(vs);
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
    // initialize TBB
    const bool parallel = true;
    if (parallel) {
        tbb::task_scheduler_init init_parallel();
    } else {
        tbb::task_scheduler_init init_serial(1);
    }

    // streaming EMTree
    const int maxIters = 2;
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

