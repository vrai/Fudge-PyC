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
#include "envelope.h"
#include "field.h"
#include "modulemethods.h"

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
    { "Message",  &MessageType,  NULL },
    { NULL }
};

static PyMethodDef module_methods [] =
{
    { "init", ( PyCFunction ) fudgepyc_init, METH_NOARGS, DOC_fudgepyc_init },
    { NULL }
};

#define DOC_fudgepyc_module "TODO Module documentation"

PyMODINIT_FUNC initimpl ( void )
{
    PyObject * module = 0;
    ModuleTypeDef * mtdef;

    if ( ! ( module = Py_InitModule3 ( "impl",
                                       module_methods,
                                       DOC_fudgepyc_module ) ) )
        goto clean_and_fail;

    if ( exception_init ( module ) )
        goto clean_and_fail;

    for ( mtdef = module_types; mtdef->name; ++mtdef )
    {
        if ( PyType_Ready ( mtdef->type ) )
            goto clean_and_fail;
        if ( mtdef->initfunc && mtdef->initfunc ( module ) )
            goto clean_and_fail;
        Py_INCREF( mtdef->type );
        if ( PyModule_AddObject ( module,
                                  mtdef->name,
                                  ( PyObject * ) mtdef->type ) )
            goto clean_and_fail;
    }
    return;

clean_and_fail:
    Py_XDECREF( module );
}

