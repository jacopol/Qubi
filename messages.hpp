// (c) Jaco van de Pol
// Aarhus University

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
    string input;
    int line;
public:
    QBFexception(string input, int line) {
        this->input = input;
        this->line = line;
    }
    virtual string what() {
        return "QCIR error (unexpected)";
    }
};

class InputUndefined : public QBFexception {
public:
    InputUndefined(string input, int line) : QBFexception(input, line) {}
    string what() {
        return "Error: Input \"" + input + "\" on line " + to_string(line) + " is undefined";
    }
};

class VarDefined : public QBFexception {
public:
    VarDefined(string input, int line): QBFexception(input, line) {}
    string what() {
        return "Error: Variable \"" + input + "\" on line " + to_string(line) + " was already defined";
    }
};

class ParseError : public QBFexception {
public:
    ParseError(string input, int line): QBFexception(input, line) {}
    string what() {
        return "Error: could not parse \"" + input + "\" on line " + to_string(line);
    }
};

class OutputMissing : public QBFexception {
public:
    OutputMissing(): QBFexception("", -1) {}
    string what() {
        return "Parse error: no output(...) gate specified";
    }
};

#endif