// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <assert.h>
#include "circuit.hpp"
#include "settings.hpp"
#include "messages.hpp"

int Circuit::addVar(string name) {
    assert(matrix.size()==0); // must create all variables before any gate exists
    if (name=="") name = string("?" + to_string(freshname++));
    varnames.push_back(name);
    allnames.insert(name);
    return maxvar++;
}

int Circuit::addGate(const Gate& g, string name)          { 
    matrix.push_back(g);
    int max = maxGate()-1;
    if (name=="") name = string("?" + to_string(freshname++));
    varnames.push_back(name);
    allnames.insert(name);
    return max; 
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
    if (c == Ex)
        return All;
    if (c == All)
        return Ex;
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
            else LOG(1, "- Removed variable: " << varString(x) << std::endl);
            i++;
        }
        b.variables = newblock;
        if (newblock.size()>0) newprefix.push_back(b);
    }

    int newmaxvar = index;

    LOG(3, "- Removed Gates: ");

    vector<Gate> newmatrix;
    for (Gate &g : matrix) {
        if (marking.count(i)>0) {
            newmatrix.push_back(g);
            reordering[i] = index++;
        }
        else LOG(3, varString(i) << ", ");
        i++;
    }
    LOG(3, std::endl);

    maxvar = newmaxvar;
    prefix = newprefix;
    matrix = newmatrix;

    return permute(reordering); // update all indices, also updates output

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

    LOG(3, "- Removed Gates: ");

    vector<Gate> newmatrix;
    for (Gate &g : matrix) {
        if (marking.count(i)>0) {
            newmatrix.push_back(g);
            reordering[i] = index++;
        }
        else LOG(3, varString(i) << ", ");
        i++;
    }
    LOG(3, std::endl);

    matrix = newmatrix;

    return permute(reordering); // update all indices, also updates output
}

// Apply the reordering and store its inverse
// The reordering applies to variables and possibly to gates
Circuit& Circuit::permute(std::vector<int>& reordering) {

    // compose inverse of reordering with varnames
    {
    vector<string> oldnames(varnames);
    for (int i=0; i<reordering.size(); i++)
        varnames[reordering[i]] = oldnames[i];
    }
    varnames.resize(maxGate());

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
            else { 
                const vector<int>& inputs = getGate(v).inputs;
                for (int i=inputs.size()-1; i>=0; i--)  // visit all inputs of gate v
                    todo.push_back(abs(inputs[i]));
            }
        }
    }

    // Add unused variables to reordering 
    LOG(3, "- Unused variables/gates: ");
    for (int x : allvars) {
        LOG(3, varString(x) << ", "); // one , too much
        if (x < maxVar()) 
            reordering[x] = next++;
    }
    LOG(3, std::endl);
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
    LOG(1, "Moving quantifiers to top of matrix" << std::endl)
    while (maxBlock()>1) { // keep outermost block...
        const Block& b = prefix.back();
        Connective c = (b.quantifier==Forall ? All : Ex);
        output = addGate(Gate(c,vector<int>(b.variables),vector<int>({output})));
        prefix.pop_back();
    };

    return *this;
}

// compute positive/negative occurrences, and their union (dependencies)
// currently, we report (and use) only all dependencies
vector<varset> Circuit::posneg() {
    vector<varset> possets({varset()});
    vector<varset> negsets({varset()});

    // a variable occurs positively in itself
    for (int i=1; i<maxVar(); i++) {
        possets.push_back(varset().set(i));
        negsets.push_back(varset());
        if (VERBOSE>=4) { // currently switched off
            std::cerr <<  "var " << i << " : ";
            for (int j=1; j<maxVar(); j++) { std::cerr << (possets[i][j] ? "1" : "0"); }
            std::cerr << std::endl;
        }
    }
    // do all gates
    for (int i=maxVar(); i<maxGate(); i++) {
        varset pos;
        varset neg;
        // this works for And, Or, All, Ex
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
        if (VERBOSE>=4) { // currently switched off
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

    if (VERBOSE>=4) { // Currently switched off
        for (int i=0; i<maxGate(); i++) {
            std::cerr << "dep " << i << " : ";
            for (int j=1; j<maxVar(); j++)
                std::cerr << (dependencies[i][j] ? "1" : "0");
            std::cerr << std::endl;
        }
    }
    return dependencies;
}

Connective Quant2Conn(Quantifier q) { return (q==Forall ? All : Ex); }

// TODO: could do flattening
// TODO: eliminate T/F
int Circuit::buildConn(Connective c, const vector<int>& gates) {
    assert(c==And || c==Or);
    if (gates.size()==1)
        return gates[0];
    else {
        return addGate(Gate(c, gates));
    }
}

// TODO: can simplify variable case
int Circuit::buildQuant(Connective q, const vector<int>& xs, int gate) {
    assert(q==All || q==Ex);
    if (abs(gate) < maxVar()) { // Variable: just add the quantifier
        return addGate(Gate(q, xs, vector<int>({gate})));
    }
    Gate g = getGate(abs(gate));
    if (gate > 0 && g.output == q) { // combine quantifiers as in Ex xs (Ex ys A) => Ex (ys,xs) A
        vector<int> newx(g.quants);
        for (int x : xs) newx.push_back(x); // append(newx,xs)
        return addGate(Gate(q, newx, vector<int>({g.inputs[0]})));
    }
    if (gate < 0 && g.output == dualC(q)) { // combine quantifiers as in Ex xs -(All ys A) => Ex (ys,xs) -A
        vector<int> newx(g.quants);
        for (int x=0; x<xs.size(); x++) newx.push_back(x); // append(newx,xs)
        return addGate(Gate(q, newx, vector<int>({-g.inputs[0]}))); // negation!
    } else {
        return addGate(Gate(q, xs, vector<int>({gate})));
    }
}

// TODO: needs an operations cache?
int Circuit::bringitdown(Quantifier q, int x, int gate, const vector<varset>& dependencies) {

    if (gate<0) { // handle negative edges by dual quantifier
        return -bringitdown(dualQ(q), x, -gate, dependencies); 
    }

    if (gate < maxVar()) { // the gate is a single input variable (leaf of circuit)
        LOG(3,"- Eliminating " << Qtext[q] << " " << varString(x) 
            << " over input " << varString(gate) << std::endl);
        if (gate == x) {
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
    LOG(3,"- Pushing " << Qtext[q] << " " << varString(x) 
            << " over gate " << varString(gate) << " ("<< Ctext[g.output] << ")" << std::endl);
    if (g.inputs.size()==0) {
        return gate; // short-cut. quantification over constant can be dropped.
    }

    if ( (g.output==And && q==Forall) || (g.output==Or && q==Exists)) {
        // Can push Forall/Exists down directly to all arguments of And/Or.
        // Takes care of negative edges (pushes dual quantifier down)
        // As in Forall x (A1 /\ !A2 /\ A3) ==> (All x A1) /\ !(Ex x A2) /\ (All x A3)            
        vector<int> newargs;
        for (int arg: g.inputs) {
            int newarg = arg;
            if (dependencies[abs(arg)][x]) { // push quantifier into dependent arguments
                newarg = bringitdown(q, x, arg, dependencies);
            }
            newargs.push_back(newarg);
        }
        return addGate(Gate(g.output, newargs));
    }

    if ( g.output==And || g.output==Or) {
        // split args in dependent and independent args
        // case distinction on split args empty
        // As in Exists x (A1(x) /\ A2(x) /\ A3 /\ A4) ==> A3 /\ A4 /\ Exists x ( A1(x) /\ A2(x))
        vector<int>args_pos, args_neg;
        for (int arg : g.inputs) { 
            if (dependencies[abs(arg)][x]) {
                args_pos.push_back(arg);
            } else {
                args_neg.push_back(arg);
            }
        }
        
        if (args_pos.size()==0) {
            return gate; // the quantified variable doesn't occur, just drop quantifier
        }
        else {
            int newarg;
            if (args_pos.size()==1) {
                    // push quantifier into the only dependent argument
                newarg = bringitdown(q, x, args_pos[0], dependencies);
            }
            else {// make two new gates for the dependent arguments
                newarg = addGate(Gate(g.output, args_pos));
                newarg = addGate(Gate(Quant2Conn(q), vector<int>({x}), vector<int>({newarg})));
            }
            if (args_neg.size()==0) {
                return newarg;
            }
            else {
                args_neg.push_back(newarg);
                return addGate(Gate(g.output, args_neg));
            }
        }
    }

    if (g.output==All || g.output==Ex) {
        if (g.output == Quant2Conn(q)) { // push new quantifier through the (same) old quantifier
            int new_arg = bringitdown(q, x, g.inputs[0], dependencies);
            return buildQuant(g.output, g.quants, new_arg);
        } else { // Just add the quantifier
            return addGate(Gate(Quant2Conn(q), vector<int>({x}), vector<int>({gate})));
        }
    }
    // all cases should be covered
    assert(false);
}

Circuit& Circuit::miniscope() {
    LOG(1,"Moving quantifiers inside (early quantification)" << std::endl);
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
