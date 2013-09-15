// EMTree.cpp : Defines the entry point for the console application.
//

// For threadpool
#define TIME_UTC_ TIME_UTC

#include "Funcs.h"
#include "SVector.h"
#include "Cluster.h"
#include "Clusterer.h"
#include "Seeder.h"
#include "DSquaredSeeder.h"
#include "RandomSeeder.h"
#include "VectorGenerator.h"
#include "KMeans.h"
#include "BitMapList8.h"
#include "StdIncludes.h"

#include "EMTree.h"
#include "KTree.h"
#include "TSVQ.h"

#include <set>
using std::set;

void sigKmeansCluster(vector<SVector<bool>*> &vectors, const string& clusterFile) {
    // Define the types we want to use
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef RandomSeeder<vecType> seederType;
    //typedef DSquaredSeeder<vecType, distanceType > seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;
    int k = 36;
    int maxiters = 10;
    clustererType clusterer(k);
    clusterer.setMaxIters(maxiters);
    {
        boost::timer::auto_cpu_timer all;
        cout << "clustering " << vectors.size() << " vectors into " << k
                << " clusters using kmeans with maxiters = " << maxiters
                << std::endl;
        vector<Cluster<vecType>*>& clusters = clusterer.cluster(vectors);
        cout << "cluster count = " << clusters.size() << std::endl;
        cout << "RMSE = " << clusterer.getRMSE(vectors) << std::endl;
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
    // Define the types we want to use
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef RandomSeeder<vecType> seederType;
    //typedef DSquaredSeeder<vecType, distanceType > seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;

    // EMTree
    int depth = 3;
    int iters = 2;
    vector<int> nodeSizes = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    for (int m : nodeSizes) {
        std::cout << "-------------------" << std::endl;
        TSVQ<vecType, clustererType, distanceType, protoType> tsvq(m, depth, iters);
        boost::timer::auto_cpu_timer all;
        tsvq.cluster(vectors);
        tsvq.printStats();
        std::cout << std::endl << "TSVQ order = " << m << std::endl;
    }
    std::cout << "-------------------" << std::endl;
}

void sigEMTreeCluster(vector<SVector<bool>*> &vectors) {
    // Define the types we want to use
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef RandomSeeder<vecType> seederType;
    //typedef DSquaredSeeder<vecType, distanceType > seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;

    // EMTree
    int depth = 3;
    int iters = 10;
    vector<int> nodeSizes = {10}; //{10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    for (int m : nodeSizes) {
        std::cout << "-------------------" << std::endl;
        EMTree<vecType, clustererType, distanceType, protoType> emt(m);

        boost::timer::auto_cpu_timer all;
        {
            boost::timer::auto_cpu_timer seed("\nseeding tree: %w seconds\n");
            emt.seed(vectors, depth);
            emt.printStats();
        }
        for (int i = 0; i < iters; ++i) {
            {
                //std::cout << i << std::endl;
                //boost::timer::auto_cpu_timer loop("iteration: %w seconds\n");
                emt.EMStep();
                //emt.printStats();
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

    // Define the types we want to use
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef Node<vecType> nodeType;
    //typedef DSquaredSeeder<vecType, distanceType > seederType;
    typedef RandomSeeder<vecType> seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;

    ClusterCounter<nodeType> counter;

    // KTree algorithm
    int order = 100;
    int maxiters = 2;
    KTree<vecType, clustererType, distanceType, protoType> kt(order, maxiters);

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

void readSignatures(vector<SVector<bool>*> &vectors, string docidFile, string signatureFile,
        size_t sigSize, size_t maxVectors) {
    using namespace std;
    cout << docidFile << endl << signatureFile << endl;

    // setup streams
    const size_t numBytes = sigSize / 8;
    char *data = new char[numBytes];
    ifstream docidStream(docidFile);
    ifstream sigStream(signatureFile, ios::in | ios::binary);
    string docid;
    if (!docidStream || !sigStream) {
        cout << "unable to open file" << endl;
        return;
    }

    // read data
    while (getline(docidStream, docid)) {
        sigStream.read(data, numBytes);
        SVector<bool>* vector = new SVector<bool>(data, sigSize);
        vector->setID(docid);
        vectors.push_back(vector);
        if (vectors.size() % 1000 == 0) {
            cout << "." << flush;
        }
        if (vectors.size() % 100000 == 0) {
            cout << vectors.size() << flush;
        }
        if (maxVectors != -1 && vectors.size() == maxVectors) {
            break;
        }
    }
    cout << endl << vectors.size() << endl;
    delete[] data;
}

void loadWikiSignatures(vector<SVector<bool>*>& vectors, int veccount) {
    string docidFile = "data/wiki.4096.docids";
    string signatureFile = "data/wiki.4096.sig";
    int dims = 4096;
    readSignatures(vectors, docidFile, signatureFile, dims, veccount);
}

void loadSubset(vector<SVector<bool>*>& vectors, vector<SVector<bool>*>& subset,
        string docidFile) {
    using namespace std;
    ifstream docidStream(docidFile);
    string docid;
    set<string> docids;
    while (getline(docidStream, docid)) {
        docids.insert(docid);
    }
    for (SVector<bool>* vector : vectors) {
        if (docids.find(vector->getID()) != docids.end()) {
            subset.push_back(vector);
        }
    }
    cout << "filtered " << subset.size() << " vectors to create the INEX 2010 XML Mining subset" << endl;
}

void genData(vector<SVector<bool>*> &vectors, size_t sigSize, size_t numVectors) {

    // Setup Beroulli random number generator
    RND_ENG eng((unsigned int) std::time(0));
    RND_BERN bd(0.5);
    RND_BERN_GEN_01 gen(eng, bd);

    // Define the types we want to use
    typedef SVector<bool> vecType;

    typedef VectorGenerator<RND_BERN_GEN_01, vecType> vecGenerator;

    // Create the seeder
    //seederType seeder;

    // Data.
    vecGenerator::genVectors(vectors, numVectors, gen, sigSize);
}

void testReadVectors() {
    vector < SVector<bool>*> vectors;
    loadWikiSignatures(vectors, 100 * 1000);
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

void testMeanVersusNNSpeed(vector<SVector<bool>*>& vectors) {
    if (!vectors.empty()) {
        const int dims = vectors[0]->size();
        SVector<bool>* mean = new SVector<bool>(dims);
        vector<int> weights;
        {
            boost::timer::auto_cpu_timer time("calculating mean: %w seconds\n");
            meanBitPrototype2 proto;
            proto(mean, vectors, weights);
        }
        uint64_t sum = 0;
        {
            boost::timer::auto_cpu_timer hammingTime("hamming distance: %w seconds\n");
            hammingDistance distance;
            for (auto vector : vectors) {
                sum += distance(mean, vector);
            }
        }
        cout << "global mean error = " << ((double) sum / vectors.size()) << endl;
    }
}

void journalPaperExperiments(vector<SVector<bool>*>& vectors) {
    //determine cost of operations
    if (true) {
        testMeanVersusNNSpeed(vectors);
    }

    // types for data
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef Node<vecType> nodeType;
    typedef RandomSeeder<vecType> seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;
    
    // run TSVQ vs EM-tree convergence
    if (true) {
        int depth = 3, m = 10;
        int iterRange = 10; // test RMSE at 1 to maxiters iterations

        //TSVQ
        if (true) {
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int maxiters = 1; maxiters <= iterRange; ++maxiters) {
                boost::timer::auto_cpu_timer all;
                srand(1234);
                TSVQ<vecType, clustererType, distanceType, protoType> tsvq(m, depth, maxiters);
                tsvq.cluster(vectors);
                all.stop();
                cout << "." << flush;
                clusters.push_back(tsvq.getClusterCount());
                rmse.push_back(tsvq.getRMSE());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            cout << endl;
            for (int i = 0; i < clusters.size(); ++i) {
                cout << clusters[i] << "," << rmse[i] << "," << seconds[i] << endl;
            }
        }

        //EM-tree
        if (true) {
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int maxiters = 1; maxiters <= iterRange; ++maxiters) {
                deque<int> splits;
                for (int i = 0; i < depth - 1; ++i) {
                    splits.push_back(m);
                }
                boost::timer::auto_cpu_timer all;
                srand(1234);
                EMTree<vecType, clustererType, distanceType, protoType> emt(m);
                // seeding does first iteration
                emt.seedSingleThreaded(vectors, splits);
                for (int i = 1; i < maxiters; ++i) {
                    emt.EMStep();
                }
                all.stop();
                cout << "." << flush;
                rmse.push_back(emt.getRMSE());
                clusters.push_back(emt.getClusterCount());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            cout << endl;
            for (int i = 0; i < clusters.size(); ++i) {
                cout << clusters[i] << "," << rmse[i] << "," << seconds[i] << endl;
            }
        }
    }
    

    // run TSVQ vs EM-tree streaming experiments
    if (false) {
        int depth = 4;
        vector<int> orders = {4,5,6,7,8,9,10};
        //vector<int> orders = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
        //vector<int> orders = {10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60}; //, 65, 70, 75, 80, 85, 90, 95, 100};

        //TSVQ
        if (true) {
            int maxiters = 2;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                boost::timer::auto_cpu_timer all;
                TSVQ<vecType, clustererType, distanceType, protoType> tsvq(m, depth, maxiters);
                tsvq.cluster(vectors);
                all.stop();
                cout << "." << flush;
                clusters.push_back(tsvq.getClusterCount());
                rmse.push_back(tsvq.getRMSE());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            cout << endl;
            for (int i = 0; i < clusters.size(); ++i) {
                cout << clusters[i] << "," << rmse[i] << "," << seconds[i] << endl;
            }
        }

        //EM-tree
        if (false) {
            int maxiters = 6;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                deque<int> splits;
                for (int i = 0; i < depth - 1; ++i) {
                    splits.push_back(m);
                }
                boost::timer::auto_cpu_timer all;
                EMTree<vecType, clustererType, distanceType, protoType> emt(m);
                // seeding does first iteration
                emt.seedSingleThreaded(vectors, splits);
                //double currentRMSE = emt.getRMSE();
                for (int i = 1; i < maxiters; ++i) {
                    //double lastRMSE = currentRMSE;
                    emt.EMStep();
                    //currentRMSE = emt.getRMSE();
                    //double delta = fabs(currentRMSE - lastRMSE);
                    //cout << i << "=" << delta << " " << flush;
                    //cout << "." << endl;
                    //if (delta < 1e-4) {
                    //    break;
                    //}
                }
                all.stop();
                cout << "." << flush;
                rmse.push_back(emt.getRMSE());
                clusters.push_back(emt.getClusterCount());
                double elapsed = all.elapsed().wall / 1e9;
                seconds.push_back(elapsed);
                //cout << " time=" << elapsed << " ..." << endl;
            }
            cout << endl;
            for (int i = 0; i < clusters.size(); ++i) {
                cout << clusters[i] << "," << rmse[i] << "," << seconds[i] << endl;
            }
        }
    }

    // run kmeans exerpiments
    if (true) {
        int maxiters = 1000;
        cout << "k-means maxiters=" << maxiters << endl;
        vector<double> rmse;
        vector<int> targetClusters = {24};//100, 225, 400, 625, 900};
        vector<int> clusters;
        vector<double> seconds;
        for (int k : targetClusters) {
            boost::timer::auto_cpu_timer all;
            clustererType kmeans;
            kmeans.setNumClusters(k);
            kmeans.setMaxIters(maxiters);
            int finalClusters = kmeans.cluster(vectors).size();
            all.stop();
            cout << "." << flush;
            clusters.push_back(finalClusters);
            rmse.push_back(kmeans.getRMSE(vectors));
            seconds.push_back(all.elapsed().wall / 1e9);
        }
        cout << endl;
        for (int i = 0; i < clusters.size(); ++i) {
            cout << clusters[i] << "," << rmse[i] << "," << seconds[i] << endl;
        }
    }

    // run K-tree experiments
    if (false) {
        // build tree
        vector<int> orders = {10000, 5000, 2500, 1000, 750, 500, 400, 300, 250, 200, 150, 100, 75, 50, 25};
        for (int m : orders) {
            const int maxiters = 1000;
            cout << "-----" << endl;
            cout << "Building K-tree of order m=" << m
                    << ", k-means maxiters=" << maxiters << endl;
            boost::timer::auto_cpu_timer all;
            KTree<vecType, clustererType, distanceType, protoType> kt(m, maxiters);
            {
                boost::timer::auto_cpu_timer insert("inserting vectors: %w seconds\n");
                for (size_t i = 0; i < vectors.size(); i++) {
                    kt.add(vectors[i]);
                    size_t next = i + 1;
                    if (next % 10000 == 0) {
                        cout << next << flush;
                    }
                    if (next % 1000 == 0) {
                        cout << "." << flush;
                    }
                }
            }
            cout << endl;
            kt.printStats();

            // rearrange tree by reinserting all vectors
            {
                boost::timer::auto_cpu_timer insert("rearranging vectors: %w seconds\n");
                kt.rearrange();
            }
            kt.printStats();
        }
    }
}

int main(int argc, char** argv) {
    std::srand(std::time(0));

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

    // run experiments
    if (!vectors.empty() && !subset.empty()) {
        journalPaperExperiments(subset);
        //sigKTreeCluster(vectors);
        //sigTSVQCluster(vectors);
        //sigEMTreeCluster(subset);
        //testHistogram(vectors);
        //testMeanVersusNNSpeed(vectors);
        //testReadVectors();
        //TestSigEMTree();
    } else {
        cout << "error - vectors or subset empty" << endl;
    }
    return EXIT_SUCCESS;
}

