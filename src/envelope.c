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
#include "envelope.h"
#include <fudge/codec.h>

/****************************************************************************
 * Constructor/destructor implementations
 */

static int Envelope_init ( Envelope * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "message", "directives", "schema", "taxonomy", 0 };

    FudgeStatus status;
    PyObject * message;
    fudge_byte directives = 0,
               schema = 0;
    fudge_i16 taxonomy = 0;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O!|bbh", kwlist,
                                         &MessageType, &message,
                                         &directives, &schema, &taxonomy ) )
        return -1;

    status = FudgeMsgEnvelope_create ( &self->envelope,
                                       directives,
                                       schema,
                                       taxonomy,
                                       ( ( Message * ) message )->msg );
    if ( exception_raiseOnError ( status ) )
        return -1;

    Py_INCREF( ( self->message = message ) );
    return 0;
}

static PyObject * Envelope_new ( PyTypeObject * type, PyObject * args, PyObject * kwds )
{
    Envelope * obj = ( Envelope * ) type->tp_alloc ( type, 0 );
    if ( obj )
    {
        obj->envelope = 0;
        obj->message = 0;
    }
    return ( PyObject * ) obj;
}

static void Envelope_dealloc ( Envelope * self )
{
    FudgeMsgEnvelope_release ( self->envelope );
    Py_XDECREF( self->message );
    self->ob_type->tp_free ( self );
}

/****************************************************************************
 * Method implementations
 */

#define DOC_fudgepyc_envelope_directives "TODO Method directives"
PyObject * Envelope_directives ( Envelope * self )
{
    return PyInt_FromLong ( FudgeMsgEnvelope_getDirectives ( self->envelope ) );
}

#define DOC_fudgepyc_envelope_schema "TODO Method schema"
PyObject * Envelope_schema ( Envelope * self )
{
    return PyInt_FromLong ( FudgeMsgEnvelope_getSchemaVersion ( self->envelope ) );
}

#define DOC_fudgepyc_envelope_taxonomy "TODO Method taxonomy"
PyObject * Envelope_taxonomy ( Envelope * self )
{
    return PyInt_FromLong ( FudgeMsgEnvelope_getTaxonomy ( self->envelope ) );
}

#define DOC_fudgepyc_envelope_message "TODO Method message"
PyObject * Envelope_message ( Envelope * self )
{
    Py_INCREF( self->message );
    return self->message;
}

#define DOC_fudgepyc_envelope_encode "TODO Method encode"
PyObject * Envelope_encode ( Envelope * self )
{
    PyObject * target = 0;
    fudge_byte * bytes;
    fudge_i32 numbytes;
    FudgeStatus status;

    Py_BEGIN_ALLOW_THREADS
    status = FudgeCodec_encodeMsg ( self->envelope, &bytes, &numbytes );
    Py_END_ALLOW_THREADS

    if ( exception_raiseOnError ( status ) )
        return 0;

    target = PyString_FromStringAndSize ( ( const char * ) bytes, numbytes );
    free ( bytes );
    return target;
}

#define DOC_fudgepyc_envelope_decode "TODO Method decode"
PyObject * Envelope_decode ( PyTypeObject * type, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "bytes", 0 };

    PyObject * target;
    FudgeStatus status;
    FudgeMsgEnvelope envlpe;
    const void * bytes;
    Py_ssize_t numbytes;
    PyObject * buffer;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O", kwlist, &buffer ) )
        return 0;
    if ( ! PyObject_CheckReadBuffer ( buffer ) )
    {
        exception_raise_any ( PyExc_TypeError,
                              "Cannot decode object that doesn't implement "
                              "the Buffer protocol (e.g. String)" );
        return 0;
    }

    if ( PyObject_AsReadBuffer ( buffer, &bytes, &numbytes ) )
        return 0;

    Py_BEGIN_ALLOW_THREADS
    status = FudgeCodec_decodeMsg ( &envlpe, bytes, ( fudge_i32 ) numbytes );
    Py_END_ALLOW_THREADS

    if ( exception_raiseOnError ( status ) )
        return 0;

    if ( ! ( target = Envelope_create ( envlpe ) ) )
        FudgeMsgEnvelope_release ( envlpe );
    return target;
}


/****************************************************************************
 * Type and method list definitions
 */

static PyMethodDef Envelope_methods [] =
{
    { "directives", ( PyCFunction ) Envelope_directives, METH_NOARGS, DOC_fudgepyc_envelope_directives },
    { "schema",     ( PyCFunction ) Envelope_schema,     METH_NOARGS, DOC_fudgepyc_envelope_schema },
    { "taxonomy",   ( PyCFunction ) Envelope_taxonomy,   METH_NOARGS, DOC_fudgepyc_envelope_taxonomy },
    { "message",    ( PyCFunction ) Envelope_message,    METH_NOARGS, DOC_fudgepyc_envelope_message },
    { "encode",     ( PyCFunction ) Envelope_encode,     METH_NOARGS, DOC_fudgepyc_envelope_encode },

    { "decode",     ( PyCFunction ) Envelope_decode,     METH_VARARGS | METH_KEYWORDS | METH_CLASS , DOC_fudgepyc_envelope_decode },
    { NULL }
};

#define DOC_fudgepyc_envelope "TODO Envelope documentation"

PyTypeObject EnvelopeType =
{
    PyObject_HEAD_INIT( NULL )
    0,                                              /* ob_size */
    "fudgepyc.Envelope",                            /* tp_name */
    sizeof ( Envelope ),                            /* tp_basicsize */
    0,                                              /* tp_itemsize */
    ( destructor ) Envelope_dealloc,                /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_compare */
    0,                                              /* tp_repr */
    0,                                              /* tp_as_number */
    0,                                              /* tp_as_sequence */
    0,                                              /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    0,                                              /* tp_str */
    0,                                              /* tp_getattro */
    0,                                              /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    DOC_fudgepyc_envelope,                          /* tp_doc */
    0,                                              /* tp_traverse */
    0,                                              /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Envelope_methods,                               /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    ( initproc ) Envelope_init,                     /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    Envelope_new                                    /* tp_new */
};


/****************************************************************************
 * Type functions
 */

PyObject * Envelope_create ( FudgeMsgEnvelope envelope )
{
    Envelope * envobj = ( Envelope * ) EnvelopeType.tp_new ( &EnvelopeType, 0, 0 );
    if ( ! envobj )
        return 0;

    PyObject * msgobj = Message_create ( FudgeMsgEnvelope_getMessage ( envelope ) );
    if ( ! msgobj )
    {
        Py_DECREF( envobj );
        return 0;
    }

    FudgeMsgEnvelope_retain ( ( envobj->envelope = envelope ) );
    envobj->message = msgobj;
    return ( PyObject * ) envobj;
}

