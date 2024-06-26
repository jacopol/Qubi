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

        // The following functions must be called in this order:
        void matrix2bdd();  // transform all gates up to output to BDD 
        void prefix2bdd();  // quantifier elimination up to first block
        bool verdict() const;
        void computeCleanup(); // compute when bdds can be cleaned up

    public:
        Solver(const Circuit& circuit);
        bool solve();
        Valuation example() const; // can only be called after solve()
};

#endif // SOLVER_H
