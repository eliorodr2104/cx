#ifndef CX_USER_H
#define CX_USER_H

#module Users

struct User {
    int name;
    private int token;
    private(set) int id;
    internal int generation;
    internal private(set) int cacheVersion;

    ~mutating int readAll();
    void bump();
};

#endif
