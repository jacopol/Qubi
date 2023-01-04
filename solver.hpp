// (c) Jaco van de Pol
// Aarhus University

#ifndef SYLVANBDD_H
#define SYLVANBDD_H

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include "circuit.hpp"

class Solver {
    private:
        const Circuit& c;   // the circuit to solve
        sylvan::Bdd matrix; // keeps current state of algorithm

        // The following functions must be called in this order:
        void matrix2bdd();  // transform all gates up to output to BDD 
        void prefix2bdd();  // quantifier elimination up to first block
        bool verdict() const;

    public:
        Solver(int workers, long long maxnodes, const Circuit &);
        ~Solver();
        bool solve();
        Valuation& example() const; // can only be called after solve()
};

#endif // SYLVANBDD_H
