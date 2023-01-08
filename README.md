# **Qubi**: a simple BDD-based solver for QBF circuits

## Usage:

solving:

    qubi [-e] [-r=n] [-s | -c] [-t=n] [-w=n] [-v=n] [infile]

printing:

    qubi -p [-r=n] [-s | -c] [-k] [-v=n] [infile]

help:

    qubi -h

_Input:_  QBF problem in QCIR format (from file or stdin)

_Output:_  solving:   [TRUE | FALSE] : the solution of the QBF + (counter)example 
    _or:_  printing:  preprocessed QBF in QCIR format


### Options:

    -e, -example:           solve and show witness for outermost quantifiers
    -p, -print:             print the (transformed) qbf to stdout
    -k, -keep:              keep the original gate/var-names (or else: renumber)
    -s, -split:             transform: split blocks in single quantifier
    -c, -combine:           transform: combine blocks with same quantifier
    -r, -reorder=<n>:       variable reordering: 0=none, 1=dfs (*), 2=matrix
    -t, -table=<n>:         BDD: set max table size to 2^n, n in [15..42], 30=(*)
    -w, -workers=<n>:       BDD: use n threads, n in [0..64], 0=#cores, 4=(*)
    -v, -verbose=<n>:       verbose level (0=quiet, 1=normal (*), 2=verbose)
    -h, -help:              this usage message
    (*) = default values

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