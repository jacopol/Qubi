// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include "circuit.hpp"
#include "qcir_io.hpp"
#include "solver.hpp"

using namespace std;

bool BLOCK = true; // eliminate single quantifiers or as block
bool EXAMPLE = false;
bool PRINT = false;
bool KEEP = false;
int VERBOSE = 1;


string NAME;
istream* INFILE;

int WORKERS = 4;
long long TABLE = 1L<<28;

void usage() {
    cout << "Usage:"
         << "\tqubi [-e] [-q | -b] [-p] [-k] [-v | -s] [infile]\n"
         << "\tqubi [-h]\n\n"
         << "Input:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Output:\t Result: [TRUE | FALSE] -- the solution of the QBF\n\n"
         << "Options:\n"
         << "  -e, -example: \tshow witness for outermost quantifiers.\n"
         << "  -q, -quant: \t\ttransform: each quantifier is a block.\n"
         << "  -b, -block: \t\ttransform: each block is maximal.\n"
         << "  -p, -print: \t\tprint the (transformed) qcir to stdout.\n"
         << "  -k, -keep: \t\tkeep the original gate/var names.\n"
         << "  -v, -verbose: \tverbose, show intermediate progress.\n"
         << "  -s, -silent: \t\tshow the output only.\n"
         << "  -h, -help: \t\tthis usage message\n"
         << endl;
    exit(-1);
}

ifstream & openInput(string filename) {
    ifstream* infile = new ifstream(filename);
    if (infile->fail()) {
        cerr << "Could not open file: " << filename << endl;
        exit(-1);
    }
    return *infile;
}

void parseArgs(int argc, char* argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        string arg = string(argv[i]);
        if (arg == "-example" || arg == "-e")
            { EXAMPLE = true; continue; }
        if (arg == "-quant" || arg == "-q")
            { BLOCK = false; continue; }
        if (arg == "-block" || arg == "-b")
            { BLOCK = true; continue; }
        if (arg == "-print" || arg == "-p")
            { PRINT = true; continue; }
        if (arg == "-keep" || arg == "-k")
            { KEEP = true; continue; }
        if (arg == "-verbose" || arg == "-v")
            { VERBOSE = 2; continue; }
        if (arg == "-silent" || arg == "-s")
            { VERBOSE = 0; continue; }
        if (arg == "-help" || arg == "-h")
            { usage(); }
        if (arg[0] != '-' && NAME=="") {
            NAME = arg;
            INFILE = &openInput(NAME);
            continue;
        }
        cerr << "Couldn't parse argument: " << argv[i] << endl;
        usage();
    }

    if (NAME == "") {
        NAME = "stdin";
        INFILE = &cin;
        // INFILE = &openInput("Test/qbf2.qcir"); // for debugging

    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    Circuit qcir(NAME); 
    Qcir_IO rw(qcir,KEEP);
    rw.readQcir(*INFILE);
    if (VERBOSE>=1) qcir.printInfo(cerr);
    if (PRINT) {
        rw.writeQcir(cout);
    } else {
        Solver solver = Solver(WORKERS, TABLE);
        solver.setVerbose(VERBOSE).setExample(EXAMPLE);
        solver.solve(qcir);
    }
    return 0;
}
