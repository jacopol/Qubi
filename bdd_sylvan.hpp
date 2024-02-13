// (c) Jaco van de Pol
// Aarhus University

#ifndef BDD_SYLVAN_H
#define BDD_SYLVAN_H

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include "settings.hpp"

class Sylvan_mgr {

// There should be at most one instance of a Sylvan_mgr alive at any moment.
// This is currently not enforced.

public:
    Sylvan_mgr(int workers=0, int table=30);
    ~Sylvan_mgr();
};


class Sylvan_Bdd {

// Sylvan_Bdd can only be used when there is a Sylvan_mgr active.
// This is currently not enforced.

public:

/* public constructors */

    // create constant BDD true/false
    Sylvan_Bdd(bool b) { bdd = (b ? sylvan::sylvan_true : sylvan::sylvan_false); }

    // create BDD variable(i)
    Sylvan_Bdd(int i) { bdd = sylvan::Bdd::bddVar(i);}

/* wrapping sylvan_obj.hpp functions */

    bool isConstant() const                         { return bdd.isConstant(); }
    bool operator==(const Sylvan_Bdd& other) const  { return bdd == other.bdd; }
    Sylvan_Bdd& operator+=(const Sylvan_Bdd& other) { bdd += other.bdd; peak(); return *this; }
    Sylvan_Bdd& operator*=(const Sylvan_Bdd& other) { bdd *= other.bdd; peak(); return *this; }
    Sylvan_Bdd  operator!() const                   { return Sylvan_Bdd(!bdd); }
    Sylvan_Bdd& restrict(const Sylvan_Bdd& other)   { bdd.Restrict(other.bdd); peak(); return *this;  }

/* sylvan functions with convenient API */

    Sylvan_Bdd UnivAbstract(const std::vector<int>& variables) const;
    Sylvan_Bdd ExistAbstract(const std::vector<int>& variables) const;
    std::vector<bool> PickOneCube(const std::vector<int>& variables) const;

/* folding operations */

    static Sylvan_Bdd bigAnd(const std::vector<Sylvan_Bdd>&);
    static Sylvan_Bdd bigOr(const std::vector<Sylvan_Bdd>&);



/* statistics */

    size_t NodeCount() const                        { return bdd.NodeCount(); };

    const Sylvan_Bdd& peak() {
        if (STATISTICS) { 
            size_t count = NodeCount();
            if (count > PEAK) {
                PEAK = count;
                LOG(1, "[peak " << PEAK << "]");
            }
        }
        return *this;
    }

private:

    sylvan::Bdd bdd;
    Sylvan_Bdd(const sylvan::Bdd& b) { bdd=b; }

};

#endif // BDD_SYLVAN_H