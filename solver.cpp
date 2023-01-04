// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <vector>
#include <algorithm>
#include "solver.hpp"
#include "settings.hpp"

using std::cout;
using std::cerr;
using std::endl;

using namespace sylvan;

Solver::Solver(int workers, long long maxnodes, const Circuit& circuit) : c(circuit) {
    LOG(2, "Opening Sylvan BDDs" << endl);
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
}

Solver::~Solver() {
    sylvan_stats_report(stdout); // requires SYLVAN_STATS=on during Sylvan compilation
    sylvan_quit();
    lace_stop();
    LOG(2, "Closed Sylvan BDDs" << endl);
}

bool Solver::solve() {
    matrix2bdd();
    prefix2bdd();
    return verdict();
}

// Here we assume that either the matrix is a leaf, or all variables 
// except for the first (outermost) block have been eliminated
bool Solver::verdict() const {
    if (matrix.isConstant()) {
        return matrix == sylvan_true;
    }
    else { // there must be at least one variable
        return (c.getBlock(0).quantifier == Exists);
    }
}

// Here we assume that either the matrix is a leaf, or all variables 
// except for the first (outermost) block have been eliminated
Valuation& Solver::example() const {
    Valuation*valuation = new Valuation(); // empty
    if (c.maxBlock() == 0 ||
        verdict() != (c.getBlock(0).quantifier == Exists)) {
        return *valuation; // no example possible
    } else {
        // compute list of all top-level variables
        auto vars = vector<int>();
        Quantifier q = c.getBlock(0).quantifier;
        for (int i=0; i<c.maxBlock(); i++) {
            Block b = c.getBlock(i);
            if (b.quantifier != q) break; // stop at first quantifier alternation
            for (int v : b.variables) {   // else: add all variables from this block
                vars.push_back(v);
            }
        }

        // Let Sylvan compute a valuation
        BddSet varSet = BddSet();
        for (int x : vars) varSet.add(x);
        vector<bool> val;
        if (q==Exists)
            val = matrix.PickOneCube(varSet);
        else
            val = (!matrix).PickOneCube(varSet);

        // Note: Sylvan valuation is sorted on identifiers
        // Example: if vars = [2,7,3,8,9] then vals = [v2,v3,v7,v8,v9]
        // we use varsorted = [2,3,7,8,9] to find the proper location

        vector<int>varsorted = vars;
        std::sort(varsorted.begin(),varsorted.end());

        // TODO: should be possible in O(n log n)
        for (int i=0; i<vars.size(); i++) {
            int loc = std::find(varsorted.begin(),varsorted.end(),vars[i]) - varsorted.begin();
            (*valuation).push_back(pair<int,bool>(vars[i], val[loc]));
        }
        return *valuation;
    }
}

void Solver::matrix2bdd() {
    vector<Bdd> bdds;                    // lookup table previous BDDs
    bdds.push_back(sylvan_true);         // unused entry 0
    auto toBdd = [&bdds](int i)-> Bdd {  // negate (if necessary) and look up Bdd
        if (i>0)
            return bdds[i];
        else
            return !bdds[-i];
    };
    for (int i=1; i<c.maxVar(); i++) {
        bdds.push_back(Bdd::bddVar(i));
    }
    LOG(1,"Building BDD for Matrix" << endl;);
    for (int i=c.maxVar(); i<=abs(c.getOutput());i++) {
        LOG(2,"- gate " << c.varString(i));
        Gate g = c.getGate(i);
        bool isAnd = g.output==And;
        // Build a conjunction or disjunction:
        Bdd bdd = ( isAnd ? sylvan_true : sylvan_false); // neutral element
        for (int arg: g.inputs) {
            LOG(2,".");
            if (isAnd)
                bdd *= toBdd(arg);  // conjunction
            else
                bdd += toBdd(arg);  // disjunction
            }
        LOG(2," (" << bdd.NodeCount() << " nodes)" << endl);
        bdds.push_back(bdd);
        // bdd.PrintDot(stdout);
    }
    matrix = toBdd(c.getOutput()); // final result
}

void Solver::prefix2bdd() {
    LOG(1,"Quantifying Prefix" << endl);

    // Quantify blocks from last to second, unless fully resolved
    for (int i=c.maxBlock()-1; i>0; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            LOG(2, "  (early termination)" << endl);
            break;
        }
        Block b = c.getBlock(i);
        LOG(2,"- block " << i+1 << " (" << b.size() << "x " << Qtext[b.quantifier] << "): ");

        Bdd cube = sylvan_true;        
        for (int var : b.variables) {
            cube *= Bdd::bddVar(var);
        }
        if (b.quantifier == Forall)
            matrix = matrix.UnivAbstract(cube);
        else
            matrix = matrix.ExistAbstract(cube);
        LOG(2,"(" << matrix.NodeCount() << " nodes)" << endl);
    }
}
