// (c) Jaco van de Pol
// Aarhus University

#ifndef SOLVER_H
#define SOLVER_H

#include "bdd_sylvan.hpp"
#include "circuit.hpp"
#include <set>

class Solver {
    private:
        const Circuit& c;   // the circuit to solve
        Sylvan_Bdd matrix;  // keeps current state of algorithm
        vector<std::set<int>> cleanup; // bdds that can be cleaned up at each stage

        vector<int> restricted_vars; // Variables set to a constant under a 'restrict' operation
        // The following functions must be called in this order:
        void matrix2bdd();  // transform all gates up to output to BDD 
        void prefix2bdd();  // quantifier elimination up to first block
        bool verdict() const;
        void computeCleanup(); // compute when bdds can be cleaned up

        std::map<int, bool> detectUnitLits(Sylvan_Bdd bdd);
        Sylvan_Bdd unitpropagation(Sylvan_Bdd bdd); // compute generalized unit clauses and restrict bdd
        vector<Sylvan_Bdd> unitprop_general(vector<Sylvan_Bdd>);
        vector<int> polarity(); // polarity of variables, 1 (or 0) indicates positively (or negatively) pure literals,
                                // -2 indicates unitialized, -1 indicates non-pure literal
        void pureLitElim();
        void bdd2Qcir(std::ostream&, Sylvan_Bdd) const;
        void bdd2CNF(std::ostream&, Sylvan_Bdd) const;

        void write_output(string filename, bool cir, bool cnf) const;
    public:
        Solver(const Circuit& circuit);
        bool solve(string filename, int prop, bool to_circuit, bool to_cnf);
        Valuation example() const; // can only be called after solve()
};

#endif // SOLVER_H
