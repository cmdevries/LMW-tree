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
    
    virtual void accept(int level, T* cluster, double RMSE, uint64_t objectCount) = 0;    
};

class ClusterStats : public ClusterVisitor<SVector<bool>> {
public:
    ClusterStats(const int levels, const string& filenamePrefix) {
        for (int level = 1; level <= levels; level++) {
            stringstream ss;
            ss << filenamePrefix << "_level" << level;
            ofstream* l = new ofstream(ss.str() + "_stats.txt");
            *l << "cluster ID, RMSE, object count" << endl;
            _levels.push_back(l);
        }
        // TODO(cdevries): check state of streams        
    }    
    
    void accept(int level, SVector<bool>* cluster, double RMSE, uint64_t objectCount) {
        *_levels[level - 1] << hex << size_t(cluster) << dec << "," << RMSE << "," << objectCount << endl;
    }
    
private:
    vector<ofstream*> _levels;
};

} // namespace lmw

#endif	/* CLUSTERVISITOR_H */