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
        int verbose;        // level of verbosity

        // Protocol:
        // - call matrix2bdd                  (defines the Bdd matrix)
        // - then call prefix2bdd             (eliminates prefix except first block)
        // - finally call result / example    (prints verdict and counter-example)
        
        void matrix2bdd(Circuit &);
        void prefix2bdd(Circuit &);
        void result(Circuit &);
        void example(Circuit &);

    public:
        Solver(int workers, long long maxnodes);
        Solver& setExample(bool example);
        Solver& setVerbose(int verbosity);
        ~Solver();
        void solve(Circuit &);
};

#endif // SYLVANBDD_H
