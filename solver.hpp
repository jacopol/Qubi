#ifndef SYLVANBDD_H
#define SYLVANBDD_H

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include "circuit.hpp"
class BddSolver {
    protected:
        sylvan::Bdd matrix;

        // protocol:
        // - call matrix2bdd                    (defines Bdd matrix)
        // - then call prefix2bdd or blocks2bdd (eliminates prefix except first block)
        // - finally call result                (concludes and prints counter-example)
        
        virtual void matrix2bdd(Circuit &);
        virtual void prefix2bdd(Circuit &);
        virtual void result(Circuit &);

    public:
        BddSolver(int workers, long long maxnodes);
        ~BddSolver();
        void solveVars(Circuit &);
};

class BlockSolver : public BddSolver {
    private:
        void prefix2bdd(Circuit &); // overridden
    public:
        BlockSolver(int workers, long long maxnodes);
};

#endif // SYLVANBDD_H
