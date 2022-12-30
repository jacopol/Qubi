#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <exception>

using std::string;
using std::to_string;
using std::exception;

//void assertThrow(bool assrt, exception excpt);
#define assertThrow(assrt, excpt) { if (!(assrt)) { throw excpt; }}

class QBFexception : public exception {
protected:
    int index;
    int size;
public:
    QBFexception(int index, int size) {
        this->index = index;
        this->size = size;
    }
    virtual string what() {
        return "QBF exception: index=" + to_string(index) + " size=" + to_string(size-1);
    }
};

class PrefixOutOfBound : public QBFexception {
public:
    PrefixOutOfBound(int index, int size) : QBFexception(index,size) {}
    string what() {
        return "Variable " + to_string(index) + " is not in Prenex [1.." + to_string(size-1) + "]";
    }
};

class MatrixOutOfBound : public QBFexception {
public:
    MatrixOutOfBound(int index, int size) : QBFexception(index,size) {}
    string what(){
       return "Gate " + to_string(index) + " is not defined [1.." + to_string(size-1) + "]";
}
};

class InputUndefined : public QBFexception {
public:
    InputUndefined(int index, int size) : QBFexception(index,size) {}
    string what() {
        return "Input " + to_string(index) + " to Gate " + to_string(size) + " is not yet defined";
    }
};

class OutputUndefined : public QBFexception {
public:
    OutputUndefined(int index, int size) : QBFexception(index,size) {}
    string what() {
        return "Input " + to_string(index) + " to Output Gate is not defined";
    }
};

class PrefixGap : public QBFexception {
public:
    PrefixGap(int index): QBFexception(index,-1) {}
    string what() {
        return "Prefix has a gap at variable " + to_string(index);
    }
};

#endif