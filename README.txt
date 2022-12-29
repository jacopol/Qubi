Usage:

    ./qubi [-block | -quant] [infile]
    ./qubi -help

    (options can be abbreviated to -b, -q, -h)

Test:

    ./qubi Test/qbf1.qcir

Input:

    File (or stdin) in qcir-14 format (but see Limitations)

Output:

    TRUE/FALSE + (counter)example: instantiation of top-level variables

Limitations:

- currently only supports and/or gates
- currently only supports prenex format

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
