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
        return "Error line " + to_string(line) + ": ";
    }
};

class VarDefined : public QBFexception {
public:
    VarDefined(string input, int line): QBFexception(input, line) {}
    string what() {
        return QBFexception::what() + "Variable \"" + input + "\" is already defined";
    }
};

class InputUndefined : public QBFexception {
public:
    InputUndefined(string input, int line) : QBFexception(input, line) {}
    string what() {
        return QBFexception::what() + "Literal \"" + input + "\" is undefined";
    }
};

class ConnectiveError : public QBFexception {
public:
    ConnectiveError(string input, int line): QBFexception(input, line) {}
    string what() {
        return QBFexception::what() + "Expected a connective, but got: \"" + input + "\"";
    }
};

class ParseError : public QBFexception {
public:
    ParseError(string input, int line): QBFexception(input, line) {}
    string what() {
        return QBFexception::what() + "Expecting quantifier, connective or output, but got \"" + input + "\"";
    }
};

class OutputMissing : public QBFexception {
public:
    OutputMissing(int line): QBFexception("", line) {}
    string what() {
        return QBFexception::what() + "The \"output(...)\" gate is missing";
    }
};

#endif