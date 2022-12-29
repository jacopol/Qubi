#include <iostream>
#include <regex>
#include "circuit.hpp"
#include "messages.hpp"

using namespace std;

const regex number("-?[1-9][0-9]*");

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

Circuit::Circuit(string filename, istream &input) {
    Circuit c(filename); // we start constructing an empty circuit
    string line;
    const auto contains = [&line](string s)->bool 
        { return line.find(s) != string::npos; };
try {
    while (getline(input, line)) {
        if (contains("#")) { // ignore comments
        }
        else if (contains("exists")) {
            smatch m;
            while (regex_search(line, m, number)) {
                assertThrow(stoi(m[0]) == c.maxVar(), PrefixGap(stoi(m[0])));
                c.addVar(Exists); // identified by position
                line = m.suffix();
            }
        }
        else if (contains("forall")) {
            smatch m;
            while (regex_search(line, m, number)) {
                assertThrow(stoi(m[0]) == c.maxVar(), PrefixGap(stoi(m[0])));
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
    name   = c.name;
    prefix = c.prefix;
    matrix = c.matrix;
} catch (InputUndefined& err) { 
    cout << err.what() << endl; 
    exit(-1);
} catch (OutputUndefined& err) { 
    cout << err.what() << endl; 
    exit(-1);
} catch (MatrixOutOfBound& err) { 
    cout << err.what() << endl; 
    exit(-1);
} catch (PrefixOutOfBound& err) { 
    cout << err.what() << endl; 
    exit(-1);
} catch (PrefixGap& err) { 
    cout << err.what() << endl; 
    exit(-1);
}
}