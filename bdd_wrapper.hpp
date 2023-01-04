#ifndef BDD_WRAPPER_H
#define BDD_WRAPPER_H

// TODO interface for bdds

class QuBdd {

    virtual QuBdd& operator+=(const QuBdd& other) = 0;
    virtual QuBdd& operator*=(const QuBdd& other) = 0;

};

class BDD_wrapper {

};

#endif // BDD_WRAPPER_H