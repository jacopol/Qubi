#ifndef QCIR_IO
#define QCIR_IO

#include "circuit.hpp"

class Qcir_IO {
    private:
        Circuit &c ; // wrapped circuit to read/write
        bool keep;   // keep original names or new ids

    public:
        Qcir_IO(Circuit &, bool keep_names);    // initialize with an (empty) circuit
        Circuit& readQcir(std::istream &input); // read from qcir file format
        Circuit& writeQcir(std::ostream& s);    // write to qcir file format

    private:
        map<string,int> vars;       // map external var/gate names to identifiers
        int getVarOrGateIdx(string line);

        // for printing:
        void commaSeparate(std::ostream& s, vector<int>literals);
        string litString(int literal);

        // for parsing:
        int lineno=0;
        void setVarOrGate(string var, int i);
        int createVar(string line);
        void createGate(string line);
        int getLiteral(string);        
        vector<int> getLiterals(string);
        vector<int> createVariables(string line);

};
#endif
