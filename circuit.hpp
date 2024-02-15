// (c) Jaco van de Pol
// Aarhus University

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <map>
#include <set>
#include <bitset>
#include <array>
#include <string>

using std::vector;
using std::map;
using std::array;
using std::string;
using std::pair;
using std::set;

enum Connective {And, Or, All, Ex};
constexpr array<Connective,4> Connectives = {And, Or, All, Ex};
const array<string,4> Ctext = {"and", "or", "forall", "exists"};

enum Quantifier {Forall, Exists};
constexpr array<Quantifier,2> Quantifiers = {Forall, Exists};
const array<string,2> Qtext = {"forall", "exists"};

typedef vector<pair<int,bool>> Valuation; // ordered list of pairs var->bool

// A Gate is a logical connective applied to a vector of inputs
// (negative numbers indicate logical negation)
// (the connective can be a quantifier with a vector of variables (non-prenex))

class Gate {
public:
    vector<int> inputs;
    vector<int> quants; // for Ex and All gates only
    Connective output;
    Gate(Connective c, const vector<int>& args) : inputs(args), output(c) { }
    Gate(Connective c, const vector<int>& vars, const vector<int>& args) : inputs(args), quants(vars), output(c) { }
    int operator[](int i) const  { return inputs[i]; }
    int size() const             { return inputs.size(); }
};

// A Block is a vector of variables with the same quantifier

class Block {
public:
    Quantifier quantifier;          
    vector<int> variables;
    Block(Quantifier q, const vector<int>& args) : quantifier(q), variables(args) { }
    int operator[](int i) const { return variables[i]; }
    int size() const            { return variables.size(); }
};

// Or: use vector<bool>? Union/Intersection become more cumbersome.
typedef std::bitset<1024UL> varset;

// A Circuit is a QBF (with sharing) consisting of 
// - a prefix: a vector of blocks of quantifiers [0..maxvar)
// - a matrix: a vector of gate definitions, interpreted as  [maxvar..maxvar+size)
// - output: a designated output gate

class Circuit {
private:
    vector<Block> prefix;       // blocks of quantifiers
    vector<Gate> matrix;        // gate definitions (shifted by -maxvar)
    int output;                 // output gate
    int maxvar=1;               // next unused variable, start at 1
    vector<string> varnames;    // map identifiers to external names, start at 1
    set<string> allnames;       // keeps all known names
    int freshname=0;            // to generate fresh names
    vector<Quantifier> quants;  // vector of quantifiers for quant(var) lookup

public:
    int maxVar() const                  { return maxvar; }
    int maxBlock() const                { return prefix.size(); }
    int maxGate() const                 { return matrix.size() + maxvar; }

    Circuit() : varnames({""})          {  } // var ids start from 1


    const Block& getBlock(int i) const  { return prefix.at(i); }
    void addBlock(const Block& b)       { prefix.push_back(b); }
    void addQuant(const Quantifier& q)  { quants.push_back(q); }
    const Quantifier& quantAt(int i) const{ return quants[i-1];  }

    // all Vars need to be created before all Gates
    int addVar(Quantifier q, string name="");                   // create input variable
    int addGate(const Gate& g, string name="");   // create new gate

    const string& varString(int i) const    { return varnames.at(abs(i)); }
    bool freshName(const string name) const { return allnames.count(name)==0; }
    const Gate& getGate(int i) const        { return matrix.at(i - maxvar); }
    
    int getOutput() const               { return output; }
    void setOutput(int out)             { output = out; }

    void printInfo(std::ostream& s) const;

    int buildConn(Connective c, const vector<int>& gates);
    int buildQuant(Connective c, const vector<int>& x, int gate);

// Transformations

    Circuit& split();           // every block gets single quantifier
    Circuit& combine();         // blocks become strictly alternating
    Circuit& flatten();         // flatten and/or gates (and/or become alternating)
    Circuit& cleanup();         // remove unused variables / gates. ONLY FOR PRENEX FORM
    Circuit& reorderDfs();      // reorder by order of appearance in DFS pass
    Circuit& reorderMatrix();   // reorder by order of appearance in matrix
    Circuit& prefix2circuit();  // move prefix on top of circuit gates
    Circuit& miniscope();       // move prefix down into circuit gates

private:
    Circuit& permute(vector<int>& reordering); // store and apply reordering
    void flatten_rec(int gate);
    void gather(int gate, int sign, vector<int>& args); // gather args with same connective 
    void mark(int gate, set<int>& mark); // mark all reachable variables and gates from gate
    vector<varset> posneg(); // compute positive / negative input dependencies per gate
    int bringitdown(Quantifier q, int x, int gate, const vector<varset>& dependencies);   
        // move quantifier (q x) into circuit below (gate), return new gate.
        // use the input-variable dependencies for each gate.
    Circuit& cleanup_matrix();  // Only cleanup matrix. Can be used for NON-PRENEX as well

};

#endif