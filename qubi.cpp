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

bool EXAMPLE    = false;
bool PRINT      = false;
bool SPLIT      = false;
bool COMBINE    = false;
bool REORDER    = false;
int WORKERS     = DEFAULT_WORKERS;
int TABLE       = DEFAULT_TABLE;

string NAME;
istream* INFILE;

void usage_short() {
    cout << "Usage:\n"
         << "solve:\tqubi [-e] [-r] [-s | -c] [-t=n] [-w=n] [-v | -q ] [infile]\n"
         << "print:\tqubi  -p  [-r] [-s | -c] [-k]          [-v | -q ] [infile]\n"
         << "help :\tqubi  -h"
         << endl;
}

void usage() {
    usage_short();
    cout << "\nInput:\t [infile] (DEFAULT: stdin). Input QBF problem in QCIR format\n"
         << "Output:\t solving : [TRUE | FALSE] -- the solution of the QBF\n"
         << "    or:\t printing: the preprocessed QBF in QCIR format\n\n"
         << "Options:\n"
         << "\t-e, -example: \t\tshow witness for outermost quantifiers\n"
         << "\t-p, -print: \t\tprint the (transformed) qbf to stdout\n"
         << "\t-k, -keep: \t\tkeep the original gates/vars (or else: renumber)\n"
         << "\t-r, -reorder: \t\ttransform: variable reordering based on DFS\n"
         << "\t-s, -split: \t\ttransform: split blocks in single quantifier\n"
         << "\t-c, -combine: \t\ttransform: combine blocks with same quantifier\n"
         << "\t-t, -table=<n>: \tBDD: set max table size to 2^n (n>=17)\n"
         << "\t-w, -workers=<n>: \tBDD: set number of worker threads to n\n"
         << "\t-v, -verbose: \t\tverbose, show intermediate progress\n"
         << "\t-q, -quiet: \t\tshow the output only\n"
         << "\t-h, -help: \t\tthis usage message\n"
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
    if (arg == "-reorder" || arg == "-r") { REORDER = true; return true; }
    if (arg == "-print"   || arg == "-p") { PRINT   = true; return true; }
    if (arg == "-keep"    || arg == "-k") { KEEPNAMES = true; return true; }
    if (arg == "-verbose" || arg == "-v") { VERBOSE = verbose; return true; }
    if (arg == "-quiet"   || arg == "-q") { VERBOSE = quiet; return true; }
    if (arg == "-workers" || arg == "-w") { WORKERS = checkInt(arg,val,1,64); return true; }
    if (arg == "-table"   || arg == "-t") { TABLE   = checkInt(arg,val,17,32); return true; }
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
    if (REORDER) qbf.reorder();
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
