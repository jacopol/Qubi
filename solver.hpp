#ifndef SYLVANBDD_H
#define SYLVANBDD_H

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include "circuit.hpp"

class BddSolver {
    protected:
        sylvan::Bdd matrix; // keeps current state of algorithm
        bool witness;       // example requested?
        int verbose;        // level of verbosity

        // Protocol:
        // - call matrix2bdd                  (defines the Bdd matrix)
        // - then call prefix2bdd             (eliminates prefix except first block)
        // - finally call result / example    (prints verdict and counter-example)
        
        void matrix2bdd(Circuit &);
        virtual void prefix2bdd(Circuit &);
        void result(Circuit &);
        void example(Circuit &);

    public:
        BddSolver(int workers, long long maxnodes);
        BddSolver& setExample(bool example);
        BddSolver& setVerbose(int verbosity);
        ~BddSolver();
        void solve(Circuit &);
};

class BlockSolver : public BddSolver {
    private:
        void prefix2bdd(Circuit &); // overridden
    public:
        BlockSolver(int workers, long long maxnodes);
};

#endif // SYLVANBDD_H
