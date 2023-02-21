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

Solver::Solver(const Circuit& circuit) : c(circuit), matrix(false) { }

bool Solver::solve() {
    matrix2bdd();
    prefix2bdd();
    return verdict();
}

// Here we assume that either the matrix is a leaf, or all variables 
// except for the first (outermost) block have been eliminated
bool Solver::verdict() const {
    if (matrix.isConstant()) {
        return matrix == Sylvan_Bdd(true);
    }
    else { // there must be at least one variable
        return (c.getBlock(0).quantifier == Exists);
    }
}

// Here we assume that either the matrix is a leaf, or all variables 
// except for the first (outermost) block have been eliminated
Valuation Solver::example() const {
    Valuation valuation;
    if (c.maxBlock() == 0 ||
        verdict() != (c.getBlock(0).quantifier == Exists)) {
        return valuation; // no example possible: empty valuation
    } else {
        // compute list of all top-level variables
        vector<int> vars;
        Quantifier q = c.getBlock(0).quantifier;
        for (int i=0; i<c.maxBlock(); i++) {
            Block b = c.getBlock(i);
            if (b.quantifier != q) break; // stop at first quantifier alternation
            for (int v : b.variables) vars.push_back(v);
        }
        // let Sylvan compute a valuation
        vector<bool> val;
        if (q==Exists)
            val = matrix.PickOneCube(vars);
        else
            val = (!matrix).PickOneCube(vars);
        // return the result
        for (int i=0; i<vars.size(); i++)
            valuation.push_back(pair<int,bool>({vars[i], val[i]}));
        return valuation;
    }
}

// compute the last use of each bdd and store in cleanup

void Solver::computeCleanup() {
        std::set<int> cleaned;
        for (int i=c.maxVar(); i<=abs(c.getOutput()); i++) {
            cleanup.push_back(std::set<int>());
        }

        for (int i=abs(c.getOutput()); i>=c.maxVar(); i--) {
            Gate g = c.getGate(i);
            for (int arg: g.inputs) {
                arg = abs(arg);
                if (arg>=c.maxVar() && cleaned.count(arg)==0) { //this is the last time arg is going to be used
                    cleanup[i-c.maxVar()].insert(arg);
                    cleaned.insert(arg);
                }
            }
        }
}


void Solver::matrix2bdd() {
    vector<Sylvan_Bdd> bdds({Sylvan_Bdd(false)}); // lookup table previous BDDs, start at 1
    auto toBdd = [&bdds](int i)->Sylvan_Bdd {     // negate (if necessary) and look up Bdd
        if (i>0)
            return bdds[i];
        else
            return !bdds[-i];
    };
    for (int i=1; i<c.maxVar(); i++) {
        bdds.push_back(Sylvan_Bdd(i));
    }
    LOG(1,"Building BDD for Matrix" << endl;);
    if (GARBAGE) computeCleanup();
    for (int i=c.maxVar(); i<=abs(c.getOutput()); i++) {
        LOG(2,"- gate " << c.varString(i) << ": ");
        Gate g = c.getGate(i);
        if (g.quants.size()==0)
            LOG(2, Ctext[g.output] << "(" << g.inputs.size() << ")")
        else
            LOG(2, Ctext[g.output] << "(" << g.quants.size() << "x)")
        vector<Sylvan_Bdd> args;
        for (int arg: g.inputs) args.push_back(toBdd(arg));

        if (GARBAGE) {
            set<int> garbage = cleanup[i-c.maxVar()];
            if (garbage.size()>0) LOG(3, " (garbage:");
            for (int j : garbage) { // Do the cleanup
                bdds[j] = Sylvan_Bdd(false);
                LOG(3, " " << c.varString(j))
            }
            if (garbage.size()>0) LOG(3,")");
        }

        Sylvan_Bdd bdd(false);
        if (g.output == And)
            bdd = Sylvan_Bdd::bigAnd(args);
        else if (g.output == Or)
            bdd = Sylvan_Bdd::bigOr(args);
        else if (g.output == Ex)
            bdd = args[0].ExistAbstract(g.quants);
        else if (g.output == All)
            bdd = args[0].UnivAbstract(g.quants);
        else
            assert(false);
        LOG(2," (" << bdd.NodeCount() << " nodes)" << endl);
        bdds.push_back(bdd);
    }
    matrix = toBdd(c.getOutput()); // final result
}

void Solver::prefix2bdd() {
    LOG(1,"Quantifying Prefix" << endl);
    // Quantify blocks from last to second, unless fully resolved
    for (int i=c.maxBlock()-1; i>0; i--) {
        if (matrix.isConstant()) {
            LOG(2, "(early termination)" << endl);
            break;
        }
        Block b = c.getBlock(i);
        LOG(2,"- block " << i+1 << " (" << b.size() << "x " << Qtext[b.quantifier] << "): ");
        if (b.quantifier == Forall)
            matrix = matrix.UnivAbstract(b.variables);
        else
            matrix = matrix.ExistAbstract(b.variables);
        LOG(2,"(" << matrix.NodeCount() << " nodes)" << endl);
    }
}
