#include <iostream>
#include <vector>
#include "messages.hpp"
#include "solver.hpp"

using std::cout;
using std::cerr;
using std::endl;

using namespace sylvan;

BddSolver::BddSolver(int workers, long long maxnodes) {
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
    // cerr << "Opening Sylvan BDDs" << endl;
}

BddSolver& BddSolver::setExample(bool example) {
    witness = example;
    return *this;
}

BddSolver& BddSolver::setVerbose(int verbosity) {
    verbose = verbosity;
    return *this;
}


BlockSolver::BlockSolver(int workers, long long maxnodes) 
    : BddSolver(workers, maxnodes) {}

BddSolver::~BddSolver() {
    sylvan_stats_report(stdout); // if SYLVAN_STATS is set
    sylvan_quit();
    lace_stop();
    // cerr << "Closed Sylvan BDDs" << endl;
}

void BddSolver::solve(Circuit &c) {
    matrix2bdd(c);
    prefix2bdd(c);
    result(c);
    if (witness) example(c);
}

void BddSolver::result(Circuit &c) {
    // Here we assume that all variables except for 
    // the first (outermost) block have been eliminated
    cout << "Result: ";
    if (matrix == sylvan_true)
        cout << "TRUE" << endl;
    else if (matrix == sylvan_false) 
        cout << "FALSE" << endl;
    else {
        Quantifier q = c.getQuant(1);
        if (q==Forall)
            cout << "FALSE" << endl;
        else
            cout << "TRUE" << endl;
    }
}

void BddSolver::example(Circuit &c) {
    // Here we assume that all variables except for 
    // the first (outermost) block have been eliminated
    if (matrix != sylvan_true && matrix != sylvan_false) {
        cout << "Example: ";
        BddSet vars = BddSet();
        int firstBlock = c.getBlocks()[0].size();
        // QBF variables start at 1
        for (int i=1; i<=firstBlock; i++) {
            vars.add(i);
        }
        // Sylvan vector starts at 0
        vector<bool> val = matrix.PickOneCube(vars);
        for (int i=1; i<=firstBlock; i++)
            cout << (val[i-1] ? i : -i) << " ";
        cout << endl;
    }
}

void BddSolver::matrix2bdd(Circuit &c) {
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
    LOGln(1,"Building BDD for Matrix");
    for (int i=c.maxVar(); i<c.maxGate();i++) {
        LOG(2,"- gate " << i);
        Gate g = c.getGate(i);
        bool isAnd = g.getConn()==And;
        // Build a conjunction or disjunction:
        Bdd bdd = ( isAnd ? sylvan_true : sylvan_false); // neutral element
        for (int arg: g.getArgs()) {
            LOG(2,".");
            if (isAnd)
                bdd *= toBdd(arg);  // conjunction
            else
                bdd += toBdd(arg);  // disjunction
            }
        LOGln(2," (" << bdd.NodeCount() << " nodes)");
        bdds.push_back(bdd);
        // bdd.PrintDot(stdout);
    }
    matrix = toBdd(c.getOutput()); // final result
}

void BddSolver::prefix2bdd(Circuit &c) {
    LOGln(1,"Quantifying Prefix, per quantifier");

    // determine start of second block
    int second;
    for (second=1 ; second < c.maxVar(); second++) {
        if (c.getQuant(second) != c.getQuant(1)) 
            break;
    }

    // quantify the prefix, from end to "second", until fully resolved
    for (int i=c.maxVar()-1; i>=second; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            LOGln(1,"  (early termination)");
            break;
        }
        LOG(2,"- var " << i << " (" << c.Quant(i) << "): ");
        Bdd var = Bdd::bddVar(i);
        if (c.getQuant(i) == Forall)
            matrix = matrix.UnivAbstract(var);
        else
            matrix = matrix.ExistAbstract(var);
        LOGln(2,"(" << matrix.NodeCount() << " nodes)");
    }
}

// override
void BlockSolver::prefix2bdd(Circuit &c) {
    LOGln(1,"Quantifying Prefix, per block");
    vector<vector<int>> blocks = c.getBlocks();

    // Quantify blocks from last to second, unless fully resolved
    for (int i=blocks.size()-1; i>0; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            LOGln(1, "  (early termination)");
            break;
        }
        int q = blocks[i][0];
        LOG(2,"- block " << i+1 << " (" << blocks[i].size() << "x " << c.Quant(q) << "): ");

        Bdd cube = sylvan_true;        
        for (int var : blocks[i]) {
            cube *= Bdd::bddVar(var);
        }
        if (c.getQuant(q) == Forall)
            matrix = matrix.UnivAbstract(cube);
        else
            matrix = matrix.ExistAbstract(cube);
        LOGln(2,"(" << matrix.NodeCount() << " nodes)");
    }
}
