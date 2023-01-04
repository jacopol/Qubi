#include "bdd_sylvan.hpp"
#include "settings.hpp"

using namespace sylvan;

BDD_Sylvan::BDD_Sylvan(int workers, long long maxnodes) {
    LOG(2, "Opening Sylvan BDDs" << std::endl);
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
}

BDD_Sylvan::~BDD_Sylvan() {
    sylvan_stats_report(stdout); // requires SYLVAN_STATS=on during Sylvan compilation
    sylvan_quit();
    lace_stop();
    LOG(2, "Closed Sylvan BDDs" << std::endl);
}
/*
       BddSet varSet = BddSet();
        for (int x : vars) varSet.add(x);
        vector<bool> val;
        if (q==Exists)
            val = matrix->PickOneCube(varSet);
        else
            val = (!*matrix).PickOneCube(varSet);
*/            