#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <string>

using std::vector;
using std::string;

// where should this go?
enum Connective {And, Or};
enum Quantifier {Forall, Exists};

class Gate {
    private:
        Connective output;
        vector<int> inputs;

    public:
        Gate(Connective);
        Connective getConn();
        vector<int> getArgs();
        void addInput(int);
        void checkLess(int);
};

class Circuit {
    private:
        string name;
        vector<Quantifier> prefix; // start counting at 1 (so ignore entry 0)
        vector<Gate> matrix;       // gate definitions (shift by prefix.size())
        int output;                // output gate

    public:
        Circuit(string name);
        Circuit(string name, std::istream &input);
        int maxVar();
        int maxGate();
        void addVar(Quantifier);
        void addGate(Gate);
        void setOutput(int);
        int getOutput();
        Gate getGate(int);
        Quantifier getQuant(int);
        string Quant(int);
        vector<vector<int>> getBlocks(); // get alternating blocks in prefix
        void printInfo(std::ostream& s);
};

#endif