// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "solver.hpp"
#include "settings.hpp"

using std::cout;
using std::cerr;
using std::endl;

string file;

Solver::Solver(const Circuit& circuit) : c(circuit), matrix(false) { }

bool Solver::solve(string filename) {
    file = filename;
    matrix2bdd();
    matrix = unitpropagation(matrix);

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
        /*
        bool noMoreOrGates = true;
        for(int j = i; j<=abs(c.getOutput()); j++){
            if(c.getGate(j).output== Or)
                noMoreOrGates = false;
        }
        if(noMoreOrGates) bdd = unitpropagation(bdd);
        */

        bdds.push_back(bdd);
        
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

std::map<int, bool> Solver::detectUnitLits(Sylvan_Bdd bdd) {

    enum Unit {UnitTrue, UnitFalse, NotUnit};

    map<int, Unit> isUnitMap;

    // DFS search of the BDD starting from root
    // For every node, if lo(n) (or hi(n)) is BDD_FALSE, then the corresponding variable (or its negation) might be a unit literal
    // If all nodes corresponding to variable v have either lo(n) == FALSE (or hi(n)==FALSE), then v (or not(v)) is in fact a unit literal
    std::set<Sylvan_Bdd> visited; 
    vector<Sylvan_Bdd> todo({bdd}); 
    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        if (visited.count(b)==0){ // Node has not yet been visited
            visited.insert(b);
            //cout << "debug " << b.getRootVar()<< endl;
            if(b==Sylvan_Bdd(false) || b == Sylvan_Bdd(true)) continue; //ignore leaves

            todo.push_back(b.lo());
            todo.push_back(b.hi());

            if(c.quantAt(b.getRootVar())==Forall) continue; // Only consider existential variables
            map<int,Unit>::iterator v = isUnitMap.find(b.getRootVar());

            // If v.lo() is BDD_FALSE, then v might be a unit literal, if we haven't previously ruled it out and if we hadn't previously considered not(v) as a unit literal 
            // If v.lo() is not BDD_FALSE, then v is not a unit literal -- if we already stored it in isUnitMap, we need to update the map to v->NotUnit
            if(b.lo()==Sylvan_Bdd(false)){ 
                if(v == isUnitMap.end())
                    isUnitMap.insert({b.getRootVar(), UnitTrue});
                else if(v->second ==UnitFalse){
                    isUnitMap.erase(b.getRootVar());
                    isUnitMap.insert({b.getRootVar(), NotUnit});
                }
            } else if(v != isUnitMap.end() && v->second == UnitTrue){
                isUnitMap.erase(b.getRootVar());
                isUnitMap.insert({b.getRootVar(), NotUnit});
            }

            // The below is symmetric to the above for determining if not(v) is a unit literal
            if(b.hi() == Sylvan_Bdd(false)){ 
                map<int,Unit>::iterator v = isUnitMap.find(b.getRootVar());
                if(v == isUnitMap.end())
                    isUnitMap.insert({b.getRootVar(), UnitFalse});
                else if(v->second ==UnitTrue){
                    isUnitMap.erase(b.getRootVar());
                    isUnitMap.insert({b.getRootVar(), NotUnit});
                }
            }
            else if(v != isUnitMap.end() && v->second == UnitFalse){
                isUnitMap.erase(b.getRootVar());
                isUnitMap.insert({b.getRootVar(), NotUnit});
            }
        }
    }
    // Return a map of variables to their unit values
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

void Solver::bdd2Qcir(std::ostream& s, Sylvan_Bdd bdd) const {
    s << "#QCIR-G14" << endl;
    std::set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({bdd}); 
    //BFS, then output result in reverse order
    //TODO

    // To compute prefix, make sure to only include those variables, that are in the BDD (could be a smaller set than the ones in the initial circuit)
    // This can e.g. be done by keeping track of all visited variables in the BFS, thus only adding variables found in the BFS to the prefix
    // Using the quantAt-function on the original Circuit object, we can easily look up the quantification of a specific variable.

    // Idea: after computing matrix and building some set of found vars, loop over blocks in initial circuit and build prefix using these blocks,
    // but only including the variables found in the BFS.
}

map<int,int> varsInBdd(Sylvan_Bdd b){
    map<int,int> varmap;
    int newname = 1;

    set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({b}); 
    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        if (visited.count(b)==0){ 
            visited.insert(b);
            todo.push_back(b.lo());
            todo.push_back(b.hi());

            if(varmap.count(b.getRootVar())==0) varmap.insert({b.getRootVar(),newname++}); 
        }
    }
    return varmap;
}

void Solver::bdd2CNF(std::ostream& s, Sylvan_Bdd bdd) const {
    // same idea as above in terms of building matrix first, prefix second. We do not need to reverse the list of clauses (unlike QCIR, we don't have gates, 
    // so we don't need to make sure that a gate is declared before it is used as input to some other gate). 
    // However, we do need to add the fresh "gate variables" to an innermost existential block
    //  - adding them to an outer block is not necessarily sound (TODO: why?)


    std::map<Sylvan_Bdd, int> gatevars; // give every node in the BDD a unique variable name
    vector<vector<int>> clauses;        // Clauses are vectors of literals, the ORs are implicit

    std::map<int,int> varmap = varsInBdd(bdd); // Keep track of which input variables actually appear in BDD, likely fewer than in initial circuit
    int fresh_gate_var = varmap.size()+1;

    std::set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({bdd}); 

    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        
        if (visited.count(b)== 0){ // Node has not yet been visited
            visited.insert(b);
            if(b==Sylvan_Bdd(false) || b == Sylvan_Bdd(true)) {continue;}

            todo.push_back(b.lo());
            todo.push_back(b.hi());
            // b.root is a non-terminal node, i.e. represents some variable.
            if(gatevars.count(b)==0) {gatevars.insert({b,fresh_gate_var}); fresh_gate_var++;}
            if(gatevars.count(b.lo())==0 && !b.lo().isConstant()) gatevars.insert({b.lo(),fresh_gate_var++});
            if(gatevars.count(b.hi())==0 && !b.hi().isConstant()) gatevars.insert({b.hi(),fresh_gate_var++});

            // Node n, var x, children l and h
            int n = gatevars.at(b);
            int l = b.lo().isConstant() ? 100 : gatevars.at(b.lo()); 
            int h = b.hi().isConstant() ? 200 : gatevars.at(b.hi()); 
            int x = varmap.at(b.getRootVar());
            
            
    
            //TODO: refactor and remove code duplication
            if(b.lo()==Sylvan_Bdd(true)){
                clauses.push_back({n,x});
            } else if (b.lo()==Sylvan_Bdd(false)){
                clauses.push_back({-n,x});
            } else {
                clauses.push_back({-n,x,l});
                clauses.push_back({n,x,-l});
            }

            if(b.hi()==Sylvan_Bdd(true)){
                clauses.push_back({n,-x});
            } else if (b.hi()==Sylvan_Bdd(false)){
                clauses.push_back({-n,-x});
            } else {
                clauses.push_back({-n,-x,h});
                clauses.push_back({n,-x,-h});
            }

            if(b.lo()==Sylvan_Bdd(true) && !b.hi()==Sylvan_Bdd(false)) {
                clauses.push_back({n,-h});
            } else if (b.hi()==Sylvan_Bdd(true) && !b.lo()==Sylvan_Bdd(false)) { 
                clauses.push_back({n,-l});
            } else if (!b.hi().isConstant() && !b.lo().isConstant()) { 
                clauses.push_back({n,-l,-h});
            }

        }
    }

    s << "p cnf " << varmap.size() + gatevars.size() << " " << clauses.size() << endl; // QDIMACS needs specified the number of input variables and the number of clauses

    for(int i = 0; i < c.maxBlock(); i++){
        
        Block b = c.getBlock(i);
        bool empty = true;
        for(int j = 0; j < b.variables.size(); j++){
            if(varmap.count(b.variables[j])==1){           
                empty = false;                               
            }
        }
        if(empty) continue;
        string q = b.quantifier==Forall ? "a" : "e";
        s << q << " ";
        for(int j = 0; j < b.variables.size(); j++){
            if(varmap.count(b.variables[j])==1){           // Only put variables in the prefix, which are present in the BDD
                s << varmap.at(b.variables[j]) << " ";     // Use the new variable name
            }
        }
        s << 0 << endl;
    }
    // Put Tseitin variables in the inner-most (existential) block:
    s << "e ";
    for(int i = varmap.size()+1; i < varmap.size() + gatevars.size() +1; i++){
        s << i << " ";
    }
    s << 0 << endl;
    // Prefix has been printed, what is left is the matrix

    for(int i = 0; i < clauses.size(); i++){
        for(int j = 0; j< clauses[i].size(); j++){
            s << clauses[i][j] << " ";
        }
        s << 0 << endl;
    }

}


Sylvan_Bdd Solver::unitpropagation(Sylvan_Bdd bdd) {
 
    map<int,bool> units = detectUnitLits(bdd);
    map<int, bool>::iterator it;
    vector<Sylvan_Bdd> unitbdds; 
    for(it = units.begin(); it != units.end(); it++){
        unitbdds.push_back(it->second ? Sylvan_Bdd(it->first) : !Sylvan_Bdd(it->first));
        restricted_vars.push_back(it->second ? it->first : -(it->first)); LOG(1,"Detected unit var:" << it->first << endl);
    }
    for(int i = 0; i < unitbdds.size(); i++){
        bdd = bdd.restrict(unitbdds[i]); 
    }
    return bdd;
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

        if(i==c.maxBlock()-5) {
            std:: ofstream myfile;
            myfile.open(file + "_inPrefix.qdimacs");
            bdd2CNF(myfile, matrix); // << print debugging, looks okay as of now
            myfile.close();
        }
    }
}
