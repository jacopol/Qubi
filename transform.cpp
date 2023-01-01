#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "circuit.hpp"

Circuit& Circuit::split() {
    vector<Block> oldprefix = prefix;
    prefix = vector<Block>();
    for (Block b : oldprefix) {
        for (int v : b.variables) {
            addBlock(Block(b.quantifier,vector<int>({v})));
        }
    }
    return *this;
}

Circuit& Circuit::combine() {
    if (maxBlock() == 0) {
        return *this;
    } else {
    vector<Block> oldprefix = prefix;
        prefix = vector<Block>();
        auto current = vector<int>();
        Quantifier q = oldprefix[0].quantifier;
        for (Block b : oldprefix) {
            if (b.quantifier != q) {
                addBlock(Block(q, current));
                current = vector<int>();
                q = b.quantifier;
            }
            for (int v : b.variables) {
                current.push_back(v);
            }
        }
        addBlock(Block(q, current));
    }
    return *this;
}

#include <iostream>
Circuit& Circuit::reorder() {
    std::cerr << "Reordering is not yet implemented" << std::endl;
    return *this;
}


#endif