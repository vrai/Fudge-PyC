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
#include <datetime.h>
#include <fudge/datetime.h>

static PyObject * s_timezonemodule = 0,
                * s_timezonetype = 0;

int fudgepyc_initialiseConverters ( PyObject * module )
{
    if ( ! s_timezonemodule &&
         ! ( s_timezonemodule = PyImport_ImportModule ( "fudgepyc.timezone" ) ) )
        return -1;
    if ( ! s_timezonetype &&
         ! ( s_timezonetype = PyObject_GetAttrString ( s_timezonemodule,
                                                       "Timezone" ) ) )
    {
        exception_raise_any ( PyExc_ImportError,
                              "Could not find \"Timezone\" class in "
                              "fudgepyc/timezone module" );
        return -1;
    }

    PyDateTime_IMPORT;
    return 0;
}

int fudgepyc_convertPythonToBool ( fudge_bool * target, PyObject * source )
{
    int istrue;

    if ( ( istrue = PyObject_IsTrue ( source ) ) == -1 )
        return -1;

    *target = ( istrue ? FUDGE_TRUE : FUDGE_FALSE );
    return 0;
}

#define CONVERT_PYTHON_TO_INTEGER( TYPENAME, NAME, CTYPE, LOW, HIGH )       \
int fudgepyc_convertPythonTo ## TYPENAME ( CTYPE * target,                  \
                                           PyObject * source )              \
{                                                                           \
    PY_LONG_LONG value;                                                     \
                                                                            \
    if ( PyInt_Check ( source ) )                                           \
        value = PyInt_AsLong ( source );                                    \
    else if ( PyLong_Check ( source ) )                                     \
        value = PyLong_AsLongLong ( source );                               \
    else                                                                    \
    {                                                                       \
        PyObject * valueobj = 0;                                            \
        if ( PyNumber_Check ( source ) )                                    \
            valueobj = PyNumber_Long ( source );                            \
        if ( ! valueobj )                                                   \
        {                                                                   \
            exception_raise_any ( PyExc_ValueError,                         \
                                  "Cannot use object as %s, not numeric",   \
                                  NAME );                                   \
            return -1;                                                      \
        }                                                                   \
        value = PyLong_AsLongLong ( valueobj );                             \
        Py_DECREF( valueobj );                                              \
    }                                                                       \
                                                                            \
    if ( value == -1 && PyErr_Occurred ( ) )                                \
        return -1;                                                          \
                                                                            \
    if ( value < LOW || value > HIGH )                                      \
    {                                                                       \
        exception_raise_any ( PyExc_OverflowError,                          \
                              "Cannot use value %lld as %s, out of range",  \
                              value, NAME );                                \
        return -1;                                                          \
    }                                                                       \
                                                                            \
    *target = ( CTYPE ) value;                                              \
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

static int fudgepyc_convertAttrToInt ( int * target,
                                       PyObject * obj,
                                       const char * attr )
{
    PyObject * attrobj = PyObject_GetAttrString ( obj, attr );
    if ( attrobj )
    {
        *target = ( int ) PyInt_AsLong ( attrobj );
        Py_DECREF( attrobj );
        return 0;
    }
    else
        return -1;
}

int fudgepyc_convertPythonToDate ( FudgeDate * target, PyObject * source )
{
    FudgeStatus status;
    int year, month, day;

    if ( ! ( PyDate_Check ( source ) || PyDateTime_Check ( source ) ) )
    {
        exception_raise_any ( PyExc_TypeError,
                              "Only datetime.date and datetime.datetime "
                              "types can be converted in to FudgeDate" );
        return -1;
    }

    if ( fudgepyc_convertAttrToInt ( &year, source, "year" ) ||
         fudgepyc_convertAttrToInt ( &month, source, "month" ) ||
         fudgepyc_convertAttrToInt ( &day, source, "day" ) )
        return -1;

    status = FudgeDate_initialise ( target, year, month, day );
    return exception_raiseOnError ( status );
}

static int fudgepyc_convertUtcOffset ( int * target, PyObject * obj )
{
    int days, seconds, microseconds;

    if ( ! PyDelta_Check ( obj ) )
    {
        exception_raise_any ( PyExc_TypeError,
                              "datetime.time.utcoffset() did not return "
                              "a datetime.timedelta instance as expected" );
        return -1;
    }

    if ( fudgepyc_convertAttrToInt ( &days, obj, "days" ) ||
         fudgepyc_convertAttrToInt ( &seconds, obj, "seconds" ) ||
         fudgepyc_convertAttrToInt ( &microseconds, obj, "microseconds" ) )
        return -1;

    seconds += days * 86400;

    if ( seconds % 60 || microseconds )
    {
        exception_raise_any ( PyExc_ValueError,
                              "The maximum resolution for datetime.tzinfo "
                              "instances is 15 minutes; UTC offsets not "
                              "exactly divisible by this are not supported" );
        return -1;
    }

    *target = seconds / 900;    /* 15 minutes in seconds */
    return 0;
}

int fudgepyc_convertPythonToTime ( FudgeTime * target, PyObject * source )
{
    FudgeStatus status;
    int hour, minute, second, microsecond, offset = 0, result, hastz;
    PyObject * utcoffset;

    if ( ! ( PyTime_Check ( source ) || PyDateTime_Check ( source ) ) )
    {
        exception_raise_any ( PyExc_TypeError,
                              "Only datetime.time and datetime.datetime "
                              "types can be converted in to FudgeTime" );
        return -1;
    }

    if ( ! ( utcoffset = PyObject_CallMethod ( source, "utcoffset", "" ) ) )
        return -1;
    if ( ( hastz = ( utcoffset != Py_None ) ) )
        result = fudgepyc_convertUtcOffset ( &offset, utcoffset );
    else
        result = offset = 0;
    Py_DECREF( utcoffset );
    if ( result )
        return -1;

    if ( fudgepyc_convertAttrToInt ( &hour, source, "hour" ) ||
         fudgepyc_convertAttrToInt ( &minute, source, "minute" ) ||
         fudgepyc_convertAttrToInt ( &second, source, "second" ) ||
         fudgepyc_convertAttrToInt ( &microsecond, source, "microsecond" ) )
        return -1;

    if ( hastz )
        status = FudgeTime_initialiseWithTimezone ( target,
                                                    second + minute * 60 + hour * 3600,
                                                    microsecond * 1000,
                                                    FUDGE_DATETIME_PRECISION_MICROSECOND,
                                                    offset );
    else
        status = FudgeTime_initialise ( target,
                                        second + minute * 60 + hour * 3600,
                                        microsecond * 1000,
                                        FUDGE_DATETIME_PRECISION_MICROSECOND );
    return exception_raiseOnError ( status );
}

int fudgepyc_convertPythonToDateTime ( FudgeDateTime * target, PyObject * source )
{
    return fudgepyc_convertPythonToDate ( &target->date, source ) ||
           fudgepyc_convertPythonToTime ( &target->time, source );
}

static int fudgepyc_convertToBoundedInt ( int * target,
                                          PyObject * source,
                                          int lower,
                                          int upper )
{
    if ( ! source )
    {
        *target = 0;
        return 0;
    }

    if ( ( *target = ( int ) PyInt_AsLong ( source ) ) == -1
         && PyErr_Occurred ( ) )
        return -1;

    if ( *target < lower || *target > upper )
    {
        exception_raise_any ( PyExc_OverflowError,
                              "Integer %d is out of expected bound %d - %d",
                              *target, lower, upper );
        return -1;
    }
    return 0;
}

int fudgepyc_convertPythonToDateEx ( FudgeDate * target,
                                     PyObject * yearobj,
                                     PyObject * monthobj,
                                     PyObject * dayobj )
{
    FudgeStatus status;
    int year, month, day;

    if ( fudgepyc_convertToBoundedInt ( &year, 
                                        yearobj,
                                        FUDGEDATE_MIN_YEAR,
                                        FUDGEDATE_MAX_YEAR ) ||
         fudgepyc_convertToBoundedInt ( &month, monthobj, 0, 12 ) ||
         fudgepyc_convertToBoundedInt ( &day, dayobj, 0, 31 ) )
        return -1;

    status = FudgeDate_initialise ( target, year, month, day );
    return exception_raiseOnError ( status );
}

int fudgepyc_convertPythonToTimeEx ( FudgeTime * target,
                                     unsigned int precision,
                                     PyObject * hourobj,
                                     PyObject * minuteobj,
                                     PyObject * secondobj,
                                     PyObject * nanoobj,
                                     PyObject * offsetobj )
{
    FudgeStatus status;
    int hour, minute, second, nano;

    if ( fudgepyc_convertToBoundedInt ( &hour, hourobj, 0, 23 ) ||
         fudgepyc_convertToBoundedInt ( &minute, minuteobj, 0, 59 ) ||
         fudgepyc_convertToBoundedInt ( &second, secondobj, 0, 59 ) ||
         fudgepyc_convertToBoundedInt ( &nano, nanoobj, 0, 1000000000 ) )
        return -1;

    if ( offsetobj )
    {
        int offset;
        if ( fudgepyc_convertToBoundedInt ( &offset, offsetobj, -127, 127 ) )
            return -1;

        status = FudgeTime_initialiseWithTimezone ( target,
                                                    second + minute * 60 + hour * 3600,
                                                    nano,
                                                    precision,
                                                    offset );
    }
    else
    {
        status = FudgeTime_initialise ( target,
                                        second + minute * 60 + hour * 3600,
                                        nano,
                                        precision );
    }
    return exception_raiseOnError ( status );
}

int fudgepyc_convertPythonToDateTimeEx ( FudgeDateTime * target,
                                         unsigned int precision,
                                         PyObject * yearobj,
                                         PyObject * monthobj,
                                         PyObject * dayobj,
                                         PyObject * hourobj,
                                         PyObject * minuteobj,
                                         PyObject * secondobj,
                                         PyObject * nanoobj,
                                         PyObject * offsetobj )
{
    return fudgepyc_convertPythonToDateEx ( &target->date,
                                            yearobj,
                                            monthobj,
                                            dayobj ) ||
           fudgepyc_convertPythonToTimeEx ( &target->time,
                                            precision,
                                            hourobj,
                                            minuteobj,
                                            secondobj,
                                            nanoobj,
                                            offsetobj );
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

typedef struct
{
    int years, months, days,
        hours, minutes, seconds, microseconds;
    PyObject * tzinfo;
} PythonDateTime;

static int fudgepyc_convertDateToPythonDateTime ( PythonDateTime * target,
                                                  FudgeDate * source )
{
    target->years = source->year > 0 ? source->year : 1;
    target->months = source->month > 0 ? source->month : 1;
    target->days = source->day > 0 ? source->day : 1;
    return 0;
}

static int fudgepyc_convertTimeToPythonDateTime ( PythonDateTime * target,
                                                  FudgeTime * source )
{
    if ( source->hasTimezone )
    {
        PyObject * args = Py_BuildValue ( "(i)", source->timezoneOffset );
        if ( ! args )
            return -1;

        target->tzinfo = PyObject_CallObject ( s_timezonetype, args );
        Py_DECREF( args );

        if ( ! target->tzinfo )
            return -1;
    }
    else
        Py_INCREF( target->tzinfo = Py_None );

    target->seconds = source->seconds;
    target->seconds -= ( target->hours = target->seconds / 3600 ) * 3600;
    target->seconds -= ( target->minutes = target->seconds / 60 ) * 60;
    target->microseconds = source->nanoseconds / 1000;
    return 0;
}

PyObject * fudgepyc_convertDateToPython ( FudgeDate * source )
{
    PythonDateTime pdt;

    if ( fudgepyc_convertDateToPythonDateTime ( &pdt, source ) )
        return 0;

    return PyDate_FromDate ( pdt.years, pdt.months, pdt.days );
}

PyObject * fudgepyc_convertTimeToPython ( FudgeTime * source )
{
    PythonDateTime pdt;
    PyObject * target = 0,
             * args;

    if ( fudgepyc_convertTimeToPythonDateTime ( &pdt, source ) )
        return 0;

    args = Py_BuildValue ( "iiiiO", pdt.hours,
                                    pdt.minutes,
                                    pdt.seconds,
                                    pdt.microseconds,
                                    pdt.tzinfo );
    Py_DECREF( pdt.tzinfo );
    if ( ! args )
        return 0;

    target = PyObject_CallObject ( ( PyObject * ) PyDateTimeAPI->TimeType,
                                    args );
    Py_DECREF( args );
    return target;
}

PyObject * fudgepyc_convertDateTimeToPython ( FudgeDateTime * source )
{
    PythonDateTime pdt;
    PyObject * target = 0,
             * args;

    if ( fudgepyc_convertDateToPythonDateTime ( &pdt, &source->date ) )
        return 0;
    if ( fudgepyc_convertTimeToPythonDateTime ( &pdt, &source->time ) )
        return 0;

    args = Py_BuildValue ( "iiiiiiiO", pdt.years,
                                       pdt.months,
                                       pdt.days,
                                       pdt.hours,
                                       pdt.minutes,
                                       pdt.seconds,
                                       pdt.microseconds,
                                       pdt.tzinfo );
    Py_DECREF( pdt.tzinfo );
    if ( ! args )
        return 0;

    target = PyObject_CallObject ( ( PyObject * ) PyDateTimeAPI->DateTimeType,
                                    args );
    Py_DECREF( args );
    return target;
}

PyObject * fudgepyc_convertDateToPythonEx ( FudgeDate * source )
{
    return Py_BuildValue ( "iii", source->year,
                                  source->month,
                                  source->day );
}

PyObject * fudgepyc_convertTimeToPythonEx ( FudgeTime * source )
{
    int hours, minutes, seconds;
    PyObject * offset;

    seconds = source->seconds;
    seconds -= ( hours = seconds / 3600 ) * 3600;
    seconds -= ( minutes = seconds / 60 ) * 60;

    if ( source->hasTimezone )
        offset = PyInt_FromLong ( source->timezoneOffset );
    else
        Py_INCREF( offset = Py_None );

    return Py_BuildValue ( "iiiiiO", source->precision,
                                     hours,
                                     minutes,
                                     seconds,
                                     source->nanoseconds,
                                     offset );
}

PyObject * fudgepyc_convertDateTimeToPythonEx ( FudgeDateTime * source )
{
    int hours, minutes, seconds;
    PyObject * offset;

    seconds = source->time.seconds;
    seconds -= ( hours = seconds / 3600 ) * 3600;
    seconds -= ( minutes = seconds / 60 ) * 60;

    if ( source->time.hasTimezone )
        offset = PyInt_FromLong ( source->time.timezoneOffset );
    else
        Py_INCREF( offset = Py_None );

    return Py_BuildValue ( "iiiiiiiiO", source->time.precision,
                                        source->date.year,
                                        source->date.month,
                                        source->date.day,
                                        hours,
                                        minutes,
                                        seconds,
                                        source->time.nanoseconds,
                                        offset );
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

