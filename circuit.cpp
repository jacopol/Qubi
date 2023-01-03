// (c) Jaco van de Pol
// Aarhus University

#include <vector>
#include <iostream>
#include <assert.h>
#include "circuit.hpp"

Circuit::Circuit(string name_) {
    name = name_;
}

int Circuit::maxVar() const { 
    return maxvar; 
}

int Circuit::maxBlock() const {
    return prefix.size();
}

int Circuit::maxGate() const {
    return matrix.size() + maxvar;
}

Circuit& Circuit::addBlock(const Block& b) { 
    prefix.push_back(b);
    return *this;
}

const Block& Circuit::getBlock(int i) const { 
    assert(i<prefix.size());
    return prefix[i]; 
}

Circuit& Circuit::addGate(const Gate& g) {
    matrix.push_back(g);
    return *this;
}

const Gate& Circuit::getGate(int i) const {
    assert(maxvar <= i && i < maxvar + matrix.size());
    return matrix[i-maxvar];
}

Circuit& Circuit::setOutput(int out) {
    output = out;
    return *this;
}

int Circuit::getOutput() const {
    return output;
}

const string& Circuit::getVarOrGate(int i) const {
    assert(i<=varnames.size());
    return varnames[i];
}

void Circuit::printInfo(std::ostream &s) const {
    s   << "Quantified Circuit \"" << name << "\" (" 
        << maxvar-1 << " vars in " 
        << prefix.size() << " blocks, "
        << matrix.size() << " gates)" 
        << std::endl;
}
