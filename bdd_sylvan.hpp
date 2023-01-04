#ifndef BDD_SYLVAN_H
#define BDD_SYLVAN_H

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include "bdd_wrapper.hpp"
#include "settings.hpp"

class Syl_Bdd : public QuBdd {
    sylvan::Bdd bdd;

public:
    Syl_Bdd& operator+=(const Syl_Bdd& other) { bdd += other.bdd; return *this; }
    Syl_Bdd& operator*=(const Syl_Bdd& other) { bdd *= other.bdd; return *this; }

};

class BDD_Sylvan : public BDD_wrapper {
public:
    BDD_Sylvan(int workers=DEFAULT_WORKERS, long long maxnodes=DEFAULT_TABLE);
    ~BDD_Sylvan();
};

#endif // BDD_SYLVAN_H