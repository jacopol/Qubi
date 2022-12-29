#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <exception>

using std::string;
using std::to_string;
using std::exception;

//void assertThrow(bool assrt, exception excpt);
#define assertThrow(assrt, excpt) { if (!(assrt)) { throw excpt; }}

#define LOG(level, msg) { if (level<=verbose) {cerr << msg; }}
#define LOGln(level, msg) { if (level<=verbose) {cerr << msg << endl; }}

class PrefixOutOfBound : public exception {
private:
    int index;
    int size;
public:
    PrefixOutOfBound(int i, int s) {
        index = i;
        size = s;
    }
    string what() {
        return "Variable " + to_string(index) + " is not in Prenex [1.." + to_string(size-1) + "]";
    }
};

class MatrixOutOfBound : public exception {
private:
    int index;
    int size;
public:
    MatrixOutOfBound(int i, int s) {
        index = i;
        size = s;
    }
    string what(){
       return "Gate " + to_string(index) + " is not defined [1.." + to_string(size-1) + "]";
}
};

class InputUndefined : public exception {
private:
    int index;
    int size;
public:
    InputUndefined(int i, int s) {
        index = i;
        size = s;
    }
    string what() {
        return "Input " + to_string(index) + " to Gate " + to_string(size) + " is not yet defined";
    }
};

class OutputUndefined : public exception {
private:
    int index;
    int size;
public:
    OutputUndefined(int i, int s) {
        index = i;
        size = s;
    }
    string what() {
        return "Input " + to_string(index) + " to Output Gate is not defined";
    }
};

class PrefixGap : public exception {
private:
    int index;
public:
    PrefixGap(int i) {
        index = i;
    }
    string what() {
        return "Prefix has a gap at variable " + to_string(index);
    }
};

#endif