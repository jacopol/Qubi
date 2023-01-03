// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <regex>
#include <cassert>
#include <cctype>
#include <algorithm>
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
void Qcir_IO::commaSeparate(std::ostream& s, const vector<int>& literals) {
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

void Qcir_IO::setVarOrGate(const string& var, int i) {
    assert(i==c.varnames.size());
    vars[var] = i;
    c.varnames.push_back(var);
}

// lookup an existing variable and check that it exists
int Qcir_IO::getVarOrGateIdx(const string& line) {
    assertThrow(vars.find(line) != vars.end(), InputUndefined(line,lineno))
    return vars[line];
}

// create a new gate check that it didn't exist
void Qcir_IO::createGate(const string& gate) {
    assertThrow(vars.find(gate) == vars.end(), VarDefined(gate,lineno))
    setVarOrGate(gate, c.maxGate());
}

// create a new variable and check that it didn't exist
int Qcir_IO::createVar(const string& var) {
    assertThrow(vars.find(var)==vars.end(), VarDefined(var,lineno))
    setVarOrGate(var, c.maxvar);
    return c.maxvar++;
}

// Note: currently, we allow any separators, not just ','
vector<int> Qcir_IO::createVariables(string &line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(variable))) {
        result.push_back(createVar(m[0]));
        line = m.suffix();
    }
    return result;
}

int Qcir_IO::getLiteral(const string& line) {
    assertThrow(line.size()>0, InputUndefined(line, lineno));
    if (line[0] != '-') 
        return getVarOrGateIdx(line);
    else 
        return -getVarOrGateIdx(line.substr(1, line.size()));
}

// Note: currently, we allow any separators, not just ','
vector<int> Qcir_IO::getLiterals(string& line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(literal))) {
        result.push_back(getLiteral(m[0]));
        line = m.suffix();
    }
    return result;
}

// check if line starts with keyword and remove it
// assume that kewyord is in lowercase
// the match should be case-insensitive
bool find_keyword(string& line, const string& keyword) {
    if (keyword.size() > line.size())
        return false;
    for (int pos=0; pos<keyword.size(); pos++) {
    if (tolower(line[pos]) != keyword[pos])
        return false;
    }
    line.erase(0,keyword.size());
    return true;
}

// Check and match line as "connective(lit1,...,litn)"
Gate Qcir_IO::getGate(string& line) {
    Connective connective;
    for (Connective q : Connectives) {
        string ctext = Ctext[q];
        if (find_keyword(line, ctext)) {
            return Gate(q, getLiterals(line));
        }
    }
    // the line didn't match
    assertThrow(false, ConnectiveError(line, lineno));
}

// Process block declaration "quantifier(var1,...,varn)"
bool Qcir_IO::readBlock(string& line) {
    for (Quantifier q : Quantifiers) {
        string qtext = Qtext[q];
        if (find_keyword(line, qtext)) {
            c.addBlock(Block(q, createVariables(line)));
            return true; // skip other quantifiers
        }
    }
    return false;
}

// Process gate definition "gatename = connective(lit1,...,litn)"
bool Qcir_IO::readGateDef(string& line) {
    int pos = line.find("=");
    if (pos == string::npos) return false;
    string gatename = line.substr(0, pos);
    line.erase(0, pos+1);
    // Note: 
    // - we need to check the gate before creating the gate name
    // - we need to add the gate after creating the gate name
    Gate g = getGate(line);
    createGate(gatename);
    c.addGate(g);
    return true;
}

// Recognize output definition "output(literal)"
// save the output line for later processing
bool Qcir_IO::readOutput(string& line) {
    if (find_keyword(line, "output")) {
        outputline = line;
        outputlineno = lineno;
        return true;
    }
    else return false;
}

void removeWhiteSpace(string& line) {
    auto p2 = line.begin();
    for (auto p1=line.begin(); p1 != line.end(); p1++) {
        if (*p1 == ' ' or *p1 == '\t') continue;   // skip white space
        if (*p1 == '#') break;                     // skip comments until end of line
        *p2++ = *p1;                               // copy normal input
    }
    line.erase(p2, line.end());
}

Circuit& Qcir_IO::readQcir(istream &input) {
    string line;
try {
    while (getline(input, line)) {
        lineno++;
        
        // remove spaces
        removeWhiteSpace(line);

        // ignore empty lines and comments
        if (line.size() == 0 || line[0] == '#') {
            continue; // next line
        }

        if (readBlock(line)) continue;
        if (readOutput(line)) continue;
        if (readGateDef(line)) continue;

        // line could not be parsed
        assertThrow(false, ParseError(line, lineno));
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