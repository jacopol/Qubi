Usage:  qubi [-e] [-q | -b] [-p] [-k] [-v | -s] [infile]
        qubi [-h]

Input:   [infile] (DEFAULT: stdin). Input QBF problem in QCIR format
Output:  Result: [TRUE | FALSE] -- the solution of the QBF

Options:
  -e, -example:         show witness for outermost quantifiers.
  -q, -quant:           transform: each quantifier is a block.
  -b, -block:           transform: each block is maximal.
  -p, -print:           print the (transformed) qcir to stdout.
  -k, -keep:            keep the original gate/var names.
  -v, -verbose:         verbose, show intermediate progress.
  -s, -silent:          show the output only.
  -h, -help:            this usage message

Test:

    ./qubi -p -e -v Test/qbf1.qcir

Input:

    File (or stdin) in qcir-14 format (but see Limitations)

Output:

    TRUE/FALSE + (counter)example: instantiation of top-level variables

Limitations:

- currently only supports and/or gates
- currently only supports prenex format
- currently only accepts closed QBF (no free variables)
- currently brittle with parsing

Build:

    g++ *.cpp -o qubi -lsylvan -lpthread

Dependencies:

    Sylvan - Multi-core BDD package 
        - install from https://github.com/trolando/sylvan
        - make sure sylvan.h and sylvan.a can be found by g++
          (may have to add -I and -L options to compiler)

Author:

    Jaco van de Pol
    Aarhus/Højbjerg
    Christmas 2022
