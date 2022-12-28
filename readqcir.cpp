#include <iostream>
#include <fstream>
#include <regex>
#include <assert.h>
#include "circuit.hpp"

using namespace std;

const regex number("[1-9][0-9]*");

void addArgs(Gate &g, string line) {
    // first skip over the = in gate definition
    line = line.substr(line.find("="),line.size());
    // next collect all numbers in the remainder
    smatch m;
    while (regex_search(line, m, number)) {
        g.addInput(stoi(m[0]));
        line = m.suffix();
    }
}

Circuit::Circuit(string filename, ifstream &infile) {
    Circuit c(filename); // we start constructing an empty circuit
    string line;
    const auto contains = [&line](string s)->bool 
        { return line.find(s) != string::npos; };
    while (getline(infile, line)) {
        if (contains("#QCIR-G14")) { // ignore header-line
        }
        else if (contains("exists")) {
            smatch m;
            while (regex_search(line, m, number)) {
                assert(stoi(m[0]) == c.maxVar());
                c.addVar(Exists); // identified by position
                line = m.suffix();
            }
        }
        else if (contains("forall")) {
            smatch m;
            while (regex_search(line, m, number)) {
                assert(stoi(m[0]) == c.maxVar());
                c.addVar(Forall); // identified by position
                line = m.suffix();
            }
        }
        else if (contains("and")) {
            Gate g(And);
            addArgs(g, line);
            c.addGate(g);
        }
        else if (contains("or")) {
            Gate g(Or);
            addArgs(g, line);
            c.addGate(g);
        }
        else if (contains("output")) {
            smatch m;
            regex_search(line, m, number);
            output = stoi(m[0]); // store output directly
        }
        else {
            cout << "Line not recognized: " << line << endl;
            exit(-1);
        }
    }
    c.setOutput(output); // only to check the output gate
    // output is already set
    name = c.name;
    prefix = c.prefix;
    matrix = c.matrix;
}
