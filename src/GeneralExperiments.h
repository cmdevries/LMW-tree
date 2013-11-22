#ifndef GENERALEXPERIMENTS_H
#define	GENERALEXPERIMENTS_H

#include "lmw/StdIncludes.h"
#include "ExperimentTypedefs.h"

void sigKmeansCluster(vector<SVector<bool>*> &vectors, const string& clusterFile) {
    // Define the types we want to use
    int k = 36;
    int maxiters = 10;
    KMeans_t clusterer(k);
    clusterer.setMaxIters(maxiters);
    {
        boost::timer::auto_cpu_timer all;
        cout << "clustering " << vectors.size() << " vectors into " << k
                << " clusters using kmeans with maxiters = " << maxiters
                << std::endl;
        vector<Cluster<vecType>*>& clusters = clusterer.cluster(vectors);
        cout << "cluster count = " << clusters.size() << std::endl;
        cout << "RMSE = " << clusterer.getRMSE() << std::endl;
        cout << "writing clustering results to " << clusterFile << endl;
        std::ofstream ofs(clusterFile);
        for (size_t i = 0; i < clusters.size(); ++i) {
            for (SVector<bool>* vector : clusters[i]->getNearestList()) {
                ofs << vector->getID() << " " << i << endl;
            }
        }
    }
}

void sigTSVQCluster(vector<SVector<bool>*> &vectors) {
    // EMTree
    int depth = 3;
    int iters = 2;
    vector<int> nodeSizes = {10};
    for (int m : nodeSizes) {
        std::cout << "-------------------" << std::endl;
        TSVQ_t tsvq(m, depth, iters);
        boost::timer::auto_cpu_timer all;
        tsvq.cluster(vectors);
        tsvq.printStats();
        std::cout << std::endl << "TSVQ order = " << m << std::endl;
    }
    std::cout << "-------------------" << std::endl;
}

void sigEMTreeCluster(vector<SVector<bool>*> &vectors) {
    // EMTree
    int depth = 3;
    int iters = 2;
    vector<int> nodeSizes = {30}; //{10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    for (int m : nodeSizes) {
        std::cout << "-------------------" << std::endl;
        EMTree_t emt(m);
        deque<int> splits;
        for (int i = 0; i < depth - 1; i++) {
            splits.push_back(m);
        }

        boost::timer::auto_cpu_timer all;
        {
            boost::timer::auto_cpu_timer seed("\nseeding tree: %w seconds\n");
            bool updateMeans = false;
            emt.seed(vectors, splits, updateMeans);
            emt.printStats();
        }
        for (int i = 0; i < iters; ++i) {
            {
                //std::cout << i << std::endl;
                boost::timer::auto_cpu_timer loop("iteration: %w seconds\n");
                emt.EMStep();
                emt.printStats();
                //std::cout << std::endl;
            }
        }

        emt.printStats();
        std::cout << std::endl << "EM-tree order = " << m << std::endl;
    }
    std::cout << "-------------------" << std::endl;
    /*
    for (int i=0; i<data.size(); i++) {
            kt.add(data[i]);

    }*/

}

void sigKTreeCluster(vector<SVector<bool>*> &vectors) {
    ClusterCounter<Node<SVector<bool>>> counter;

    // KTree algorithm
    int order = 10;
    int maxiters = 2;
    KTree_t kt(order, maxiters);

    boost::timer::auto_cpu_timer t;

    string input = "";

    for (int i = 0; i < vectors.size(); i++) {
        kt.add(vectors[i]);

        //cout << "\n";
        //kt.printStats();
        //cout << "\n";

        //getline(std::cin, input);
    }

    // Cluster and get the clusters
    //vector<Cluster<vecType>*> &clusters = km.cluster(data);

    //t.stop();
    //t.report();

    kt.visit(counter);

    cout << "\nCluster count: " << counter.getCount();

    cout << "\nFinished inserting ... \n";
    kt.printStats();


    kt.rearrange();

    cout << "\nFinished re-arranging ...\n";
    kt.printStats();

    //kt.rebuildInternal();

    //kt.EMStep();

    //cout << "\nFinished rebuilding internal ... No pruning ...\n";

    //kt.printStats();

    /*
    for (int i=0; i<75; i++) {
            kt.EMStep();
            kt.printStats();
    }*/


    cout << "\n";
    t.report();

    cout << "\n\n";

    // Clean up data
    Utils::purge(vectors);

}

// returns top half of dimensions

set<int> dimensionHistogram(vector<SVector<bool>*>& vectors, int dims) {
    cout << "calculating dimension histogram" << endl;
    vector<int> histogram(dims, 0);
    int count = 0;
    for (SVector<bool>* v : vectors) {
        if (++count % 1000 == 0) {
            std::cout << "." << std::flush;
        }
        if (count % 100000 == 0) {
            std::cout << count << std::flush;
        }
        for (size_t i = 0; i < histogram.size(); ++i) {
            if (v->at(i) == 1) {
                histogram[i] += 1;
            } else {
                histogram[i] -= 1;
            }
        }
    }
    cout << endl;

    struct Dimension {
        size_t index;
        int weight;

        bool operator<(const Dimension& rhs) const {
            return weight < rhs.weight;
        }
    };
    vector<Dimension> sorted;
    for (size_t i = 0; i < histogram.size(); ++i) {
        Dimension d = {i, abs(histogram[i])};
        sorted.push_back(d);
    }
    std::sort(sorted.begin(), sorted.end());
    //std::cout << std::endl;
    //for (Dimension d : sorted) {
    //    std::cout << d.weight << std::endl;
    //}
    set<int> topbits;
    for (size_t i = 0; i < (dims / 4); ++i) {
        topbits.insert(sorted[i].index);
    }
    return topbits;
}

void reduceDims(const set<int>& topbits, vector<SVector<bool>*>& vectors,
        vector<SVector<bool>*>& reducedVectors) {
    const size_t reducedDims = topbits.size();
    for (auto vector : vectors) {
        SVector<bool>* reducedVector = new SVector<bool>(reducedDims);
        reducedVector->setID(vector->getID());
        reducedVector->setAllBlocks(0);
        size_t reducedDim = 0;
        for (int dim : topbits) {
            if (vector->at(dim) == 1) {
                reducedVector->set(reducedDim);
            }
            ++reducedDim;
        }
        reducedVectors.push_back(reducedVector);
    }
}

void testHistogram(vector<SVector<bool>*>& vectors) {
    if (!vectors.empty()) {
        vector < SVector<bool>*> subset;
        int dims = vectors[0]->size();
        set<int> topbits;
        {
            boost::timer::auto_cpu_timer seed("calculating histogram: %w seconds\n");
            topbits = dimensionHistogram(vectors, dims);
        }
        loadSubset(vectors, subset, "data/inex_xml_mining_subset_2010.txt");
        vector < SVector<bool>*> reducedSubset;
        cout << "reducing dimensionality to " << topbits.size() << endl;
        reduceDims(topbits, subset, reducedSubset);
        sigKmeansCluster(subset, "data/fulldim_clusters.txt");
        sigKmeansCluster(reducedSubset, "data/reduceddim_clusters.txt");
    }
}

#endif	/* GENERALEXPERIMENTS_H */