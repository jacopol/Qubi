// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <set>
#include <assert.h>
#include "circuit.hpp"
#include "settings.hpp"

// default string when variable names are unknown
const string& Circuit::varString(int i) const {
    return *(new string("id:" + std::to_string(i))); 
}

void Circuit::printInfo(std::ostream &s) const {
    s   << "Quantified Circuit \"" << myname << "\" (" 
        << maxvar-1 << " vars in " 
        << prefix.size() << " blocks, "
        << matrix.size() << " gates)" 
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
    LOG(1, "Reordering Variables" << std::endl)
    int next=1;                                        // next variable index to use
    auto reordering = std::vector<int>(maxVar(),0);   // no indices are assigned yet
    auto allvars = std::set<int>(); 
    for (int i=1; i<maxGate(); i++)                    // all vars/gates are yet unseen
        allvars.insert(i);

    // DFS search starting from output
    // - remove vars/gates from allvars
    // - add vars->next to reordering

    auto todo = vector<int>({abs(getOutput())});
    while (todo.size()!=0) {
        int v = todo.back();
        todo.pop_back();
        if (allvars.count(v)!=0) { // var/gate is now visited the first time
            if (v<maxVar()) {
                allvars.erase(v);
                reordering[v] = next++;              // map variable v to next index
            } else {                                  
                allvars.erase(v);
                for (int x: getGate(v).inputs)        // visit all inputs of gate v
                    todo.push_back(abs(x));
            }
        }
    }

    // Add unused variables to the 

    LOG(2,"Unused variables/gates: ");
    for (int x : allvars) {
        LOG(2, varString(x) << ", "); // one , too much
        if (x<maxVar()) 
            reordering[x] = next++;
    }
    LOG(2, std::endl);
    assert(next == maxVar());

    // Store the inverse permutation (to trace back original names)
    permutation = std::vector<int>(maxVar(),0);
    for (int i=1; i<maxVar(); i++)
        permutation[reordering[i]] = i;

    // Apply the reordering ...
    // ...to prefix
    for (Block &b : prefix)
        for (int &var : b.variables)
            var = reordering[var];

    // ...to matrix
    for (Gate &g : matrix)
        for (int &input : g.inputs) {
            if (abs(input) < maxVar()) {
                if (input > 0) 
                    input = reordering[input];
                else
                    input = -reordering[-input];
            }
        }

    // ...to output
    if (abs(output) < maxVar()) {
        if (output > 0)
            output = reordering[output];
        else
            output = -reordering[-output];
    }

    return *this;
}
