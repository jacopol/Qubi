#ifndef BDD_SYLVAN_H
#define BDD_SYLVAN_H

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include "settings.hpp"

class Sylvan_mgr {
public:
    Sylvan_mgr(int workers=DEFAULT_WORKERS, long long maxnodes=DEFAULT_TABLE);
    ~Sylvan_mgr();
};


class Sylvan_Bdd {
public:
    Sylvan_Bdd()                { Sylvan_Bdd(false); }
    Sylvan_Bdd(bool b)          { bdd = (b ? sylvan::sylvan_true : sylvan::sylvan_false); }
    Sylvan_Bdd(int i)           { bdd = sylvan::Bdd::bddVar(i);}
    Sylvan_Bdd(const sylvan::Bdd& b) { bdd=b; }

    bool isConstant() const     { return bdd.isConstant(); }
    size_t NodeCount() const    { return bdd.NodeCount(); };

    bool operator==(const Sylvan_Bdd& other) const  { return bdd == other.bdd; }
    Sylvan_Bdd& operator+=(const Sylvan_Bdd& other) { bdd += other.bdd; return *this; }
    Sylvan_Bdd& operator*=(const Sylvan_Bdd& other) { bdd *= other.bdd; return *this; }
    Sylvan_Bdd  operator!() const                   { return Sylvan_Bdd(!bdd); }    
    Sylvan_Bdd UnivAbstract(const std::vector<int>& variables)  const;
    Sylvan_Bdd ExistAbstract(const std::vector<int>& variables) const;

    std::vector<bool> PickOneCube(const std::vector<int>& variables) const;
private:
    sylvan::Bdd bdd;

};

#endif // BDD_SYLVAN_H