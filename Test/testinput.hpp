Circuit test_sat1() { // (x + !y) * (y | !z) * (z)
    Circuit c("sat1");
    c.addVar(Exists);
    c.addVar(Exists); // block 1
    c.addVar(Exists);

    Gate g1(Or);
    g1.addInput(1);
    g1.addInput(-2);
    c.addGate(g1);

    Gate g2(Or);
    g2.addInput(2);
    g2.addInput(-3);
    c.addGate(g2);

    Gate g3(And);
    g3.addInput(4);
    g3.addInput(5);
    g3.addInput(3);
    c.addGate(g3);

    c.setOutput(6);
    return c;
}

Circuit test_sat2() { // (x + !y) * (y | !z) * (!z)
    Circuit c("sat2");
    c.addVar(Exists);
    c.addVar(Exists); // block 1
    c.addVar(Exists);

    Gate g1(Or);
    g1.addInput(1);
    g1.addInput(-2);
    c.addGate(g1);

    Gate g2(Or);
    g2.addInput(2);
    g2.addInput(-3);
    c.addGate(g2);

    Gate g3(And);
    g3.addInput(4);
    g3.addInput(5);
    g3.addInput(-3);
    c.addGate(g3);

    c.setOutput(6);
    return c;
}

Circuit test_qbf0() { // (x | !y)
    Circuit c("qbf0");
    c.addVar(Forall); // block 1
    c.addVar(Exists); // block 2

    Gate g1(Or);
    g1.addInput(1);
    g1.addInput(-2);
    c.addGate(g1);

    c.setOutput(3);
    return c;
}

Circuit test_qbf1() { // (x * !y)
    Circuit c("qbf1");
    c.addVar(Forall); // block 1
    c.addVar(Exists); // block 2

    Gate g1(And);
    g1.addInput(1);
    g1.addInput(-2);
    c.addGate(g1);

    c.setOutput(3);
    return c;
}

Circuit test_qbf2() {
    Circuit c("qbf2");
    c.addVar(Exists); // block 1
    c.addVar(Forall); // block 2
    c.addVar(Exists); // block 3

    Gate g1(Or);
    g1.addInput(1);
    g1.addInput(-2);
    c.addGate(g1);

    Gate g2(Or);
    g2.addInput(2);
    g2.addInput(-3);
    c.addGate(g2);

    Gate g3(And);
    g3.addInput(4);
    g3.addInput(5);
    g3.addInput(3);
    c.addGate(g3);

    c.setOutput(6);
    return c;
}

Circuit test_qbf3() { // test multiple (vacuous) blocks 
    Circuit c("qbf3");
    c.addVar(Forall); // block 1
    c.addVar(Exists); // block 2
    c.addVar(Exists);
    c.addVar(Forall); // block 3
    c.addVar(Forall);
    c.addVar(Exists); // block 4

    Gate g1(Or);
    g1.addInput(2);
    g1.addInput(-4);
    c.addGate(g1);

    Gate g2(Or);
    g2.addInput(4);
    g2.addInput(-6);
    c.addGate(g2);

    Gate g3(And);
    g3.addInput(4);
    g3.addInput(5);
    g3.addInput(6);
    c.addGate(g3);

    c.setOutput(9);
    return c;
}