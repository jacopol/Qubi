// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include "circuit_rw.hpp"
#include "solver.hpp"
#include "bdd_sylvan.hpp"
#include "settings.hpp"

using namespace std;

bool EXAMPLE    = false;
bool PRINT      = false;
bool SPLIT      = false;
bool COMBINE    = false;
bool REORDER    = false;

string NAME;
istream* INFILE;

int WORKERS = 4;
long long TABLE = DEFAULT_TABLE;

void usage() {
    cout << "Usage:"
         << "\tqubi [-e | -p [-k]] [-s | -c] [-r] [-v | -q | -h] [infile]\n"
         << "\tqubi [-h]\n\n"
         << "Input:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Output:\t Result: [TRUE | FALSE] -- the solution of the QBF\n"
         << "    or:\t the preprocessed QBF in QCIR format\n\n"
         << "Options:\n"
         << "\t-e, -example: \tshow witness for outermost quantifiers\n"
         << "\t-p, -print: \tprint the (transformed) qbf to stdout\n"
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
            { KEEPNAMES = true; continue; }
        if (arg == "-verbose" || arg == "-v")
            { VERBOSE = verbose; continue; }
        if (arg == "-quiet" || arg == "-q")
            { VERBOSE = quiet; continue; }
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

void report_result(const CircuitRW& qbf, bool verdict, const Valuation& valuation) {
    cout << "Result: " << (verdict ? "TRUE" : "FALSE") << endl;
    if (EXAMPLE) {
        if (valuation.size() == 0)
            cout << "No example" << endl;
        else {
            cout << "Example: ";
            qbf.writeVal(cout, valuation);
        }
    }
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    CircuitRW qbf(NAME); 
    qbf.readQcir(*INFILE);
    if (VERBOSE>=1) qbf.printInfo(cerr);
    if (SPLIT) qbf.split();
    if (COMBINE) qbf.combine();
    if (REORDER) qbf.reorder();
    if (PRINT) {
        qbf.writeQcir(cout);
    } else {
        bool verdict;
        Valuation valuation;
        {   BDD_Sylvan bddpackage;
            Solver solver = Solver(qbf, bddpackage);
            verdict = solver.solve();
            if (EXAMPLE) valuation = solver.example();
        }
        report_result(qbf, verdict, valuation);
    }
    return 0;
}
