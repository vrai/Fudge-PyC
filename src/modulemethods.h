#ifndef INC_FUDGEPYC_MODULEMETHOD_H
#define INC_FUDGEPYC_MODULEMETHOD_H

#include "exception.h"

static const char DOC_fudgepyc_init [] =
    "\nWrapper around Fudge_init. Initialises the Fudge library and should\n"
    "be called before using any other functions/classes within fudgepyc.\n\n"
    "Can safely be called more than once. All calls after the first are noops.\n\n"
    "@return: None or fudgepyc.Exception on error\n";
extern PyObject * fudgepyc_init ( void );

#endif

