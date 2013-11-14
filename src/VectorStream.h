#ifndef VECTORSTREAM_H
#define	VECTORSTREAM_H

#include "StdIncludes.h"
#include "SVector.h"

using namespace std;

/**
 * The VectorStream concept provides a stream of vectors usually read from disk. 
 * 
 * The only operations supported are:
 * 
 * vector<SVector<T>*> VectorStream.read(size_t n)
 *      where n is the number of vectors to read
 *      stream has ended when returned data contains 0 vectors
 * 
 * VectorStream.free(vector<T*> data)
 *      frees the memory allocated by the stream
 * 
 * For example,
 *      BitVectorStream bvs(idFile, signatureFile);
 *      for (;;) {
 *          vector<SVector<bool>*> data = bvs.read(10000);
 *          if (data.empty()) {
 *              break;
 *          }
 *          process(data);
 *          bvs.free(data);
 *      }
 */

// Should this be a specialization of VectorStream<T> ?
class BitVectorStream {
public:
    /**
     * @param idFile An ASCII file with one object ID per line.
     * @param signatureFile A file of binary signatures containing as many
     *                      signatures as there are lines in idFile. 
     * @param signatureLength The length of a signature in bits.
     */
    BitVectorStream(const string& idFile, const string& signatureFile,
            const size_t signatureLength) : _buffer(signatureLength / 8, 0),
            _idStream(idFile),
            _signatureStream(signatureFile, ios::in | ios::binary),
            _signatureLength(signatureLength) {
                if (signatureLength % 64 != 0) {
                    throw new runtime_error("length is not divisible by 64");
                }
                if (!_idStream) {
                    throw new runtime_error("failed to open " + idFile);
                }
                if (!_signatureStream) {
                    throw new runtime_error("failed to open " + signatureFile);
                }
    }

    vector<SVector<bool>*> read(size_t n) {
        string id;
        vector < SVector<bool>*> data;
        while (getline(_idStream, id)) {
            _signatureStream.read(&_buffer[0], _buffer.size());
            SVector<bool>* vector = new SVector<bool>(&_buffer[0],
                    _signatureLength);
            vector->setID(id);
            data.push_back(vector);
            if (data.size() == n) {
                break;
            }
        }
        return data;
    }
    
    void free(vector<SVector<bool>*> data) {
        for (auto vector : data) {
            delete vector;
        }
    }
    
private:
    vector<char> _buffer; // temporary buffer for reading a signature
    ifstream _idStream;
    ifstream _signatureStream;
    size_t _signatureLength; // the length of signatures in _signatureStream
};

#endif	/* VECTORSTREAM_H */

