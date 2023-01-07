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
    Sylvan_mgr(int workers=DEFAULT_WORKERS, int table=DEFAULT_TABLE);
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

    bool  isConstant() const                        { return bdd.isConstant(); }
    size_t NodeCount() const                        { return bdd.NodeCount(); };
    bool operator==(const Sylvan_Bdd& other) const  { return bdd == other.bdd; }
    Sylvan_Bdd& operator+=(const Sylvan_Bdd& other) { bdd += other.bdd; return *this; }
    Sylvan_Bdd& operator*=(const Sylvan_Bdd& other) { bdd *= other.bdd; return *this; }
    Sylvan_Bdd  operator!() const                   { return Sylvan_Bdd(!bdd); }

/* sylvan functions with convenient API */

    Sylvan_Bdd UnivAbstract(const std::vector<int>& variables) const;
    Sylvan_Bdd ExistAbstract(const std::vector<int>& variables) const;
    std::vector<bool> PickOneCube(const std::vector<int>& variables) const;

private:

    sylvan::Bdd bdd;
    Sylvan_Bdd(const sylvan::Bdd& b) { bdd=b; }

};

#endif // BDD_SYLVAN_H