#include "readqcir.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include "circuit.hpp"

using namespace std;

const regex number("[1-9][0-9]*");

void addArgs(Gate &g, string line) {
    smatch m;
    // first skip over the = in gate definition
    regex_search(line, m, regex("="));
    line = m.suffix();
    while (regex_search(line, m, number)) {
        g.addInput(stoi(m[0]));
        line = m.suffix();
    }
}

Circuit readCircuit(string filename) {
    ifstream infile(filename);
    if (infile.fail()) {
        cerr << "Could not open file: " << filename << endl;
        exit(-1);
    }
    Circuit c(filename);
    string line;
    int output;
    Gate g(And);
    while (getline(infile, line)) {
        if (regex_search(line, regex("#QCIR-G14"))) {}
        else if (regex_search(line,regex("exists"))) {
            smatch m;
            while (regex_search(line, m, number)) {
                c.addVar(Exists);
                line = m.suffix();
            }
        }
        else if (regex_search(line,regex("forall"))) {
            smatch m;
            while (regex_search(line, m, number)) {
                c.addVar(Forall);
                line = m.suffix();
            }
        }
        else if (regex_search(line,regex("output"))) {
            smatch m;
            if (regex_search(line, m, number)) {
                output = stoi(m[0]);
            }
        }
        else if (regex_search(line,regex("and"))) {
            Gate g(And);
            addArgs(g,line);
            c.addGate(g);
        }
        else if (regex_search(line,regex("or"))) {
            Gate g(Or);
            addArgs(g,line);
            c.addGate(g);
        }
        else {
            cout << "Line not recognized: " << line << endl;
            exit(-1);
        }
    }
    c.setOutput(output);
    return c;
}
