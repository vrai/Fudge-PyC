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
#include "converters.h"
#include "envelope.h"
#include "field.h"
#include "modulemethods.h"
#include "version.h"

typedef struct
{
    const char * name;
    PyTypeObject * type;
    int ( *initfunc ) ( PyObject * );
} ModuleTypeDef;

static ModuleTypeDef module_types [] =
{
    { "Envelope", &EnvelopeType, NULL },
    { "Field",    &FieldType,    Field_modinit },
    { "Message",  &MessageType,  Message_modinit },
    { NULL }
};

static PyMethodDef module_methods [] =
{
    { "init", ( PyCFunction ) fudgepyc_init, METH_NOARGS, DOC_fudgepyc_init },
    { NULL }
};

static const char DOC_fudgepyc_module [] =
    "\nFudge-PyC provides a Python wrapper around the Fudge-C implementation\n"
    "of the Fudge message encoding specification. This interface is similar to\n"
    "Fudge-C but uses Python idioms (objects and exceptions rather than\n"
    "struct and return values).\n"
    "\n"
    "Like Fudge-C the library is safe to use across multiple threads, but\n"
    "individual objects (Envelope, Field, Message) must not be used across\n"
    "multiple threads concurrently. The library will release the GIL during\n"
    "potentially long running actions (encoding/decoding) and so can be used\n"
    "to handle multiple messages concurrent - just as long as each Message\n"
    "is only manipulated by one thread at any given time.\n"
    "\n"
    "Before Fudge-PyC can be used, the fudgepyc.init method must be called.\n"
    "This initialises various structures used by Fudge-C (such as the type\n"
    "registry) and can be called multiple times without any problems; only the\n"
    "first call actually does anything.\n"
    "\n"
    "Simple example:\n"
    "  >>> from fudgepyc import init, Envelope, Field, Message\n"
    "  >>> import fudgepyc.types\n"
    "  >>> init ( )\n"
    "  >>> message = Message ( )\n"
    "  >>> message.addField ( True, 'bool field' )\n"
    "  >>> message.addFieldF32 ( 1.23, '32bit float' )\n"
    "  >>> message.addField ( 'A string', 'string field', 1 )\n"
    "  >>> print len ( message )\n"
    "  3\n"
    "  >>> encoded = Envelope ( message ).encode ( )\n"
    "  >>> print type ( encoded ), len ( encoded )\n"
    "  <type 'str'> 66\n"
    "  >>> message = Envelope.decode ( encoded ).message ( )\n"
    "  >>> print len ( message )\n"
    "  3\n"
    "  >>> print message.getFieldAtIndex ( 0 ).value ( )\n"
    "  True\n"
    "  >>> print message [ 1 ].value ( )\n"
    "  A string\n"
    "  >>> field = message [ '32bit float' ]\n"
    "  >>> print fudgepyc.types.TYPE_NAMES [ field.type ( ) ]\n"
    "  float\n"
    "  >>> print type ( field.value ( ) )\n"
    "  <type 'float'>\n"
    "  >>> print '%.2f' % float ( field )\n"
    "  1.23\n"
    "\n"
    "As well as the main fudgepyc module there are the fudgepyc.timezone and\n"
    "fudgepyc.types submodules.\n"
    "\n"
    "The timezone module includes a single class fudgepyc.timezone.Timezone, that\n"
    "provides a simple datetime.tzinfo implementation. This is used by any\n"
    "datetime.time/datetime.datetime instances that are returned by Fudge-Pyc and\n"
    "which include timezone information.\n"
    "\n"
    "The types module contains a list of enumerations for each Fudge type as well\n"
    "as a dictionary (fudgepyc.TYPE_NAMES) mapping each type to a human readable\n"
    "name.\n";

PyMODINIT_FUNC initimpl ( void )
{
    PyObject * module = 0;
    ModuleTypeDef * mtdef;

    if ( ! ( module = Py_InitModule3 ( "impl",
                                       module_methods,
                                       DOC_fudgepyc_module ) ) )
        return;

    if ( exception_init ( module ) )
        return;

    if ( fudgepyc_initialiseConverters ( module ) )
        return;

    PyModule_AddStringConstant ( module, "__version__", fudgepyc_version );

    for ( mtdef = module_types; mtdef->name; ++mtdef )
    {
        if ( PyType_Ready ( mtdef->type ) )
            return;
        if ( mtdef->initfunc && mtdef->initfunc ( module ) )
            return;
        Py_INCREF( mtdef->type );
        if ( PyModule_AddObject ( module,
                                  mtdef->name,
                                  ( PyObject * ) mtdef->type ) )
            return;
    }
}

