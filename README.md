# **Qubi**: a simple BDD-based solver for QBF circuits

## Usage:

solving:

    qubi [-e] [-r=n] [-q=n] [-f] [-c] [-x=n] [-i=n] [-g] [-t=n] [-w=n] [-v=n] [infile]

printing:

    qubi -p [-r=n] [-q=n] [-f] [-c] [-x=n] [-k] [-v=n] [infile]

help:

    qubi -h

_Input:_  QBF problem in QCIR format (from file or stdin)

_Output:_  solving:   [TRUE | FALSE] : the solution of the QBF + (counter)example 
    _or:_  printing:  preprocessed QBF in QCIR format


### Options:

    -e, -example:           solve and show witness for outermost quantifiers
    -p, -print:             print the (transformed) qbf to stdout
    -k, -keep:              keep the original gate/var-names (or else: renumber)
    -f, -flatten:           flattening transformation on and/or subcircuits
    -c, -cleanup:           remove unused variable and gate names
    -q, -quant=<n>:         quantifier block transformation: 0=keep (*), 1=split, 2=combine
    -x, -prefix=<n>:        move prefix into circuit: 0=prenex (*), 1=ontop, 2=miniscope
    -r, -reorder=<n>:       variable reordering: 0=none, 1=dfs (*), 2=matrix
    -i, -iterate=<n>:       evaluate and/or: 0=left-to-right, 1=pairwise (*)
    -g, -gc:                switch on garbage collection (experimental)
    -t, -table=<n>:         BDD: set max table size to 2^n, n in [15..42], 29=(*)
    -w, -workers=<n>:       BDD: use n threads, n in [0..64], 0=#cores, 4=(*)
    -v, -verbose=<n>:       verbose level (0=quiet, 1=normal (*), 2=verbose, 3=debug)
    -s, -stats:             turn statistics on (leads to slow-down)
    -h, -help:              this usage message
    (*) = default values

## Test:

Solving (with example generation, and verbose)

    ./qubi -e -v=2 Test/qbf3.qcir

Preprocessing (print after combining quantifier blocks, keeping original names)

    ./qubi -p -q=2 -k Test/qbf3.qcir

## Current Limitations:

- currently only supports and/or gates
- currently only supports prenex format
- currently only accepts closed QBF (no free variables)
- Note: Qubi is slightly more liberal than QCIR

## Build:

    g++ *.cpp -o qubi -lsylvan -lpthread -llace

## Dependencies:

Sylvan -- Multi-core BDD package
- install from https://github.com/utwente-fmt/sylvan
- make sure that sylvan.h and sylvan.a can be found by g++
  (using -I"path to sylvan.h" -L"path to libsylvan.a")

## Author:

    Jaco van de Pol
    Aarhus/Højbjerg
    Christmas 2022