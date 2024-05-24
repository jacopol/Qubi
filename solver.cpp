// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <deque>
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
    std:: ofstream myfile;
    myfile.open(file + "_fromBDD.qcir");
    bdd2Qcir(myfile, matrix); // << print debugging, looks okay as of now
    myfile.close();

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

        // TODO: generalize, double check that an indifferent unit lit. is a gen. unit lit.
        // TODO: restrict the gen. unit lit. in the other bdds.
        //Reichl synthesis specifically
        /*if(i==c.getOutput() && g.output == And){
            args = unitprop_general(args);
        }*/


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

vector<Sylvan_Bdd> Solver::unitprop_general(vector<Sylvan_Bdd> bdds){

    vector<Sylvan_Bdd> unitbdds;

    for(int i = 0; i < bdds.size(); i++){
        Sylvan_Bdd bdd = bdds[i];
        map<int,bool> units = detectUnitLits(bdd);
        map<int, bool>::iterator it;
        for(it = units.begin(); it != units.end(); it++){
            Sylvan_Bdd u = it->second ? Sylvan_Bdd(it->first) : !(Sylvan_Bdd(it->first));
            if(bdd.restrict(!u)==Sylvan_Bdd(false)){ // u is a generalized unit clause
                unitbdds.push_back(u);
                restricted_vars.push_back(it->second ? it->first : -(it->first));
            }
        }
    }
    
    for(int i = 0; i < unitbdds.size(); i++){
        Sylvan_Bdd u = unitbdds[i];
        for(int k = 0; k < bdds.size(); k++){
            bdds[k] = bdds[k].restrict(u);
        }
        LOG(1,"Detected unit var:" << u.getRootVar() << endl);
    }
    return bdds;
}


map<int,int> varsInBdd(Sylvan_Bdd bdd){
    map<int,int> varmap;
    int newname = 1;

    set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({bdd}); 
    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        if (visited.count(b)==0 && !b.isConstant()){ 
            visited.insert(b);
            todo.push_back(b.lo());
            todo.push_back(b.hi());

            if(varmap.count(b.getRootVar())==0) varmap.insert({b.getRootVar(),newname++}); 
        }
    }
    return varmap;
}



void Solver::bdd2Qcir(std::ostream& s, Sylvan_Bdd bdd) const {
    vector<string> gates;
    map<int,int> varmap = varsInBdd(bdd);
  
/*
    // BFS, gate variables must be declared in the file before used as input to other gates.
    std::set<Sylvan_Bdd> visited;
    std::deque<Sylvan_Bdd> todo({bdd}); 
    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.front(); todo.pop_front();
        
        if (visited.count(b)== 0){ // Node has not yet been visited
            visited.insert(b);
            if(b.isConstant()) {continue;}

            todo.push_back(b.lo());
            todo.push_back(b.hi());

            if(gatevars.count(b)==0) {gatevars.insert({b,fresh_gate_var}); fresh_gate_var++;}
            if(gatevars.count(b.lo())==0) gatevars.insert({b.lo(),fresh_gate_var++});
            if(gatevars.count(b.hi())==0) gatevars.insert({b.hi(),fresh_gate_var++});

            string n = std::to_string(gatevars.at(b));
            string l = std::to_string(gatevars.at(b.lo())); 
            string h = std::to_string(gatevars.at(b.hi())); 
            string x = std::to_string(varmap.at(b.getRootVar()));

            gates.push_back(n+" = ite(" + x+ ", " + h + ", " + l+")");
        }
    }
    gates.push_back(std::to_string(gatevars.at(Sylvan_Bdd(false))) + " = or()"); // Encode 0-leaf as false
    gates.push_back(std::to_string(gatevars.at(Sylvan_Bdd(true))) + " = and()"); // Encode 1-leaf as true
*/
    // Print prefix

    s << "#QCIR-G14" << endl;

    for(int i = 0; i < c.maxBlock(); i++){
        Block b = c.getBlock(i);
        bool empty = true;
        for(int j = 0; j < b.variables.size(); j++){
            if(varmap.count(b.variables[j])==1){           
                empty = false;                               
            }
        }    
        if(empty) continue;
        string q = b.quantifier==Forall ? "forall" : "exists";
        s << q << "(";
        bool first = true;
        for(int j = 0; j < b.variables.size(); j++){
            if(varmap.count(b.variables[j])==1){
                if(!first) s << ", ";           // Only put variables in the prefix, which are present in the BDD
                s << varmap.at(b.variables[j]);     // Use the new variable name
                first = false;
            }
        }
        s << ")" << endl;
    }
    s << "output(" << c.getOutput() << ")" << endl; // set root node as output gate.

    for(int i = c.maxVar(); i <= abs(c.getOutput()); i++){
        Gate g = c.getGate(i);
        int gateisconstant = -1; // -1 indicates no, 0 indicates false (i.e. or()) and 1 indicates true (i.e. and())
        vector<int> args;
        for (int arg: g.inputs) {
            
            if(abs(arg) >= c.maxVar()){
                args.push_back(arg);
            } else if(varmap.count(abs(arg))==1 ){ // Include any non-trivial literals as well as any gates as input
                int argument = arg >= 0 ? varmap.at(abs(arg)) : -varmap.at(abs(arg));

                args.push_back(argument); // TODO: use new name instead
            } else{                                                    // The input variable does not appear in the BDD
                for(int j=0; j<restricted_vars.size(); j++){
                    if(abs(arg)==abs(restricted_vars[j])){
                        bool polarity = restricted_vars[j] >= 0;
                        bool arg_is_true = (arg >= 0) == polarity;     // does the input to the gate have the same sign, as what we assigned the variable?
                                                                        // I.e. if the input is -2, does -2 also appear in restricted_vars? Or is it 2?
                        if(g.output == And && !arg_is_true) gateisconstant = 0;
                        if(g.output == Or && arg_is_true) gateisconstant = 1;


                    }
                // There is no reason for an else-branch... if 'arg' is not any of the variables detected in unit propagation, 
                // then its truth value is completely irrelevant -- 'arg' does not appear in the matrix-BDD at all.
                // Thus there is no reason to add it to the list of arguments.
                }
            }
        }
        if(gateisconstant == -1){
            s << i << " = " << Ctext[g.output] << "(";
            bool first = true;
            for(int j = 0; j < args.size(); j++){
                if(!first) s << ", ";
                s << args[j];
                first = false;
            }
            s << ")" << endl;
        }
        else if(gateisconstant == 0){
            s << i << " = or()" << endl;
        } else if(gateisconstant == 1){
            s << i << " = and()" << endl;
        }
    }
}




void Solver::bdd2CNF(std::ostream& s, Sylvan_Bdd bdd) const {
    // same idea as above in terms of building matrix first, prefix second. We do not need to reverse the list of clauses (unlike QCIR, we don't have gates, 
    // so we don't need to make sure that a gate is declared before it is used as input to some other gate). 
    // However, we do need to add the fresh "gate variables" to an innermost existential block
    //  - adding them to an outer block is not necessarily sound (TODO: why?)


// give every node in the BDD a unique variable name
    vector<vector<int>> clauses;        // Clauses are vectors of literals, the ORs are implicit

    std::map<int,int> varmap = varsInBdd(bdd); // Keep track of which input variables actually appear in BDD, likely fewer than in initial circuit
    std::map<Sylvan_Bdd, int> gatevars;
    int fresh_gate_var = varmap.size()+1;

    std::set<Sylvan_Bdd> visited;
    vector<Sylvan_Bdd> todo({bdd}); 

    while (todo.size()!=0) {
        Sylvan_Bdd b = todo.back(); todo.pop_back();
        
        if (visited.count(b)== 0){ // Node has not yet been visited
            visited.insert(b);
            if(b.isConstant()) {continue;}

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
    if(!bdd.isConstant()) clauses.push_back({gatevars.at(bdd)}); // Similar to Tseitin transformation, we need the root node to always be visited one way or another


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
        if (b.quantifier == Forall){
            /*std:: ofstream myfile;
            myfile.open(file + "_inPrefix" + std::to_string(i)  +".qdimacs");
            bdd2CNF(myfile, matrix); // << print debugging, looks okay as of now
            myfile.close();
*/


            matrix = matrix.UnivAbstract(b.variables);
        }
        else
            matrix = matrix.ExistAbstract(b.variables);
        if (STATISTICS) { LOG(2," (" << matrix.NodeCount() << " nodes)"); }
        LOG(2,endl);

    }
}
