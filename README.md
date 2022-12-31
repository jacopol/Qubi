# **Qubi**: a simple BDD-based solver for QBF circuits

## Usage:

_Input:_  QBF problem in QCIR format (file or stdin)

_Output:_  [TRUE | FALSE] the solution of the QBF + valuation 1st block _or:_ preprocessed QBF

    qubi [-e] [-q | -b] [-p] [-k] [-v | -s] [-h] [infile]
    


### Options:

    -e, -example:         show witness for outermost quantifiers.
    -q, -quant:           transform: each quantifier is a block.
    -b, -block:           transform: each block is maximal.
    -p, -print:           print the (transformed) qcir to stdout.
    -k, -keep:            keep the original gate/var names.
    -v, -verbose:         verbose, show intermediate progress.
    -s, -silent:          show the output only.
    -h, -help:            this usage message

## Test:

    ./qubi -p -b -k Test/qbf3.qcir
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