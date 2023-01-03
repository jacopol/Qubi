// (c) Jaco van de Pol
// Aarhus University

#ifndef SYLVANBDD_H
#define SYLVANBDD_H

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include "circuit.hpp"

class Solver {
    protected:
        sylvan::Bdd matrix; // keeps current state of algorithm
        bool witness;       // example requested?

        // Protocol:
        // - call matrix2bdd                  (defines the Bdd matrix)
        // - then call prefix2bdd             (eliminates prefix except first block)
        // - finally call result / example    (prints verdict and counter-example)
        
        void matrix2bdd(const Circuit&);
        void prefix2bdd(const Circuit&);
        void result(const Circuit&);
        void example(const Circuit&);

    public:
        Solver(int workers, long long maxnodes);
        Solver& setExample(bool example);
        ~Solver();
        void solve(const Circuit &);
};

#endif // SYLVANBDD_H
