#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>

extern int VERBOSE;
extern bool KEEPNAMES;

#define LOG(level, msg) { if (level<=VERBOSE) {std::cerr << msg; }}

#endif