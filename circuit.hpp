// (c) Jaco van de Pol
// Aarhus University

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <map>
#include <array>
#include <string>

using std::vector;
using std::map;
using std::array;
using std::string;
using std::pair;

enum Connective {And, Or};
constexpr array<Connective,2> Connectives = {And, Or};
const array<string,2> Ctext = {"and", "or"};

enum Quantifier {Forall, Exists};
constexpr array<Quantifier,2> Quantifiers = {Forall, Exists};
const array<string,2> Qtext = {"forall", "exists"};

typedef vector<pair<int,bool>> Valuation; // ordered list of pairs var->bool

class Gate {
public:
    vector<int> inputs;
    Connective output;
    Gate(Connective c, const vector<int>& args) : inputs(args), output(c) { }
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
    private:
        string name;                // name of this circuit
        int maxvar=1;               // next unused variable, start at 1
        vector<Block> prefix;
        vector<Gate> matrix;        // gate definitions (shifted by -maxvar)
        int output;                 // output gate

        vector<string> varnames;    // map identifiers to external names, start at 1
                                    // initialized by friend class Qcir_IO class
    public:
        Circuit(string name);
        int maxVar() const;
        int maxBlock() const;
        int maxGate() const;

        const Block& getBlock(int i) const;
        void addBlock(const Block& b);

        const Gate& getGate(int i) const;
        void addGate(const Gate& g);
        
        int getOutput() const;
        void setOutput(int out);

        const string& getVarOrGate(int i) const;
        void printInfo(std::ostream& s) const;

    // Transformations

        Circuit& split();   // every block gets single quantifier
        Circuit& combine(); // blocks become strictly alternating
        Circuit& reorder(); // reorder by DFS pass

    // Delegating IO

        friend class Qcir_IO;
};

#endif