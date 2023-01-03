#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "set"
#include "vector"
#include "assert.h"
#include "circuit.hpp"


Circuit& Circuit::split() {
    vector<Block> oldprefix = prefix;
    prefix = vector<Block>();
    for (Block b : oldprefix) {
        for (int v : b.variables) {
            addBlock(Block(b.quantifier,vector<int>({v})));
        }
    }
    return *this;
}

Circuit& Circuit::combine() {
    if (maxBlock() == 0) {
        return *this;
    } else {
    vector<Block> oldprefix = prefix;
        prefix = vector<Block>();
        auto current = vector<int>();
        Quantifier q = oldprefix[0].quantifier;
        for (Block b : oldprefix) {
            if (b.quantifier != q) {
                addBlock(Block(q, current));
                current = vector<int>();
                q = b.quantifier;
            }
            for (int v : b.variables) {
                current.push_back(v);
            }
        }
        addBlock(Block(q, current));
    }
    return *this;
}

#include <iostream>
Circuit& Circuit::reorder() {
    int next=1;                                        // next variable index to use
    auto permutation = std::vector<int>(maxVar(),0);   // no indices are assigned yet
    auto allvars = std::set<int>(); 
    for (int i=1; i<maxGate(); i++)                    // all vars/gates are yet unseen
        allvars.insert(i);

    // DFS search starting from output
    // - remove vars/gates from allvars
    // - add vars->next to permutation

    auto todo = vector<int>({abs(getOutput())});
    while (todo.size()!=0) {
        int v = todo.back();
        todo.pop_back();
        if (allvars.count(v)!=0) { // var/gate is now visited the first time
            if (v<maxVar()) {
                allvars.erase(v);
                permutation[v] = next++;              // map variable v to next index
            } else {                                  
                allvars.erase(v);
                for (int x: getGate(v).inputs)        // visit all inputs of gate v
                    todo.push_back(abs(x));
            }
        }
    }

    // Add unused variables to the 

    std::cerr << "Unused variables/gates: ";
    for (int x : allvars) {
        std::cerr << getVarOrGate(x) << ", "; // one , too much
        if (x<maxVar()) 
            permutation[x] = next++;
    }
    std::cerr << std::endl;
    assert(next == maxVar());

    // Apply the permutation ...
    // ...to prefix
    for (Block &b : prefix)
        for (int i=0; i< b.variables.size(); i++)
            b.variables[i] = permutation[b.variables[i]];

    // ...to matrix
    for (Gate &g : matrix)
        for (int i=0; i< g.inputs.size(); i++) {
            int &input = g.inputs[i];
            if (abs(input)<maxVar()) {
                if (input<0) 
                    input = -permutation[-input];
                else
                    input = permutation[input];
            }
        }

    // ...to varnames
    auto newnames = vector<string>(maxVar(),"");
    for (int i=1; i<maxVar(); i++) 
        newnames[permutation[i]] = varnames[i];
    for (int i=1; i<maxVar(); i++) 
        varnames[i] = newnames[i];

    // ...to output
    if (abs(output)<maxVar()) {
        if (output > 0)
            output = permutation[output];
        else
            output = -permutation[-output];
    }

    return *this;
}

#endif