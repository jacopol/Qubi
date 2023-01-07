// (c) Jaco van de Pol
// Aarhus University

#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>

enum Verbose {quiet, normal, verbose, debug};

// Qubi-wide:
constexpr int DEFAULT_VERBOSE   = normal;

// for Sylvan:
constexpr int DEFAULT_WORKERS   = 4;
constexpr int DEFAULT_TABLE     = 1L<<28;

// global variables

extern int VERBOSE;
extern bool KEEPNAMES;

#define LOG(level, msg) { if (level<=VERBOSE) {std::cerr << msg; }}

#endif // SETTINGS_H