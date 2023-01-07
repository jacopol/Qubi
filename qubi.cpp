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

void usage_short() {
    cout << "Usage:"
         << "\tqubi [-e | -p [-k]] [-s | -c] [-r] [-v | -q | -h] [infile]\n"
         << "Help:"
         << "\tqubi -h"
         << endl;
}
void usage() {
    usage_short();
    cout << "\nInput:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
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
}

ifstream & openInput(string filename) {
    ifstream* infile = new ifstream(filename);
    if (infile->fail()) {
        cerr << "Could not open file: " << filename << endl;
        exit(-1);
    }
    return *infile;
}

bool parseOption(string arg) {
    if (arg == "-example" || arg == "-e")   { EXAMPLE = true; return true; }
    if (arg == "-split" || arg == "-s")     { SPLIT = true; return true; }
    if (arg == "-combine" || arg == "-c")   { COMBINE = true; return true; }
    if (arg == "-reorder" || arg == "-r")   { REORDER = true; return true; }
    if (arg == "-print" || arg == "-p")     { PRINT = true; return true; }
    if (arg == "-keep" || arg == "-k")      { KEEPNAMES = true; return true; }
    if (arg == "-verbose" || arg == "-v")   { VERBOSE = verbose; return true; }
    if (arg == "-quiet" || arg == "-q")     { VERBOSE = quiet; return true; }
    if (arg == "-help" || arg == "-h")      { usage(); exit(1); }
    return false;
}

void parseArgs(int argc, char* argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        string arg = argv[i];
        if (parseOption(arg)) continue;
        if (arg[0] != '-' && NAME=="") {
            NAME = arg;
            INFILE = &openInput(NAME);
            continue;
        }
        LOG(0, "Couldn't parse argument: " << argv[i] << endl);
        usage_short(); exit(-1);
    }

    if (SPLIT && COMBINE) {
        LOG(1, "Warning: ignoring contradictory -s(plit) and -c(ombine)" << endl);
        SPLIT = COMBINE = false;
    }

    if (EXAMPLE && PRINT) {
        LOG(1, "Warning: ignoring -e(xample) in case of -p(rint)" << endl);
    }

    if (KEEPNAMES && !PRINT) {
        LOG(1, "Warning: ignoring -k(eep) without -p(rint)" << endl);
    }

    if (NAME == "") {
        NAME = "stdin";
        INFILE = &cin;
        // INFILE = &openInput("Test/qbf3.qcir"); // for debugging
    }

    LOG(1, "Reading input from \"" << NAME << "\"" << endl);
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
    CircuitRW qbf(*INFILE);
    if (VERBOSE>=1) qbf.printInfo(cerr);
    if (SPLIT) qbf.split();
    if (COMBINE) qbf.combine();
    if (REORDER) qbf.reorder();
    if (PRINT) {
        qbf.writeQcir(cout);
    } else {
        bool verdict;
        Valuation valuation;
        {   Sylvan_mgr _;
            Solver solver(qbf);
            verdict = solver.solve();
            if (EXAMPLE) valuation = solver.example();
        } // Sylvan_mgr is closed before reporting the results
        report_result(qbf, verdict, valuation);
    }
    return 0;
}
