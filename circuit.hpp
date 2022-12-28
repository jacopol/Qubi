#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <string>
#include <fstream>

using namespace std;

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
        Circuit(string name, ifstream &file);
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
        void printInfo(ostream& s);
};

// Exceptions:

struct PrefixOutOfBound : public exception {
    private:
        int index;
        int size;
    public:
        PrefixOutOfBound(int i, int s);
        string what();
};

struct MatrixOutOfBound : public exception {
    private:
        int index;
        int size;
    public:
        MatrixOutOfBound(int i, int s);
        string what();
};

struct InputUndefined : public exception { 
    private:
        int index;
        int size;
    public:
        InputUndefined(int i, int s);
        string what();
};

#endif