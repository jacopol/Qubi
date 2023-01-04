// (c) Jaco van de Pol
// Aarhus University

#ifndef CIRCUIT_RW
#define CIRCUIT_RW

#include "circuit.hpp"

class CircuitRW : public Circuit {
    private:
        vector<string> varnames;                 // map identifiers to external names, start at 1

    public:
        CircuitRW(const string& name);
        const string& varString(int i) const;

        void readQcir(std::istream&);            // read from qcir file format
        void writeQcir(std::ostream&) const;     // write to qcir file format
        void writeVal(std::ostream&, const Valuation&) const; // write valuation 

    private:

        // for printing:
        void commaSeparate(std::ostream& s, const vector<int>& literals) const;
        string litString(int literal) const;

        // for parsing:
        int lineno=0;
        map<string,int> vars;       // map external var/gate names to identifiers
        string outputline;          // save outputline
        int outputlineno;           // save line number of outputline

        void setVarOrGate(const string& var, int i);
        void declGate(const string& gate);
        int declVar(const string& var);
        vector<int> declVars(string& vars);
        
        int getIndex(const string& ident) const;
        int readLiteral(const string& line) const;        
        vector<int> readLiterals(string& lits) const;
        Gate readGate(string& line) const;

        bool readBlock(string& line);
        bool readOutput(string& line);
        bool readGateDef(string& line);

};
#endif
