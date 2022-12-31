#include <iostream>
#include <regex>
#include "circuit.hpp"
#include "messages.hpp"

using namespace std;

/// Writing QCIR specification

string Circuit::litString(int literal) {
    if (literal > 0) {
        return varnames[literal];
    }
    else {
        return "-" + varnames[-literal];
    }
}

void Circuit::commaSeparate(std::ostream& s, vector<int>literals) {
    if (literals.size()>0) {
        s << litString(literals[0]);
    }
    for (int i=1; i < literals.size(); i++) {
        s << ", " << litString(literals[i]);
    }

}
void Circuit::writeQcir(std::ostream& s) {
    s << "#QCIR-G14" << endl;
    for (int i=0; i < maxBlock() ; i++) {
        Block b = getBlock(i);
        s << b.getQuantifier() << "(";
        commaSeparate(s, b.variables);
        s << ")" << endl;
    }
    s << "output(" << litString(output) << ")" << endl;
    for (int i=maxVar() ; i<maxGate(); i++) {
        Gate g = getGate(i);
        s << litString(i) << " = " << g.getConnective() << "(";
        commaSeparate(s, g.inputs);
        s << ")" << endl;
    }
}


/// Reading QCIR specification

const string variable("[a-zA-z0-9_]+");
const string literal("-?" + variable);

// lookup an existing variable
// check that it exists
int Circuit::getVarOrGate(string line) {
    assertThrow(vars.find(line) != vars.end(), InputUndefined(line,lineno))
    return vars[line];
}

// create a new variable
// check that it didn't exist
int Circuit::createVar(string line) {
    assertThrow(vars.find(line)==vars.end(), VarDefined(line,lineno))
    setVar(line, maxvar);
    return maxvar++;
}

// create a new gate
// check that it didn't exist
void Circuit::createGate(string gatename) {
    assertThrow(vars.find(gatename) == vars.end(), VarDefined(gatename,lineno))
    setVar(gatename, maxGate());
}

int Circuit::getLiteral(string line) {
    assertThrow(line.size()>0, InputUndefined(line,lineno));
    if (line[0] != '-') 
        return getVarOrGate(line);
    else 
        return -getVarOrGate(line.substr(1,line.size()));
}

vector<int> Circuit::createVariables(string line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(variable))) {
        result.push_back(createVar(m[0]));
        line = m.suffix();
    }
    return result;
}

vector<int> Circuit::getLiterals(string line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(literal))) {
        result.push_back(getLiteral(m[0]));
        line = m.suffix();
    }
    return result;
}

Circuit& Circuit::readQcir(istream &input) {
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
            addBlock(Block(Exists, createVariables(line)));
        }
        else if (contains("forall")) { // 6 characters
            line = line.substr(6,line.size());
            addBlock(Block(Forall, createVariables(line)));
        }
        else if (contains("and")) { // 3 characters
            int pos = line.find("=");
            assertThrow(pos != string::npos,ParseError(line,lineno))
            string prefix = line.substr(0,pos-1);            // TODO: this is too fragile (whitespace)
            string suffix = line.substr(pos+5, line.size());
            createGate(prefix);
            addGate(Gate(And, getLiterals(suffix)));
        }
        else if (contains("or")) { // 2 characters
            int pos = line.find("=");
            assertThrow(pos != string::npos,ParseError(line,lineno))
            string prefix = line.substr(0,pos-1);       // TODO: this is too fragile (whitespace)
            string suffix = line.substr(pos+4, line.size());
            createGate(prefix);
            addGate(Gate(Or, getLiterals(suffix)));
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
        setOutput(getLiteral(m[0]));
    }
    return *this;
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