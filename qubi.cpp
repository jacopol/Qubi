#include <iostream>
#include <fstream>
#include "circuit.hpp"
#include "solver.hpp"

using namespace std;

bool BLOCK = true; // eliminate single quantifiers or as block
bool EXAMPLE = false;
int VERBOSE = 1;

string NAME;
istream* INFILE;

int WORKERS = 4;
long long TABLE = 1L<<28;

void usage() {
    cout << "Usage: qubi [-e] [-q | -b] [-v | -s] [-h] [infile]\n"
         << "Input: [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Options:\n"
         << "  -e, -example: \tshow witness for outermost quantifiers.\n"
         << "  -q, -quant: \t\teliminate quantifiers one by one.\n"
         << "  -b, -block: \t\teliminate quantifiers in blocks. (DEFAULT)\n"
         << "  -v, -verbose: \tverbose, show intermediate progress.\n"
         << "  -s, -silent: \tshow the output only.\n"
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
        // INFILE = &openInput("Test/sat2.qcir"); // for debugging

    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    Circuit qcir(NAME, *INFILE);
    qcir.printInfo(cerr);
    BddSolver* solver =
        (BLOCK 
            ? new BlockSolver(WORKERS, TABLE)
            : new BddSolver(WORKERS, TABLE));
    solver->setVerbose(VERBOSE).setExample(EXAMPLE);
    solver->solve(qcir);
    delete solver;
    return 0;
}
