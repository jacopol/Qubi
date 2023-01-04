// (c) Jaco van de Pol
// Aarhus University

#ifndef QCIR_IO
#define QCIR_IO

#include "circuit.hpp"

class Qcir_IO {
    private:
        Circuit &c ; // wrapped circuit to read/write

    public:
        Qcir_IO(Circuit &);                      // initialize with an (empty) circuit
        void readQcir(std::istream&);            // read from qcir file format
        void writeQcir(std::ostream&) const;     // write to qcir file format
        void writeVal(std::ostream&, const Valuation&) const; // write valuation 

    private:
        map<string,int> vars;       // map external var/gate names to identifiers
        string outputline;          // save outputline
        int outputlineno;           // save line number of outputline

        // for printing:
        void commaSeparate(std::ostream& s, const vector<int>& literals) const;
        string litString(int literal) const;

        // for parsing:
        int lineno=0;

        void setVarOrGate(const string& var, int i);
        void createGate(const string& gate);
        int createVar(const string& var);
        vector<int> createVariables(string& line);

        int getVarOrGateIdx(const string& line) const;
        int getLiteral(const string& literal) const;        
        vector<int> getLiterals(string& line) const;
        Gate getGate(string& line) const;

        bool readOutput(string& line);
        bool readBlock(string& line);
        bool readGateDef(string& line);

};
#endif
