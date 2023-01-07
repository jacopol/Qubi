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
    for (int i=c.maxVar(); i<=abs(c.getOutput());i++) {
        LOG(2,"- gate " << c.varString(i));
        Gate g = c.getGate(i);
        bool isAnd = g.output==And;
        // Build a conjunction or disjunction:
        Sylvan_Bdd bdd = Sylvan_Bdd(isAnd); // neutral element
        for (int arg: g.inputs) {
            LOG(2,".");
            if (isAnd)
                bdd *= toBdd(arg);  // conjunction
            else
                bdd += toBdd(arg);  // disjunction
            }
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
