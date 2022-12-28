#include <vector>
#include <iostream>
#include <assert.h>
#include "circuit.hpp"

const vector<string> Q = {"Forall", "Exists"};

Gate::Gate(Connective c) {
    output=c;
}

Connective Gate::getConn() {
    return output;
}

vector<int> Gate::getArgs() {
    return inputs;
}

void Gate::addInput(int i) {
    inputs.push_back(i);
}

void Gate::checkLess(int max) {
    for (int i : inputs) {
        assert(0 < abs(i) && abs(i) < max);
    }
}

Circuit::Circuit(string s) {
    // index 0 will never be used to avoid confusion 0/-0
    prefix.push_back(Forall);
    name=s;
}
void Circuit::addVar(Quantifier q) {
    prefix.push_back(q);
}
int Circuit::maxVar() {
    return prefix.size();
}
int Circuit::maxGate() {
    return prefix.size()+matrix.size();
}
void Circuit::addGate(Gate g) {
    // check that all inputs are defined
    g.checkLess(prefix.size() + matrix.size());
    matrix.push_back(g);
}
void Circuit::setOutput(int i) {
    int max = prefix.size() + matrix.size();
    assert(0 < abs(i) && abs(i) < max);
    output = i;
}
int Circuit::getOutput() {
    return output;
}

Quantifier Circuit::getQuant(int i) {
    assert(0 < i && i < prefix.size());
    return prefix[i];
}
string Circuit::Quant(int i) {
    return Q[getQuant(i)];
}

Gate Circuit::getGate(int i) {
    assert(prefix.size() <= i && i < matrix.size() + prefix.size());
    return matrix[i-prefix.size()];
}
vector<vector<int>> Circuit::getBlocks() {
    vector<vector<int>> allBlocks;
    vector<int> curBlock;
    Quantifier q = getQuant(1);
    for (int i=1; i<prefix.size();i++) {
        if (getQuant(i) == q) {
            curBlock.push_back(i);
        }
        else { // start a new block
            q = getQuant(i);
            allBlocks.push_back(curBlock);
            curBlock = vector<int>();
            curBlock.push_back(i);
        }
    }
    allBlocks.push_back(curBlock);
    return allBlocks;
}

void Circuit::printInfo(ostream &s) {
    s   << "Quantified Circuit \"" << name << "\", with " 
        << prefix.size()-1 << " variables, " 
        << matrix.size() << " gates, " 
        << getBlocks().size() << " quantifier blocks."
        << endl;
}