// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <assert.h>
#include "circuit.hpp"
#include "settings.hpp"
#include "messages.hpp"

// TODO create fresh name if name==""

int Circuit::addVar(string name) {
    assert(matrix.size()==0); // must create all variables before any gate exists
    varnames.push_back(name);
    allnames.insert(name);
    permutation.push_back(maxvar);
    return maxvar++;
}

int Circuit::addGate(const Gate& g, string name)          { 
    matrix.push_back(g);
    int max = maxGate()-1;
    permutation.push_back(max);
    varnames.push_back(name);
    allnames.insert(name);
    return max; 
}

// default string when variable names are unknown
const string& Circuit::varString(int i) const { 
    const string& name = varnames.at(permutation.at(abs(i)));
    if (name=="")
        return *new string("?" + to_string(i)); // HACK: should invent unique names
    else 
        return name;
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

Connective dualC(Connective c) {
    if (c == And)
        return Or;
    if (c == Or)
        return And;
    assert(false);
}

Quantifier dualQ(Quantifier q) {
    if (q == Forall)
        return Exists;
    if (q == Exists)
        return Forall;
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
            else if (arg < 0 && child == dualC(g.output)) 
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
    if (gate >= maxVar()) 
        for (int arg : getGate(gate).inputs)
            mark(abs(arg), marking);
}

// TODO: This is only correct for PRENEX normal form

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

    permute(reordering); // update all indices, also updates output
    permutation.resize(index);

    return *this;
}

// TODO: Temporary version for MATRIX with quantifiers

Circuit& Circuit::cleanup_matrix() {
    std::set<int> marking;
    mark(abs(output), marking);
    std::vector<int> reordering(maxGate(),0);

    for (int i=1; i<maxVar(); i++) {
        reordering[i] = i;
    }

    int index=maxVar(); // new variable/gate-index
    int i=maxVar();     // old variables / gates-index

    LOG(2, "...Removed Gates: ");

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

    matrix = newmatrix;

    permute(reordering); // update all indices, also updates output
    permutation.resize(index);

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

Circuit& Circuit::prefix2circuit() {
    while (maxBlock()>1) { // keep outermost block...
        const Block& b = prefix.back();
        Connective c = (b.quantifier==Forall ? All : Ex);
        output = addGate(Gate(c,vector<int>(b.variables),vector<int>({output})));
        prefix.pop_back();
    };

    return *this;
}

vector<varset> Circuit::posneg() {
    vector<varset> possets({varset()});
    vector<varset> negsets({varset()});

    // a variable occurs positively in itself
    for (int i=1; i<maxVar(); i++) {
        possets.push_back(varset().set(i));
        negsets.push_back(varset());
        if (VERBOSE>=3) {
            std::cerr <<  "var " << i << " : ";
            for (int j=1; j<maxVar(); j++) { std::cerr << (possets[i][j] ? "1" : "0"); }
            std::cerr << std::endl;
        }
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
        if (VERBOSE>=3) {
            std::cerr << "pos " << i << " : ";
            for (int j=1; j<maxVar(); j++) { std::cerr << (possets[i][j] ? "1" : "0"); }
            std::cerr << std::endl;
            std::cerr << "neg " << i << " : ";
            for (int j=1; j<maxVar(); j++) { std::cerr << (negsets[i][j] ? "1" : "0"); }
            std::cerr << std::endl;
        }
    }

    vector<varset> dependencies;
    for (int i=0; i<maxGate(); i++) {
        dependencies.push_back(possets[i] | negsets[i]);
    }

    if (VERBOSE>=3) {
        for (int i=0; i<maxGate(); i++) {
            std::cerr << "dep " << i << " : ";
            for (int j=1; j<maxVar(); j++) { std::cerr << (dependencies[i][j] ? "1" : "0"); }
            std::cerr << std::endl;
        }
    }
    return dependencies;
}

Connective Quant2Conn(Quantifier q) { return (q==Forall ? All : Ex); }

// TODO: needs an operations cache?

int Circuit::bringitdown(Quantifier q, int x, int gate, const vector<varset>& dependencies) {
    if (abs(gate) < maxVar()) { // the gate is a single input variable (leaf of circuit)
        LOG(2,"Eliminating " << Qtext[q] << " " << varString(x) 
            << " over input " << varString(gate) << std::endl);
        if (abs(gate) == x) {
            if (q==Exists) { // Exist x (x) == Exists x (!x) == TRUE
                return addGate(Gate(And,vector<int>()));
            } else { // Forall x (x) == Forall x (!x) == FALSE
                return addGate(Gate(Or,vector<int>()));
            }
        } else {
            return gate; // ignore quantifier as in Exist/Forall x (y)
        }
    }

    Gate g = getGate(gate);
    LOG(2,"Pushing " << Qtext[q] << " " << varString(x) 
            << " over gate " << varString(gate) << " ("<< Ctext[g.output] << ")" << std::endl);
    if (g.inputs.size()==0)
        return gate; // short-cut. quantification over constant can be dropped.

    if ( (g.output==And && q==Forall) || (g.output==Or && q==Exists)) {
        // Can push Forall/Exists down directly to all arguments of And/Or.
        // Takes care of negative edges (pushes dual quantifier down)
        // As in Forall x (A1 /\ !A2 /\ A3) ==> (All x A1) /\ !(Ex x A2) /\ (All x A3)            
        vector<int> newargs;
        for (int i: g.inputs) {
            Quantifier newq = (i>0 ? q : dualQ(q));
            int newarg = bringitdown(newq, x, abs(i), dependencies);
            newarg = (i>0 ? newarg : -newarg);
            newargs.push_back(newarg);
        }
        return addGate(Gate(g.output,newargs));
    }

    if ( g.output==And || g.output==Or) {
        // split args in dependent and independent args
        // case distinction on split args empty
        // As in Exists x (A1(x) /\ A2(x) /\ A3 /\ A4) ==> A3 /\ A4 /\ Exists x ( A1(x) /\ A2(x))
        vector<int>args_pos;
        vector<int>args_neg;
        for (int arg : g.inputs) { 
            if (dependencies[abs(arg)][x]) {
                LOG(3,"pos: " << arg << std::endl)
                args_pos.push_back(arg);
            } else {
                args_neg.push_back(arg);
                LOG(3,"neg: " << arg << std::endl)
            }
        }
        
        if (args_pos.size()==0)
            return gate; // the quantified variable doesn't occur, just drop quantifier
        else {
            int newarg;
            if (args_pos.size()==1) {
                    // push quantifier into the only dependent argument
                    // take care of negations
                Quantifier newq = (args_pos[0]>0 ? q : dualQ(q));
                newarg = bringitdown(newq, x, abs(args_pos[0]), dependencies);
                newarg = (args_pos[0]>0 ? newarg : -newarg);
            }
            else {// make a new gate for the dependent arguments
                newarg = addGate(Gate(g.output, args_pos));
                newarg = addGate(Gate(Quant2Conn(q), vector<int>({x}), vector<int>({newarg})));
            }
            if (args_neg.size()==0)
                return newarg;
            else {
                args_neg.push_back(newarg);
                return addGate(Gate(g.output, args_neg));
            }
        }
    }

    if (g.output==All || g.output==Ex) {
        if (g.output == Quant2Conn(q)) { // push new quantifier through the (same) old quantifier
            Quantifier newq = (g.inputs[0]>0 ? q : dualQ(q));
            int new_arg = bringitdown(newq, x, abs(g.inputs[0]), dependencies);
            if (new_arg > maxVar() && getGate(new_arg).output == Quant2Conn(newq)) {
                // combine both quantifiers
                vector<int> newx(g.quants);
                newx.push_back(x);
                new_arg = getGate(new_arg).inputs[0];
                new_arg = (g.inputs[0]>0 ? new_arg : -new_arg);
                return addGate(Gate(g.output, newx, vector<int>({new_arg})));
            } else {
                new_arg = (g.inputs[0]>0 ? new_arg : -new_arg);
                return addGate(Gate(g.output, g.quants, vector<int>({new_arg})));
            }
        } else { // apply quantifier q to gate of opposite quantifier
            int i = addGate(Gate(Quant2Conn(q), vector<int>({x}), vector<int>({gate})));
            return i;
        }
    }
    // all cases should be covered
    assert(false);
}

Circuit& Circuit::miniscope() {
    LOG(2,"Moving quantifiers inside" << std::endl);
    while (maxBlock()>1) {
        Block b = prefix.back();
        for (int var : b.variables) {
            output = bringitdown(b.quantifier, var, output, posneg());
            cleanup_matrix();
        }
        prefix.pop_back();
    }
    return *this;
}
