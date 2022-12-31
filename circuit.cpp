// (c) Jaco van de Pol
// Aarhus University

#include <vector>
#include <iostream>
#include <assert.h>
#include "circuit.hpp"

Circuit::Circuit(string name_) {
    name = name_;
}

int Circuit::maxVar() { 
    return maxvar; 
}

int Circuit::maxBlock() {
    return prefix.size();
}

int Circuit::maxGate() {
    return matrix.size() + maxvar;
}

Circuit& Circuit::addBlock(Block b) { 
    prefix.push_back(b);
    return *this;
}

Block& Circuit::getBlock(int i) { 
    assert(i<prefix.size());
    return prefix[i]; 
}

Circuit& Circuit::addGate(Gate g) {
    matrix.push_back(g);
    return *this;
}

Gate& Circuit::getGate(int i) {
    assert(maxvar <= i && i < maxvar + matrix.size());
    return matrix[i-maxvar];
}

Circuit& Circuit::setOutput(int out) {
    output = out;
    return *this;
}

int Circuit::getOutput() {
    return output;
}

string& Circuit::getVarOrGate(int i) {
    assert(i<=varnames.size());
    return varnames[i];
}

void Circuit::printInfo(std::ostream &s) {
    s   << "Quantified Circuit \"" << name << "\" (" 
        << maxvar-1 << " vars in " 
        << prefix.size() << " blocks, "
        << matrix.size() << " gates)" 
        << std::endl;
}
