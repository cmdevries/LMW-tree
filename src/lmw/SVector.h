#ifndef SVECTOR_H
#define SVECTOR_H

#include "StdIncludes.h"

namespace lmw {

// Defines for bit vector
#define W_SIZE 64
#define BITS_WS 6
#define MASK 0x3f

// Typedef for bit vector
typedef uint64_t block_type;

template <class T>
class SVector {
protected:

    T* _data;
    size_t _length;
	string _id;

public:

    SVector(size_t length) {
        _length = length;
        _data = new T[_length];
    }

    SVector(SVector<T> &other) {
        _length = other._length;
        _data = new T[_length];
        for (size_t i = 0; i < _length; i++) {
            _data[i] = other._data[i];
        }
    }

    ~SVector() {
        delete[] _data;
    }

    typedef T * iterator;
    typedef const T * const_iterator;

    iterator begin() {
        return &_data[0];
    }

    iterator end() {
        return &_data[_length];
    }

    T& operator[](size_t i) {
        return at(i);
    }

    T& at(size_t i) {
        return _data[i];
    }

	void setID(const string& id) {
		_id = id;
	}

	const string& getID() {
		return _id;
	}

    void set(size_t i, T val) {
        _data[i] = val;
    }

    void setAll(T a) {
        // Maybe use memset here instead
        for (size_t i = 0; i < _length; i++) {
            _data[i] = a;
        }
    }

    void add(SVector &other) {

        for (size_t i = 0; i < _length; i++) {
            _data[i] = _data[i] + other._data[i];
        }
    }

    void addMult(SVector &other, float coef) {

        for (size_t i = 0; i < _length; i++) {
            _data[i] = _data[i] + other._data[i] * coef;
        }
    }

    void scale(T val) {

        for (size_t i = 0; i < _length; i++) {
            _data[i] = _data[i] * val;
        }
    }

    size_t size() {
        return _length;
    }

    void print() {
        for (size_t i = 0; i < _length; i++) {
            std::cout << _data[i] << " ";
        }
    }

};

// Template specialization for bit vector

template <>
class SVector <bool> {
protected:

    block_type* _data;

    int _numBlocks;
    size_t _length;
    string _id;

public:

    SVector(size_t length) {
        _length = length;
        _numBlocks = _length >> BITS_WS;
        _data = new block_type[_numBlocks];
    }

    SVector(char *bytes, size_t length) {
        size_t numBytes = length / 8;
        _length = length;
        _numBlocks = _length >> BITS_WS;
        _data = new block_type[_numBlocks];
        memcpy(_data, bytes, numBytes);
    }

    SVector(SVector<bool> &vec) {
        _length = vec._length;
        _numBlocks = vec._numBlocks;
        _data = new block_type[_numBlocks];

        // initialise bit vector
        for (int i = 0; i < _numBlocks; i++) {
            setBlock(i, vec._data[i]);
        }
    }

    SVector(SVector<bool> *vec) {
        _length = vec->_length;
        _numBlocks = vec->_numBlocks;
        _data = new block_type[_numBlocks];

        // initialise bit vector
        for (int i = 0; i < _numBlocks; i++) {
            setBlock(i, vec->_data[i]);
        }
    }

    ~SVector() {
        delete[] _data;
    }

    void setID(const string& id) {
        _id = id;
    }

    const string& getID() {
        return _id;
    }

    size_t size() {
        return _length;
    }

    size_t getNumBlocks() {
        return _numBlocks;
    }

    block_type* getData() {
        return _data;
    }

    void setAllBlocks(block_type v) {
        for (size_t i = 0; i < _numBlocks; i++) {
            _data[i] = v;
        }
    }

	// Be careful of the semantics of this method.
	// The block is or'd, not made equal to.
    void setBlock(int i, block_type b) {
        _data[i] |= b;
    }

    void set(int i) {
        _data[i >> BITS_WS] |= (1LL << (i & MASK));
    }

    int isSet(int i) {
        return ((_data[i >> BITS_WS] & (1LL << (i & MASK))) != 0);
    }

    int operator[](int i) {
        return at(i);
    }

    int at(int i) {
        return ((_data[i >> BITS_WS] & (1LL << (i & MASK))) != 0);
    }

    int popCount() {

        int count = 0;

        for (int i = 0; i < _numBlocks; i++) count += popcnt64(_data[i]);

        return count;
    }

    void exclusiveor(SVector<bool>& v1) {
        int count = 0;
        block_type t;
        for (int i = 0; i < _numBlocks; i++) {
            t = _data[i] ^ v1._data[i];
        }
    }

    int hammingDIstance(SVector<bool> &other) {
        int count = 0;
        block_type exclusiveor;
        for (int i = 0; i < _numBlocks; ++i) {
            exclusiveor = _data[i] ^ other._data[i];
            count += popcnt64(exclusiveor);
        }
        return count;
    }

    void mean(SVector<bool> *t1, vector<SVector<bool>*> &objs, vector<int> &weights) {

        float total = 0.0f;

        t1->setAllBlocks(0);

        int *bitCountPerDimension = new int[t1->size()];
        memset(bitCountPerDimension, 0, t1->size());

        int halfCount = 0;

        if (weights.size() != 0) {
            for (size_t t = 0; t < objs.size(); t++) {
                for (size_t s = 0; s < t1->size(); s++) {
                    bitCountPerDimension[s] += (objs[t]->at(s) * weights[t]);
                }
            }
            for (int w : weights) {
                halfCount += w;
            }
            halfCount /= 2;
        } else {
            for (size_t t = 0; t < objs.size(); t++) {
                for (size_t s = 0; s < t1->size(); s++) {
                    bitCountPerDimension[s] += (objs[t]->at(s));
                }
            }
            halfCount = objs.size() / 2;
        }
        for (size_t s = 0; s < t1->size(); s++) {
            if (bitCountPerDimension[s] > halfCount) t1->set(s);
        }
    }

    void invert() {
        for (int i = 0; i < _numBlocks; i++) ~_data[i];
    }

    void print() {
        size_t count = 0;

        for (int i = 0; i < _numBlocks; i++) {
            for (int j = W_SIZE - 1; j >= 0; j--) {
                if (_data[i] & (1LL << (j & MASK))) std::cout << '1';
                else std::cout << '0';
                count++;
                if (j % 16 == 0) std::cout << ' ';
            }
        }
    }

    static inline int popcnt64(block_type b64) {
#ifdef __GNUC__
        // uses POPCNT instruction if available, otherwise lookup table
        return __builtin_popcountll(b64);
#else
        block_type x(b64);
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        x = (x * 0x0101010101010101ULL) >> 56;
        return int(x);
#endif
    }

    static int hammingDistance(SVector<bool> &v1, SVector<bool> &v2) {

        int count = 0;
        size_t numBlocks = v1.getNumBlocks();
        block_type *data1, *data2;
        block_type exor;

        data1 = v1.getData();
        data2 = v2.getData();

        for (int i = 0; i < numBlocks; ++i) {
            exor = data1[i] ^ data2[i];
            count += popcnt64(exor);
        }
        return count;
    }

};

} // namespace lmw

#endif