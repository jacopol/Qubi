// (c) Jaco van de Pol
// Aarhus University

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <map>
#include <set>
#include <array>
#include <string>

using std::vector;
using std::map;
using std::array;
using std::string;
using std::pair;

enum Connective {And, Or, All, Ex};
constexpr array<Connective,4> Connectives = {And, Or, All, Ex};
const array<string,4> Ctext = {"and", "or", "forall", "exists"};

enum Quantifier {Forall, Exists};
constexpr array<Quantifier,2> Quantifiers = {Forall, Exists};
const array<string,2> Qtext = {"forall", "exists"};

typedef vector<pair<int,bool>> Valuation; // ordered list of pairs var->bool

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

class Block {
public:
    Quantifier quantifier;          
    vector<int> variables;
    Block(Quantifier q, const vector<int>& args) : variables(args), quantifier(q) { }
    int operator[](int i) const { return variables[i]; }
    int size() const            { return variables.size(); }
};

class Circuit {
protected:
    int maxvar=1;               // next unused variable, start at 1
    vector<int> permutation;    // permutation of variables (empty if identity)

private:
    vector<Block> prefix;
    vector<Gate> matrix;        // gate definitions (shifted by -maxvar)
    int output;                 // output gate

public:
    int maxVar() const                  { return maxvar; }
    int maxBlock() const                { return prefix.size(); }
    int maxGate() const                 { return matrix.size() + maxvar; }

    const Block& getBlock(int i) const  { return prefix.at(i); }
    void addBlock(const Block& b)       { prefix.push_back(b); }

    const Gate& getGate(int i) const    { return matrix.at(i - maxvar); }
    void addGate(const Gate& g)         { matrix.push_back(g); }
    
    int getOutput() const               { return output; }
    void setOutput(int out)             { output = out; }

    virtual const string& varString(int i) const;
    void printInfo(std::ostream& s) const;

// Transformations

    Circuit& split();           // every block gets single quantifier
    Circuit& combine();         // blocks become strictly alternating
    Circuit& flatten();         // flatten and/or gates (and/or become alternating)
    Circuit& cleanup();         // remove unused variables / gates
    Circuit& reorderDfs();      // reorder by order of appearance in DFS pass
    Circuit& reorderMatrix();   // reorder by order of appearance in matrix
    Circuit& prefix2circuit();   // move prefix into circuit gates

    void posneg(); // experiment with presence of positive / negative occurrences

private:
    Circuit& permute(std::vector<int>& reordering); // store and apply reordering
    void flatten_rec(int gate);
    void gather(int gate, int sign, vector<int>& args); // gather args with same connective 
    void mark(int gate, std::set<int>& mark); // mark all reachable variables and gates from gate
};

#endif