// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <regex>
#include <assert.h>
#include "qcir_io.hpp"
#include "messages.hpp"

using namespace std;

// Initialisation: this wraps around an (empty) Circuit reference
// lifetime of the circuit should survive qcir_io

Qcir_IO::Qcir_IO(Circuit& circuit, bool keep_names) : c(circuit) {   
    c.varnames = vector<string>({""}); // start from 1st position
    keep = keep_names;
}

/*** Writing QCIR specification ***/

// convert a literal to a string
string Qcir_IO::litString(int literal) {
    if (!keep) {
        return to_string(literal);
    } else {
        if (literal > 0) {
            return c.varnames[literal];
        } else {
            return "-" + c.varnames[-literal];
        }
    }
}

// output a vector of literals in comma-separated form
void Qcir_IO::commaSeparate(std::ostream& s, vector<int>literals) {
    if (literals.size()>0) {
        s << litString(literals[0]);
    }
    for (int i=1; i < literals.size(); i++) {
        s << ", " << litString(literals[i]);
    }
}

// write a QCIR specification to output
Circuit& Qcir_IO::writeQcir(std::ostream& s) {
    s << "#QCIR-G14" << endl;
    for (int i=0; i < c.maxBlock() ; i++) {
        Block b = c.getBlock(i);
        s << b.getQuantifier() << "(";
        commaSeparate(s, b.variables);
        s << ")" << endl;
    }
    s << "output(" << litString(c.getOutput()) << ")" << endl;
    for (int i=c.maxVar() ; i<c.maxGate(); i++) {
        Gate g = c.getGate(i);
        s << litString(i) << " = " << g.getConnective() << "(";
        commaSeparate(s, g.inputs);
        s << ")" << endl;
    }
    return c;
}


/// Reading QCIR specification

const string variable("[a-zA-z0-9_]+");
const string literal("-?" + variable);

void Qcir_IO::setVarOrGate(string var, int i) {
    assert(i==c.varnames.size());
    vars[var] = i;
    c.varnames.push_back(var);
}

// lookup an existing variable
// check that it exists
int Qcir_IO::getVarOrGateIdx(string line) {
    assertThrow(vars.find(line) != vars.end(), InputUndefined(line,lineno))
    return vars[line];
}

// create a new variable
// check that it didn't exist
int Qcir_IO::createVar(string line) {
    assertThrow(vars.find(line)==vars.end(), VarDefined(line,lineno))
    setVarOrGate(line, c.maxvar);
    return c.maxvar++;
}

// create a new gate
// check that it didn't exist
void Qcir_IO::createGate(string gatename) {
    assertThrow(vars.find(gatename) == vars.end(), VarDefined(gatename,lineno))
    setVarOrGate(gatename, c.maxGate());
}

int Qcir_IO::getLiteral(string line) {
    assertThrow(line.size()>0, InputUndefined(line,lineno));
    if (line[0] != '-') 
        return getVarOrGateIdx(line);
    else 
        return -getVarOrGateIdx(line.substr(1,line.size()));
}

vector<int> Qcir_IO::createVariables(string line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(variable))) {
        result.push_back(createVar(m[0]));
        line = m.suffix();
    }
    return result;
}

vector<int> Qcir_IO::getLiterals(string line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(literal))) {
        result.push_back(getLiteral(m[0]));
        line = m.suffix();
    }
    return result;
}

Circuit& Qcir_IO::readQcir(istream &input) {
    string line;
    string outputline;
    int outputlineno;
    const auto contains = [&line](string s)->bool 
        { return line.find(s) != string::npos; };
try {
    while (getline(input, line)) {
        lineno++;
        if (contains("#")) { // ignore comments
        }
        else if (contains("exists")) { // 6 characters
            line = line.substr(6,line.size());
            c.addBlock(Block(Exists, createVariables(line)));
        }
        else if (contains("forall")) { // 6 characters
            line = line.substr(6,line.size());
            c.addBlock(Block(Forall, createVariables(line)));
        }
        else if (contains("and")) { // 3 characters
            int pos = line.find("=");
            assertThrow(pos != string::npos,ParseError(line,lineno))
            string prefix = line.substr(0,pos-1);            // TODO: this is too fragile (whitespace)
            string suffix = line.substr(pos+5, line.size());
            createGate(prefix);
            c.addGate(Gate(And, getLiterals(suffix)));
        }
        else if (contains("or")) { // 2 characters
            int pos = line.find("=");
            assertThrow(pos != string::npos,ParseError(line,lineno))
            string prefix = line.substr(0,pos-1);       // TODO: this is too fragile (whitespace)
            string suffix = line.substr(pos+4, line.size());
            createGate(prefix);
            c.addGate(Gate(Or, getLiterals(suffix)));
        }
        else if (contains("output")) {
            outputline = line.substr(6,line.size());
            outputlineno = lineno;
        }
        else {
            assertThrow(false, ParseError(line, lineno));
        }
    }
    assertThrow(outputline != "", OutputMissing());
    line = outputline;
    lineno = outputlineno; // only to get correct error message
    smatch m;
    if (regex_search(line, m, regex(literal))) {
        c.setOutput(getLiteral(m[0]));
    }
    return c;
} 
catch (InputUndefined& err) { 
    cout << err.what() << endl; 
    exit(-1);
} 
catch (VarDefined& err) { 
    cout << err.what() << endl; 
    exit(-1);
} 
catch (ParseError& err) { 
    cout << err.what() << endl; 
    exit(-1);
} 
catch (OutputMissing& err) { 
    cout << err.what() << endl; 
    exit(-1);
}
}