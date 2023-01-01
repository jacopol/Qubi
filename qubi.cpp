// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include "circuit.hpp"
#include "qcir_io.hpp"
#include "solver.hpp"

using namespace std;

bool EXAMPLE    = false;
bool PRINT      = false;
bool KEEP       = false;
bool SPLIT      = false;
bool COMBINE    = false;
bool REORDER    = false;
int VERBOSE     = 1;


string NAME;
istream* INFILE;

int WORKERS = 4;
long long TABLE = 1L<<28;

void usage() {
    cout << "Usage:"
         << "\tqubi [-e | -p [-k]] [-s | -c] [-r] [-v | -q | -h] [infile]\n"
         << "\tqubi [-h]\n\n"
         << "Input:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Output:\t Result: [TRUE | FALSE] -- the solution of the QBF\n"
         << "    or:\t the preprocessed QBF in QCIR format\n\n"
         << "Options:\n"
         << "\t-e, -example: \tshow witness for outermost quantifiers\n"
         << "\t-p, -print: \tprint the (transformed) qcir to stdout\n"
         << "\t-k, -keep: \tkeep the original gates/vars (or else: renumber)\n"
         << "\t-s, -split: \ttransform: split blocks in single quantifier\n"
         << "\t-c, -combine: \ttransform: combine blocks with same quantifier\n"
         << "\t-r, -reorder: \ttransform: variable reordering based on DFS\n"
         << "\t-v, -verbose: \tverbose, show intermediate progress\n"
         << "\t-q, -quiet: \tshow the output only\n"
         << "\t-h, -help: \tthis usage message\n"
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
        if (arg == "-split" || arg == "-s")
            { SPLIT = true; continue; }
        if (arg == "-combine" || arg == "-c")
            { COMBINE = true; continue; }
        if (arg == "-reorder" || arg == "-r")
            { REORDER = true; continue; }
        if (arg == "-print" || arg == "-p")
            { PRINT = true; continue; }
        if (arg == "-keep" || arg == "-k")
            { KEEP = true; continue; }
        if (arg == "-verbose" || arg == "-v")
            { VERBOSE = 2; continue; }
        if (arg == "-quiet" || arg == "-q")
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

    if (SPLIT && COMBINE) {
        cerr << "Split (-s) and Combine (-s) contradict each other" << endl;
        usage();
    }

    if (NAME == "") {
        NAME = "stdin";
        INFILE = &cin;
        // INFILE = &openInput("Test/qbf3.qcir"); // for debugging
    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    Circuit qcir(NAME); 
    Qcir_IO rw(qcir,KEEP);
    rw.readQcir(*INFILE);
    if (VERBOSE>=1) qcir.printInfo(cerr);
    if (SPLIT) qcir.split();
    if (COMBINE) qcir.combine();
    if (REORDER) qcir.reorder();
    if (PRINT) {
        rw.writeQcir(cout);
    } else {
        Solver solver = Solver(WORKERS, TABLE);
        solver.setVerbose(VERBOSE).setExample(EXAMPLE);
        solver.solve(qcir);
    }
    return 0;
}
