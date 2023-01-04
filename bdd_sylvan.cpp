#include "bdd_sylvan.hpp"
#include "settings.hpp"

using namespace sylvan;

Sylvan_mgr::Sylvan_mgr(int workers, long long maxnodes) {
    LOG(2, "Opening Sylvan BDDs" << std::endl);
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
}

Sylvan_mgr::~Sylvan_mgr() {
    sylvan_stats_report(stdout); // requires SYLVAN_STATS=on during Sylvan compilation
    sylvan_quit();
    lace_stop();
    LOG(2, "Closed Sylvan BDDs" << std::endl);
}
