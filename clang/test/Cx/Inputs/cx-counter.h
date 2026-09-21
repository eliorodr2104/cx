#ifndef CX_COUNTER_H
#define CX_COUNTER_H

#module Counters

struct Counter {
    int value;

    void increment();
    ~mutating int current();
};

#endif
