// (c) Jaco van de Pol
// Aarhus University

#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>

// global variables, defined in main program qubi.cpp

extern int VERBOSE;
extern int ITERATE;
extern bool KEEPNAMES;
extern bool GARBAGE;

#define LOG(level, msg) { if (level<=VERBOSE) {std::cerr << msg; }}

extern bool STATISTICS;
extern size_t PEAK;

#endif // SETTINGS_H