#ifndef STREAMINGEMTREEEXPERIMENTS_H
#define	STREAMINGEMTREEEXPERIMENTS_H

#include "CreateSignatures.h"
#include "lmw/StdIncludes.h"
#include "lmw/ClusterVisitor.h"
#include "lmw/InsertVisitor.h"
#include "tbb/mutex.h"
#include "tbb/task_scheduler_init.h"
#include "lmw/StreamingEMTree.h"

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
    const int m = 10;
    const int depth = 4;
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

const char wikiDocidFile[] = "data/wikisignatures/wiki.4096.docids";
const char wikiSignatureFile[] = "data/wikisignatures/wiki.4096.sig";
const size_t wikiSignatureLength = 4096;


void report(StreamingEMTree_t* emtree) {
    int maxDepth = emtree->getMaxLevelCount();
    cout << "max depth = " << maxDepth << endl;
    for (int i = 0; i < maxDepth; i++) {
        cout << "cluster count level " << i + 1 << " = "
                << emtree->getClusterCount(i + 1) << endl;
    }
    cout << "streaming EM-tree had " << emtree->getObjCount() << " vectors inserted" << endl;
    cout << "RMSE = " << emtree->getRMSE() << endl;
}

void insertWriteClusters(StreamingEMTree_t* emtree) {
    // open files
    SVectorStream<SVector<bool>> vs(wikiDocidFile, wikiSignatureFile, wikiSignatureLength);

    // setup output streams for all levels in the tree
    const string prefix = "wikipedia_clusters";

    // insert and write cluster assignments
    {
        boost::timer::auto_cpu_timer insert("inserting and writing clusters: %w seconds\n");
        ClusterWriter cw(emtree->getMaxLevelCount(), prefix);
        emtree->visit(vs, cw);
    }

    // prune
    cout << emtree->prune() << " nodes pruned" << endl;

    // report tree stats
    report(emtree);

    // write out cluster statistics
    {
        boost::timer::auto_cpu_timer update("writing cluster stats: %w seconds\n");
        ClusterStats cs(emtree->getMaxLevelCount(), prefix);
        emtree->visit(cs);
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
    cout << "pruning" << endl;
    int pruned = emtree->prune();
    cout << pruned << " nodes pruned" << endl;

    // report tree stats
    report(emtree);
}

void streamingEMTree() {
    // initialize TBB
    const bool parallel = true;
    if (parallel) {
        tbb::task_scheduler_init init_parallel;
    } else {
        tbb::task_scheduler_init init_serial(1);
    }

    // streaming EMTree
    const int maxIters = 10;
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

    // last iteration writes cluster assignments and does not update accumulators
    insertWriteClusters(emtree);
}

#endif	/* STREAMINGEMTREEEXPERIMENTS_H */

