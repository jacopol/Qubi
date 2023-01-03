// (c) Jaco van de Pol
// Aarhus University

#ifndef QCIR_IO
#define QCIR_IO

#include "circuit.hpp"

class Qcir_IO {
    private:
        Circuit &c ; // wrapped circuit to read/write

    public:
        Qcir_IO(Circuit &);                     // initialize with an (empty) circuit
        Circuit& readQcir(std::istream &input); // read from qcir file format
        Circuit& writeQcir(std::ostream& s);    // write to qcir file format

    private:
        map<string,int> vars;       // map external var/gate names to identifiers
        string outputline;          // save outputline
        int outputlineno;           // save line number of outputline

        // for printing:
        void commaSeparate(std::ostream& s, const vector<int>& literals);
        string litString(int literal);

        // for parsing:
        int lineno=0;
        void setVarOrGate(const string& var, int i);
        void createGate(const string& gate);
        int createVar(const string& var);
        int getVarOrGateIdx(const string& line);
        int getLiteral(const string& literal);        
        vector<int> getLiterals(string& line);
        vector<int> createVariables(string& line);
        Gate getGate(string& line);

        bool readOutput(string& line);
        bool readBlock(string& line);
        bool readGateDef(string& line);

};
#endif
