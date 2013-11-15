/**
 * This file contains helper functions for loading and generating signatures.
 */
#ifndef LOADSIGNATURES_H
#define	LOADSIGNATURES_H

#include "StdIncludes.h"

void genData(vector<SVector<bool>*> &vectors, size_t sigSize, size_t numVectors) {

    // Setup Beroulli random number generator
    RND_ENG eng((unsigned int) std::time(0));
    RND_BERN bd(0.5);
    RND_BERN_GEN_01 gen(eng, bd);

    // Define the types we want to use
    typedef VectorGenerator<RND_BERN_GEN_01, SVector<bool>> vecGenerator;

    // Create the seeder
    //seederType seeder;

    // Data.
    vecGenerator::genVectors(vectors, numVectors, gen, sigSize);
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
    constexpr char docidFile[] = "data/wiki.4096.docids";
    constexpr char signatureFile[] = "data/wiki.4096.sig";
    constexpr size_t signatureLength = 4096;
    readSignatures(vectors, docidFile, signatureFile, signatureLength, veccount);
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

void testReadVectors() {
    vector < SVector<bool>*> vectors;
    loadWikiSignatures(vectors, 100 * 1000);
}

#endif	/* LOADSIGNATURES_H */

