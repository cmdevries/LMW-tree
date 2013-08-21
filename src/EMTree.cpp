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

void sigTSVQCluster(vector<SVector<bool>*> &vectors) {

    // Define the types we want to use
    typedef SVector<bool> vecType;
    typedef hammingDistance distanceType;
    typedef meanBitPrototype2 protoType;
    typedef RandomSeeder<vecType> seederType;
    //typedef DSquaredSeeder<vecType, distanceType > seederType;
    typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;

    // EMTree
    int depth = 5;
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
    /*
    for (int i=0; i<data.size(); i++) {
            kt.add(data[i]);

    }*/

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
    int depth = 5;
    int iters = 2;
    vector<int> nodeSizes = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    for (int m : nodeSizes) {
        std::cout << "-------------------" << std::endl;
        EMTree<vecType, clustererType, distanceType, protoType> emt(m, depth);

        boost::timer::auto_cpu_timer all;
        {
            boost::timer::auto_cpu_timer seed("\nseeding tree: %w seconds\n");
            emt.seed(vectors);
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
    int order = 80;
    KTree<vecType, clustererType, distanceType, protoType> kt(order);

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

void readSignatures(vector<SVector<bool>*> &vectors, string fileName, size_t sigSize, size_t maxVectors) {

    using namespace std;

    char *data;
    size_t numBytes = sigSize / 8;
    SVector<bool> *vec;

    cout << fileName;

    ifstream file(fileName, ios::in | ios::binary);
    if (file.is_open()) {

        data = new char[numBytes];

        while (!file.eof() && vectors.size() < maxVectors) {

            file.read(data, numBytes);

            vec = new SVector<bool>(data, sigSize);
            vectors.push_back(vec);

            if (vectors.size() % 1000 == 0) cout << "\n" << vectors.size();
        }

        delete[] data;

        file.close();
    }
    else cout << "Unable to open file";

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
    string filename = "C:/Data/wikisignatures/wiki.4096.sig";
    readSignatures(vectors, filename, 4096, 100 * 1000);

}

void printDimensionHistogram(vector<SVector<bool>*>& vectors, int dims) {
    vector<int> histogram(dims, 0);
    int count = 0;
    for (SVector<bool>* v : vectors) {
        if (++count % 1000 == 0) {
            std::cout << "." << std::flush;
        } else if (count % 100001 == 0) {
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
    std::cout << std::endl;
    for (Dimension d : sorted) {
        std::cout << d.weight << std::endl;
    }    
}

int main(int argc, char** argv) {

    vector < SVector<bool>*> vectors;
    string filename = "/Users/chris/LMW-tree/data/wiki.4096.sig";

    int dims = 4096;
    int veccount = 100000;
    readSignatures(vectors, filename, dims, veccount);

    //readSignatures(vectors, filename, 4096, 100000);
    
    printDimensionHistogram(vectors, dims);
    //sigKTreeCluster(vectors);
    //sigTSVQCluster(vectors);
    //sigEMTreeCluster(vectors);

    //testReadVectors();

    //TestSigEMTree();



    return 0;
}

