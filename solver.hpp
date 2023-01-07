// (c) Jaco van de Pol
// Aarhus University

#ifndef SOLVER_H
#define SOLVER_H

#include "bdd_sylvan.hpp"
#include "circuit.hpp"

class Solver {
    private:
        const Circuit& c;   // the circuit to solve
        Sylvan_Bdd matrix;  // keeps current state of algorithm

        // The following functions must be called in this order:
        void matrix2bdd();  // transform all gates up to output to BDD 
        void prefix2bdd();  // quantifier elimination up to first block
        bool verdict() const;

    public:
        Solver(const Circuit& circuit);
        bool solve();
        Valuation example() const; // can only be called after solve()
};

#endif // SOLVER_H
