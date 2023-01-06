#include <vector>
#include <algorithm>

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

inline Bdd makeCube(const std::vector<int>& variables) {
    Bdd cube = sylvan_true;
    for (int var : variables) cube *= Bdd::bddVar(var);
    return cube;
}


Sylvan_Bdd Sylvan_Bdd::UnivAbstract(const std::vector<int>& variables)  const {
    Bdd cube = makeCube(variables);
    return bdd.UnivAbstract(cube);
}

Sylvan_Bdd Sylvan_Bdd::ExistAbstract(const std::vector<int>& variables) const{
    Bdd cube = makeCube(variables);
    return bdd.ExistAbstract(cube);
}

std::vector<bool> Sylvan_Bdd::PickOneCube(const std::vector<int>& vars) const {

    // Sylvan's PickOneCube returns a SORTED valuation
    // We will need the reverse sorting permutation

    // Example:
    // - vars = [3, 10, 2]
    // - idx  = [0, 1, 2]   (before sorting)
    // - idx  = [2, 0, 1]   (after sorting)
    // - val1 = [v2, v3, v10] (standard order given by Sylvan)
    // - val2 = [v3,v10,v2]

    // compute the sorting permutation (index)
    std::vector<int> index;
    for (int i=0; i<vars.size(); i++) 
        index.push_back(i);
    const auto cmp = [&vars](int a, int b) { return vars[a] < vars[b]; };
    std::sort(index.begin(), index.end(), cmp);

    // Let Sylvan pick a vector
    BddSet varSet = BddSet();
    for (int x : vars) varSet.add(x);
    std::vector<bool> val1 = bdd.PickOneCube(varSet);

    // Apply the reverse sorting permutation (index)
    std::vector<bool> val2 = std::vector<bool>(val1.size(), false);
    for (int i=0; i<val1.size(); i++)
        val2[index[i]] = val1[i];

    return val2;
}
