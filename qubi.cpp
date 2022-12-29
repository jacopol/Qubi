#include <iostream>
#include <fstream>
#include "circuit.hpp"
#include "solver.hpp"

using namespace std;

int BLOCK = true; // eliminate single variables or block (default)
string NAME;
istream* INFILE;

int WORKERS = 4;
long long TABLE = 1L<<26;

void usage() {
    cout << "Usage: qubi [-single | -block] [-help] [infile]\n"
         << "Options:\n"
         << "  -single: \teliminate variables one by one.\n"
         << "  -block: \teliminate variables in blocks. (DEFAULT)\n"
         << "  infile: file in qcir (quantified circuit) format (DEFAULT: stdin)" 
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
        if (arg == "-single" || arg == "-s")
            { BLOCK = false; continue; }
        if (arg == "-block" || arg == "-b")
            { BLOCK = true; continue; }
        if (arg == "-help" || arg == "-h")
            { usage(); }
        if (arg[0] != '-' && NAME=="") {
            NAME = arg;
            INFILE = &openInput(NAME);
            continue;
        cerr << "Couldn't parse argument: " << argv[i] << endl;
        usage();
        }
    }

    if (NAME == "") {
        NAME = "stdin";
        INFILE = &cin;
        // INFILE = &openInput("Test/sat1.qcir");

    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    Circuit qcir(NAME, *INFILE);
    qcir.printInfo(cerr);
    BddSolver* solver =
        BLOCK ? new BlockSolver(WORKERS, TABLE) : new BddSolver(WORKERS, TABLE);
    solver->solve(qcir);
    delete solver;
    return 0;
}
