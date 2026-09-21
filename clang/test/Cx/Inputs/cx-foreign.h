#ifndef CX_FOREIGN_H
#define CX_FOREIGN_H

/* An ordinary C header with no Cx ownership. */
struct Foreign { int a; };

/* Data declared here is a C entity, and stays one when an owned file
   defines it. */
extern int foreign_global;

#endif
