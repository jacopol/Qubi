#include <iostream>
#include <fstream>
#include "circuit.hpp"
#include "solver.hpp"

int main(int argc, char *argv[]) {
    if (argc<=1) {
        cerr << "Expected at least one argument <<filename>>" << endl;
        return 1;
    }
    string filename = argv[argc-1];
    ifstream infile(filename);
    if (infile.fail()) {
        cerr << "Could not open file: " << filename << endl;
        exit(-1);
    }
    Circuit qcir(filename, infile);
    BlockSolver solver = BlockSolver(4, 1L<<26);
    qcir.printInfo(cerr);
    solver.solveVars(qcir);
    return 0;
}
