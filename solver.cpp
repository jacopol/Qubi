// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <vector>
#include <algorithm>
#include "solver.hpp"

using std::cout;
using std::cerr;
using std::endl;

using namespace sylvan;

#define LOG(level, msg) { if (level<=verbose) {cerr << msg; }}

Solver::Solver(int workers, long long maxnodes) {
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
    // cerr << "Opening Sylvan BDDs" << endl;
}

Solver& Solver::setExample(bool example) {
    witness = example;
    return *this;
}

Solver& Solver::setVerbose(int verbosity) {
    verbose = verbosity;
    return *this;
}


Solver::~Solver() {
    sylvan_stats_report(stdout); // if SYLVAN_STATS is set
    sylvan_quit();
    lace_stop();
    // cerr << "Closed Sylvan BDDs" << endl;
}

void Solver::solve(Circuit &c) {
    matrix2bdd(c);
    prefix2bdd(c);
    result(c);
    if (witness) example(c);
}

void Solver::result(Circuit &c) {
    // Here we assume that all variables except for 
    // the first (outermost) block have been eliminated
    cout << "Result: ";
    if (matrix == sylvan_true)
        cout << "TRUE" << endl;
    else if (matrix == sylvan_false) 
        cout << "FALSE" << endl;
    else {
        Quantifier q = c.getBlock(0).quantifier;
        if (q==Forall)
            cout << "FALSE" << endl;
        else
            cout << "TRUE" << endl;
    }
}

void Solver::example(Circuit &c) {
    // Here we assume that all variables except for 
    // the first (outermost) block have been eliminated
    cout << "Example: ";
    if (matrix.isOne() || matrix.isZero()) {
        cout << "None" << endl;
    } else {
        // compute list of all top-level variables
        auto vars = vector<int>();
        Quantifier q = c.getBlock(0).quantifier;
        for (int i=0; i<c.maxBlock(); i++) {
            Block b = c.getBlock(i);
            if (b.quantifier != q) break; // stop at first quantifier alternation
            for (int v : b.variables) {
                vars.push_back(v);
            }
        }

        // Let Sylvan compute a valuation
        BddSet varSet = BddSet();
        for (int x : vars) varSet.add(x);
        vector<bool> val = matrix.PickOneCube(varSet);

        // Note: Sylvan valuation is sorted on identifiers
        // Example: if vars = [2,7,3,8,9] then vals = [v2,v3,v7,v8,v9]
        // we use varsorted = [2,3,7,8,9] to find the proper location

        vector<int>varsorted = vars;
        std::sort(varsorted.begin(),varsorted.end());

        // Print valuation
        for (int i=0; i<vars.size(); i++) {
            int loc = std::find(varsorted.begin(),varsorted.end(),vars[i]) - varsorted.begin();
            cout << c.getVarOrGate(vars[i]) << "=" << (val[loc] ? "true" : "false") << " ";
        }
        cout << endl;
    }
}

void Solver::matrix2bdd(Circuit &c) {
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
    for (int i=c.maxVar(); i<=c.getOutput();i++) {
        LOG(2,"- gate " << c.getVarOrGate(i));
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

// override
void Solver::prefix2bdd(Circuit &c) {
    LOG(1,"Quantifying Prefix" << endl);

    // Quantify blocks from last to second, unless fully resolved
    for (int i=c.maxBlock()-1; i>0; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            LOG(1, "  (early termination)" << endl);
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
