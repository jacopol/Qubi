// (c) Jaco van de Pol
// Aarhus University

#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>

// global variables, defined in main program qubi.cpp

extern int VERBOSE;
extern bool KEEPNAMES;

#define LOG(level, msg) { if (level<=VERBOSE) {std::cerr << msg; }}

#endif // SETTINGS_H