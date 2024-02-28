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
    unitpropagation();
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
        for (size_t i=0; i<vars.size(); i++){
            pair<int,bool> res;
            //remember to consider truth values assigned during unit propagation TODO: clean code
            for(int j=0; j<restricted_vars.size(); j++){
                if(vars[i]==abs(restricted_vars[j])){
                    res = pair<int,bool>({vars[i], restricted_vars[j] > 0}); break;
                }
                else
                    res = pair<int,bool>({vars[i], val[i]});
            }
            valuation.push_back(res);
        }
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
            if (garbage.size()>0) LOG(3, " [garbage:");
            for (int j : garbage) { // Do the cleanup
                bdds[j] = Sylvan_Bdd(false);
                LOG(3, " " << c.varString(j))
            }
            if (garbage.size()>0) LOG(3,"]");
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
        if (STATISTICS) { LOG(2," (" << bdd.NodeCount() << " nodes)"); }
        LOG(2, endl);
        bdds.push_back(bdd);
        // If there are no more or-gates (can be checked by linear run through remaining gates): unit propagataion on subBDD secures logical equivalence.
        // Should also be the case when the matrix can include for Exist- and Forall-gates (i.e. when miniscoping) due to the logical equivalence upheld when miniscoping.
        // Perhaps a transformation of the matrix into NNF would help?
    }
    matrix = toBdd(c.getOutput()); // final result
}

void updatePolarity(vector<int> polarity, int v, int pol){
    if(polarity[v] == -2)
        polarity[v] = pol; 
    else if(polarity[v] != pol) //only set to -1 if non-pure
        polarity[v] = -1;
}
vector<int> Solver::polarity(){
    vector<int> polarity;
    for(int i = 1; i<c.maxVar();i++)
        polarity.push_back(-2);
    for(int i =1; i<c.maxVar(); i++){
        if(matrix.restrict(Sylvan_Bdd(i)) == Sylvan_Bdd(true)) updatePolarity(polarity, i, 1);
        else if(matrix.restrict(!Sylvan_Bdd(i)) == Sylvan_Bdd(true)) updatePolarity(polarity, i, 0);
        else if(matrix.restrict(!Sylvan_Bdd(i)) == matrix.UnivAbstract({i})) updatePolarity(polarity, i, 1);
        else if(matrix.restrict(Sylvan_Bdd(i)) == matrix.UnivAbstract({i})) updatePolarity(polarity, i, 0);
        else updatePolarity(polarity, i, !polarity[i]);
    }
    return polarity;
}
// TODO: Is this useful in non-CNF setting?
void Solver::pureLitElim(){
    bool pureVar = true;
    while (pureVar){ //repeat until fixpoint
        vector<int> pol = polarity();
        Sylvan_Bdd polBdd = Sylvan_Bdd(true);
        pureVar = false;
        for(int v = 0; v< pol.size(); v++){
            if(pol[v] != -2 && pol[v] != -1){
                pureVar = true;
                // Remove universal literals from clauses and remove clauses containing existential literals
                if((c.quantAt(v)==Forall && pol[v]==0) || (c.quantAt(v)==Exists && pol[v]==1)){
                    polBdd *= Sylvan_Bdd(v); restricted_vars.push_back(v);
                } else{
                    polBdd *= !Sylvan_Bdd(v); restricted_vars.push_back(-v);    
                } 
            }
        }
        matrix = matrix.restrict(polBdd);
    }
}

//TODO: generalize to allow unitpropagation on subcircuits (take a bdd as argument)
void Solver::unitpropagation() {
    vector<Sylvan_Bdd> unitbdds;
    for (int i=1; i< c.maxVar(); i++) {
        if(c.quantAt(i)==Forall){
                continue;
        }
        if(matrix.restrict(!Sylvan_Bdd(i)) == Sylvan_Bdd(false)){
            matrix = matrix.restrict(Sylvan_Bdd(i));
            restricted_vars.push_back(i);
        }
        else if (matrix.restrict(Sylvan_Bdd(i)) == Sylvan_Bdd(false)){
            matrix = matrix.restrict(!Sylvan_Bdd(i));
            restricted_vars.push_back(-i);
        }
    }

}


std::map<int, bool> detectUnitLits(Sylvan_Bdd bdd) {

    enum Unit {UnitTrue, UnitFalse, NotUnit};

    map<int, Unit> isUnitMap;

    // DFS search of the BDD starting from root
    // For every node, if lo(n) or hi(n) is BDD_FALSE, then the corresponding variable might be a unit literal
    // If all nodes corresponding to variable v have either lo(n) == FALSE or hi(n)==FALSE, then v is in fact a unit literal

    set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({bdd});
    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        if (visited.count(b)==0) { 
            visited.insert(b);
            if(b==Sylvan_Bdd(false) || b== Sylvan_Bdd(true)) continue; //ignore leaves
            if(b.lo()==Sylvan_Bdd(false)){
                map<int,Unit>::iterator v = isUnitMap.find(b.getRootVar());
                if(v == isUnitMap.end())
                    isUnitMap.insert({b.getRootVar(), UnitTrue});
                else if(v->second ==UnitFalse){
                    isUnitMap.erase(b.getRootVar());
                    isUnitMap.insert({b.getRootVar(), NotUnit});
                }
            } else if(b.hi() == Sylvan_Bdd(true)){
                map<int,Unit>::iterator v = isUnitMap.find(b.getRootVar());
                if(v == isUnitMap.end())
                    isUnitMap.insert({b.getRootVar(), UnitFalse});
                else if(v->second ==UnitTrue){
                    isUnitMap.erase(b.getRootVar());
                    isUnitMap.insert({b.getRootVar(), NotUnit});
                }
            }
            todo.push_back(b.lo());
            todo.push_back(b.hi());
        }
    }
    // Return a map of variables to its value
    map<int, bool> unitLits;
    map<int, Unit>::iterator it;
    for (it = isUnitMap.begin(); it != isUnitMap.end(); it++){
            if(it->second==UnitFalse)
                unitLits.insert({it->first,false});
            if(it->second==UnitTrue)
                unitLits.insert({it->first,true});
    }
    return unitLits;
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
        if (STATISTICS) { LOG(2," (" << matrix.NodeCount() << " nodes)"); }
        LOG(2,endl);


        //TODO: call unitpopagation again?
    }
}
