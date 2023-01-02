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
        s << Qtext[b.quantifier] << "(";
        commaSeparate(s, b.variables);
        s << ")" << endl;
    }
    s << "output(" << litString(c.getOutput()) << ")" << endl;
    for (int i=c.maxVar() ; i<c.maxGate(); i++) {
        Gate g = c.getGate(i);
        s << litString(i) << " = " << Ctext[g.output] << "(";
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

// create a new gate check that it didn't exist
void Qcir_IO::createGate(string gatename) {
    assertThrow(vars.find(gatename) == vars.end(), VarDefined(gatename,lineno))
    setVarOrGate(gatename, c.maxGate());
}

// create a new variable and check that it didn't exist
int Qcir_IO::createVar(string line) {
    assertThrow(vars.find(line)==vars.end(), VarDefined(line,lineno))
    setVarOrGate(line, c.maxvar);
    return c.maxvar++;
}

// Note: currently, we allow any separators, not just ','
vector<int> Qcir_IO::createVariables(string line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(variable))) {
        result.push_back(createVar(m[0]));
        line = m.suffix();
    }
    return result;
}

int Qcir_IO::getLiteral(string line) {
    assertThrow(line.size()>0, InputUndefined(line, lineno));
    if (line[0] != '-') 
        return getVarOrGateIdx(line);
    else 
        return -getVarOrGateIdx(line.substr(1, line.size()));
}

// Note: currently, we allow any separators, not just ','
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
        
        // remove spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());

        // ignore empty lines and comments
        if (line.size() == 0 || line[0] == '#') {
            continue; // next line
        }

        // save output line for later processing
        const string OUTPUT = "output";
        if (line.find(OUTPUT)==0) {
            outputline = line.erase(0, OUTPUT.size());
            outputlineno = lineno;
            continue; // next line
        }

        // process quantifier block
        bool foundQ = false;
        for (Quantifier q : Quantifiers) {
            string qtext = Qtext[q];
            if (line.find(qtext)==0) {
                foundQ = true;
                line.erase(0, qtext.size());
                c.addBlock(Block(q, createVariables(line)));
                break; // skip other quantifiers
            }
        }
        if (foundQ) continue; // next line

        // Process gate definition "var = connective(lit1,...,litn)"
        int pos = line.find("=");
        assertThrow(pos != string::npos, ParseError(line, lineno));
        string gatename = line.substr(0, pos);
        line.erase(0, pos+1);

        // now line should contain connective(lit1,...,litn)
        Connective connective;
        bool foundC = false;
        for (Connective q : Connectives) {
            string ctext = Ctext[q];

            if (line.find(ctext)==0) {
                foundC = true;
                line.erase(0, ctext.length());
                connective = q;
            }
        }
        assertThrow(foundC, ConnectiveError(line, lineno));
        Gate g = Gate(connective, getLiterals(line));
        createGate(gatename);
        c.addGate(g);
        // continue next line
    }

    // Finally, process the postponed output-line
    assertThrow(outputline != "", OutputMissing(lineno));
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
catch (ConnectiveError& err) { 
    cout << err.what() << endl; 
    exit(-1);
} 
catch (OutputMissing& err) { 
    cout << err.what() << endl; 
    exit(-1);
}
catch (ParseError& err) { 
    cout << err.what() << endl; 
    exit(-1);
}
catch (QBFexception& err) { 
    cout << "Unexpected exception: " + err.what() << endl; 
    exit(-1);
}
}