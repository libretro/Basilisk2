//fixes compiler errors for files that include this file
#ifndef COMPEMU_H
#define COMPEMU_H

#include "newcpu.h"

static __inline__ void flush_icache(int) { }
static __inline__ void build_comp() { }

#endif /* COMPEMU_H */
