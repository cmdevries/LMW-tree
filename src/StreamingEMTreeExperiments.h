#ifndef STREAMINGEMTREEEXPERIMENTS_H
#define	STREAMINGEMTREEEXPERIMENTS_H

#include "StdIncludes.h"
#include "tbb/pipeline.h"
#include "tbb/task_scheduler_init.h"

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

class InputFilter : public tbb::filter {
public:
    InputFilter(BitVectorStream* bvs, size_t readsize = 1000) : 
        filter(serial_out_of_order),
        _bvs(bvs),
        _readsize(readsize),
        _read(0)
        { }
        
    ~InputFilter() { }

    size_t read() {
        return _read;
    }
    
private:
    BitVectorStream* _bvs;
    size_t _readsize;
    size_t _read;

    void* operator()(void*) {
        auto data = new vector<SVector<bool>*>;
        *data = _bvs->read(_readsize);
        if (data->empty()) {
            delete data;
            return NULL;
        }
        _read += data->size();
        return data;
    }
};

class InsertFilter : public tbb::filter {
public:
    InsertFilter(BitVectorStream* bvs, StreamingEMTree_t* emtree) :
        filter(parallel),
        _bvs(bvs),
        _emtree(emtree)
        { }
    
    ~InsertFilter() { }
    
private:
    BitVectorStream* _bvs;
    StreamingEMTree_t* _emtree;
    
    void* operator()(void* item) {
        auto data = (vector<SVector<bool>*>*)item;
        _emtree->insert(*data);
        _bvs->free(*data);
        delete data;
    }
};

void streamingEMTreeInsertPruneReport(StreamingEMTree_t* emtree) {
    constexpr char docidFile[] = "data/wiki.4096.docids";
    constexpr char signatureFile[] = "data/wiki.4096.sig";
    constexpr size_t signatureLength = 4096;
    constexpr size_t readSize = 1000;

    // open files
    BitVectorStream bvs(docidFile, signatureFile, signatureLength);
    
    // setup processing pipeline
    constexpr int threads = 4;
    tbb::pipeline pipeline;
    InputFilter inputFilter(&bvs);
    pipeline.add_filter(inputFilter);
    InsertFilter insertFilter(&bvs, emtree);
    pipeline.add_filter(insertFilter);    

    // insert from stream
    boost::timer::auto_cpu_timer insert("inserting into streaming EM-tree: %w seconds\n");
    insert.start();
    pipeline.run(threads);
    insert.stop();
    cout << inputFilter.read() << " vectors streamed from disk" << endl;
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

