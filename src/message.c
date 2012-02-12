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


/****************************************************************************
 * Method helper macros
 */

#define MESSAGE_ADD_FIELD_SCALAR( TYPENAME, CTYPE, PYSTR, FUDGESTR )        \
static const char DOC_fudgepyc_message_addField ## TYPENAME [] =            \
    "Adds a " FUDGESTR " field to the Message; value must be of Python "    \
    "type\n" PYSTR ". Field name and ordinal are optional.\n\n"             \
    "@param value: field value, of type " PYSTR "\n"                        \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defauls to None\n"              \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## TYPENAME ( Message * self,                   \
                                          PyObject * args,                  \
                                          PyObject * kwds )                 \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    FudgeStatus status;                                                     \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    CTYPE value;                                                            \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
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
                                                                            \
    if ( exception_raiseOnError ( status ) )                                \
        return 0;                                                           \
    Py_RETURN_NONE;                                                         \
}

#define MESSAGE_ADD_FIELD_ARRAY( TYPENAME, CTYPE, PYSTR, FUDGESTR )         \
static const char DOC_fudgepyc_message_addField ## TYPENAME ## Array [] =   \
    "Adds a " FUDGESTR " field to the Message; value must be of Python\n"   \
    "type " PYSTR ". Field name and ordinal are optional.\n\n"              \
    "@param value: field value\n"                                           \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defauls to None\n"              \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## TYPENAME ## Array ( Message * self,          \
                                                   PyObject * args,         \
                                                   PyObject * kwds )        \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    FudgeStatus status;                                                     \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    CTYPE * array;                                                          \
    fudge_i32 size;                                                         \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
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

#define MESSAGE_ADD_FIELD_FIXED_ARRAY( WIDTH )                              \
static const char DOC_fudgepyc_message_addField ## WIDTH ## ByteArray [] =  \
    "Adds a Byte[" #WIDTH "] field to the Message; value must be of\n"      \
    "Python type String, Unicode or [int, ...] and have a length of "       \
    #WIDTH ".\nField name and ordinal are optional.\n\n"                   \
    "@param value: field value\n"                                           \
    "@param name: field name String, defaults to None\n"                    \
    "@param ordinal: field ordinal integer, defauls to None\n"              \
    "@return: None or Exception on failure\n";                              \
PyObject * Message_addField ## WIDTH ## ByteArray (                         \
               Message * self,                                              \
               PyObject * args,                                             \
               PyObject * kwds )                                            \
{                                                                           \
    static char * kwlist [] = { "value", "name", "ordinal", 0 };            \
                                                                            \
    FudgeStatus status;                                                     \
    PyObject * ordobj = 0, * nameobj = 0, * valobj;                         \
    FudgeString name = 0;                                                   \
    fudge_i16 ordinal;                                                      \
    fudge_byte array [ WIDTH ];                                             \
                                                                            \
    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,       \
                                         &valobj,                           \
                                         &nameobj,                          \
                                         &PyInt_Type, &ordobj ) )           \
        return 0;                                                           \
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

MESSAGE_ADD_FIELD_SCALAR( Bool,   fudge_bool,  "bool",           "Boolean" )
MESSAGE_ADD_FIELD_SCALAR( Byte,   fudge_byte,  "int/long",       "Byte" )
MESSAGE_ADD_FIELD_SCALAR( I16,    fudge_i16,   "int/long",       "Short" )
MESSAGE_ADD_FIELD_SCALAR( I32,    fudge_i32,   "int/long",       "Int" )
MESSAGE_ADD_FIELD_SCALAR( I64,    fudge_i64,   "int/long",       "Long" )
MESSAGE_ADD_FIELD_SCALAR( F32,    fudge_f32,   "float",          "Float" )
MESSAGE_ADD_FIELD_SCALAR( F64,    fudge_f64,   "float",          "Double" )
MESSAGE_ADD_FIELD_SCALAR( String, FudgeString, "String/Unicode", "String" )

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

    FudgeStatus status;
    PyObject * ordobj = 0, * nameobj = 0, * msgobj;
    FudgeString name = 0;
    fudge_i16 ordinal;
    FudgeMsg message;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O!|OO!", kwlist,
                                         &MessageType, &msgobj,
                                         &nameobj,
                                         &PyInt_Type, &ordobj ) )
        return 0;
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

MESSAGE_ADD_FIELD_ARRAY( Byte, fudge_byte, "String, Unicode or [int, ...]", "Byte[]" )
MESSAGE_ADD_FIELD_ARRAY( I16,  fudge_i16,  "[int, ...]",                    "Short[]" )
MESSAGE_ADD_FIELD_ARRAY( I32,  fudge_i32,  "[int, ...]",                    "Int[]" )
MESSAGE_ADD_FIELD_ARRAY( I64,  fudge_i64,  "[int, ...]",                    "Long[]" )
MESSAGE_ADD_FIELD_ARRAY( F32,  fudge_f32,  "[float, ...]",                  "Float[]" )
MESSAGE_ADD_FIELD_ARRAY( F64,  fudge_f64,  "[float, ...]",                  "Double[]" )

MESSAGE_ADD_FIELD_FIXED_ARRAY( 4 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 8 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 16 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 20 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 32 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 64 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 128 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 256 );
MESSAGE_ADD_FIELD_FIXED_ARRAY( 512 );

static const char DOC_fudgepyc_message_addField [] =
    "\nAdds a field to the Message, with the type determined by that of value.\n"
    "The Python types map to Fudge as follows:\n"
    "\n"
    "  - None: Indicator\n"
    "  - bool: Boolean\n"
    "  - int: Byte/Short/Int/Long (depends on bits required to hold value)\n"
    "  - long: See previous\n"
    "  - float: Double\n"
    "  - String: String\n"
    "  - Unicode: String\n"
    "  - fudgepyc.Message: FudgeMsg\n"
    "\n"
    "All other types must be added using the explicitly typed methods. Field\n"
    "name and ordinal are optional.\n"
    "\n"
    "@param value: field value\n"
    "@param name: field name String, defaults to None\n"
    "@param ordinal: field ordinal integer, defaults to None\n"
    "@return: None or Exception on failure\n";
PyObject * Message_addField ( Message * self, PyObject * args, PyObject * kwds )
{
    static char * kwlist [] = { "value", "name", "ordinal", 0 };

    PyObject * ordobj = 0, * nameobj = 0, * valobj;

    if ( ! PyArg_ParseTupleAndKeywords ( args, kwds, "O|OO!", kwlist,
                                         &valobj,
                                         &nameobj,
                                         &PyInt_Type, &ordobj ) )
        return 0;

    /* Figure out what type the value object is, then hand off to the correct
       add field method */
    if ( valobj == Py_None )
        return Message_addFieldIndicatorImpl ( self, nameobj, ordobj );
    else if ( PyBool_Check ( valobj ) )
        return Message_addFieldBool ( self, args, kwds );
    else if ( PyInt_Check ( valobj ) || PyLong_Check ( valobj ) )
        return Message_addFieldI64 ( self, args, kwds );
    else if ( PyFloat_Check ( valobj ) )
        return Message_addFieldF64 ( self, args, kwds );
    else if ( PyString_Check ( valobj ) || PyUnicode_Check ( valobj ) )
        return Message_addFieldString ( self, args, kwds );
    else if ( PyObject_IsInstance ( ( PyObject * ) self,
                                    ( PyObject * ) &MessageType ) )
        return Message_addFieldMsg ( self, args, kwds );
    else
    {
        /* Try to generate a useful error message */
        PyObject * typeobj = 0, * typestr = 0;

        if ( ! ( typeobj = PyObject_Type ( valobj ) ) )
            goto clean_and_fail;
        if ( ! ( typestr = PyObject_Str ( typeobj ) ) )
            goto clean_and_fail;

        exception_raise_any ( PyExc_TypeError,
                              "Don't know how to addField of %s",
                              PyString_AsString ( typestr ) );
clean_and_fail:
        Py_XDECREF( typeobj );
        Py_XDECREF( typestr );
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

    /* TODO Date/time */

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

/* TODO Add Message_modinit function and change Message dictionary to add
   documentation for __getitem__. */

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

