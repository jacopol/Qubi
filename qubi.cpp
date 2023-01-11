// (c) Jaco van de Pol
// Aarhus University

#include <iostream>
#include <fstream>
#include "circuit_rw.hpp"
#include "solver.hpp"
#include "bdd_sylvan.hpp"
#include "settings.hpp"
#include "chrono"

using namespace std;
using namespace chrono;

enum Verbose {quiet, normal, verbose};
enum Reorder {none, dfs, matrix};
enum Fold {left2right, pairwise};

constexpr int DEFAULT_VERBOSE = normal;
constexpr int DEFAULT_REORDER = dfs;
constexpr int DEFAULT_FOLD = pairwise;
constexpr int DEFAULT_WORKERS = 4;
constexpr int DEFAULT_TABLE   = 30;

bool EXAMPLE    = false;
bool PRINT      = false;
bool SPLIT      = false;
bool COMBINE    = false;
bool KEEPNAMES  = false;
int FOLDING     = DEFAULT_FOLD;
int REORDER     = DEFAULT_REORDER;
int WORKERS     = DEFAULT_WORKERS;
int TABLE       = DEFAULT_TABLE;
int VERBOSE     = DEFAULT_VERBOSE;

string NAME;
istream* INFILE;

void usage_short() {
    cout << "Usage:\n"
         << "solve:\tqubi [-e] [-r=n] [-s | -c] [-f=n] [-t=n] [-w=n] [-v=n] [infile]\n"
         << "print:\tqubi  -p  [-r=n] [-s | -c] [-k] [-v=n] [infile]\n"
         << "help :\tqubi  -h"
         << endl;
}

void usage() {
    usage_short();
    cout << "\nInput:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Output:\t solving : [TRUE | FALSE] -- the solution of the QBF\n"
         << "    or:\t printing: the preprocessed QBF in QCIR format\n\n"
         << "Options:\n"
         << "\t-e, -example: \t\tsolve and show witness for outermost quantifiers\n"
         << "\t-p, -print: \t\tprint the (transformed) qbf to stdout\n"
         << "\t-k, -keep: \t\tkeep the original gate/var-names (or else: renumber)\n"
         << "\t-s, -split: \t\ttransform: split blocks in single quantifier\n"
         << "\t-c, -combine: \t\ttransform: combine blocks with same quantifier\n"
         << "\t-r, -reorder=<n>: \tvariable reordering: 0=none, 1=dfs (*), 2=matrix\n"
         << "\t-f, -fold=<n>: \tevaluate and/or: 0=left-to-right, 1=pairwise (*)\n"
         << "\t-t, -table=<n>: \tBDD: set max table size to 2^n, n in [15..42], 30=(*)\n"
         << "\t-w, -workers=<n>: \tBDD: use n threads, n in [0..64], 0=#cores, 4=(*)\n"
         << "\t-v, -verbose=<n>: \tverbose level (0=quiet, 1=normal (*), 2=verbose)\n"
         << "\t-h, -help: \t\tthis usage message\n"
         << "\t(*) = default values"
         << endl;
}

istream* openInput(string& filename) {
    if (filename == "") {
        filename = "stdin";
        // return openInput("Test/qbf3.qcir"); // for debugging
        return &cin;
    }
    else {
        istream* infile = new ifstream(filename);
        if (infile->fail()) {
            cerr << "Could not open file: " << filename << endl;
            exit(-1);
        }
        return infile;
    }
}

string getArgument(string& arg) {
    auto pos = arg.find("=");
    if (pos == string::npos) 
        return "";
    else {
        string result = arg.substr(pos+1, arg.size());
        arg.erase(pos, arg.size());
        return result;
    }
}

int checkInt(string& arg, string& val, int low, int high) {
    try {
        int result = stoi(val);
        if (low <= result && result <= high)
            return result;
        else
            throw std::invalid_argument("");
    }
    catch (const std::invalid_argument& _) {
        cerr << "Error: \"" << arg << "=" << val 
             << "\": expected integer in [" << low << "," << high << "]" << endl;
        exit(-1); 
    }
}

bool parseOption(string& arg) {
    string val = getArgument(arg);
    if (arg == "-example" || arg == "-e") { EXAMPLE = true; return true; }
    if (arg == "-split"   || arg == "-s") { SPLIT   = true; return true; }
    if (arg == "-combine" || arg == "-c") { COMBINE = true; return true; }
    if (arg == "-print"   || arg == "-p") { PRINT   = true; return true; }
    if (arg == "-keep"    || arg == "-k") { KEEPNAMES = true; return true; }
    if (arg == "-fold"    || arg == "-f") { FOLDING = checkInt(arg,val,0,1); return true; }
    if (arg == "-reorder" || arg == "-r") { REORDER = checkInt(arg,val,0,2); return true; }
    if (arg == "-verbose" || arg == "-v") { VERBOSE = checkInt(arg,val,0,2); return true; }
    if (arg == "-workers" || arg == "-w") { WORKERS = checkInt(arg,val,0,64); return true; }
    if (arg == "-table"   || arg == "-t") { TABLE   = checkInt(arg,val,15,42); return true; }
    if (arg == "-help"    || arg == "-h") { usage(); exit(1); }
    return false;
}

void parseArgs(int argc, char* argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        string arg = argv[i];
        if (parseOption(arg)) continue;
        if (arg[0] != '-' && NAME=="") {
            NAME = arg;
            continue;
        }
        LOG(0, "Error: Couldn't parse argument \"" << argv[i] << "\"" << endl);
        usage_short(); exit(-1);
    }

    if (SPLIT && COMBINE) {
        LOG(0, "Error: -s(plit) and -c(ombine) are inconsistent" << endl);
        usage_short(); exit(-1);
    }

    if (EXAMPLE && PRINT) {
        LOG(0, "Error: -e(xample) and -p(rint) are inconsistent" << endl);
        usage_short(); exit(-1);
    }

    if (KEEPNAMES && !PRINT) {
        LOG(0, "Error: -k(eep) requires -p(rint)" << endl);
        usage_short(); exit(-1);
    }

    INFILE = openInput(NAME);

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
    system_clock::time_point starttime = system_clock::now();
    parseArgs(argc, argv);
    CircuitRW qbf(*INFILE);
    if (VERBOSE>=1) qbf.printInfo(cerr);
    if (SPLIT) qbf.split();
    if (COMBINE) qbf.combine();
    if (REORDER==1) qbf.reorderDfs();
    if (REORDER==2) qbf.reorderMatrix();
    // qbf.posneg(); // experimental
    if (PRINT) {
        qbf.writeQcir(cout);
    } else {
        bool verdict;
        Valuation valuation;
        {   Sylvan_mgr _(WORKERS, TABLE);
            Solver solver(qbf);
            verdict = solver.solve();
            if (EXAMPLE) valuation = solver.example();
        } // Sylvan_mgr is closed before reporting the results
        report_result(qbf, verdict, valuation);
    }
    auto timespent = duration_cast<milliseconds>(system_clock::now() - starttime);
    LOG(1, "Total time spent: " << timespent.count() << " ms" << endl);
    return 0;
}
