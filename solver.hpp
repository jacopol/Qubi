#ifndef SYLVANBDD_H
#define SYLVANBDD_H

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include "circuit.hpp"
class BddSolver {
    private:
        sylvan::Bdd matrix;

        // protocol:
        // - call matrix2bdd (defines Bdd matrix)
        // - then call prefix2bdd or blocks2bdd
        // - finally call result
        
        void matrix2bdd(Circuit &);
        void prefix2bdd(Circuit &);
        void blocks2bdd(Circuit &);
        void result(Circuit &);

    public:
        BddSolver(int workers, long long maxnodes);
        ~BddSolver();
        void solveVars(Circuit &);
        void solveBlocks(Circuit &);
};

class BlockSolver : public BddSolver {
    private:
        void prefix2bdd(Circuit &);
};

#endif // SYLVANBDD_H
