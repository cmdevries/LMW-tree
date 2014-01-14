#ifndef VECTORSTREAM_H
#define	VECTORSTREAM_H

#include "StdIncludes.h"
#include "SVector.h"

namespace lmw {

/**
 * The VectorStream concept provides a stream of vectors usually read from disk. 
 * 
 * The only operations supported are:
 * 
 * size_t VectorStream<T>.read(size_t n, vector<SVector<T>*>* data)
 *      where n is the number of vectors to read into data
 *      stream has ended when return == 0
 * 
 * VectorStream<T>.free(vector<T*>& data)
 *      frees the memory allocated by the stream
 * 
 * For example,
 *      VectorStream<bool> bvs(idFile, signatureFile);
 *      for (;;) {
 *          vector<SVector<bool>*> data;
 *          size_t read = bvs.read(10000, &data);
 *          if (read == 0) {
 *              break;
 *          }
 *          process(&data);
 *          bvs.free(&data);
 *      }
 */
template <typename SVECTOR>
class SVectorStream {
    size_t read(size_t n, vector<SVECTOR*>* data) {
        return 0;
    }
    
    void free(vector<SVECTOR*>* data) {
        
    }
};

template <>
class SVectorStream<SVector<bool>> {
public:
    /**
     * @param idFile An ASCII file with one object ID per line.
     * @param signatureFile A file of binary signatures containing as many
     *                      signatures as there are lines in idFile. 
     * @param signatureLength The length of a signature in bits.
     */
    SVectorStream(const string& idFile, const string& signatureFile,
            const size_t signatureLength) : _buffer(signatureLength / 8, 0),
            _idStream(idFile),
            _signatureStream(signatureFile, ios::in | ios::binary),
            _signatureLength(signatureLength),
			_maxToRead(-1),
			_count(0) {
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
	/**
	* @param idFile An ASCII file with one object ID per line.
	* @param signatureFile A file of binary signatures containing as many
	*                      signatures as there are lines in idFile.
	* @param signatureLength The length of a signature in bits.
	* @param maxToRead The maximum number of vectors to read. A value of -1 indicates
	* to read all.
	*/
	SVectorStream(const string& idFile, const string& signatureFile,
		const size_t signatureLength, const size_t maxToRead) : _buffer(signatureLength / 8, 0),
		_idStream(idFile),
		_signatureStream(signatureFile, ios::in | ios::binary),
		_signatureLength(signatureLength),
		_maxToRead(maxToRead),
		_count(0) {
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
    size_t read(size_t n, vector<SVector<bool>*>* data) {
        string id;
        size_t read = 0;
		if (_maxToRead != -1 && _count > (_maxToRead - 1)) return 0;
		while (getline(_idStream, id)) {
            _signatureStream.read(&_buffer[0], _buffer.size());
            SVector<bool>* vector = new SVector<bool>(&_buffer[0],
                    _signatureLength);
            vector->setID(id);
            data->push_back(vector);
			++_count;
			if (_maxToRead != -1 && _count > (_maxToRead - 1)) break;
            if (++read == n) {
                break;
            }
        }
        return read;
    }
    
    void free(vector<SVector<bool>*>* data) {
        for (auto vector : *data) {
            delete vector;
        }
    }
    
private:
    vector<char> _buffer; // temporary buffer for reading a signature
    ifstream _idStream;
    ifstream _signatureStream;
    size_t _signatureLength; // the length of signatures in _signatureStream
	size_t _maxToRead;
	size_t _count; // Number of vectors read so far
};

} // namespace lmw

#endif	/* VECTORSTREAM_H */

