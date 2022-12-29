#include "circuit.hpp"
#include "solver.hpp"
#include "testinput.hpp"
#include <iostream>

using namespace std;

void test1() {
    Gate g1(And);
    g1.addInput(1);
    g1.addInput(-2);

    Gate g2(Or);
    g2.addInput(2);
    g2.addInput(3);

    Circuit c("test");
    c.addVar(Exists);
    c.addVar(Forall);
    c.addGate(g1);
    c.addGate(g2);

    cout << "Variable 1: " << c.Quant(1) << endl;
    cout << "Variable 2: " << c.Quant(2) << endl;
    // c.Quant(3); // raises assertion error

    Gate g3(And);
    g3.addInput(5);
    // c.addGate(g3); // raises assertion error
    cout << endl;
}


void test2() { // check if Sylvan works properly
    BddSolver syl(4,1L<<26);
    sylvan::Bdd x = sylvan::Bdd::bddVar(0);
    x.PrintDot(stdout);
    sylvan::Bdd y = sylvan::Bdd::bddVar(1);
    y.PrintDot(stdout);
    sylvan::Bdd z = x * y;
    z.PrintDot(stdout);
    (!z).PrintDot(stdout);
}

void test3() { // check if Sylvan works properly
    BddSolver syl(1,1L<<26);
    sylvan::Bdd x = sylvan::Bdd::bddVar(1);
    sylvan::Bdd y = sylvan::Bdd::bddVar(2);
    sylvan::Bdd z = sylvan::Bdd::bddVar(3);
    sylvan::Bdd or1 = x + !y;
    sylvan::Bdd or2 = y + !z;
    sylvan::Bdd result1 = or1 * or2;
    result1.PrintDot(stdout);
    z.PrintDot(stdout);
    sylvan::Bdd result2 = result1 * z;
    result2.PrintDot(stdout);
}

void testQbf(BddSolver &s, Circuit c) {
    c.printInfo(cerr);
    s.solve(c);
}

void testall(BddSolver &solver) {
    testQbf(solver, test_sat1());
    testQbf(solver, test_sat2());
    testQbf(solver, test_qbf0());
    testQbf(solver, test_qbf1());
    testQbf(solver, test_qbf2());
}

void test4() {
    BddSolver solver1(1,1L<<26);
    testall(solver1);
}

void test5() {
    BlockSolver solver1(1,1L<<26);
    testall(solver1);
}

int test() {
    //test1();
    //test2();
    //test3();
    test4();
    test5();

    return 0;
}