// (c) Jaco van de Pol
// Aarhus University

#include <vector>
#include <iostream>
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

void Circuit::addBlock(const Block& b) { 
    prefix.push_back(b);
}

const Block& Circuit::getBlock(int i) const { 
    return prefix.at(i); 
}

void Circuit::addGate(const Gate& g) {
    matrix.push_back(g);
}

const Gate& Circuit::getGate(int i) const {
    return matrix.at(i-maxvar);
}

void Circuit::setOutput(int out) {
    output = out;
}

int Circuit::getOutput() const {
    return output;
}

const string& Circuit::getVarOrGate(int i) const {
    return varnames.at(i);
}

void Circuit::printInfo(std::ostream &s) const {
    s   << "Quantified Circuit \"" << name << "\" (" 
        << maxvar-1 << " vars in " 
        << prefix.size() << " blocks, "
        << matrix.size() << " gates)" 
        << std::endl;
}
