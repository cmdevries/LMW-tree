#ifndef JOURNALPAPEREXPERIMENTS_H
#define	JOURNALPAPEREXPERIMENTS_H

#include "lmw/StdIncludes.h"
#include "ExperimentTypedefs.h"

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
        {
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
        {
            uint64_t sum = 0;
            {
                boost::timer::auto_cpu_timer hammingTime("hamming distance: %w seconds\n");
                hammingDistance distance;
                for (auto vector : vectors) {
                    sum += distance(vectors[0], vector);
                }
            }
            cout << "global error to first vector = " << ((double) sum / vectors.size()) << endl;
        }
    }
}

void print_results(const vector<vector<double>>& all_rmse,
            const vector<vector<int>>& all_clusters,
            const vector<vector<double>>& all_seconds) {
    int examples = all_clusters[0].size();
    int samples = all_clusters.size();
    
    // print header
    for (int i = 0; i < samples; ++i) {
        cout << "clusters,";
    }
    for (int i = 0; i < samples; ++i) {
        cout << "rmse,";
    }
    for (int i = 0; i < samples; ++i) {
        cout << "seconds,";
    }
    cout << endl;
    
    // print body
    for (int j = 0; j < examples; ++j) {
        for (int i = 0; i < samples; ++i) {
            cout << all_clusters[i][j] << ",";
        }
        for (int i = 0; i < samples; ++i) {
            cout << all_rmse[i][j] << ",";
        }
        for (int i = 0; i < samples; ++i) {
            cout << all_seconds[i][j] << ",";
        }
        cout << endl;
    }
}

void journalPaperExperiments(vector<SVector<bool>*>& vectors) {
    //determine cost of operations
    if (true) {
        testMeanVersusNNSpeed(vectors);
    }
    
    // global experimental parameters
    const int trials = 20;
    
    // run TSVQ vs EM-tree convergence
    if (false) {
        int depth = 3, m = 10;
        int iterRange = 20; // test RMSE at 1 to maxiters iterations

        //TSVQ
        if (false) {
            cout << "TSVQ convergence" << endl;
            vector<vector<int>> all_clusters;
            vector<vector<double>> all_rmse;
            vector<vector<double>> all_seconds;
            for (int i = 0; i < trials; ++i) {
                cout << i + 1 << flush;
                vector<int> clusters;
                vector<double> rmse;
                vector<double> seconds;
                for (int maxiters = 0; maxiters <= iterRange; ++maxiters) {
                    boost::timer::auto_cpu_timer all;
                    TSVQ_t tsvq(m, depth, maxiters);
                    tsvq.cluster(vectors);
                    all.stop();
                    cout << "." << flush;
                    clusters.push_back(tsvq.getClusterCount());
                    rmse.push_back(tsvq.getRMSE());
                    seconds.push_back(all.elapsed().wall / 1e9);
                }
                all_clusters.push_back(clusters);
                all_rmse.push_back(rmse);
                all_seconds.push_back(seconds);
                cout << endl;
            }
            print_results(all_rmse, all_clusters, all_seconds);
        }

        //EM-tree
        if (true) {
            cout << "EM-tree convergence" << endl;
            vector < vector<int >> all_clusters;
            vector < vector<double >> all_rmse;
            vector < vector<double >> all_seconds;
            for (int i = 0; i < trials; ++i) {
                cout << i + 1 << flush;
                vector<double> rmse;
                vector<int> clusters;
                vector<double> seconds;
                for (int maxiters = 0; maxiters <= iterRange; ++maxiters) {
                    deque<int> splits;
                    for (int i = 0; i < depth - 1; ++i) {
                        splits.push_back(m);
                    }
                    boost::timer::auto_cpu_timer all;
                    EMTree_t emt(m);
                    if (maxiters == 0) {
                        // no iterations so dont update means
                        bool updateMeans = false;
                        emt.seed(vectors, splits, updateMeans);
                    } else {
                        // seeding does first iteration
                        emt.seed(vectors, splits);
                    }
                    for (int i = 1; i < maxiters; ++i) {
                        emt.EMStep();
                    }
                    all.stop();
                    cout << "." << flush;
                    rmse.push_back(emt.getRMSE());
                    clusters.push_back(emt.getClusterCount());
                    seconds.push_back(all.elapsed().wall / 1e9);
                }
                all_clusters.push_back(clusters);
                all_rmse.push_back(rmse);
                all_seconds.push_back(seconds);
                cout << endl;
            }
            print_results(all_rmse, all_clusters, all_seconds);
        }
    }


    // run TSVQ vs EM-tree streaming experiments
    if (false) {
        int depth = 4;

        //TSVQ
        if (false) {
            vector<int> orders = {4, 5, 6, 7, 8, 9, 10};
            int maxiters = 2;
            cout << "TSVQ maxiters = " << maxiters << endl;
            vector < vector<int >> all_clusters;
            vector < vector<double >> all_rmse;
            vector < vector<double >> all_seconds;
            for (int i = 0; i < trials; ++i) {
                cout << i + 1 << flush;
                vector<double> rmse;
                vector<int> clusters;
                vector<double> seconds;
                for (int m : orders) {
                    boost::timer::auto_cpu_timer all;
                    TSVQ_t tsvq(m, depth, maxiters);
                    tsvq.cluster(vectors);
                    all.stop();
                    cout << "." << flush;
                    clusters.push_back(tsvq.getClusterCount());
                    rmse.push_back(tsvq.getRMSE());
                    seconds.push_back(all.elapsed().wall / 1e9);
                }
                all_clusters.push_back(clusters);
                all_rmse.push_back(rmse);
                all_seconds.push_back(seconds);
                cout << endl;
            }
            print_results(all_rmse, all_clusters, all_seconds);
        }

        //EM-tree
        if (true) {
            vector<int> orders = {4, 5, 6, 7, 8, 10, 12};
            int maxiters = 6;
            cout << "EM-tree maxiters = " << maxiters << endl;
            vector < vector<int >> all_clusters;
            vector < vector<double >> all_rmse;
            vector < vector<double >> all_seconds;
            for (int i = 0; i < trials; ++i) {
                cout << i + 1 << flush;
                vector<double> rmse;
                vector<int> clusters;
                vector<double> seconds;
                for (int m : orders) {
                    deque<int> splits;
                    for (int i = 0; i < depth - 1; ++i) {
                        splits.push_back(m);
                    }
                    boost::timer::auto_cpu_timer all;
                    EMTree_t emt(m);
                    // seeding does first iteration
                    emt.seed(vectors, splits);
                    for (int i = 1; i < maxiters; ++i) {
                        emt.EMStep();
                    }
                    all.stop();
                    cout << "." << flush;
                    rmse.push_back(emt.getRMSE());
                    clusters.push_back(emt.getClusterCount());
                    double elapsed = all.elapsed().wall / 1e9;
                    seconds.push_back(elapsed);
                    //cout << " time=" << elapsed << " ..." << endl;
                }
                all_clusters.push_back(clusters);
                all_rmse.push_back(rmse);
                all_seconds.push_back(seconds);
                cout << endl;
            }
            print_results(all_rmse, all_clusters, all_seconds);
        }
    }

    // TSVQ all cluster sizes
    if (false) {
        cout << "TSVQ varying clusters" << endl;
        vector<int> targetClusters = {230, 310, 470, 590, 810, 1010, 1300, 1800, 2900, 4000};        
        vector<int> orders;
        for (int target : targetClusters) {
            orders.push_back((int)sqrt(target));
        }
        int maxiters = 10;
        int depth = 3;
        cout << "maxiters = " << maxiters << endl;
        vector < vector<int >> all_clusters;
        vector < vector<double >> all_rmse;
        vector < vector<double >> all_seconds;
        for (int i = 0; i < trials; ++i) {
            cout << i + 1 << flush;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                boost::timer::auto_cpu_timer all;
                TSVQ_t tsvq(m, depth, maxiters);
                tsvq.cluster(vectors);
                all.stop();
                cout << "." << flush;
                clusters.push_back(tsvq.getClusterCount());
                rmse.push_back(tsvq.getRMSE());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            all_clusters.push_back(clusters);
            all_rmse.push_back(rmse);
            all_seconds.push_back(seconds);
            cout << endl;
        }
        print_results(all_rmse, all_clusters, all_seconds);
    }    

    // EM-tree all cluster sizes
    if (false) {
     // vector<int> targetClusters = {230, 310, 470, 590, 810, 1010, 1300, 1800, 2900, 4000};
        vector<int> targetClusters = {240, 350, 550, 710, 1100, 1550, 2200, 3800, 7500, 11500};
        vector<int> orders;
        for (int target : targetClusters) {
            orders.push_back((int) sqrt(target));
        }
        int depth = 3;
        int maxiters = 10;
        cout << "EM-tree all cluster sizes maxiters = " << maxiters << endl;
        vector < vector<int >> all_clusters;
        vector < vector<double >> all_rmse;
        vector < vector<double >> all_seconds;
        for (int i = 0; i < trials; ++i) {
            cout << i + 1 << flush;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                deque<int> splits;
                for (int i = 0; i < depth - 1; ++i) {
                    splits.push_back(m);
                }
                boost::timer::auto_cpu_timer all;
                EMTree_t emt(m);
                // seeding does first iteration
                emt.seed(vectors, splits);
                for (int i = 1; i < maxiters; ++i) {
                    emt.EMStep();
                }
                all.stop();
                cout << "." << flush;
                rmse.push_back(emt.getRMSE());
                clusters.push_back(emt.getClusterCount());
                double elapsed = all.elapsed().wall / 1e9;
                seconds.push_back(elapsed);
            }
            all_clusters.push_back(clusters);
            all_rmse.push_back(rmse);
            all_seconds.push_back(seconds);
            cout << endl;
        }
        print_results(all_rmse, all_clusters, all_seconds);
    }
    

    // run kmeans exerpiments
    if (false) {
        int maxiters = 10;
        vector < vector<int >> all_clusters;
        vector < vector<double >> all_rmse;
        vector < vector<double >> all_seconds;
        cout << "running k-means experiments" << endl;
        for (int i = 0; i < trials; ++i) {
            cout << i + 1 << flush;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            //vector<int> targetClusters = {230, 310, 470, 590, 810, 1010};
            vector<int> targetClusters = {1300, 1800, 2900, 4000};
            for (int k : targetClusters) {
                boost::timer::auto_cpu_timer all;
                KMeans_t kmeans(k);
                kmeans.setMaxIters(maxiters);
                int finalClusters = kmeans.cluster(vectors).size();
                all.stop();
                cout << "." << flush;
                clusters.push_back(finalClusters);
                rmse.push_back(kmeans.getRMSE());
                seconds.push_back(all.elapsed().wall / 1e9);
                cout << all.elapsed().wall / 1e9 << flush;
            }
            all_clusters.push_back(clusters);
            all_rmse.push_back(rmse);
            all_seconds.push_back(seconds);
            cout << endl;
        }
        print_results(all_rmse, all_clusters, all_seconds);
    }

    // run K-tree experiments
    if (false) {
        // build tree
        const int maxiters = 10;
        vector<int> orders = {1000, 750, 500, 400, 300, 250, 200, 150, 100, 75};
        vector < vector<int >> all_clusters;
        vector < vector<double >> all_rmse;
        vector < vector<double >> all_seconds;
        for (int i = 0; i < trials; ++i) {
            cout << i + 1 << flush;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                cout << "-----" << endl;
                cout << "Building K-tree of order m=" << m
                        << ", k-means maxiters=" << maxiters << endl;
                boost::timer::auto_cpu_timer all;
                KTree_t kt(m, maxiters);
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
                cout << endl;
                kt.rearrange();
                all.stop();
                rmse.push_back(kt.getRMSE());
                clusters.push_back(kt.getClusterCount());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            all_clusters.push_back(clusters);
            all_rmse.push_back(rmse);
            all_seconds.push_back(seconds);
            cout << endl;
        }
        print_results(all_rmse, all_clusters, all_seconds);
    }
    
    // run K-tree delayed updates experiments
    if (true) {
        // build tree
        const int maxiters = 10;
        vector<int> orders = {1000, 750, 500, 400, 300, 250, 185, 138, 85, 65};
        vector < vector<int >> all_clusters;
        vector < vector<double >> all_rmse;
        vector < vector<double >> all_seconds;
        for (int i = 0; i < trials; ++i) {
            cout << i + 1 << flush;
            vector<double> rmse;
            vector<int> clusters;
            vector<double> seconds;
            for (int m : orders) {
                cout << "-----" << endl;
                cout << "Building DELAYED UPDATES delay=1000 K-tree of order m=" << m
                        << ", k-means maxiters=" << maxiters << endl;
                boost::timer::auto_cpu_timer all;
                KTree_t kt(m, maxiters);
                kt.setDelayedUpdates(true);
                kt.setUpdateDelay(1000);
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
                cout << endl;
                kt.rearrange();
                all.stop();
                rmse.push_back(kt.getRMSE());
                clusters.push_back(kt.getClusterCount());
                seconds.push_back(all.elapsed().wall / 1e9);
            }
            all_clusters.push_back(clusters);
            all_rmse.push_back(rmse);
            all_seconds.push_back(seconds);
            cout << endl;
        }
        print_results(all_rmse, all_clusters, all_seconds);
    }    
}

void clueweb() {    
    // load vectors
    string docidFile = "data/clueweb.4096.docids";
    string signatureFile = "data/clueweb.4096.signatures";
    int dims = 4096;
    int veccount = -1;
    vector < SVector<bool>*> vectors;
    {
        boost::timer::auto_cpu_timer load("loading document vectors %w seconds\n");
        readSignatures(vectors, docidFile, signatureFile, dims, veccount);
    }

    // k-tree
    if (true) {
        // build tree
        int m = 1000, maxiters = 10;
        cout << "-----" << endl;
        cout << "Building K-tree of order m=" << m
                << ", k-means maxiters=" << maxiters << endl;
        boost::timer::auto_cpu_timer all;
        boost::timer::auto_cpu_timer building;
        KTree_t kt(m, maxiters);
        kt.setDelayedUpdates(true);
        kt.setUpdateDelay(1000);
        for (size_t i = 0; i < vectors.size(); i++) {
            kt.add(vectors[i]);
            size_t next = i + 1;
            if (next % 1000000 == 0) {
                cout << next << flush;
            }
            if (next % 10000 == 0) {
                cout << "." << flush;
            }
        }
        cout << endl;
        building.stop();
        cout << "Building K-tree took " << building.elapsed().wall / 1e9 << " seconds" << endl;
        
        // rearrange leaves
        boost::timer::auto_cpu_timer rearranging;
        cout << "Rearranging K-tree" << endl;
        kt.rearrange();
        cout << "Rearranging K-tree took " << rearranging.elapsed().wall / 1e9 << " seconds" << endl;
        
        // print stats
        all.stop();
        kt.printStats();        
        double seconds = all.elapsed().wall / 1e9;
        cout << "K-tree took " << seconds << " seconds" << endl;
    }
    
    // EM-tree
    if (false) {
        int maxiters = 4;
        int clusters = 110000;
        int m = (int)sqrt(clusters);        
        deque<int> splits = {m, m};
        boost::timer::auto_cpu_timer all;
        EMTree_t emt(m);
        // seeding does first iteration
        {
            cout << "----" << endl;
            cout << "iteration 0" << endl;
            {
                boost::timer::auto_cpu_timer seeding("seeding %w seconds\n");
                const bool updateMeans = false;
                emt.seed(vectors, splits, updateMeans);
            }
            {
                boost::timer::auto_cpu_timer seeding("printing statistics %w seconds\n");            
                emt.printStats();            
            }
            cout << "----" << endl;
            cout << "iteration 1" << endl;
            {
                boost::timer::auto_cpu_timer seeding("updating means %w seconds\n");
                emt.rearrangeInternal();
            }
            {
                boost::timer::auto_cpu_timer seeding("printing statistics %w seconds\n");            
                emt.printStats();            
            }            
        }
        // iterate until maxiters
        for (int i = 1; i < maxiters; ++i) {
            cout << "----" << endl;
            cout << "iteration " << i + 1 << endl;
            {
                boost::timer::auto_cpu_timer iter("emtree iteration %w seconds\n");
                emt.EMStep();
            }
            {
                boost::timer::auto_cpu_timer seeding("printing statistics %w seconds\n");            
                emt.printStats();            
            }
        }
    }
    
    // TSVQ EM-tree hybrid
    if (false) {
        // record time for all operations
        boost::timer::auto_cpu_timer all;
        
        // sample data
        int sampleSize = 2000000;
        vector < SVector<bool>*> sample = vectors;
        random_shuffle(sample.begin(), sample.end());
        sample.resize(sampleSize);
                
        // build TSVQ on sample
        int clusters = 110000;
        int m = (int)sqrt(clusters);        
        int depth = 3;
        int tsvqMaxiters = 5;        
        boost::timer::auto_cpu_timer tsvqTimer;
        TSVQ_t tsvq(m, depth, tsvqMaxiters);
        tsvq.cluster(sample);
        tsvqTimer.stop();
        tsvq.printStats();
        cout << endl << "Building TSVQ on sample took " << tsvqTimer.elapsed().wall / 1e9 << " seconds" << endl;        
        cout << "--------" << endl;
        
        // 2 iterations of EM-tree on all data, using TSVQ sample as seed
        int emtreeMaxiters = 2;
        EMTree_t emtree(tsvq.getMWayTree());
        boost::timer::auto_cpu_timer emtreeTimer;
        {
            boost::timer::auto_cpu_timer iter;
            
            // place all data into TSVQ initialized tree
            emtree.replace(vectors);
            cout << "placed all points into TSVQ tree" << endl;
            emtree.printStats();
            cout << endl << "--------" << endl;
            
            // prune
            int pruned = 1;
            while (pruned > 0) {
                pruned = emtree.prune();
            }
            
            // update means
            emtree.rebuildInternal();
            
            // print stats
            iter.stop();
            cout << "iteration 1 took " << iter.elapsed().wall / 1e9 << " seconds" << endl;
            emtree.printStats();
            cout << endl << "--------" << endl;
        }
        for (int i = 1; i < emtreeMaxiters; ++i) {
            boost::timer::auto_cpu_timer iter;
            emtree.EMStep();
            iter.stop();
            cout << "iteration " << i + 1 << " took " << iter.elapsed().wall / 1e9 << " seconds" << endl;
            emtree.printStats();
            cout << "--------" << endl;                    
        }
        emtreeTimer.stop();
        emtree.printStats();
        cout << endl << emtreeMaxiters << " iterations of EM-tree took " << emtreeTimer.elapsed().wall / 1e9 << " seconds" << endl;        
        
        // report all time
        all.stop();
        cout << "all operations took " << all.elapsed().wall / 1e9 << " seconds" << endl;        
    }
}

#endif	/* JOURNALPAPEREXPERIMENTS_H */

