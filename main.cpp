#include "circuit.hpp"
#include "solver.hpp"
#include "readqcir.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc<=1) {
        cout << "Expected at least one argument <<filename>>" << endl;
        return 1;
    }
    Circuit qcir = readCircuit(argv[argc-1]);
    BlockSolver solver = BlockSolver(4, 1L<<26);
    qcir.printInfo(cerr);
    solver.solveVars(qcir);
    return 0;
}
