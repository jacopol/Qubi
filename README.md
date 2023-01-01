# **Qubi**: a simple BDD-based solver for QBF circuits

## Usage:

_Input:_  QBF problem in QCIR format (from file or stdin)

_Output:_  [TRUE | FALSE] : the solution of the QBF + (counter)example _or:_ preprocessed QBF

    qubi [-e | -p [-k]] [-s | -c] [-r] [-v | -q | -h] [infile]
    


### Options:

    -e, -example:   show witness for outermost quantifiers
    -p, -print:     print the (transformed) qcir to stdout
    -k, -keep:      keep the original gates/vars (or else: renumber)
    -s, -split:     transform: split blocks in single quantifier
    -c, -combine:   transform: combine blocks with same quantifier
    -r, -reorder:   transform: variable reordering based on DFS
    -v, -verbose:   verbose, show intermediate progress
    -q, -quiet:     show the output only
    -h, -help:      this usage message

## Test:

    ./qubi -p -c -k Test/qbf3.qcir
    ./qubi -e -v Test/qbf3.qcir

## Current Limitations:

- currently only supports and/or gates
- currently only supports prenex format
- currently only accepts closed QBF (no free variables)
- currently brittle with parsing

## Build:

    g++ *.cpp -o qubi -lsylvan -lpthread

## Dependencies:

Sylvan -- Multi-core BDD package
- install from https://github.com/trolando/sylvan
- make sure that sylvan.h and sylvan.a can be found by g++

## Author:

    Jaco van de Pol
    Aarhus/Højbjerg
    Christmas 2022