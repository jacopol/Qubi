#include <iostream>
#include <vector>
#include "solver.hpp"

using namespace sylvan;

BddSolver::BddSolver(int workers, long long maxnodes) {
    lace_start(workers, 0); // deque_size 0
    long long initnodes = maxnodes >> 8;
    long long maxcache = maxnodes >> 4;
    long long initcache = maxcache >> 4;
    sylvan_set_sizes(initnodes, maxnodes, initcache, maxcache);
    sylvan_init_package();
    sylvan_init_bdd();
    std::cerr << "Opening Sylvan BDDs" << std::endl;
}

BddSolver::~BddSolver() {
    sylvan_stats_report(stdout); // if SYLVAN_STATS is set
    sylvan_quit();
    lace_stop();
    std::cerr << "Closed Sylvan BDDs" << std::endl;
}

void BddSolver::solveVars(Circuit &c) {
    matrix2bdd(c);
    prefix2bdd(c);
    result(c);
}

void BddSolver::solveBlocks(Circuit &c) {
    matrix2bdd(c);
    blocks2bdd(c);
    result(c);
}

void BddSolver::matrix2bdd(Circuit &c) {
    vector<Bdd> bdds;                   // lookup table previous BDDs
    bdds.push_back(sylvan_true);        // unused entry 0
    auto toBdd = [&bdds](int i)->Bdd {  // negate (if necessary) and look up Bdd
        if (i>0)
            return bdds[i];
        else
            return !bdds[-i];
    };
    for (int i=1; i<c.maxVar();i++) {
        bdds.push_back(Bdd::bddVar(i));
    }
    cerr << "Building BDD for Matrix" << endl;
    for (int i=c.maxVar(); i<c.maxGate();i++) {
        cerr << "gate " << i;
        Gate g = c.getGate(i);
        bool isAnd = g.getConn()==And;
        // Build a conjunction or disjunction:
        Bdd bdd = ( isAnd ? sylvan_true : sylvan_false); // neutral element
        for (int arg: g.getArgs()) {
            cerr << ".";
            if (isAnd)
                bdd *= toBdd(arg);  // conjunction
            else
                bdd += toBdd(arg);  // disjunction
            }
        cerr << " (" << bdd.NodeCount() << " nodes)" << endl;
        bdds.push_back(bdd);
        // bdd.PrintDot(stdout);
    }
    matrix = toBdd(c.getOutput()); // final result
}

void BddSolver::prefix2bdd(Circuit &c) {
    cerr << "Quantifying Prefix Inside-Out, one-by-one" << endl;

    // determine start of second block
    int second;
    for (second=1 ; second < c.maxVar(); second++) {
        if (c.getQuant(second) != c.getQuant(1)) 
            break;
    }

    // quantify the prefix, from end to "second", until fully resolved
    for (int i=c.maxVar()-1; i>=second; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            cerr << "(early termination)" << endl;
            break;
        }
        Quantifier q = c.getQuant(i);
        cerr << "var " << i << " (" << Quant(q) << "): ";
        Bdd var = Bdd::bddVar(i);
        if (q==Forall)
            matrix = matrix.UnivAbstract(var);
        else
            matrix = matrix.ExistAbstract(var);
        cerr << "(" << matrix.NodeCount() << " nodes)" << endl;
    }
}

void BddSolver::blocks2bdd(Circuit &c) {
    cerr << "Quantifying Prefix Inside-Out, per block" << endl;
    vector<vector<int>> blocks = c.getBlocks();

    // Quantify blocks from last to second, unless fully resolved
    for (int i=blocks.size()-1; i>0; i--) {
        if (matrix == sylvan_true || matrix == sylvan_false) {
            cerr << "(early termination)" << endl;
            break;
        }
        Quantifier q = c.getQuant(blocks[i][0]);
        cerr << "block " << i+1 << " (" << blocks[i].size() << "x " << Quant(q) << "): ";

        Bdd cube = sylvan_true;        
        for (int var : blocks[i]) {
            cube *= Bdd::bddVar(var);
        }
        if (q==Forall)
            matrix = matrix.UnivAbstract(cube);
        else
            matrix = matrix.ExistAbstract(cube);
        cerr << "(" << matrix.NodeCount() << " nodes)" << endl;
    }
}

void BddSolver::result(Circuit &c) {
    cout << "Result: ";
    if (matrix == sylvan_true)
        cout << "TRUE" << endl;
    else if (matrix == sylvan_false) 
        cout << "FALSE" << endl;
    else {
        Quantifier q = c.getQuant(1);
        if (q==Forall)
            cout << "FALSE. Counterexample: ";
        else
            cout << "TRUE. Example: ";

        BddSet vars = BddSet();
        int firstBlock = c.getBlocks()[0].size();
        // QBF variables start at 1
        for (int i=1; i<=firstBlock; i++) {
            vars.add(i);
        }
        // Sylvan vector starts at 0
        vector<bool> val = matrix.PickOneCube(vars);
        for (int i=1; i<c.maxVar();i++) {
            if (val[i-1]==false)
                cout << -i << " ";
            else if (val[i-1]==true)
                cout << i << " ";
        }
        cout << endl;
    }
    cout << endl;
}
