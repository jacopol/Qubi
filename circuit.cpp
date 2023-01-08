// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <set>
#include <assert.h>
#include "circuit.hpp"
#include "settings.hpp"
#include "messages.hpp"
#include <bitset>

// default string when variable names are unknown
const string& Circuit::varString(int i) const {
    return *(new string("id:" + std::to_string(i))); 
}

void Circuit::printInfo(std::ostream &s) const {
    s   << "Quantified Circuit: "
        << maxvar-1 << " vars in " 
        << prefix.size() << " blocks, "
        << matrix.size() << " gates" 
        << std::endl;
}

Circuit& Circuit::split() {
    LOG(1, "Splitting Quantifiers" << std::endl)
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
    LOG(1, "Combining Quantifiers" << std::endl)
    if (maxBlock() == 0) return *this;
    vector<Block> oldprefix = prefix;
    prefix = vector<Block>();
    vector<int> current;
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
    return *this;
}

// Store and apply the reodering
Circuit& Circuit::permute(std::vector<int>& reordering) {
    // Store the inverse permutation (to trace back original names)
    permutation = std::vector<int>(maxVar(),0);
    for (int i=1; i<maxVar(); i++)
        permutation[reordering[i]] = i;

    // Apply the reordering ...
    const auto reorder = [&](int& x) {
        if (abs(x) < maxVar()) {
            if (x > 0) 
                x = reordering[x];
            else
                x = -reordering[-x];
        }
    };
    // ...to prefix
    for (Block &b : prefix)
        for (int &var : b.variables)
            reorder(var);
    // ...to matrix
    for (Gate &g : matrix)
        for (int &input : g.inputs)
            reorder(input);
    // ...to output
    reorder(output);

    return *this;
}

Circuit& Circuit::reorderDfs() {
    LOG(1, "Reordering Variables (Dfs)" << std::endl)
    int next=1;                                // next variable index to use
    std::vector<int> reordering(maxVar());     // no indices are assigned yet
    std::set<int> allvars; 
    for (int i=1; i<maxGate(); i++)            // all vars/gates are yet unseen
        allvars.insert(i);

    // DFS search starting from output
    // - remove vars/gates from allvars
    // - add vars->next to reordering

    // Note: "todo" contains positive numbers only

    vector<int> todo({abs(getOutput())});
    while (todo.size()!=0) {
        int v = todo.back(); todo.pop_back();
        if (allvars.count(v)!=0) { // var/gate is now visited the first time
            allvars.erase(v);
            if (v < maxVar())
                reordering[v] = next++;         // map variable v to next index
            else
                for (int x: getGate(v).inputs)  // visit all inputs of gate v
                    todo.push_back(abs(x));
        }
    }

    // Add unused variables to reordering 
    LOG(2, "Unused variables/gates: ");
    for (int x : allvars) {
        LOG(2, varString(x) << ", "); // one , too much
        if (x < maxVar()) 
            reordering[x] = next++;
    }
    LOG(2, std::endl);
    assert(next == maxVar());

    return permute(reordering);
}

Circuit& Circuit::reorderMatrix() {
    LOG(1, "Reordering Variables (Matrix)" << std::endl)
    int next=1;                                // next variable index to use
    std::vector<int> reordering(maxVar());     // no indices are assigned yet
    std::set<int> allvars; 
    for (int i=1; i<maxGate(); i++)            // all vars/gates are yet unseen
        allvars.insert(i);

    // Just proceed through the matrix
    // - remove vars/gates from allvars
    // - add vars->next to reordering

    for (Gate g : matrix) {
        for (int x : g.inputs) {
            int v = abs(x);
            if (allvars.count(v)!=0) { // seeing var/gate for the first time
                allvars.erase(v);
                if (v < maxVar())
                    reordering[v] = next++;         // map variable v to next index
            }
        }
    }

    // Add unused variables to reordering 
    LOG(2, "Unused variables/gates: ");
    for (int x : allvars) {
        LOG(2, varString(x) << ", "); // one , too much
        if (x < maxVar()) 
            reordering[x] = next++;
    }
    LOG(2, std::endl);
    assert(next == maxVar());

    return permute(reordering);
}

// todo: check #vars. Or: use vector<bool>?
typedef std::bitset<96UL> varset;

void Circuit::posneg() {
    vector<varset> possets({varset()});
    vector<varset> negsets({varset()});

    // a variable occurs positively in itself
    for (int i=1; i<maxVar(); i++) {
        possets.push_back(varset().set(i));
        negsets.push_back(varset());
        LOG(2,"var " << i << " : " << possets[i] << std::endl);
    }
    // do gates
    for (int i=maxVar(); i<maxGate(); i++) {
        varset pos;
        varset neg;
        // this must change when we add xor and ite
        for (int lit : getGate(i).inputs) {
            if (lit > 0) { 
                pos |= possets[lit];
                neg |= negsets[lit];
            } else {
                pos |= negsets[-lit];
                neg |= possets[-lit];
            }
        }
        possets.push_back(pos);
        negsets.push_back(neg);
        LOG(2, "pos " << i << " : " << possets[i] << std::endl);
        LOG(2, "neg " << i << " : " << negsets[i] << std::endl);
    }
}