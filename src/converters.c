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
#include "converters.h"
#include "message.h"

int fudgepyc_convertPythonToBool ( fudge_bool * target, PyObject * source )
{
    int istrue;

    if ( ( istrue = PyObject_IsTrue ( source ) ) == -1 )
        return -1;

    *target = ( istrue ? FUDGE_TRUE : FUDGE_FALSE );
    return 0;
}

/* TODO Make this more efficient - Python objects that are already of the
   correct type should not be converted */
#define CONVERT_PYTHON_TO_INTEGER( TYPENAME, NAME, CTYPE, LOW, HIGH )       \
int fudgepyc_convertPythonTo ## TYPENAME ( CTYPE * target,                  \
                                           PyObject * source )              \
{                                                                           \
    int64_t interval;                                                       \
    PyObject * interobj = 0;                                                \
                                                                            \
    if ( PyNumber_Check ( source ) )                                        \
        interobj = PyNumber_Long ( source );                                \
    if ( ! interobj )                                                       \
    {                                                                       \
        exception_raise_any ( PyExc_ValueError,                             \
                              "Cannot use object as %s, not numeric",       \
                              NAME );                                       \
        return -1;                                                          \
    }                                                                       \
    interval = PyLong_AsLongLong ( interobj );                              \
    Py_DECREF( interobj );                                                  \
                                                                            \
    if ( interval < LOW || interval > HIGH )                                \
    {                                                                       \
        exception_raise_any ( PyExc_OverflowError,                          \
                              "Cannot use value %lld as %s, out of range",  \
                              interval,                                     \
                              NAME );                                       \
        return -1;                                                          \
    }                                                                       \
                                                                            \
    *target = ( CTYPE ) interval;                                           \
    return 0;                                                               \
}

CONVERT_PYTHON_TO_INTEGER( Byte, "byte",  fudge_byte, INT8_MIN,  INT8_MAX )
CONVERT_PYTHON_TO_INTEGER( I16,  "short", fudge_i16,  INT16_MIN, INT16_MAX )
CONVERT_PYTHON_TO_INTEGER( I32,  "int",   fudge_i32,  INT32_MIN, INT32_MAX )
CONVERT_PYTHON_TO_INTEGER( I64,  "long",  fudge_i64,  INT64_MIN, INT64_MAX )

#define CONVERT_PYTHON_TO_FLOAT( TYPENAME, NAME, CTYPE )                    \
int fudgepyc_convertPythonTo ## TYPENAME ( CTYPE * target,                  \
                                           PyObject * source )              \
{                                                                           \
    if ( PyFloat_Check ( source ) )                                         \
        *target = ( CTYPE ) PyFloat_AS_DOUBLE( source );                    \
    else                                                                    \
    {                                                                       \
        PyObject * floatobj = 0;                                            \
        if ( PyNumber_Check ( source ) )                                    \
            floatobj = PyNumber_Float ( source );                           \
        if ( ! floatobj )                                                   \
        {                                                                   \
            exception_raise_any ( PyExc_ValueError,                         \
                                  "Cannot use object as %s, not numeric",   \
                                  NAME );                                   \
            return -1;                                                      \
        }                                                                   \
                                                                            \
        *target = ( CTYPE ) PyFloat_AS_DOUBLE( floatobj );                  \
        Py_DECREF( floatobj );                                              \
    }                                                                       \
    return 0;                                                               \
}

CONVERT_PYTHON_TO_FLOAT( F32, "float",  fudge_f32 )
CONVERT_PYTHON_TO_FLOAT( F64, "double", fudge_f64 )

int fudgepyc_convertPythonToMsg ( FudgeMsg * target, PyObject * source )
{
    if ( ! PyObject_TypeCheck ( source, &MessageType ) )
    {
         exception_raise_any ( PyExc_ValueError,
                               "Object not a Message instance" );
        return -1;
    }

    *target = ( ( Message * ) source )->msg;
    return 0;
}

int fudgepyc_convertPythonToString ( FudgeString * target, PyObject * source )
{
    FudgeStatus status;
    if ( PyString_Check ( source ) )
    {
        status = FudgeString_createFromASCII ( target,
                                               PyString_AS_STRING ( source ),
                                               PyString_GET_SIZE ( source ) );
        return exception_raiseOnError ( status );
    }
    else if ( PyUnicode_Check ( source ) )
    {
        /* The Python docs specify that the internal representation of Unicode
           objects is either UCS2 or UCS4 */
        switch ( sizeof ( Py_UNICODE ) )
        {
            case 2:
                status = FudgeString_createFromUTF16 (
                             target,
                             ( fudge_byte * ) PyUnicode_AS_DATA ( source ),
                             PyUnicode_GET_DATA_SIZE ( source ) );
                break;

            case 4:
                status = FudgeString_createFromUTF32 (
                             target,
                             ( fudge_byte * ) PyUnicode_AS_DATA ( source ),
                             PyUnicode_GET_DATA_SIZE ( source ) );
                break;

            default:
                exception_raise_any ( PyExc_RuntimeError,
                                     "Cannot encode Python string; Python "
                                     "interpreter not using UCS2 or UCS4 for"
                                     "internal unicode encoding" );
                return -1;
        }
        return exception_raiseOnError ( status );
    }
    else
    {
        exception_raise_any ( PyExc_ValueError,
                              "Cannot use object as string (must be String "
                              "or Unicode)" );
        return -1;
    }
}

#define CONVERT_PYTHON_SEQ_TO_BLOCK( TYPENAME, NAME, CTYPE )                \
int fudgepyc_convertPythonSeqTo ## TYPENAME ## Block ( CTYPE * target,      \
                                                       fudge_i32 size,      \
                                                       PyObject * source )  \
{                                                                           \
    int index, result = 0;                                                  \
    PyObject * seq = PySequence_Fast ( source,                              \
                                       "PySequence_Fast/" NAME " failed" ); \
    if ( ! seq )                                                            \
        return -1;                                                          \
                                                                            \
    for ( index = 0; index < size; ++index )                                \
    {                                                                       \
        if ( fudgepyc_convertPythonTo ## TYPENAME (                         \
                 &( target [ index ] ),                                     \
                 PySequence_Fast_GET_ITEM ( seq, index ) ) )                \
        {                                                                   \
            result = -1;                                                    \
            break;                                                          \
        }                                                                   \
    }                                                                       \
                                                                            \
    Py_DECREF( seq );                                                       \
    return result;                                                          \
}

static CONVERT_PYTHON_SEQ_TO_BLOCK( Byte, "byte",   fudge_byte )
static CONVERT_PYTHON_SEQ_TO_BLOCK( I16,  "short",  fudge_i16 )
static CONVERT_PYTHON_SEQ_TO_BLOCK( I32,  "int",    fudge_i32 )
static CONVERT_PYTHON_SEQ_TO_BLOCK( I64,  "long",   fudge_i64 )
static CONVERT_PYTHON_SEQ_TO_BLOCK( F32,  "float",  fudge_f32 )
static CONVERT_PYTHON_SEQ_TO_BLOCK( F64,  "double", fudge_f64 )

#define CONVERT_PYTHON_SEQ_TO_ARRAY( TYPENAME, NAME, CTYPE )                \
int fudgepyc_convertPythonSeqTo ## TYPENAME ## Array ( CTYPE * * target,    \
                                                       fudge_i32 * size,    \
                                                       PyObject * source )  \
{                                                                           \
    int result;                                                             \
                                                                            \
    if ( ! PySequence_Check ( source ) )                                    \
    {                                                                       \
        exception_raise_any ( PyExc_ValueError,                             \
                              "Cannot convert object in to %s array",       \
                              NAME );                                       \
        return -1;                                                          \
    }                                                                       \
                                                                            \
    *size = ( fudge_i32 ) PySequence_Size ( source );                       \
    *target = ( CTYPE * ) PyMem_Malloc ( sizeof ( CTYPE ) * *size );        \
    if ( ! *target )                                                        \
        return -1;                                                          \
                                                                            \
    result = fudgepyc_convertPythonSeqTo ## TYPENAME ## Block (             \
                 *target, *size, source );                                  \
    if ( result )                                                           \
        PyMem_Free ( *target );                                             \
    return result;                                                          \
}

static CONVERT_PYTHON_SEQ_TO_ARRAY( I16,  "short",  fudge_i16 )
static CONVERT_PYTHON_SEQ_TO_ARRAY( I32,  "int",    fudge_i32 )
static CONVERT_PYTHON_SEQ_TO_ARRAY( I64,  "long",   fudge_i64 )
static CONVERT_PYTHON_SEQ_TO_ARRAY( F32,  "float",  fudge_f32 )
static CONVERT_PYTHON_SEQ_TO_ARRAY( F64,  "double", fudge_f64 )

int fudgepyc_convertPythonToByteArray ( fudge_byte * * target,
                                        fudge_i32 * targetsize,
                                        PyObject * source )
{
    if ( PyString_Check ( source ) )
    {
        *targetsize = ( fudge_i32 ) PyString_GET_SIZE ( source );
        *target = ( fudge_byte * ) PyMem_Malloc ( *targetsize );
        if ( ! *target )
            return -1;
        memcpy ( *target, PyString_AS_STRING ( source ), *targetsize );
    }
    else if ( PyUnicode_Check ( source ) )
    {
        *targetsize = ( fudge_i32 ) PyUnicode_GET_DATA_SIZE ( source );
        *target = ( fudge_byte * ) PyMem_Malloc ( *targetsize );
        if ( ! *target )
            return -1;
        memcpy ( *target, PyUnicode_AS_DATA ( source ), *targetsize );
    }
    else if ( PySequence_Check ( source ) )
    {
        *targetsize = ( fudge_i32 ) PySequence_Size ( source );
        *target = ( fudge_byte * ) PyMem_Malloc ( *targetsize );
        if ( ! *target )
            return -1;

        if ( fudgepyc_convertPythonSeqToByteBlock ( *target,
                                                    *targetsize,
                                                    source ) )
        {
            PyMem_Free ( *target );
            return -1;
        }
    }
    else
    {
        exception_raise_any ( PyExc_ValueError,
                              "Only String, Unicode and sequence objects "
                              "can be converted in to byte arrays" );
        return -1;
    }
    return 0;
}

#define CONVERT_PYTHON_TO_VAR_ARRAY( TYPENAME, CTYPE )                      \
int fudgepyc_convertPythonTo ## TYPENAME ## Array ( CTYPE * * target,       \
                                                    fudge_i32 * size,       \
                                                    PyObject * source )     \
{                                                                           \
    return fudgepyc_convertPythonSeqTo ## TYPENAME ## Array (               \
               target, size, source );                                      \
}

CONVERT_PYTHON_TO_VAR_ARRAY( I16, fudge_i16 );
CONVERT_PYTHON_TO_VAR_ARRAY( I32, fudge_i32 );
CONVERT_PYTHON_TO_VAR_ARRAY( I64, fudge_i64 );
CONVERT_PYTHON_TO_VAR_ARRAY( F32, fudge_f32 );
CONVERT_PYTHON_TO_VAR_ARRAY( F64, fudge_f64 );

int fudgepyc_convertPythonToFixedByteArray ( fudge_byte * target,
                                             fudge_i32 size,
                                             PyObject * source )
{
    size_t actualsize;

    if ( PyString_Check ( source ) )
    {
        if ( ( actualsize = PyString_GET_SIZE ( source ) ) != size )
            goto size_mismatch;
        memcpy ( target, PyString_AS_STRING ( source ), size );
    }
    else if ( PyUnicode_Check ( source ) )
    {
        if ( ( actualsize = PyUnicode_GET_DATA_SIZE ( source ) ) != size )
            goto size_mismatch;
        memcpy ( target, PyUnicode_AS_DATA ( source ), size );
    }
    else if ( PySequence_Check ( source ) )
    {
        if ( ( actualsize = PySequence_Size ( source ) ) != size )
            goto size_mismatch;
        if ( fudgepyc_convertPythonSeqToByteBlock ( target, size, source ) )
            return -1;
    }
    else
    {
        exception_raise_any ( PyExc_ValueError,
                              "Only String, Unicode and sequence objects "
                              "can be converted in to fixed byte arrays" );
        return -1;
    }
    return 0;

size_mismatch:
    exception_raise_any (
        PyExc_ValueError,
        "Cannot convert object of length %d in to a %d byte array",
        actualsize,
        size );
    return -1;
}

PyObject * fudgepyc_convertBoolToPython ( fudge_bool source )
{
    if ( source )
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject * fudgepyc_convertByteToPython ( fudge_byte source )
{
    return PyInt_FromLong ( source );
}

PyObject * fudgepyc_convertI16ToPython ( fudge_i16 source )
{
    return PyInt_FromLong ( source );
}

PyObject * fudgepyc_convertI32ToPython ( fudge_i32 source )
{
    return PyInt_FromLong ( source );
}

PyObject * fudgepyc_convertI64ToPython ( fudge_i64 source )
{
    return PyLong_FromLongLong ( source );
}

PyObject * fudgepyc_convertF32ToPython ( fudge_f32 source )
{
    return PyFloat_FromDouble ( source );
}

PyObject * fudgepyc_convertF64ToPython ( fudge_f64 source )
{
    return PyFloat_FromDouble ( source );
}

PyObject * fudgepyc_convertStringToPython ( FudgeString source )
{
    PyObject * target = 0;
    fudge_byte * buffer;
    size_t written, bufsize;

    bufsize = FudgeString_getSize ( source ) * sizeof ( Py_UNICODE );
    if ( ! ( buffer = ( fudge_byte * ) PyMem_Malloc ( bufsize ) ) )
        return 0;

    /* See comment in fudgepyc_convertPythonToString for explanation */
    switch ( sizeof ( Py_UNICODE ) )
    {
        case 2:
            written = FudgeString_copyToUTF16 ( buffer, bufsize, source );
            break;

        case 4:
            written = FudgeString_copyToUTF32 ( buffer, bufsize, source );
            break;

        default:
            exception_raise_any ( PyExc_RuntimeError,
                                  "Cannot decode Fudge string; Python "
                                  "interpreter not using UCS2 or UCS4 for"
                                  "internal unicode encoding" );
            return 0;
    }

    target = PyUnicode_FromUnicode ( ( Py_UNICODE * ) buffer,
                                     written / sizeof ( Py_UNICODE ) );
    PyMem_Free ( buffer );
    return target;
}

#define CONVERT_ARRAY_TO_PYTHON_SEQ( TYPENAME, CTYPE )                      \
PyObject * fudgepyc_convert ## TYPENAME ## ArrayToPython (                  \
               const fudge_byte * bytes,                                    \
               fudge_i32 numbytes )                                         \
{                                                                           \
    size_t index, numelements = numbytes / sizeof ( CTYPE );                \
    PyObject * target, * element;                                           \
                                                                            \
    if ( ! ( target = PyList_New ( numelements ) ) )                        \
        return 0;                                                           \
                                                                            \
    for ( index = 0; index < numelements; ++index )                         \
    {                                                                       \
        element = fudgepyc_convert ## TYPENAME ## ToPython (                \
                      ( ( CTYPE * ) bytes ) [ index ] );                    \
        if ( ! element )                                                    \
        {                                                                   \
            Py_DECREF( target );                                            \
            return 0;                                                       \
        }                                                                   \
        PyList_SET_ITEM( target, index, element );                          \
    }                                                                       \
    return target;                                                          \
}

CONVERT_ARRAY_TO_PYTHON_SEQ( Byte, fudge_byte );
CONVERT_ARRAY_TO_PYTHON_SEQ( I16,  fudge_i16 );
CONVERT_ARRAY_TO_PYTHON_SEQ( I32,  fudge_i32 );
CONVERT_ARRAY_TO_PYTHON_SEQ( I64,  fudge_i64 );
CONVERT_ARRAY_TO_PYTHON_SEQ( F32,  fudge_f32 );
CONVERT_ARRAY_TO_PYTHON_SEQ( F64,  fudge_f64 );

PyObject * fudgepyc_convertByteStringToPython ( const fudge_byte * bytes,
                                                fudge_i32 numbytes )
{
    return PyString_FromStringAndSize ( ( const char * ) bytes, numbytes );
}

