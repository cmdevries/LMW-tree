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
    virtual void accept(const int level, const T* object, const T* cluster,
        const double distance) = 0;
};

class ClusterWriter : public InsertVisitor<SVector<bool>> {
public:

    ClusterWriter(const int levels, const string& filenamePrefix) {
        _mutexes.resize(levels);
        for (int level = 1; level <= levels; level++) {
            stringstream ss;
            ss << filenamePrefix << "_level" << level;
            string filename = ss.str() + "_clusters.txt";
            _levels.emplace_back(new ofstream(filename.c_str()));
            ofstream& level_stream = *_levels.back();
            if (!level_stream) {
                throw std::runtime_error("unable to open " + filename);
            }
            level_stream << "object ID, cluster ID, distance to cluster center" << endl;
        }
    }

    void accept(const int level, const SVector<bool>* object,
            const SVector<bool>* cluster, const double distance) {
        Mutex::scoped_lock lock(_mutexes[level - 1]);
        // using endl here causes the buffer to flush and sync() to be called which slows it down
        *_levels[level - 1] << object->getID() << "," << hex << size_t(cluster)
            << dec << "," << distance << "\n";
        return;
    }

private:
    typedef tbb::mutex Mutex;
    vector<Mutex> _mutexes;
    vector<unique_ptr<ofstream>> _levels;
};

} // namespace LMW

#endif	/* INSERTVISITOR_H */

