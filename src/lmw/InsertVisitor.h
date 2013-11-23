#ifndef INSERTVISITOR_H
#define	INSERTVISITOR_H

#include "StdIncludes.h"
#include "tbb/mutex.h"

namespace lmw {

/**
 * Visits all the clusters along an insertion path into a tree.
 */
template <typename T>
class InsertVisitor {
public:
    InsertVisitor() { }
    virtual ~InsertVisitor() { }
    
    /**
     * Must be thread safe. It can be called from multiple threads.
     */
    virtual void accept(int level, T* object, T* cluster) = 0;
};

class ClusterWriter : public InsertVisitor<SVector<bool>> {
public:

    ClusterWriter(const int levels, const string& filenamePrefix) {
        _mutexes.resize(levels);
        for (int level = 1; level <= levels; level++) {
            stringstream ss;
            ss << filenamePrefix << "_level" << level;
            _levels.push_back(new ofstream(ss.str() + "_clusters.txt"));
        }
        // TODO(cdevries): check state of streams
    }

    ~ClusterWriter() {
        for (auto stream : _levels) {
            delete stream;
        }
    }
    
    void accept(int level, SVector<bool>* object, SVector<bool>* cluster) {
        Mutex::scoped_lock lock(_mutexes[level - 1]);
        // using endl here causes the buffer to flush and sync() to be called which slows it down
        *_levels[level - 1] << object->getID() << "," << hex << size_t(cluster) << "\n";
        return;
    }  
    
private:
    typedef tbb::mutex Mutex;
    vector<Mutex> _mutexes;
    vector<ofstream*> _levels;
};

} // namespace LMW

#endif	/* INSERTVISITOR_H */

