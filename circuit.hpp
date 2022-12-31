#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <map>
#include <string>

using std::vector;
using std::map;
using std::string;

enum Connective {And, Or}; // could easily add Xor, Ite
enum Quantifier {Forall, Exists};

class Gate {
private:
    const vector<string> Ctext = {"and", "or"};
public:
    vector<int> inputs;
    Connective output;
    Gate(Connective c, vector<int>args) { output=c; inputs=args; }
    int operator[](int i)   { return inputs[i]; }
    int size()              { return inputs.size(); }
    string getConnective()  { return Ctext[output]; }
};

class Block {
private:
    const vector<string> Qtext = {"forall", "exists"};
public:
    Quantifier quantifier;          
    vector<int> variables;
    Block(Quantifier q, vector<int>args) { quantifier=q; variables=args; }
    int operator[](int i)   { return variables[i]; }
    int size()              { return variables.size(); }
    string getQuantifier()  { return Qtext[quantifier]; }
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
        int maxVar();
        int maxBlock();
        int maxGate();

        Block& getBlock(int i);
        Circuit& addBlock(Block b);

        Gate& getGate(int i);
        Circuit& addGate(Gate g);
        
        int getOutput();
        Circuit& setOutput(int out);

        string& getVarOrGate(int i);
        void printInfo(std::ostream& s);

        friend class Qcir_IO;
};

#endif