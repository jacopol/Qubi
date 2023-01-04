#ifndef BDD_WRAPPER_H
#define BDD_WRAPPER_H

#include <vector>

class QuBdd {

public:
    virtual QuBdd& operator=(const QuBdd& right) = 0;

    virtual QuBdd& QTrue() = 0; // should be static but cannot
    virtual QuBdd& QFalse() = 0; // should be static but cannot
    virtual QuBdd& bddVar(int) = 0; // should be static but cannot

    virtual bool isConstant() const = 0;
    virtual QuBdd& operator!() const = 0;    
    virtual QuBdd& operator+=(const QuBdd& other) = 0;
    virtual QuBdd& operator*=(const QuBdd& other) = 0;
    virtual bool operator==(const QuBdd& other) const = 0;

    virtual QuBdd& UnivAbstract(const QuBdd& cube) const = 0;
    virtual QuBdd& ExistAbstract(const QuBdd& cube) const = 0;
    virtual size_t NodeCount() const = 0;

    virtual std::vector<bool> PickOneCube(const std::vector<int> variables) const = 0;
};

class BDD_wrapper {

};

#endif // BDD_WRAPPER_H