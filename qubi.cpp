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

enum Verbose {quiet, normal, verbose, debug};
enum Reorder {none, dfs, matrix};
enum Iterate {left2right, pairwise};
enum QBlocks {keep, split, combine};
enum Prefix  {prenex, circuit, miniscope};

constexpr int DEFAULT_VERBOSE = normal;
constexpr int DEFAULT_REORDER = dfs;
constexpr int DEFAULT_ITERATE = pairwise;
constexpr int DEFAULT_QUANTBLOCKS = keep;
constexpr int DEFAULT_PREFIX = prenex;
constexpr int DEFAULT_WORKERS = 4;
constexpr int DEFAULT_TABLE   = 29;

bool EXAMPLE    = false;
bool PRINT      = false;
bool KEEPNAMES  = false;
bool GARBAGE    = false;
bool FLATTEN    = false;
bool CLEANUP    = false;
bool TO_CIRCUIT = false;
bool TO_CNF = false;
int UNITPROP   = 1;
int ITERATE     = DEFAULT_ITERATE;
int REORDER     = DEFAULT_REORDER;
int QUANTBLOCKS = DEFAULT_QUANTBLOCKS;
int PREFIX      = DEFAULT_PREFIX;
int WORKERS     = DEFAULT_WORKERS;
int TABLE       = DEFAULT_TABLE;
int VERBOSE     = DEFAULT_VERBOSE;

bool STATISTICS = false;
size_t PEAK = 0;

string NAME; // = "Test/sat13.qcir"; // for debugging

istream* INFILE;

void usage_short() {
    cout << "Usage:\n"
         << "solve:\tqubi [-e] [-r=n] [-q=n] [-u=n] [-f] [-c] [-x=n] [-i=n] [-g] [-t=n] [-w=n] [-v=n] [infile]\n"
         << "print:\tqubi  -p  [-r=n] [-q=n] [-u=n] [-f] [-c] [-x=n] [-k] [-v=n] [infile]\n"
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
         << "\t-f, -flatten: \t\tflattening transformation on and/or subcircuits\n"
         << "\t-c, -cleanup: \t\tremove unused variable and gate names\n"
         << "\t-q, -quant=<n>: \tquantifier block transformation: 0=keep (*), 1=split, 2=combine\n"
         << "\t-u, -unitprop=<n>: \tperform unit propagation: 0=no, 1=yes (*)\n"
         << "\t-x, -prefix=<n>: \tmove prefix into circuit: 0=prenex (*), 1=ontop, 2=miniscope\n"
         << "\t-r, -reorder=<n>: \tvariable reordering: 0=none, 1=dfs (*), 2=matrix\n"
         << "\t-i, -iterate=<n>: \tevaluate and/or: 0=left-to-right, 1=pairwise (*)\n"
         << "\t-g, -gc: \t\tswitch on garbage collection (experimental)\n"
         << "\t-t, -table=<n>: \tBDD: set max table size to 2^n, n in [15..42], 29=(*)\n"
         << "\t-w, -workers=<n>: \tBDD: use n threads, n in [0..64], 0=#cores, 4=(*)\n"
         << "\t-v, -verbose=<n>: \tverbose level (0=quiet, 1=normal (*), 2=verbose, 3=debug)\n"
         << "\t-s, -stats: \t\tturn statistics on (leads to slow-down)\n"
         << "\t-pp,-pre: \t\toutput QCIR file representing the BDD after preprocessing\n"
         << "\t-cnf: \t\t\tgenerate QDIMACS file from the BDD\n"
         << "\t-h, -help: \t\tthis usage message\n"
         << "\t(*) = default values"
         << endl;
}

istream* openInput(string& filename) {
    if (filename == "") {
        filename = "stdin";
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
    if (arg == "-print"   || arg == "-p") { PRINT   = true; return true; }
    if (arg == "-keep"    || arg == "-k") { KEEPNAMES = true; return true; }
    if (arg == "-flatten" || arg == "-f") { FLATTEN = true; CLEANUP = true; return true; }
    if (arg == "-cleanup" || arg == "-c") { CLEANUP = true; return true; }
    if (arg == "-stats"   || arg == "-s") { STATISTICS = true; return true; }
    if (arg == "-quant"   || arg == "-q") { QUANTBLOCKS = checkInt(arg,val,0,2); return true; }
    if (arg == "-prefix"  || arg == "-x") { PREFIX = checkInt(arg,val,0,2); return true; }
    if (arg == "-iterate" || arg == "-i") { ITERATE = checkInt(arg,val,0,1); return true; }
    if (arg == "-reorder" || arg == "-r") { REORDER = checkInt(arg,val,0,2); return true; }
    if (arg == "-pre"     || arg == "-pp"){ TO_CIRCUIT = true; return true; }
    if (arg == "-cnf")                    { TO_CNF = true; return true; }
    if (arg == "-gc"      || arg == "-g") { GARBAGE = true; return true; }
    if (arg == "-verbose" || arg == "-v") { VERBOSE = checkInt(arg,val,0,3); return true; }
    if (arg == "-workers" || arg == "-w") { WORKERS = checkInt(arg,val,0,64); return true; }
    if (arg == "-table"   || arg == "-t") { TABLE   = checkInt(arg,val,15,42); return true; }
    if (arg == "-unitprop"|| arg == "-u") { UNITPROP = checkInt(arg,val,0,1); return true; }
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

TASK_2(bool, solve_task, CircuitRW*, qbf, Valuation*, valuation) {   
    Solver solver(*qbf);
    bool verdict = solver.solve(NAME,UNITPROP, TO_CIRCUIT, TO_CNF); 
    // The solver has access to the filename and can thus e.g. write QDIMACS and QCIR files with similar names
    if (EXAMPLE) *valuation = solver.example();
    return verdict;
}

int main(int argc, char *argv[]) {
    system_clock::time_point starttime = system_clock::now();
    parseArgs(argc, argv);
    CircuitRW qbf(*INFILE);
    if (VERBOSE>=1) qbf.printInfo(cerr);
    if (QUANTBLOCKS==split) qbf.split();
    if (QUANTBLOCKS==combine) qbf.combine();
    if (FLATTEN) qbf.flatten();
    if (CLEANUP) qbf.cleanup();
    if (VERBOSE>=1 && (CLEANUP || QUANTBLOCKS>0)) qbf.printInfo(cerr);
    if (REORDER==dfs) qbf.reorderDfs();
    if (REORDER==matrix) qbf.reorderMatrix();
    if (PREFIX>0) {
        if (PREFIX==circuit) qbf.prefix2circuit();
        if (PREFIX==miniscope) qbf.miniscope();
        if (VERBOSE>=1) qbf.printInfo(cerr);
    }

    if (PRINT) {
        qbf.writeQcir(cout);
    } else {
        bool verdict;
        Valuation valuation;
        { Sylvan_mgr _(WORKERS, TABLE);
          verdict = RUN(solve_task, &qbf, &valuation);
          // Sylvan_mgr is closed automatically
        }
        report_result(qbf, verdict, valuation);
    }
    auto timespent = duration_cast<milliseconds>(system_clock::now() - starttime);
    LOG(1, "Total time spent: " << timespent.count() << " ms.");
    if (STATISTICS) { LOG(1, " Peak BDD nodes: " << PEAK); }
    LOG(1, std::endl);
    return 0;
}
