// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <regex>
#include <cassert>
#include <cctype>
#include <algorithm>
#include "circuit_rw.hpp"
#include "messages.hpp"
#include "settings.hpp"

using namespace std;

CircuitRW::CircuitRW(std::istream& file) { // start from 1st position
    readQcir(file);
}

/*** Writing QCIR specification ***/

// convert a literal to a string
string CircuitRW::litString(int literal) const {
    if (!KEEPNAMES)
        return to_string(literal);
    else if (literal > 0)
        return varString(literal);
    else
        return "-" + varString(-literal);
}

// output a vector of literals in comma-separated form
void CircuitRW::commaSeparate(std::ostream& s, const vector<int>& literals) const {
    if (literals.size()>0) 
        s << litString(literals[0]);
    for (size_t i=1; i < literals.size(); i++)
        s << ", " << litString(literals[i]);
}

// write a QCIR specification to output
void CircuitRW::writeQcir(std::ostream& s) const {
    s << "#QCIR-G14" << endl;
    for (int i=0; i < maxBlock() ; i++) {
        Block b = getBlock(i);
        s << Qtext[b.quantifier] << "(";
        commaSeparate(s, b.variables);
        s << ")" << endl;
    }
    s << "output(" << litString(getOutput()) << ")" << endl;
    for (int i=maxVar() ; i<maxGate(); i++) {
        Gate g = getGate(i);
        s << litString(i) << " = " << Ctext[g.output] << "(";
        if (g.output==All || g.output==Ex) {
            commaSeparate(s,g.quants);
            s << "; ";
        }            
        commaSeparate(s, g.inputs);
        s << ")" << endl;
    }
}

/*** Writing a Valuation ***/

void CircuitRW::writeVal(std::ostream& s, const Valuation& val) const {
    for (pair<int,bool> v : val) {
        s << varString(v.first) << "=" 
        << (v.second ? "true " : "false ");
    }
    cout << endl;
}

/*** Reading QCIR specification ***/

const string variable("[a-zA-z0-9_]+");
const string literal("-?" + variable);

// Note: currently, we allow any separators, not just ','
vector<int> CircuitRW::declVars(string &line) {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(variable))) {
        string var = m[0];
        assertThrow(freshName(var), VarDefined(var,lineno));
        int i = addVar(var);
        vars[var] = i;
        result.push_back(i);
        line = m.suffix();
    }
    return result;
}

// lookup an existing variable and check that it exists
int CircuitRW::getIndex(const string& line) const {
    assertThrow(vars.find(line) != vars.end(), InputUndefined(line,lineno))
    return vars.at(line);
}

int CircuitRW::readLiteral(const string& line) const {
    assertThrow(line.size()>0, InputUndefined(line, lineno));
    if (line[0] != '-') 
        return getIndex(line);
    else 
        return -getIndex(line.substr(1, line.size()));
}

// Note: currently, we allow any separators, not just ','
vector<int> CircuitRW::readLiterals(string& line) const {
    vector<int> result;
    smatch m;
    while (regex_search(line, m, regex(literal))) {
        result.push_back(readLiteral(m[0]));
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
    for (size_t pos=0; pos<keyword.size(); pos++) {
    if (tolower(line[pos]) != keyword[pos])
        return false;
    }
    line.erase(0,keyword.size());
    return true;
}

// Check and match line as "connective(lit1,...,litn)"
Gate CircuitRW::readGate(string& line) const {
    for (Connective q : Connectives) {
        string ctext = Ctext[q];
        if (find_keyword(line, ctext))
            return Gate(q, readLiterals(line));
    }
    // the line didn't match
    assertThrow(false, ConnectiveError(line, lineno));
}

// Process block declaration "quantifier(var1,...,varn)"
bool CircuitRW::readBlock(string& line) {
    for (Quantifier q : Quantifiers) {
        string qtext = Qtext[q];
        if (find_keyword(line, qtext)) {
            addBlock(Block(q, declVars(line)));
            return true; // skip other quantifiers
        }
    }
    return false;
}

// Recognize output definition "output(literal)"
// save the output line for later processing
bool CircuitRW::readOutput(string& line) {
    if (find_keyword(line, "output")) {
        outputline = line;
        outputlineno = lineno;
        return true;
    }
    else return false;
}

// Process gate definition "gatename = connective(lit1,...,litn)"
bool CircuitRW::readGateDef(string& line) {
    size_t pos = line.find("=");
    if (pos == string::npos) return false;
    string gatename = line.substr(0, pos);
    line.erase(0, pos+1);
    // Note: 
    // - we need to check the gate before creating the gate name
    // - we need to add the gate after creating the gate name
    assertThrow(freshName(gatename), VarDefined(gatename,lineno))
    int i = addGate(readGate(line), gatename);
    vars[gatename] = i;
    return true;
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

void CircuitRW::readQcir(istream &input) {
    string line;
try {
    while (getline(input, line)) {
        lineno++;
        
        // remove spaces
        removeWhiteSpace(line);

        // ignore empty lines and comments
        if (line.size() == 0 || line[0] == '#') continue; // next line

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
    if (regex_search(line, m, regex(literal)))
        setOutput(readLiteral(m[0]));
}
catch (QBFexception& err) { 
    cout << err.what() << endl; 
    exit(-1);
}
}