# **Qubi**: a simple BDD-based solver for QBF circuits

## Usage:

solving:

    qubi [-e] [-r] [-s | -c] [-t=n] [-w=n] [-v | -q ] [infile]

printing:

    qubi -p [-r] [-s | -c] [-k] [-v | -q ] [infile]

help:

    qubi -h

_Input:_  QBF problem in QCIR format (from file or stdin)

_Output:_  solving:   [TRUE | FALSE] : the solution of the QBF + (counter)example 
    _or:_  printing:  preprocessed QBF in QCIR format


### Options:

    -e, -example:       show witness for outermost quantifiers
    -p, -print:         print the (transformed) qbf to stdout
    -k, -keep:          keep the original gates/vars (or else: renumber)
    -r, -reorder:       transform: variable reordering based on DFS
    -s, -split:         transform: split blocks in single quantifier
    -c, -combine:       transform: combine blocks with same quantifier
    -t, -table=<n>:     BDD: set max table size to 2^n (n>=17)
    -w, -workers=<n>:   BDD: set number of worker threads to n
    -v, -verbose:       verbose, show intermediate progress
    -q, -quiet:         show the output only
    -h, -help:          this usage message

## Test:

    ./qubi -p -c -k Test/qbf3.qcir
    ./qubi -e -v Test/qbf3.qcir

## Current Limitations:

- currently only supports and/or gates
- currently only supports prenex format
- currently only accepts closed QBF (no free variables)
- Note: Qubi is slightly more liberal than QCIR

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