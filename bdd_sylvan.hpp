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
    Sylvan_Bdd();
    Sylvan_Bdd operator=(const Sylvan_Bdd& right);
    bool operator==(const Sylvan_Bdd& other) const;

    Sylvan_Bdd operator+=(const Sylvan_Bdd& other) { bdd += other.bdd; return *this; }
    Sylvan_Bdd operator*=(const Sylvan_Bdd& other) { bdd *= other.bdd; return *this; }
    Sylvan_Bdd operator!() const;    

    static Sylvan_Bdd QTrue();
    static Sylvan_Bdd QFalse();
    static Sylvan_Bdd bddVar(int);

    bool isConstant() const;

    Sylvan_Bdd UnivAbstract(const Sylvan_Bdd& cube) const;
    Sylvan_Bdd ExistAbstract(const Sylvan_Bdd& cube) const;
    size_t NodeCount() const;

    std::vector<bool> PickOneCube(const std::vector<int> variables) const;

private:
    sylvan::Bdd bdd;

};

#endif // BDD_SYLVAN_H