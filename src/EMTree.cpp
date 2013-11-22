// EMTree.cpp : Defines the entry point for the console application.
//

#include "Distance.h"
#include "Prototype.h"
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
#include "SVectorStream.h"
#include "Optimizer.h"

#include "EMTree.h"
#include "StreamingEMTree.h"
#include "KTree.h"
#include "TSVQ.h"

#include "ExperimentTypedefs.h"
#include "CreateSignatures.h"
#include "StreamingEMTreeExperiments.h"
#include "JournalPaperExperiments.h"
#include "GeneralExperiments.h"

int main(int argc, char** argv) {
    std::srand(std::time(0));
    
    if (false) {
        streamingEMTree();
    } else if (false) {
        clueweb();
    } else {
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
            //sigKmeansCluster(subset, "subset_clusters.txt");
            //journalPaperExperiments(subset);
            sigKTreeCluster(subset);
            //sigTSVQCluster(subset);
            //sigEMTreeCluster(subset);
            //testHistogram(vectors);
            //testMeanVersusNNSpeed(vectors);
            //testReadVectors();
            //TestSigEMTree();
        } else {
            cout << "error - vectors or subset empty" << endl;
        }
        
        for (auto v : vectors) {
            delete v;
        }
    }
    
    return EXIT_SUCCESS;
}

  
