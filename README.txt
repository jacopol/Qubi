Usage:

    ./qubi [-block | -single] [infile]
    ./qubi -help

    (options can be abbreviated to -b, -s, -h)

Test:

    ./qubi Test/qbf1.qcir

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
