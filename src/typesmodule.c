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
#include <Python.h>
#include <fudge/types.h>
#include "version.h"

typedef struct
{
    const char * name;
    int id;
    const char * str;
} AttrDef;

static AttrDef module_attrs [] =
{
    { "INDICATOR", FUDGE_TYPE_INDICATOR, "indicator" },
    { "BOOLEAN",   FUDGE_TYPE_BOOLEAN,   "boolean" },
    { "BYTE",      FUDGE_TYPE_BYTE,      "byte" },
    { "SHORT",     FUDGE_TYPE_SHORT,     "short" },
    { "INT",       FUDGE_TYPE_INT,       "int" },
    { "LONG",      FUDGE_TYPE_LONG,      "long" },
    { "FLOAT",     FUDGE_TYPE_FLOAT,     "float" },
    { "DOUBLE",    FUDGE_TYPE_DOUBLE,    "double" },

    { "BYTE_ARRAY",   FUDGE_TYPE_BYTE_ARRAY,   "byte[]" },
    { "SHORT_ARRAY",  FUDGE_TYPE_SHORT_ARRAY,  "short[]" },
    { "INT_ARRAY",    FUDGE_TYPE_INT_ARRAY,    "int[]" },
    { "LONG_ARRAY",   FUDGE_TYPE_LONG_ARRAY,   "long[]" },
    { "FLOAT_ARRAY",  FUDGE_TYPE_FLOAT_ARRAY,  "float[]" },
    { "DOUBLE_ARRAY", FUDGE_TYPE_DOUBLE_ARRAY, "double[]" },

    { "STRING",       FUDGE_TYPE_STRING,    "string" },
    { "MESSAGE",      FUDGE_TYPE_FUDGE_MSG, "message" },

    { "BYTE_ARRAY_4",   FUDGE_TYPE_BYTE_ARRAY_4,   "byte[4]" },
    { "BYTE_ARRAY_8",   FUDGE_TYPE_BYTE_ARRAY_8,   "byte[8]" },
    { "BYTE_ARRAY_16",  FUDGE_TYPE_BYTE_ARRAY_16,  "byte[16]" },
    { "BYTE_ARRAY_20",  FUDGE_TYPE_BYTE_ARRAY_20,  "byte[20]" },
    { "BYTE_ARRAY_32",  FUDGE_TYPE_BYTE_ARRAY_32,  "byte[32]" },
    { "BYTE_ARRAY_64",  FUDGE_TYPE_BYTE_ARRAY_64,  "byte[64]" },
    { "BYTE_ARRAY_128", FUDGE_TYPE_BYTE_ARRAY_128, "byte[128]" },
    { "BYTE_ARRAY_256", FUDGE_TYPE_BYTE_ARRAY_256, "byte[256]" },
    { "BYTE_ARRAY_512", FUDGE_TYPE_BYTE_ARRAY_512, "byte[512]" },

    { "DATE",     FUDGE_TYPE_DATE,     "date" },
    { "TIME",     FUDGE_TYPE_TIME,     "time" },
    { "DATETIME", FUDGE_TYPE_DATETIME, "datetime" },
    { NULL }
};

static PyMethodDef module_methods [] =
{
    { NULL }
};

static const char DOC_fudgepyc_typesmodule [] =
    "\nThe types module of Fudge-PyC provides enumerations for all the built-\n"
    "in Fudge types. The enumeration names are identical to the types in\n"
    "fudge/types.h, with the FUDGE_TYPE_ leader removed.\n\n"
    "The module also provides a dictionary, fudgepyc.types.TYPE_NAMES, that\n"
    "maps these enumerations to human readable strings.\n";

PyMODINIT_FUNC inittypes ( void )
{
    PyObject * module = 0,
             * namedict = 0,
             * intobj = 0,
             * strobj = 0;
    AttrDef * attrdef;

    if ( ! ( module = Py_InitModule3 ( "fudgepyc.types",
                                       module_methods,
                                       DOC_fudgepyc_typesmodule ) ) )
        goto clean_and_fail;

    PyModule_AddStringConstant ( module, "__version__", fudgepyc_version );

    if ( ! ( namedict = PyDict_New ( ) ) )
        goto clean_and_fail;

    for ( attrdef = module_attrs; attrdef->name; ++attrdef )
    {
        if ( PyModule_AddIntConstant ( module, attrdef->name, attrdef->id ) )
            goto clean_and_fail;

        intobj = PyInt_FromLong ( attrdef->id );
        strobj = PyString_FromString ( attrdef->str );
        if ( ! ( intobj && strobj ) )
            goto clean_and_fail_loop;

        if ( PyDict_SetItem ( namedict, intobj, strobj ) )
            goto clean_and_fail_loop;

        Py_DECREF( intobj );
        Py_DECREF( strobj );
    }

    if ( PyModule_AddObject ( module, "TYPE_NAMES", namedict ) )
        goto clean_and_fail;
    return;

clean_and_fail_loop:
    Py_XDECREF( intobj );
    Py_XDECREF( strobj );

clean_and_fail:
    Py_XDECREF( namedict );
    Py_XDECREF( module );
}

