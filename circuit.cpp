#include <vector>
#include <iostream>
#include "circuit.hpp"

string Quant(Quantifier q) {
    vector<string> Q = {"Forall", "Exists"};
    return Q[q];
}

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
        if (abs(i)<=0 || abs(i)>=max) throw InputUndefined(i,max);
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
    if (abs(i)<=0 || abs(i)>=max) throw InputUndefined(i,max);
    output = i;
}
int Circuit::getOutput() {
    return output;
}

Quantifier Circuit::getQuant(int i) {
    if (0<i && i<prefix.size()) { 
        return prefix[i];
    }
    else {
        throw PrefixOutOfBound(i,prefix.size());
    }
}

Gate Circuit::getGate(int i) {
    if (prefix.size()<=i && i<matrix.size() + prefix.size()) { 
        return matrix[i-prefix.size()];
    }
    else {
        throw MatrixOutOfBound(i,matrix.size() + prefix.size());
    }
}

vector<vector<int>> Circuit::getBlocks() {
    vector<vector<int>> allBlocks;
    vector<int> curBlock;
    Quantifier q = getQuant(1);
    for (int i=1; i<prefix.size();i++) {
        if (getQuant(i)==q) {
            curBlock.push_back(i);
        }
        else {
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

MatrixOutOfBound::MatrixOutOfBound(int i, int s) {
    index = i;
    size = s;
}
string MatrixOutOfBound::what() {
    return "Gate " + to_string(index) + " is not defined [1.." + to_string(size-1) + "]";
}

PrefixOutOfBound::PrefixOutOfBound(int i, int s) {
    index = i;
    size = s;
}
string PrefixOutOfBound::what() {
    return "Variable " + to_string(index) + " is not in Prenex [1.." + to_string(size-1) + "]";
}

InputUndefined::InputUndefined(int i, int s) {
    index = i;
    size = s;
}
string InputUndefined::what() {
    return "Input " + to_string(index) + " to Gate " + to_string(size) + " is not yet defined";
}
