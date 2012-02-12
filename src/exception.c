/**
 * Copyright (C) 2012 - 2012, Vrai Stacey.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "exception.h"
#include <stdarg.h>

PyObject * FudgePyc_Exception = 0;

int exception_init ( PyObject * module )
{
    if ( ! ( FudgePyc_Exception = PyErr_NewException ( "fudgepyc.Exception", 0, 0 ) ) )
        return -1;

    return PyModule_AddObject ( module, "Exception", FudgePyc_Exception );
}

void exception_raise ( FudgeStatus status )
{
    PyErr_SetString ( FudgePyc_Exception, FudgeStatus_strerror ( status ) );
}

int exception_raiseOnError ( FudgeStatus status )
{
    if ( status == FUDGE_OK )
        return 0;
    exception_raise ( status );
    return -1;
}

void exception_raise_any ( PyObject * exc, const char * format, ... )
{
    va_list args;
    char buffer [ 256 ];

    va_start ( args, format );
    vsnprintf ( buffer, sizeof ( buffer ), format, args );
    va_end ( args );

    PyErr_SetString ( exc, buffer );
}

