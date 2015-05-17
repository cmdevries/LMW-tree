#ifndef CLUSTERVISITOR_H
#define	CLUSTERVISITOR_H

#include "StdIncludes.h"
#include "SVector.h"

namespace lmw {

/**
 * Visits all clusters in a tree.
 */
template <typename T>
class ClusterVisitor {
public:
    ClusterVisitor() { }
    virtual ~ClusterVisitor() { }

    /**
     * parentCluster is NULL for root nodes
     */
    virtual void accept(const int level, const T* parentCluster,
        const T* cluster, const double RMSE, const uint64_t objectCount) = 0;
};

class ClusterStats : public ClusterVisitor<SVector<bool>> {
public:
    ClusterStats(const int levels, const string& filenamePrefix) {
        for (int level = 1; level <= levels; level++) {
            stringstream ss;
            ss << filenamePrefix << "_level" << level;
            string filename = ss.str() + "_stats.txt";
            _levels.emplace_back(new ofstream(filename.c_str()));
            ofstream& level_stream = *_levels.back();
            if (!level_stream) {
                throw runtime_error("unable to open: " + filename);
            }
            level_stream << "parent cluster ID, cluster ID, RMSE, object count" << endl;
        }
    }

    void accept(const int level, const SVector<bool>* parentCluster,
            const SVector<bool>* cluster, const double RMSE, const uint64_t objectCount) {
        *_levels[level - 1] << hex << size_t(parentCluster) << ","
            << size_t(cluster) << dec << "," << RMSE << "," << objectCount << endl;
    }

private:
    vector<unique_ptr<ofstream>> _levels;
};

} // namespace lmw

#endif	/* CLUSTERVISITOR_H */
