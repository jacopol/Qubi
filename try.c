class QuBDD {

  // with +=, SylvanBDDmanager::False() becomes virtual as well???

  //virtual QuBDD& operator+=(const QuBDD& other) = 0; 
};


class QuBDDManager {
    virtual QuBDD& False() = 0;
    virtual QuBDD& True() = 0;
    virtual QuBDD& Var(int) = 0;

    friend QuBDD;
};


#include <sylvan.h>
#include <sylvan_obj.hpp>
#include <iostream>

class SylvanBDD : public QuBDD {
private:
        sylvan::Bdd b;

public:
    SylvanBDD(sylvan::Bdd bdd) : b(bdd) { }
    SylvanBDD& operator+=(const SylvanBDD& other) { b += other.b; return *this; }

};

class SylvanBDDmanager : public QuBDDManager {

public:
    SylvanBDD& False() { return *new SylvanBDD(sylvan::sylvan_false); }
    SylvanBDD& True() { return *new SylvanBDD(sylvan::sylvan_true); }
    SylvanBDD& Var(int i) { return *new SylvanBDD(sylvan::Bdd::bddVar(i)); }

};


int main() { 
    std::cout << "At least it compiled!" << std::endl; 
    
    SylvanBDDmanager Smgr;
    QuBDD b1 = Smgr.False();
    QuBDD b2 = Smgr.True();
    //b1 += b2;

    std::cout << "And it didn't crash!!" << std::endl; 

    }