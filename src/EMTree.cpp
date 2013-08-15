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

#include "EMTree.h"
#include "KTree.h"


void sigEMTreeCluster(vector<SVector<bool>*> &vectors) {

	// Define the types we want to use
	typedef SVector<bool> vecType;
	typedef hammingDistance distanceType;
	typedef meanBitPrototype2 protoType;
	typedef RandomSeeder<vecType> seederType;
	//typedef DSquaredSeeder<vecType, distanceType > seederType;
	typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;

	// EMTree
	EMTree<vecType, clustererType, distanceType, protoType> emt(80, 1000);

	boost::timer::auto_cpu_timer t;
	
	string input = "";

	emt.seed(vectors);

	emt.printStats();

	/*
	for (int i=0; i<data.size(); i++) {
		kt.add(data[i]);

	}*/

}

void sigKTreeCluster(vector<SVector<bool>*> &vectors)  {

	// Define the types we want to use
	typedef SVector<bool> vecType;
	typedef hammingDistance distanceType;
	typedef meanBitPrototype2 protoType;
	typedef Node<vecType> nodeType;
	//typedef DSquaredSeeder<vecType, distanceType > seederType;
	typedef RandomSeeder<vecType> seederType;
	typedef KMeans<vecType, seederType, distanceType, protoType> clustererType;
	
	ClusterCounter<nodeType> counter;

	// KTree algorithm - order 5
	KTree<vecType, clustererType, distanceType, protoType> kt(40);

	boost::timer::auto_cpu_timer t;
	
	string input = "";

	for (int i=0; i<vectors.size(); i++) {
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

	kt.rebuildInternal();

	//kt.EMStep();

	cout << "\nFinished rebuilding internal ... No pruning ...\n";

	kt.printStats();

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
	size_t numBytes = sigSize/8;
	SVector<bool> *vec;

	cout << fileName;
	
	ifstream file ( fileName, ios::in|ios::binary);
	if (file.is_open())  {

		data = new char[numBytes];

		while(!file.eof() && vectors.size()<maxVectors) {
		
			file.read(data, numBytes);

			vec = new SVector<bool>(data, sigSize);	
			vectors.push_back(vec);

			if (vectors.size()%1000 == 0) cout << "\n" << vectors.size();
		}

		delete[] data;

		file.close();
	}  
	else cout << "Unable to open file";

}

void genData(vector<SVector<bool>*> &vectors, size_t sigSize, size_t numVectors) {

	// Setup Beroulli random number generator
	RND_ENG eng((unsigned int)std::time(0));
	RND_BERN bd(0.5);
	RND_BERN_GEN_01 gen(eng, bd);

	// Define the types we want to use
	typedef SVector<bool> vecType;

	typedef VectorGenerator<RND_BERN_GEN_01, vecType> vecGenerator;

	// Create the seeder
	//seederType seeder;

	// Data.
	vecGenerator::genVectors( vectors, numVectors, gen, sigSize);
}

void testReadVectors() {

	vector<SVector<bool>*> vectors;
	string filename = "C:/Data/wikisignatures/wiki.4096.sig";
	readSignatures(vectors, filename, 4096, 100 * 1000);

}


int main(int argc, char** argv[])  {

	vector<SVector<bool>*> vectors;
	string filename = "C:/Data/wikisignatures/wiki.4096.sig";
	
	readSignatures(vectors, filename, 4096, 400 * 1000);
	
	//sigKTreeCluster(vectors);
	sigEMTreeCluster(vectors);
	
	//testReadVectors();

	//TestSigEMTree();

	return 0;
}

