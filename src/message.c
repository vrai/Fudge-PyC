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
#include "message.h"
#include "converters.h"
#include "field.h"
#include <datetime.h>

/****************************************************************************
 * Constructor/destructor implementations
 */

static const char DOC_fudgepyc_message [] =
    "\nMessage() -> Message\n\n"
    "fudgepyc.Message contains zero or more fields, each of which may have\n"
    "a name, an ordinal (a 16-bit integer), both or none of these. Messages\n"
    "themselves contain no meta-data.\n\n"
    "Field order is maintained across encoding and decoding, fields will\n"
    "remain in insertion order; regardless of if they have a name and/or\n"
    "ordinal.\n"
    "\n"
    "@return: Message instance\n";
static int Message_init ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { 0 };

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "", kwlist ) )
        return -1;

    return exception_raiseOnError ( FudgeMsg_create ( &self->msg ) );
}

static PyObject * Message_new ( PyTypeObject * type, PyObject * args, PyObject * kwds )
{
    Message * obj = ( Message * ) type->tp_alloc ( type, 0 );
    if ( obj )
    {
        obj->msg = 0;
        obj->msgdict = 0;
    }
    return ( PyObject * ) obj;
}

static void Message_dealloc ( Message * self )
{
    FudgeMsg_release ( self->msg );
    Py_XDECREF( self->msgdict );
    self->ob_type->tp_free ( self );
}


/****************************************************************************
 * Method helper macros
 */

#define MESSAGE_ADD_FIELD_SCALAR_IMPL( TYPENAME, CTYPE, CLEANUP )           \
static PyObject * Message_addField ## TYPENAME ## Impl (                    \
           Message * self,                                                  \
           PyObject * valobj,                                               \
           PyObject * nameobj,                                              \
           PyObject * ordobj )                                              \
{                                                                           \
    FudgeStatus status;                                                     \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    CTYPE value;                                                            \
                                                                            \
    if ( fudgepyc_convertPythonTo ## TYPENAME ( &value, valobj ) )          \
        return 0;                                                           \
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )        \
        return 0;                                                           \
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )            \
        return 0;                                                           \
                                                                            \
    status = FudgeMsg_addField ## TYPENAME ( self->msg,                     \
                                             name,                          \
                                             ordobj ? &ordinal : 0,         \
                                             value );                       \
    FudgeString_release ( name );                                           \
    CLEANUP                                                                 \
                                                                            \
    if ( exception_raiseOnError ( status ) )                                \
        return 0;                                                           \
    Py_RETURN_NONE;                                                         \
}

#define MESSAGE_ADD_FIELD_PTR_IMPL( TYPENAME, CTYPE )                       \
static PyObject * Message_addField ## TYPENAME ## Impl (                    \
           Message * self,                                                  \
           PyObject * valobj,                                               \
           PyObject * nameobj,                                              \
           PyObject * ordobj )                                              \
{                                                                           \
    FudgeStatus status;                                                     \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    CTYPE value;                                                            \
                                                                            \
    memset ( &value, 0, sizeof ( value ) );                                 \
                                                                            \
    if ( fudgepyc_convertPythonTo ## TYPENAME ( &value, valobj ) )          \
        return 0;                                                           \
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )        \
        return 0;                                                           \
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )            \
        return 0;                                                           \
                                                                            \
    status = FudgeMsg_addField ## TYPENAME ( self->msg,                     \
                                             name,                          \
                                             ordobj ? &ordinal : 0,         \
                                             &value );                      \
    FudgeString_release ( name );                                           \
                                                                            \
    if ( exception_raiseOnError ( status ) )                                \
        return 0;                                                           \
    Py_RETURN_NONE;                                                         \
}

#define MESSAGE_ADD_FIELD_ARRAY_IMPL( TYPENAME, CTYPE )                     \
static PyObject * Message_addField ## TYPENAME ## ArrayImpl (               \
           Message * self,                                                  \
           PyObject * valobj,                                               \
           PyObject * nameobj,                                              \
           PyObject * ordobj )                                              \
{                                                                           \
    FudgeStatus status;                                                     \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    CTYPE * array;                                                          \
    fudge_i32 size;                                                         \
                                                                            \
    if ( fudgepyc_convertPythonTo ## TYPENAME ## Array ( &array,            \
                                                         &size,             \
                                                         valobj ) )         \
        return 0;                                                           \
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )        \
        return 0;                                                           \
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )            \
        return 0;                                                           \
                                                                            \
    status = FudgeMsg_addField ## TYPENAME ## Array ( self->msg,            \
                                                      name,                 \
                                                      ordobj ? &ordinal : 0,\
                                                      array,                \
                                                      size );               \
    PyMem_Free ( array );                                                   \
    FudgeString_release ( name );                                           \
                                                                            \
    if ( exception_raiseOnError ( status ) )                                \
        return 0;                                                           \
    Py_RETURN_NONE;                                                         \
}

#define MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( WIDTH )                         \
static PyObject * Message_addField ## WIDTH ## ByteArrayImpl (              \
           Message * self,                                                  \
           PyObject * valobj,                                               \
           PyObject * nameobj,                                              \
           PyObject * ordobj )                                              \
{                                                                           \
    FudgeStatus status;                                                     \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    fudge_byte array [ WIDTH ];                                             \
                                                                            \
    if ( fudgepyc_convertPythonToFixedByteArray ( array, WIDTH, valobj ) )  \
        return 0;                                                           \
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )        \
        return 0;                                                           \
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )            \
        return 0;                                                           \
                                                                            \
    status = FudgeMsg_addField ## WIDTH ## ByteArray (                      \
                 self->msg, name, ordobj ? &ordinal : 0, array );           \
    FudgeString_release ( name );                                           \
                                                                            \
    if ( exception_raiseOnError ( status ) )                                \
        return 0;                                                           \
    Py_RETURN_NONE;                                                         \
}

#define MESSAGE_ADD_FIELD_SCALAR( TYPENAME, PYSTR, FUDGESTR )               \
static const char DOC_fudgepyc_message_addField ## TYPENAME [] =            \
    "Adds a " FUDGESTR " field to the Message; value must be of Python "    \
    "type\n" PYSTR ". Field name and ordinal are optional.\n\n"             \
    "@param value: field value, of type " PYSTR "\n"                        \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defaults to None\n"             \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## TYPENAME ( Message * self,                   \
                                          PyObject * args,                  \
                                          PyObject * kwds )                 \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
                                                                            \
    return Message_addField ## TYPENAME ## Impl (                           \
               self, valobj, nameobj, ordobj );                             \
}

#define MESSAGE_ADD_FIELD_ARRAY( TYPENAME, PYSTR, FUDGESTR )                \
static const char DOC_fudgepyc_message_addField ## TYPENAME ## Array [] =   \
    "Adds a " FUDGESTR " field to the Message; value must be of Python\n"   \
    "type " PYSTR ". Field name and ordinal are optional.\n\n"              \
    "@param value: field value\n"                                           \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defaults to None\n"             \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## TYPENAME ## Array ( Message * self,          \
                                                   PyObject * args,         \
                                                   PyObject * kwds )        \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
                                                                            \
    return Message_addField ## TYPENAME ## ArrayImpl (                      \
               self, valobj, nameobj, ordobj );                             \
}

#define MESSAGE_ADD_FIELD_FIXED_ARRAY( WIDTH )                              \
static const char DOC_fudgepyc_message_addField ## WIDTH ## ByteArray [] =  \
    "Adds a Byte[" #WIDTH "] field to the Message; value must be of\n"      \
    "Python type String, Unicode or [int, ...] and have a length of "       \
    #WIDTH ".\nField name and ordinal are optional.\n\n"                    \
    "@param value: field value\n"                                           \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defaults to None\n"             \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## WIDTH ## ByteArray (                         \
               Message * self,                                              \
               PyObject * args,                                             \
               PyObject * kwds )                                            \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
                                                                            \
    return Message_addField ## WIDTH ## ByteArrayImpl (                     \
               self, valobj, nameobj, ordobj );                             \
}


/****************************************************************************
 * Internal functions
 */

static PyObject * Message_getFieldWithName ( Message * self,
                                             FudgeString name,
                                             int exception )
{
    FudgeField field;
    FudgeStatus status;
    size_t namelen;

    if ( ( namelen = FudgeString_getLength ( name ) ) > 256 )
    {
        exception_raiseOnError ( FUDGE_NAME_TOO_LONG );
        return 0;
    }

    status = FudgeMsg_getFieldByName ( &field, self->msg, name );
    switch ( status )
    {
        case FUDGE_OK:
            return Field_create ( field, self );

        case FUDGE_INVALID_NAME:
            if ( exception )
            {
                char * ascii = ( char * ) PyMem_Malloc ( namelen + 1 );
                if ( ascii )
                {
                    FudgeString_copyToASCII ( ascii, namelen + 1, name );
                    ascii [ namelen ] = 0;
                    exception_raise_any ( PyExc_LookupError,
                                          "No field with name \"%s\"", ascii );
                    PyMem_Free ( ascii );
                }
                return 0;
            }
            else
                Py_RETURN_NONE;

        default:
            exception_raiseOnError ( status );
            return 0;
    }
}

static PyObject * Message_getFieldWithOrdinal ( Message * self,
                                                unsigned short ordinal,
                                                int exception )
{
    FudgeField field;
    FudgeStatus status;

    status = FudgeMsg_getFieldByOrdinal ( &field, self->msg, ordinal );
    switch ( status )
    {
        case FUDGE_OK:
            return Field_create ( field, self );

        case FUDGE_INVALID_ORDINAL:
            if ( exception )
            {
                exception_raise_any ( PyExc_LookupError,
                                      "No field with ordinal %d", ordinal );
                return 0;
            }
            else
                Py_RETURN_NONE;

        default:
            exception_raiseOnError ( status );
            return 0;
    }
}

static int Message_parseOrdinalObject ( fudge_i16 * target, PyObject * source )
{
    long temp;

    if ( ! ( source && target ) ) return -1;

    temp = PyInt_AsLong ( source );
    if ( temp == -1 && PyErr_Occurred ( ) )
        return -1;

    if ( temp < 0 || temp > INT16_MAX )
    {
        exception_raise_any ( PyExc_OverflowError,
                              "Cannot use integer %ld as ordinal, out of range",
                              temp );
        return -1;
    }

    *target = ( fudge_i16 ) temp;
    return 0;
}

static int Message_parseNameObject ( FudgeString * target, PyObject * source )
{
    if ( ! ( PyString_Check ( source ) || PyUnicode_Check ( source ) ) )
    {
        exception_raise_any ( PyExc_TypeError,
                              "Only String and Unicode objects may be used as names" );
        return -1;
    }
    return fudgepyc_convertPythonToString ( target, source );
}

static int Message_createMsgDict ( Message * self )
{
    if ( ! self->msgdict )
    {
        if ( ! ( self->msgdict = PyDict_New ( ) ) )
            return -1;
    }
    return 0;
}

int Message_getFudgeType ( fudge_type_id * type, PyObject * value )
{
    if ( value == Py_None )
        *type = FUDGE_TYPE_INDICATOR;
    else if ( PyBool_Check ( value ) )
        *type = FUDGE_TYPE_BOOLEAN;
    else if ( PyInt_Check ( value ) || PyLong_Check ( value ) )
        *type = FUDGE_TYPE_LONG;
    else if ( PyFloat_Check ( value ) )
        *type = FUDGE_TYPE_DOUBLE;
    else if ( PyString_Check ( value ) || PyUnicode_Check ( value ) )
        *type = FUDGE_TYPE_STRING;
    else if ( PyObject_IsInstance ( value, ( PyObject * ) &MessageType ) )
        *type = FUDGE_TYPE_FUDGE_MSG;
    else if ( PyDateTime_Check ( value ) )
        *type = FUDGE_TYPE_DATETIME;
    else if ( PyDate_Check ( value ) )
        *type = FUDGE_TYPE_DATE;
    else if ( PyTime_Check ( value ) )
        *type = FUDGE_TYPE_TIME;
    else
        return -1;
    return 0;
}

static PyObject * Message_addFieldIndicatorImpl ( Message * self,
                                                  PyObject * nameobj,
                                                  PyObject * ordobj )
{
    FudgeStatus status;
    FudgeString name = 0;
    fudge_i16 ordinal;

    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )
        return 0;
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )
        return 0;

    status = FudgeMsg_addFieldIndicator ( self->msg, name, ordobj ? &ordinal : 0 );
    FudgeString_release ( name );

    if ( exception_raiseOnError ( status ) )
        return 0;
    Py_RETURN_NONE;
}

static PyObject * Message_addFieldMsgImpl ( Message * self,
                                            PyObject * msgobj,
                                            PyObject * nameobj,
                                            PyObject * ordobj )
{
    FudgeStatus status;
    FudgeString name = 0;
    fudge_i16 ordinal;
    FudgeMsg message;

    if ( fudgepyc_convertPythonToMsg ( &message, msgobj ) )
        return 0;
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )
        return 0;
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )
        return 0;

    status = FudgeMsg_addFieldMsg ( self->msg,
                                    name,
                                    ordobj ? &ordinal : 0,
                                    message );
    FudgeString_release ( name );
    if ( exception_raiseOnError ( status ) )
        return 0;

    Message_storeMessage ( self, ( Message * ) msgobj );
    Py_RETURN_NONE;
}

MESSAGE_ADD_FIELD_SCALAR_IMPL( Bool,   fudge_bool, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( Byte,   fudge_byte, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( I16,    fudge_i16, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( I32,    fudge_i32, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( I64,    fudge_i64, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( F32,    fudge_f32, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( F64,    fudge_f64, )
MESSAGE_ADD_FIELD_SCALAR_IMPL( String, FudgeString, FudgeString_release ( value ); )

MESSAGE_ADD_FIELD_PTR_IMPL( Date,     FudgeDate )
MESSAGE_ADD_FIELD_PTR_IMPL( Time,     FudgeTime )
MESSAGE_ADD_FIELD_PTR_IMPL( DateTime, FudgeDateTime )

MESSAGE_ADD_FIELD_ARRAY_IMPL( Byte, fudge_byte )
MESSAGE_ADD_FIELD_ARRAY_IMPL( I16,  fudge_i16 )
MESSAGE_ADD_FIELD_ARRAY_IMPL( I32,  fudge_i32 )
MESSAGE_ADD_FIELD_ARRAY_IMPL( I64,  fudge_i64 )
MESSAGE_ADD_FIELD_ARRAY_IMPL( F32,  fudge_f32 )
MESSAGE_ADD_FIELD_ARRAY_IMPL( F64,  fudge_f64 )

MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 4 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 8 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 16 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 20 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 32 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 64 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 128 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 256 );
MESSAGE_ADD_FIELD_FIXED_ARRAY_IMPL( 512 );


/****************************************************************************
 * Method implementations
 */

size_t Message_len ( Message * self )
{
    return FudgeMsg_numFields ( self->msg );
}

PyObject * Message_subscript ( Message * self, PyObject * key )
{
    PyObject * field;
    if ( PyInt_Check ( key ) )
    {
        fudge_i16 ordinal;
        if ( Message_parseOrdinalObject ( &ordinal, key ) )
            return 0;
        field = Message_getFieldWithOrdinal ( self, ordinal, 1 );
    }
    else if ( PyString_Check ( key ) || PyUnicode_Check ( key ) )
    {
        FudgeString name;
        if ( Message_parseNameObject ( &name, key ) )
            return 0;
        field = Message_getFieldWithName ( self, name, 1 );
        FudgeString_release ( name );
    }
    else
    {
        exception_raise_any ( PyExc_ValueError,
                              "Message[key] only accepts Integer, String or "
                              "Unicode types" );
        return 0;
    }
    return field;
}

static const char DOC_fudgepyc_message_addFieldIndicator [] =
    "\nAdds an indicator (empty) field to the Message. Field name and ordinal\n"
    "are optional.\n\n"
    "@param name: field name String, defaults to None\n"
    "@param ordinal: field ordinal integer, defaults to None\n"
    "@return: None or Exception on failure";
PyObject * Message_addFieldIndicator ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "name", "ordinal", 0 };

    PyObject * ordobj = 0, * nameobj = 0;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "|OO!", kwlist, &nameobj, &PyInt_Type, &ordobj ) )
        return 0;

    return Message_addFieldIndicatorImpl ( self, nameobj, ordobj );
}

MESSAGE_ADD_FIELD_SCALAR( Bool,     "bool",              "Boolean" )
MESSAGE_ADD_FIELD_SCALAR( Byte,     "int/long",          "Byte" )
MESSAGE_ADD_FIELD_SCALAR( I16,      "int/long",          "Short" )
MESSAGE_ADD_FIELD_SCALAR( I32,      "int/long",          "Int" )
MESSAGE_ADD_FIELD_SCALAR( I64,      "int/long",          "Long" )
MESSAGE_ADD_FIELD_SCALAR( F32,      "float",             "Float" )
MESSAGE_ADD_FIELD_SCALAR( F64,      "float",             "Double" )
MESSAGE_ADD_FIELD_SCALAR( String,   "String/Unicode",    "String" )
MESSAGE_ADD_FIELD_SCALAR( Date,     "datetime.date",     "Date" )
MESSAGE_ADD_FIELD_SCALAR( Time,     "datetime.time",     "Time" )
MESSAGE_ADD_FIELD_SCALAR( DateTime, "datetime.datetime", "DateTime" )

static const char DOC_fudgepyc_message_addFieldMsg [] =
    "\nAdds a Message field to the current Message; value must be a\n"
    "fudgepyc.Message instance. Note that the Message is referred to by\n"
    "reference and any changes made to it after being added will be included\n"
    "in the encoded message. Field name and ordinal are optional.\n\n"
    "@param value: field value, of type fudgepyc.Message\n"
    "@param name: field name String, defaults to None\n"
    "@param ordinal: field ordinal integer, defaults to None\n"
    "@return: None or Exception on failure";
PyObject * Message_addFieldMsg ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "value", "name", "ordinal", 0 };

    PyObject * ordobj = 0, * nameobj = 0, * msgobj;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O!|OO!", kwlist,
                                         &MessageType, &msgobj,
                                         &nameobj,
                                         &PyInt_Type, &ordobj ) )
        return 0;

    return Message_addFieldMsgImpl ( self, msgobj, nameobj, ordobj );
}

MESSAGE_ADD_FIELD_ARRAY( Byte, "String, Unicode or [int, ...]", "Byte[]" )
MESSAGE_ADD_FIELD_ARRAY( I16,  "[int, ...]",                    "Short[]" )
MESSAGE_ADD_FIELD_ARRAY( I32,  "[int, ...]",                    "Int[]" )
MESSAGE_ADD_FIELD_ARRAY( I64,  "[int, ...]",                    "Long[]" )
MESSAGE_ADD_FIELD_ARRAY( F32,  "[float, ...]",                  "Float[]" )
MESSAGE_ADD_FIELD_ARRAY( F64,  "[float, ...]",                  "Double[]" )

MESSAGE_ADD_FIELD_FIXED_ARRAY( 4 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 8 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 16 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 20 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 32 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 64 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 128 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 256 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 512 );

static const char DOC_fudgepyc_message_addFieldRawDate [] =
    "\nAdds a Date field to the message, using raw date components. Allows dates\n"
    "to be used outside the scope supported by the built-in datetime classes.\n\n"
    "@param year: year as integer, defaults to None\n"
    "@param month: integer from 0-12 (zero is \"unset\"), defaults to None\n"
    "@param day: integer from 0-31 (zero is \"unset\"), defaults to None\n"
    "@param name: field name String, defaults to None\n"
    "@param ordinal: field ordinal integer, defaults to None\n"
    "@return: None or Exception on failure\n";
PyObject * Message_addFieldRawDate ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "year", "month", "day", "name", "ordinal", 0 };

    PyObject * ordobj = 0,
             * nameobj = 0,
             * yearobj = 0,
             * monthobj = 0,
             * dayobj = 0;
    FudgeString name = 0;
    fudge_i16 ordinal;
    FudgeDate date;
    FudgeStatus status;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "|O!O!O!OO!", kwlist,
                                                     &PyInt_Type, &yearobj,
                                                     &PyInt_Type, &monthobj,
                                                     &PyInt_Type, &dayobj,
                                                     &nameobj,
                                                     &PyInt_Type, &ordobj ) )
        return 0;

    if ( fudgepyc_convertPythonToDateEx ( &date, yearobj, monthobj, dayobj ) )
        return 0;
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )
        return 0;
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )
        return 0;

    status = FudgeMsg_addFieldDate ( self->msg,
                                     name,
                                     ordobj ? &ordinal : 0,
                                     &date );
    FudgeString_release ( name );

    if ( exception_raiseOnError ( status ) )
        return 0;
    Py_RETURN_NONE;
}

static const char DOC_fudgepyc_message_addFieldRawTime [] =
    "\nAdds a Time field to the message, using raw time components. Allows times\n"
    "to be used outside the scope supported by the built-in datetime classes.\n\n"
    "@param precision: precision as an integer, see fudge.types for valid values\n"
    "@param hour: integer equal to or greater than zero, defaults to None\n"
    "@param minute: integer from 0-59, defaults to None\n"
    "@param second: integer from 0-59, defaults to None\n"
    "@param nanoseconds: integer from 0-1000000000, defaults to None\n"
    "@param offset: number of fifteen minute intervals that the localtime differs\n"
    "               from UTC by (e.g. UTC-5h would be -20). Defaults to None (no\n"
    "               timezone information)\n"
    "@return: None or Exception on failure\n";
PyObject * Message_addFieldRawTime ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "precision",
                                "hour", "minute", "second", "nanosecond", "offset",
                                "name", "ordinal", 0 };

    PyObject * ordobj = 0,
             * nameobj = 0,
             * hourobj = 0,
             * minobj = 0,
             * secobj = 0,
             * nanoobj = 0,
             * offsetobj = 0;
    unsigned int precision;
    FudgeString name = 0;
    fudge_i16 ordinal;
    FudgeTime time;
    FudgeStatus status;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "I|O!O!O!O!O!OO!", kwlist,
                                                     &precision,
                                                     &PyInt_Type, &hourobj,
                                                     &PyInt_Type, &minobj,
                                                     &PyInt_Type, &secobj,
                                                     &PyInt_Type, &nanoobj,
                                                     &PyInt_Type, &offsetobj,
                                                     &nameobj,
                                                     &PyInt_Type, &ordobj ) )
        return 0;

    if ( fudgepyc_convertPythonToTimeEx ( &time,
                                          precision,
                                          hourobj,
                                          minobj,
                                          secobj,
                                          nanoobj,
                                          offsetobj ) )
        return 0;
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )
        return 0;
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )
        return 0;

    status = FudgeMsg_addFieldTime ( self->msg,
                                     name,
                                     ordobj ? &ordinal : 0,
                                     &time );
    FudgeString_release ( name );

    if ( exception_raiseOnError ( status ) )
        return 0;
    Py_RETURN_NONE;
}

static const char DOC_fudgepyc_message_addFieldRawDateTime [] =
    "\nAdds a DateTime field to the message, using raw date and time components.\n"
    "Allows datetimes to be used outside the scope supported by the built-in\n"
    "datetime classes.\n\n"
    "@param precision: precision as an integer, see fudge.types for valid values\n"
    "@param year: year as integer, defaults to None\n"
    "@param month: integer from 0-12 (zero is \"unset\"), defaults to None\n"
    "@param day: integer from 0-31 (zero is \"unset\"), defaults to None\n"
    "@param hour: integer equal to or greater than zero, defaults to None\n"
    "@param minute: integer from 0-59, defaults to None\n"
    "@param second: integer from 0-59, defaults to None\n"
    "@param nanoseconds: integer from 0-1000000000, defaults to None\n"
    "@param offset: number of fifteen minute intervals that the localtime differs\n"
    "               from UTC by (e.g. UTC-5h would be -20). Defaults to None (no\n"
    "               timezone information)\n"
    "@return: None or Exception on failure\n";
PyObject * Message_addFieldRawDateTime ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "precision",
                                "year", "month", "day",
                                "hour", "minute", "second", "nanosecond", "offset",
                                "name", "ordinal", 0 };

    PyObject * ordobj = 0,
             * nameobj = 0,
             * yearobj = 0,
             * monthobj = 0,
             * dayobj = 0,
             * hourobj = 0,
             * minobj = 0,
             * secobj = 0,
             * nanoobj = 0,
             * offsetobj = 0;
    unsigned int precision;
    FudgeString name = 0;
    fudge_i16 ordinal;
    FudgeDateTime datetime;
    FudgeStatus status;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "I|O!O!O!O!O!O!O!O!OO!", kwlist,
                                                     &precision,
                                                     &PyInt_Type, &yearobj,
                                                     &PyInt_Type, &monthobj,
                                                     &PyInt_Type, &dayobj,
                                                     &PyInt_Type, &hourobj,
                                                     &PyInt_Type, &minobj,
                                                     &PyInt_Type, &secobj,
                                                     &PyInt_Type, &nanoobj,
                                                     &PyInt_Type, &offsetobj,
                                                     &nameobj,
                                                     &PyInt_Type, &ordobj ) )
        return 0;
    if ( fudgepyc_convertPythonToDateTimeEx ( &datetime,
                                              precision,
                                              yearobj,
                                              monthobj,
                                              dayobj,
                                              hourobj,
                                              minobj,
                                              secobj,
                                              nanoobj,
                                              offsetobj ) )
        return 0;
    if ( ordobj && Message_parseOrdinalObject ( &ordinal, ordobj ) )
        return 0;
    if ( nameobj && Message_parseNameObject ( &name, nameobj ) )
        return 0;

    status = FudgeMsg_addFieldDateTime ( self->msg,
                                         name,
                                         ordobj ? &ordinal : 0,
                                         &datetime );

    if ( exception_raiseOnError ( status ) )
        return 0;
    Py_RETURN_NONE;
}

static const char DOC_fudgepyc_message_addField [] =
    "\nAdds a field to the Message. If the type is specified (should be\n"
    "Fudge type, see fudgepyc.types) then the field is assumed to be that.\n"
    "If no type is specified then the field type is determined by the type\n"
    "of the value. The Python types map to Fudge as follows:\n"
    "\n"
    "  - None: Indicator\n"
    "  - bool: Boolean\n"
    "  - int: Byte/Short/Int/Long (depends on bits required to hold value)\n"
    "  - long: See previous\n"
    "  - float: Double\n"
    "  - String: String\n"
    "  - Unicode: String\n"
    "  - fudgepyc.Message: FudgeMsg\n"
    "  - datetime.date: Date\n"
    "  - datetime.time: Time\n"
    "  - datetime.datetime: DateTime\n"
    "\n"
    "All other types must be added using an explicity set type, or using one\n"
    "of the named type adder methods. Field name and ordinal are optional.\n"
    "\n"
    "@param value: field value\n"
    "@param name: field name String, defaults to None\n"
    "@param ordinal: field ordinal integer, defaults to None\n"
    "@param type: Fudge type id (0-255), defaults to None\n"
    "@return: None or Exception on failure\n";
PyObject * Message_addField ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "value", "name", "ordinal", "type", 0 };

    PyObject * ordobj = 0,
             * nameobj = 0,
             * typeobj = 0,
             * valobj;
    fudge_type_id fudgetype;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!O!", kwlist,
                                         &valobj,
                                         &nameobj,
                                         &PyInt_Type, &ordobj,
                                         &PyInt_Type, &typeobj ) )
        return 0;

    /* Either get the type from the type object parameter, or attempt to
       determine the type from that of the Python value object */
    if ( typeobj )
    {
        int type = ( int ) PyInt_AsLong ( typeobj );
        if ( type < 0 || type > 255 )
        {
            exception_raise_any ( PyExc_OverflowError,
                                  "Type parameter for Message.addField must "
                                  "be within range 0<=N<=255" );
            return 0;
        }

        fudgetype = ( fudge_type_id ) type;
    }
    else
    {
        if ( Message_getFudgeType ( &fudgetype, valobj ) )
        {
            /* Cannot figure out the type - raise a useful error */
            PyObject * typestr = 0;
            if ( ( typeobj = PyObject_Type ( valobj ) ) )
                typestr = PyObject_Str ( typeobj );

            if ( typeobj && typestr )
                exception_raise_any ( PyExc_TypeError,
                                      "Cannot determine Fudge type for %s",
                                      PyString_AsString ( typestr ) );

            Py_XDECREF( typeobj );
            Py_XDECREF( typestr );
            return 0;
        }
    }

    /* Pass off to correct addField implementation for fudge type */
    switch ( fudgetype )
    {
        case FUDGE_TYPE_INDICATOR:      return Message_addFieldIndicatorImpl ( self, nameobj, ordobj );
        case FUDGE_TYPE_BOOLEAN:        return Message_addFieldBoolImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE:           return Message_addFieldByteImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_SHORT:          return Message_addFieldI16Impl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_INT:            return Message_addFieldI32Impl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_LONG:           return Message_addFieldI64Impl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_FLOAT:          return Message_addFieldF32Impl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_DOUBLE:         return Message_addFieldF64Impl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_STRING:         return Message_addFieldStringImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_FUDGE_MSG:      return Message_addFieldMsgImpl ( self, valobj, nameobj, ordobj );

        case FUDGE_TYPE_BYTE_ARRAY:     return Message_addFieldByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_SHORT_ARRAY:    return Message_addFieldI16ArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_INT_ARRAY:      return Message_addFieldI32ArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_LONG_ARRAY:     return Message_addFieldI64ArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_FLOAT_ARRAY:    return Message_addFieldF32ArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_DOUBLE_ARRAY:   return Message_addFieldF64ArrayImpl ( self, valobj, nameobj, ordobj );

        case FUDGE_TYPE_BYTE_ARRAY_4:   return Message_addField4ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_8:   return Message_addField8ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_16:  return Message_addField16ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_20:  return Message_addField20ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_32:  return Message_addField32ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_64:  return Message_addField64ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_128: return Message_addField128ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_256: return Message_addField256ByteArrayImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_BYTE_ARRAY_512: return Message_addField512ByteArrayImpl ( self, valobj, nameobj, ordobj );

        case FUDGE_TYPE_DATE:           return Message_addFieldDateImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_TIME:           return Message_addFieldTimeImpl ( self, valobj, nameobj, ordobj );
        case FUDGE_TYPE_DATETIME:       return Message_addFieldDateTimeImpl ( self, valobj, nameobj, ordobj );

        default:
            exception_raise_any ( PyExc_TypeError,
                                  "No addField implemention found for Fudge "
                                  "type %d", fudgetype );
            return 0;
    }
}

static const char DOC_fudgepyc_message_getFieldAtIndex [] =
    "\nGet the field at the given index. Throws an exception if the index is\n"
    "invalid.\n\n"
    "@param index: field index integer\n"
    "@return fudgepyc.Field or throws fudgepyc.Exception if index not valid\n";
PyObject * Message_getFieldAtIndex ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "index", 0 };

    FudgeField field;
    unsigned int index;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "I", kwlist, &index ) )
        return 0;

    if ( exception_raiseOnError ( FudgeMsg_getFieldAtIndex ( &field,
                                                             self->msg,
                                                             index ) ) )
        return 0;
    return Field_create ( field, self );
}

static const char DOC_fudgepyc_message_getFieldByName [] =
    "\nGet the first field with the given name. Returns None if no such field\n"
    "is found.\n\n"
    "@param name: field name String/Unicode\n"
    "@return fudgepyc.Field or None if name not found\n";
PyObject * Message_getFieldByName ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "name", 0 };

    FudgeString name;
    PyObject * obj;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O", kwlist, &obj ) )
        return 0;
    if ( fudgepyc_convertPythonToString ( &name, obj ) )
        return 0;

    obj = Message_getFieldWithName ( self, name, 0 );
    FudgeString_release ( name );
    return obj;
}

static const char DOC_fudgepyc_message_getFieldByOrdinal [] =
    "\nGet the first field with the given ordinal. Returns None if no such\n"
    "field is found.\n\n"
    "@param ordinal: field ordinal integer\n"
    "@return fudgepyc.Field or None if ordinal not found\n";
PyObject * Message_getFieldByOrdinal ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "ordinal", 0 };

    unsigned short ordinal;
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "H", kwlist, &ordinal ) )
        return 0;

    return Message_getFieldWithOrdinal ( self, ordinal, 0 );
}

static const char DOC_fudgepyc_message_getFields [] =
    "\nGet all the fields in the message, in insertion order.\n\n"
    "@return [fudgepyc.Field, ...]\n";
PyObject * Message_getFields ( Message * self )
{
    FudgeField * source;
    PyObject * target = 0,
             * field;
    size_t numfields = FudgeMsg_numFields ( self->msg ),
           index;

    if ( ! ( source = ( FudgeField * ) PyMem_Malloc ( sizeof ( FudgeField )
                                                      * numfields ) ) )
        return 0;
    numfields = FudgeMsg_getFields ( source, ( fudge_i32 ) numfields, self->msg );

    if ( ! ( target = PyList_New ( numfields ) ) )
        goto free_mem_and_return;

    for ( index = 0; index < numfields; ++index )
    {
        if ( ! ( field = Field_create ( source [ index ], self ) ) )
            goto free_target_and_return;
        PyList_SET_ITEM ( target, index, field );
    }
    goto free_mem_and_return;

free_target_and_return:
    Py_XDECREF ( target );

free_mem_and_return:
    PyMem_Free ( source );
    return target;
}

PyObject * Message_str ( Message * self )
{
    PyObject * fields,
             * fieldstr,
             * joinstr,
             * string = 0;
    size_t index, numfields;

    /* Retrieve all the fields as Python objects */
    if ( ! ( fields = Message_getFields ( self ) ) )
        return 0;
    numfields = PyList_Size ( fields );

    /* This will be used to join the field strings */
    if ( ! ( joinstr = PyString_FromString ( ", " ) ) )
        goto clear_fields_and_return;

    if ( ! ( string = PyString_FromFormat ( "Message[" ) ) )
        goto clear_joinstr_and_return;

    for ( index = 0; index < numfields; ++index )
    {
        if ( index )
        {
            PyString_Concat ( &string, joinstr );
            if ( ! string )
                goto clear_joinstr_and_return;
        }

        /* As the fields are Python objects, can use the stringize method */
        fieldstr = PyObject_Str ( PyList_GET_ITEM( fields, index ) );
        if ( ! fieldstr )
        {
            Py_DECREF( string );
            goto clear_joinstr_and_return;
        }

        PyString_ConcatAndDel ( &string, fieldstr );
        if ( ! string )
            goto clear_joinstr_and_return;
    }

    if ( ( fieldstr = PyString_FromString ( "]" ) ) )
        PyString_ConcatAndDel ( &string, fieldstr );

clear_joinstr_and_return:
    Py_DECREF( joinstr );
clear_fields_and_return:
    Py_DECREF( fields );
    return string;
}


/****************************************************************************
 * Type and method list definitions
 */

static PyMethodDef Message_methods [] =
{
    { "addFieldIndicator",    ( PyCFunction ) Message_addFieldIndicator,    METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldIndicator },
    { "addFieldBool",         ( PyCFunction ) Message_addFieldBool,         METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldBool },
    { "addFieldByte",         ( PyCFunction ) Message_addFieldByte,         METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldByte },
    { "addFieldI16",          ( PyCFunction ) Message_addFieldI16,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI16 },
    { "addFieldI32",          ( PyCFunction ) Message_addFieldI32,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI32 },
    { "addFieldI64",          ( PyCFunction ) Message_addFieldI64,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI64 },
    { "addFieldF32",          ( PyCFunction ) Message_addFieldF32,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldF32 },
    { "addFieldF64",          ( PyCFunction ) Message_addFieldF64,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldF64 },

    { "addFieldMsg",          ( PyCFunction ) Message_addFieldMsg,          METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldMsg },
    { "addFieldString",       ( PyCFunction ) Message_addFieldString,       METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldString },

    { "addFieldByteArray",    ( PyCFunction ) Message_addFieldByteArray,    METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldByteArray },
    { "addFieldI16Array",     ( PyCFunction ) Message_addFieldI16Array,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI16Array },
    { "addFieldI32Array",     ( PyCFunction ) Message_addFieldI32Array,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI32Array },
    { "addFieldI64Array",     ( PyCFunction ) Message_addFieldI64Array,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldI64Array },
    { "addFieldF32Array",     ( PyCFunction ) Message_addFieldF32Array,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldF32Array },
    { "addFieldF64Array",     ( PyCFunction ) Message_addFieldF64Array,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldF64Array },

    { "addField4ByteArray",   ( PyCFunction ) Message_addField4ByteArray,   METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField4ByteArray },
    { "addField8ByteArray",   ( PyCFunction ) Message_addField8ByteArray,   METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField8ByteArray },
    { "addField16ByteArray",  ( PyCFunction ) Message_addField16ByteArray,  METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField16ByteArray },
    { "addField20ByteArray",  ( PyCFunction ) Message_addField20ByteArray,  METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField20ByteArray },
    { "addField32ByteArray",  ( PyCFunction ) Message_addField32ByteArray,  METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField32ByteArray },
    { "addField64ByteArray",  ( PyCFunction ) Message_addField64ByteArray,  METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField64ByteArray },
    { "addField128ByteArray", ( PyCFunction ) Message_addField128ByteArray, METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField128ByteArray },
    { "addField256ByteArray", ( PyCFunction ) Message_addField256ByteArray, METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField256ByteArray },
    { "addField512ByteArray", ( PyCFunction ) Message_addField512ByteArray, METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField512ByteArray },

    { "addFieldDate",         ( PyCFunction ) Message_addFieldDate,         METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldDate },
    { "addFieldTime",         ( PyCFunction ) Message_addFieldTime,         METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldTime },
    { "addFieldDateTime",     ( PyCFunction ) Message_addFieldDateTime,     METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldDateTime },

    { "addFieldRawDate",      ( PyCFunction ) Message_addFieldRawDate,      METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldRawDate },
    { "addFieldRawTime",      ( PyCFunction ) Message_addFieldRawTime,      METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldRawTime },
    { "addFieldRawDateTime",  ( PyCFunction ) Message_addFieldRawDateTime,  METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addFieldRawDateTime },

    { "addField",             ( PyCFunction ) Message_addField,             METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_addField },

    { "getFieldAtIndex",      ( PyCFunction ) Message_getFieldAtIndex,      METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_getFieldAtIndex },
    { "getFieldByName",       ( PyCFunction ) Message_getFieldByName,       METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_getFieldByName },
    { "getFieldByOrdinal",    ( PyCFunction ) Message_getFieldByOrdinal,    METH_VARARGS | METH_KEYWORDS, DOC_fudgepyc_message_getFieldByOrdinal },
    { "getFields",            ( PyCFunction ) Message_getFields,            METH_NOARGS,                  DOC_fudgepyc_message_getFields },
    { NULL }
};

PyMappingMethods Message_as_mapping =
{
    ( lenfunc ) Message_len,            // mp_length
    ( binaryfunc ) Message_subscript,   // mp_subscript
    0                                   // mp_ass_subscript
};

PyTypeObject MessageType =
{
    PyObject_HEAD_INIT( NULL )
    0,                                              /* ob_size */
    "fudgepyc.Message",                             /* tp_name */
    sizeof ( Message ),                             /* tp_basicsize */
    0,                                              /* tp_itemsize */
    ( destructor ) Message_dealloc,                 /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_compare */
    0,                                              /* tp_repr */
    0,                                              /* tp_as_number */
    0,                                              /* tp_as_sequence */
    &Message_as_mapping,                            /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    ( reprfunc ) Message_str,                       /* tp_str */
    0,                                              /* tp_getattro */
    0,                                              /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    DOC_fudgepyc_message,                           /* tp_doc */
    0,                                              /* tp_traverse */
    0,                                              /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Message_methods,                                /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    ( initproc ) Message_init,                      /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    Message_new                                     /* tp_new */
};


/****************************************************************************
 * Type functions
 */

PyObject * Message_create ( FudgeMsg msg )
{
    Message * obj = ( Message * ) MessageType.tp_new ( &MessageType, 0, 0 );
    if ( obj )
        FudgeMsg_retain ( ( obj->msg = msg ) );
    return ( PyObject * ) obj;
}

void Message_storeMessage ( Message * self, Message * field )
{
    PyObject * rawptr;
    int result;

    if ( Message_createMsgDict ( self ) )
        return;

    if ( ! ( rawptr = PyInt_FromLong ( ( long ) field->msg ) ) )
        return;

    result = PyDict_SetItem ( self->msgdict, rawptr, ( PyObject * ) field );
    Py_DECREF( rawptr );
}

PyObject * Message_retrieveMessage ( Message * self, FudgeMsg msg )
{
    PyObject * rawptr, * target;

    if ( Message_createMsgDict ( self ) )
        return 0;

    if ( ! ( rawptr = PyInt_FromLong ( ( long ) msg ) ) )
        return 0;

    if ( ( target = PyDict_GetItem ( self->msgdict, rawptr ) ) )
    {
        Py_INCREF( target );
        goto clean_and_return;
    }

    if ( ! ( target = Message_create ( msg ) ) )
        goto clean_and_return;

    PyDict_SetItem ( self->msgdict, rawptr, target );

clean_and_return:
    Py_DECREF( rawptr );
    return target;
}

int Message_modinit ( PyObject * module )
{
    PyDateTime_IMPORT;
    return 0;
}

