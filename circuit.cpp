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
    LOG(1, "Combining Quantifiers" << std::endl);
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

Connective dual(Connective c) {
    if (c == And)
        return Or;
    if (c == Or)
        return And;
    assert(false);
}

// Gather all args under gate with the same connective modulo duality;
// keep track of pos/neg sign
void Circuit::gather(int gate, int sign, vector<int>& args) {
    Gate g = getGate(gate);
    assert(g.output == And || g.output == Or);
    for (int arg : g.inputs) {
        if (abs(arg) < maxvar) {
            args.push_back(arg * sign);
        }
        else {
            Connective child = getGate(abs(arg)).output;
            if (arg > 0 && child == g.output)
                gather(arg, sign, args);
            else if (arg < 0 && child == dual(g.output)) 
                gather(-arg, -sign, args);
            else 
                args.push_back(arg * sign);
        }
    }
}

// Flatten and/or starting in matrix, starting from gate. 
// This operation proceeds recursively, and completely in-situ.
// Note: after this operation, the matrix may have unused gates

void Circuit::flatten_rec(int gate) {
    assert(gate>0);
    if (gate >= maxVar()) {
        vector<int> newgates;
        Gate &g = matrix[gate-maxvar]; // cannot use getGate since we will update g
        gather(gate, 1, newgates);
        g.inputs = newgates;
        for (int arg : newgates) flatten_rec(abs(arg));
    }
}

Circuit& Circuit::flatten() {
    LOG(1, "Flattening Gates" << std::endl);
    flatten_rec(abs(output));
    return *this;
}

void Circuit::mark(int gate, std::set<int>& marking) { 
    assert(gate>0);
    marking.insert(gate);
    if (gate >= maxvar) 
        for (int arg : getGate(gate).inputs)
            mark(abs(arg), marking);
}

Circuit& Circuit::cleanup() {
    LOG(1, "Cleaning up Variables and Gates" << std::endl);
    std::set<int> marking;
    mark(abs(output), marking);
    std::vector<int> reordering(maxGate(),0);
    
    LOG(2, "...Removed Variables: ");
    int index=1; // new variable/gate-index

    int i=1; // old variables / gates-index
    vector<Block> newprefix;
    for (Block &b : prefix) {
        vector<int> newblock;
        for (int &x : b.variables) {
            if (marking.count(i)>0) {
                newblock.push_back(x);
                reordering[i] = index++;
            }
            else LOG(2, varString(x) << ", ");
            i++;
        }
        b.variables = newblock;
        if (newblock.size()>0) newprefix.push_back(b);
    }

    int newmaxvar = index;

    LOG(2, "\n...Removed Gates: ");

    vector<Gate> newmatrix;
    for (Gate &g : matrix) {
        if (marking.count(i)>0) {
            newmatrix.push_back(g);
            reordering[i] = index++;
        }
        else LOG(2, varString(i) << ", ");
        i++;
    }
    LOG(2, "\n");

    maxvar = newmaxvar;
    prefix = newprefix;
    matrix = newmatrix;

    permute(reordering); // update all indices

    return *this;
}

// Apply the reordering and store its inverse
// The reordering applies to variables and possibly to gates
Circuit& Circuit::permute(std::vector<int>& reordering) {
    // Store the inverse permutation (to trace back original names)

    // compose inverse of reordering with previous permutation
    {
    vector<int> oldperm(permutation);
    for (int i=0; i<reordering.size(); i++)
        permutation[reordering[i]] = oldperm[i];
    }

    // Apply the reordering ...
    const auto reorder = [&](int& x) {
        if (abs(x) < reordering.size()) {
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
    std::vector<int> reordering(maxVar(),0);   // no indices are assigned yet
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
//                for (int x: getGate(v).inputs)  // visit all inputs of gate v
//                    todo.push_back(abs(x));
            { const vector<int>& inputs = getGate(v).inputs;
              for (int i=inputs.size()-1; i>=0; i--)  // visit all inputs of gate v
                    todo.push_back(abs(inputs[i]));
            }
        }
    }

    // Add unused variables to reordering 
    LOG(2, "...Unused variables/gates: ");
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
    LOG(2, "...Unused variables/gates: ");
    for (int x : allvars) {
        LOG(2, varString(x) << ", "); // one , too much
        if (x < maxVar()) 
            reordering[x] = next++;
    }
    LOG(2, std::endl);
    assert(next == maxVar());

    return permute(reordering);
}

// todo: check #vars. Or: use vector<bool>? Union/Intersection become more cumbersome.
typedef std::bitset<256UL> varset;

void Circuit::posneg() {
    vector<varset> possets({varset()});
    vector<varset> negsets({varset()});

    // a variable occurs positively in itself
    for (int i=1; i<maxVar(); i++) {
        possets.push_back(varset().set(i));
        negsets.push_back(varset());
        LOG(2,"var " << i << " : ");
        for (int j=1; j<maxVar(); j++) { LOG(2, (possets[i][j] ? "1" : "0")); }
        LOG(2,std::endl);
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
        LOG(2, "pos " << i << " : ");
        for (int j=1; j<maxVar(); j++) { LOG(2, (possets[i][j] ? "1" : "0")); }
        LOG(2,std::endl);
        LOG(2, "neg " << i << " : ");
        for (int j=1; j<maxVar(); j++) { LOG(2, (negsets[i][j] ? "1" : "0")); }
        LOG(2,std::endl);
    }

    for (int x : getBlock(maxBlock()-1).variables) {
        LOG(1, x << ": ");
        for (int y : getGate(output).inputs) {
            if (possets[abs(y)].test(x))
                LOG(1, y << ", ");
        }
        LOG(1, "\n");
    }
}
