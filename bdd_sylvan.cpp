// (c) Jaco van de Pol
// Aarhus University

#include <vector>
#include <deque>
#include <algorithm>

#include "bdd_sylvan.hpp"
#include "settings.hpp"

using namespace sylvan;

VOID_TASK_0(gc_start) { LOG(2,"[gc.."); }
VOID_TASK_0(gc_done) { LOG(2,"]"); }

// start Sylvan, with number of workers, and unique table size 2^size
Sylvan_mgr::Sylvan_mgr(int workers, int size) {
    LOG(2, "Opening Sylvan BDDs ("
        << workers << " workers, table=2^" << size << ")" << std::endl);
    lace_start(workers, 0); // deque_size 0
    long long maxnodes = 1L << size; 
    long long initnodes = maxnodes >> 6;
    long long maxcache  = maxnodes >> 2;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
    if (VERBOSE>=2) {
        sylvan_gc_hook_pregc(TASK(gc_start)); // message for garbage collection
        sylvan_gc_hook_postgc(TASK(gc_done)); // message for garbage collection
    }
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
    Sylvan_Bdd result(bdd.UnivAbstract(cube));
    result.peak();
    return result;
}

Sylvan_Bdd Sylvan_Bdd::ExistAbstract(const std::vector<int>& variables) const{
    Bdd cube = makeCube(variables);
    Sylvan_Bdd result(bdd.ExistAbstract(cube));
    result.peak();
    return result;
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
    for (size_t i=0; i<vars.size(); i++) 
        index.push_back(i);
    const auto cmp = [&vars](int a, int b) { return vars[a] < vars[b]; };
    std::sort(index.begin(), index.end(), cmp);

    // Let Sylvan pick a vector
    BddSet varSet = BddSet();
    for (int x : vars) varSet.add(x);
    std::vector<bool> val1 = bdd.PickOneCube(varSet);

    // Apply the reverse sorting permutation (index)
    std::vector<bool> val2 = std::vector<bool>(val1.size(), false);
    for (size_t i=0; i<val1.size(); i++)
        val2[index[i]] = val1[i];

    return val2;
}

// TODO: could use parallel reduce (TASKS)
// TODO: maybe use vector<std::reference_wrapper<Sylvan_Bdd>>? (Looks ugly)


Sylvan_Bdd bigAnd_left2right(const std::vector<Sylvan_Bdd>& args);
Sylvan_Bdd bigOr_left2right(const std::vector<Sylvan_Bdd>& args);
Sylvan_Bdd bigAnd_pairwise(const std::vector<Sylvan_Bdd>& args);
Sylvan_Bdd bigOr_pairwise(const std::vector<Sylvan_Bdd>& args);


Sylvan_Bdd Sylvan_Bdd::bigAnd(const std::vector<Sylvan_Bdd>& args) {
    if (ITERATE == 0) return bigAnd_left2right(args);
    if (ITERATE == 1) return bigAnd_pairwise(args);
    std::cerr << "Internal error: ITERATE value" << ITERATE << std::endl;
    exit(-1);
}

Sylvan_Bdd Sylvan_Bdd::bigOr(const std::vector<Sylvan_Bdd>& args) {
    if (ITERATE == 0) return bigOr_left2right(args);
    if (ITERATE == 1) return bigOr_pairwise(args);
    std::cerr << "Internal error: ITERATE value" << ITERATE << std::endl;
    exit(-1);
}

Sylvan_Bdd bigAnd_left2right(const std::vector<Sylvan_Bdd>& args) {
    Sylvan_Bdd bdd = Sylvan_Bdd(true); // neutral element
    for (Sylvan_Bdd arg: args) {
        LOG(2,".");
        bdd *= arg;
    }
    return bdd;
}

Sylvan_Bdd bigOr_left2right(const std::vector<Sylvan_Bdd>& args) {
    Sylvan_Bdd bdd = Sylvan_Bdd(false); // neutral element
    for (Sylvan_Bdd arg: args) {
        LOG(2,".");
        bdd += arg;
    }
    return bdd;
}


/* Alternative implementation: combine BDDs pairwise, etc. (map/reduce) */

Sylvan_Bdd bigAnd_pairwise(const std::vector<Sylvan_Bdd>& args) {
    Sylvan_Bdd bdd = Sylvan_Bdd(true); // neutral element
    if (args.size() == 0)
        return bdd;
    std::deque<Sylvan_Bdd> todo(args.begin(), args.end());
    while (todo.size() > 1) {
        LOG(2,".");
        Sylvan_Bdd arg1 = todo.front(); todo.pop_front();
        Sylvan_Bdd arg2 = todo.front(); todo.pop_front();
        arg1 *= arg2;
        todo.push_back(arg1);
    }
    return todo[0];
}

Sylvan_Bdd bigOr_pairwise(const std::vector<Sylvan_Bdd>& args) {
    Sylvan_Bdd bdd = Sylvan_Bdd(false); // neutral element
    if (args.size() == 0)
        return bdd;
    std::deque<Sylvan_Bdd> todo(args.begin(), args.end());
    while (todo.size() > 1) {
        LOG(2,".");
        Sylvan_Bdd arg1 = todo.front(); todo.pop_front();
        Sylvan_Bdd arg2 = todo.front(); todo.pop_front();
        arg1 += arg2;
        todo.push_back(arg1);
    }
    return todo[0];
}
