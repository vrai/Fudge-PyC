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
#include "field.h"
#include "converters.h"

/* References to fudgepyc.types module and the TYPE_NAMES dictionary
 * contained within it. Used to retrieve type names for stringization. */
static PyObject * s_typesmodule = 0,
                * s_typenamedict = 0;


/****************************************************************************
 * Constructor/destructor implementations
 */

static const char DOC_fudgepyc_field [] =
    "\nfudgepyc.Field contains a single fudgepyc.Message field. All Field\n"
    "instances hold a reference to their parent Message, so a Message\n"
    "instance will not be destroyed until all Fields referencing it have\n"
    "first been destroyed.\n"
    "\n"
    "The only way to create a Field is using one of the getter methods on a\n"
    "Message instance. Calling the Field construct will result in a\n"
    "fudgepyc.Exception being thrown.\n";
static int Field_init ( Field * self, PyObject * args, PyObject * kwds )
{
    exception_raise_any ( PyExc_NotImplementedError,
                          "Field cannot be constructed directly, only "
                          "returned from Message methods" );
    return 0;
}

static PyObject * Field_new ( PyTypeObject * type, PyObject * args, PyObject * kwds )
{
    Field * obj = ( Field * ) type->tp_alloc ( type, 0 );
    if ( obj )
        obj->parent = 0;
    return ( PyObject * ) obj;
}

static void Field_dealloc ( Field * self )
{
    Py_XDECREF( self->parent );
    self->ob_type->tp_free ( self );
}

/****************************************************************************
 * Method helper macros
 */

#define FIELD_GET_TYPE( TYPENAME, TYPEID, PYSTR, FUDGESTR )                 \
    static const char DOC_fudgepyc_field_get ## TYPENAME [] =               \
    "\nGet the Field value as a " PYSTR ", if it is of the Fudge type\n"    \
    FUDGESTR ".\n\n"                                                        \
    "@return: " PYSTR ", or fudgepyc.Exception if of incorrect type\n";     \
PyObject * Field_get ## TYPENAME ( Field * self )                           \
{                                                                           \
    if ( self->field.type == TYPEID )                                       \
        return Field_value ( self );                                        \
    exception_raise_any ( FudgePyc_Exception,                               \
                         "Invalid conversion to " #TYPENAME );              \
    return 0;                                                               \
}

#define FIELD_GET_AS_SCALAR_TYPE( TYPENAME, CTYPE, PYSTR )                  \
static const char DOC_fudgepyc_field_getAs ## TYPENAME [] =                 \
"\nGet the field value as " PYSTR ", if Fudge can coerce the value.\n\n"    \
"@return: " PYSTR ", or fudgepyc.Exception if the coercion fails\n";        \
PyObject * Field_getAs ## TYPENAME ( Field * self )                         \
{                                                                           \
    FudgeStatus status;                                                     \
    CTYPE target;                                                           \
    if ( ( status = FudgeMsg_getFieldAs ## TYPENAME (                       \
                    &( self->field ), &target ) ) )                         \
    {                                                                       \
        exception_raiseOnError ( status );                                  \
        return 0;                                                           \
    }                                                                       \
    return fudgepyc_convert ## TYPENAME ## ToPython ( target );             \
}

/****************************************************************************
 * Method implementations
 */

size_t Field_len ( Field * self )
{
    if ( self->field.type == FUDGE_TYPE_INDICATOR )
        return 0;
    if ( self->field.type < FUDGE_TYPE_BYTE_ARRAY )
        return 1;

    switch ( self->field.type )
    {
        /* Calculate number of elements in typed arrays */
        case FUDGE_TYPE_SHORT_ARRAY:
            return self->field.numbytes / sizeof ( fudge_i16 );
        case FUDGE_TYPE_INT_ARRAY:
            return self->field.numbytes / sizeof ( fudge_i32 );
        case FUDGE_TYPE_LONG_ARRAY:
            return self->field.numbytes / sizeof ( fudge_i64 );
        case FUDGE_TYPE_FLOAT_ARRAY:
            return self->field.numbytes / sizeof ( fudge_f32 );
        case FUDGE_TYPE_DOUBLE_ARRAY:
            return self->field.numbytes / sizeof ( fudge_f64 );

        /* Strings and messages have their own size methods */
        case FUDGE_TYPE_STRING:
            return FudgeString_getLength ( self->field.data.string );
        case FUDGE_TYPE_FUDGE_MSG:
            return FudgeMsg_numFields ( self->field.data.message );

        /* Fixed size byte arrays */
        case FUDGE_TYPE_BYTE_ARRAY_4:   return 4;
        case FUDGE_TYPE_BYTE_ARRAY_8:   return 8;
        case FUDGE_TYPE_BYTE_ARRAY_16:  return 16;
        case FUDGE_TYPE_BYTE_ARRAY_20:  return 20;
        case FUDGE_TYPE_BYTE_ARRAY_32:  return 32;
        case FUDGE_TYPE_BYTE_ARRAY_64:  return 64;
        case FUDGE_TYPE_BYTE_ARRAY_128: return 128;
        case FUDGE_TYPE_BYTE_ARRAY_256: return 256;
        case FUDGE_TYPE_BYTE_ARRAY_512: return 512;

        /* Dates and times are scalar */
        case FUDGE_TYPE_DATE:
        case FUDGE_TYPE_TIME:
        case FUDGE_TYPE_DATETIME:
            return 1;

        /* Treat unknown types as variable sized byte arrays */
        case FUDGE_TYPE_BYTE_ARRAY:
        default:
            return self->field.numbytes;
    }
}

static const char DOC_fudgepyc_field_value [] =
"\nGet the Field's value as a Python object. The Fudge field types map to\n"
"Python as follows:\n"
"\n"
"  - Indicator: None\n"
"  - Boolean: bool\n"
"  - Byte: int\n"
"  - Short: int\n"
"  - Int: int\n"
"  - Long: long\n"
"  - Float: float\n"
"  - Double: float\n"
"  - Byte[]: String\n"
"  - Short[]: List of int\n"
"  - Int[]: List of int\n"
"  - Long[]: List of long\n"
"  - Float[]: List of float\n"
"  - Double[]: List fo double\n"
"  - String: Unicode\n"
"  - FudgeMsg: fudgepyc.Message\n"
"\n"
"@return: Python object containing the Field value\n";
PyObject * Field_value ( Field * self )
{
    switch ( self->field.type )
    {
        case FUDGE_TYPE_INDICATOR:
            Py_RETURN_NONE;
        case FUDGE_TYPE_BOOLEAN:
            return fudgepyc_convertBoolToPython ( self->field.data.boolean );
        case FUDGE_TYPE_BYTE:
            return fudgepyc_convertByteToPython ( self->field.data.byte );
        case FUDGE_TYPE_SHORT:
            return fudgepyc_convertI16ToPython ( self->field.data.i16 );
        case FUDGE_TYPE_INT:
            return fudgepyc_convertI32ToPython ( self->field.data.i32 );
        case FUDGE_TYPE_LONG:
            return fudgepyc_convertI64ToPython ( self->field.data.i64 );
        case FUDGE_TYPE_FLOAT:
            return fudgepyc_convertF32ToPython ( self->field.data.f32 );
        case FUDGE_TYPE_DOUBLE:
            return fudgepyc_convertF64ToPython ( self->field.data.f64 );

        case FUDGE_TYPE_BYTE_ARRAY:
        case FUDGE_TYPE_BYTE_ARRAY_4:
        case FUDGE_TYPE_BYTE_ARRAY_8:
        case FUDGE_TYPE_BYTE_ARRAY_16:
        case FUDGE_TYPE_BYTE_ARRAY_20:
        case FUDGE_TYPE_BYTE_ARRAY_32:
        case FUDGE_TYPE_BYTE_ARRAY_64:
        case FUDGE_TYPE_BYTE_ARRAY_128:
        case FUDGE_TYPE_BYTE_ARRAY_256:
        case FUDGE_TYPE_BYTE_ARRAY_512:
            return fudgepyc_convertByteStringToPython ( self->field.data.bytes,
                                                        self->field.numbytes );

        case FUDGE_TYPE_SHORT_ARRAY:
            return fudgepyc_convertI16ArrayToPython ( self->field.data.bytes,
                                                      self->field.numbytes );
        case FUDGE_TYPE_INT_ARRAY:
            return fudgepyc_convertI32ArrayToPython ( self->field.data.bytes,
                                                      self->field.numbytes );
        case FUDGE_TYPE_LONG_ARRAY:
            return fudgepyc_convertI64ArrayToPython ( self->field.data.bytes,
                                                      self->field.numbytes );
        case FUDGE_TYPE_FLOAT_ARRAY:
            return fudgepyc_convertF32ArrayToPython ( self->field.data.bytes,
                                                      self->field.numbytes );
        case FUDGE_TYPE_DOUBLE_ARRAY:
            return fudgepyc_convertF64ArrayToPython ( self->field.data.bytes,
                                                      self->field.numbytes );

        case FUDGE_TYPE_STRING:
            return fudgepyc_convertStringToPython ( self->field.data.string );

        case FUDGE_TYPE_FUDGE_MSG:
            return Message_retrieveMessage ( self->parent,
                                             self->field.data.message );

        case FUDGE_TYPE_DATE:
        case FUDGE_TYPE_TIME:
        case FUDGE_TYPE_DATETIME:
            /* TODO Implement date/time support */
            exception_raise_any ( PyExc_NotImplementedError,
                                  "Date/time support not complete" );
            return 0;

        default:
            /* If in doubt - return a bundle of bytes */
            return fudgepyc_convertByteStringToPython ( self->field.data.bytes,
                                                        self->field.numbytes );
    }
}

static const char DOC_fudgepyc_field_name [] =
    "\nGet the Field's name, if it has one\n\n"
    "@return: String containing the Field name, or None if not present\n";
PyObject * Field_name ( Field * field )
{
    if ( field->field.flags & FUDGE_FIELD_HAS_NAME )
        return fudgepyc_convertStringToPython ( field->field.name );
    else
        Py_RETURN_NONE;
}

static const char DOC_fudgepyc_field_ordinal [] =
    "\nGet the Field's ordinal, if it has one\n\n"
    "@return: ordinal integer, or None if not present\n";
PyObject * Field_ordinal ( Field * field )
{
    if ( field->field.flags & FUDGE_FIELD_HAS_ORDINAL )
        return PyInt_FromLong ( field->field.ordinal );
    else
        Py_RETURN_NONE;
}

static const char DOC_fudgepyc_field_type  [] =
    "\nGet the Fudge type of Field's value. See fudgepyc.types for a list of\n"
    "types and their names\n\n"
    "@return: type integer\n";
PyObject * Field_type ( Field * field )
{
    return PyInt_FromLong ( field->field.type );
}

static const char DOC_fudgepyc_field_numbytes [] =
    "\nGet the number of bytes used by non-scalar values (e.g. string,\n"
    "message, arrays, etc).\n\n"
    "@return: number of bytes or zero if scalar type\n";
PyObject * Field_numbytes ( Field * field )
{
    return PyInt_FromLong ( field->field.numbytes );
}

static const char DOC_fudgepyc_field_bytes [] =
    "\nThe Field value as a String containing the raw bytes. Only applicable\n"
    "to arrays (of all types) and unknown (i.e. user) types.\n\n"
    "@return: String containing bytes, or fudgepyc.Exception if wrong type\n";
PyObject * Field_bytes ( Field * self )
{
    if ( ( self->field.type >= FUDGE_TYPE_BYTE_ARRAY &&
           self->field.type <= FUDGE_TYPE_DOUBLE_ARRAY ) ||
         ( self->field.type >= FUDGE_TYPE_BYTE_ARRAY_4 &&
           self->field.type <= FUDGE_TYPE_BYTE_ARRAY_512 ) ||
           self->field.type > FUDGE_TYPE_DATETIME )
        return fudgepyc_convertByteStringToPython ( self->field.data.bytes,
                                                    self->field.numbytes );
    exception_raise_any ( FudgePyc_Exception,
                          "Can only retrieve bytes from array fields" );
    return 0;
}

FIELD_GET_TYPE( Bool,     FUDGE_TYPE_BOOLEAN,      "bool",         "Boolean" )
FIELD_GET_TYPE( Byte,     FUDGE_TYPE_BYTE,         "int",          "Byte" )
FIELD_GET_TYPE( I16,      FUDGE_TYPE_SHORT,        "int",          "Short" )
FIELD_GET_TYPE( I32,      FUDGE_TYPE_INT,          "int",          "Int" )
FIELD_GET_TYPE( I64,      FUDGE_TYPE_LONG,         "long",         "Long" )
FIELD_GET_TYPE( F32,      FUDGE_TYPE_FLOAT,        "float",        "Float" )
FIELD_GET_TYPE( F64,      FUDGE_TYPE_DOUBLE,       "float",        "Double" )
FIELD_GET_TYPE( Msg,      FUDGE_TYPE_FUDGE_MSG,    "Message",      "FudgeMsg" )
FIELD_GET_TYPE( String,   FUDGE_TYPE_STRING,       "Unicode",      "String" )

FIELD_GET_TYPE( I16Array, FUDGE_TYPE_SHORT_ARRAY,  "[int, ...]",   "Short[]" )
FIELD_GET_TYPE( I32Array, FUDGE_TYPE_INT_ARRAY,    "[int, ...]",   "Int[]" )
FIELD_GET_TYPE( I64Array, FUDGE_TYPE_LONG_ARRAY,   "[long, ...]",  "Long[]" )
FIELD_GET_TYPE( F32Array, FUDGE_TYPE_FLOAT_ARRAY,  "[float, ...]", "Float[]" )
FIELD_GET_TYPE( F64Array, FUDGE_TYPE_DOUBLE_ARRAY, "[float, ...]", "Double[]" )

static const char DOC_fudgepyc_field_getByteArray [] =
"\nGet the field value as a [int, ...], if it is a byte array (either\n"
"fixed or variable width. To get the bytes as a String, use\n"
"fudgepyc.Field.bytes or fudgepyc.Field.value\n\n"
"@return: [int, ...], or fudgepyc.Exception if of incorrect type\n";
PyObject * Field_getByteArray ( Field * field )
{
    if ( field->field.type == FUDGE_TYPE_BYTE_ARRAY ||
         ( field->field.type >= FUDGE_TYPE_BYTE_ARRAY_4 &&
           field->field.type <= FUDGE_TYPE_BYTE_ARRAY_512 ) )
        return fudgepyc_convertByteArrayToPython ( field->field.data.bytes,
                                                   field->field.numbytes );
    exception_raise_any ( FudgePyc_Exception,
                          "Invalid conversion to Byte array" );
    return 0;
}

static const char DOC_fudgepyc_field_getAsBool [] =
"\nGet the field value as bool, if Fudge can coerce the value.\n\n"
"@return: bool, or fudgepyc.Exception if the coercion fails\n";
PyObject * Field_getAsBool ( Field * self )
{
    fudge_bool target;
    FudgeStatus status = FudgeMsg_getFieldAsBoolean ( &self->field, &target );
    if ( exception_raiseOnError ( status ) )
        return 0;
    return fudgepyc_convertBoolToPython ( target );
}

FIELD_GET_AS_SCALAR_TYPE( Byte, fudge_byte, "8-bit int" )
FIELD_GET_AS_SCALAR_TYPE( I16,  fudge_i16,  "16-bit int" )
FIELD_GET_AS_SCALAR_TYPE( I32,  fudge_i32,  "int" )
FIELD_GET_AS_SCALAR_TYPE( I64,  fudge_i64,  "long" )
FIELD_GET_AS_SCALAR_TYPE( F32,  fudge_f32,  "float" )
FIELD_GET_AS_SCALAR_TYPE( F64,  fudge_f64,  "float" )

PyObject * Field_int ( Field * self )
{
    FudgeStatus status;
    fudge_i32 target;
    switch ( self->field.type )
    {
        case FUDGE_TYPE_FLOAT:
        case FUDGE_TYPE_DOUBLE:
        {
            fudge_f64 intermediate;
            status = FudgeMsg_getFieldAsF64 ( &self->field, &intermediate );
            if ( exception_raiseOnError ( status ) )
                return 0;
            target = ( fudge_i32 ) intermediate;
            break;
        }

        default:
            status = FudgeMsg_getFieldAsI32 ( &self->field, &target );
            if ( exception_raiseOnError ( status ) )
               return 0;
            break;
    }
    return fudgepyc_convertI32ToPython ( target );
}

PyObject * Field_long ( Field * self )
{
    FudgeStatus status;
    fudge_i64 target;
    switch ( self->field.type )
    {
        case FUDGE_TYPE_FLOAT:
        case FUDGE_TYPE_DOUBLE:
        {
            fudge_f64 intermediate;
            status = FudgeMsg_getFieldAsF64 ( &self->field, &intermediate );
            if ( exception_raiseOnError ( status ) )
                return 0;
            target = ( fudge_i64 ) intermediate;
            break;
        }

        default:
            status = FudgeMsg_getFieldAsI64 ( &self->field, &target );
            if ( exception_raiseOnError ( status ) )
                return 0;
            break;
    }
    return fudgepyc_convertI64ToPython ( target );
}

PyObject * Field_str ( Field * self )
{
    PyObject * pyobj,
             * string,
             * typeobj;

    if ( ! ( string = PyString_FromFormat ( "Field[" ) ) )
        return 0;

    /* Output the field name - if present */
    if ( self->field.flags & FUDGE_FIELD_HAS_NAME )
    {
        PyObject * namestr,
                 * nameobj = Field_name ( self );
        if ( ! nameobj )
            goto clear_string_and_fail;

        namestr = PyObject_Str ( nameobj );
        Py_DECREF( nameobj );
        if ( ! namestr )
            goto clear_string_and_fail;

        PyString_ConcatAndDel ( &string, namestr );
        if ( ! string )
            return 0;
    }

    /* Separator between name and ordinal */
    PyString_ConcatAndDel ( &string, PyString_FromString ( "|" ) );
    if ( ! string )
        return 0;

    /* Output the field ordinal - if present */
    if ( self->field.flags & FUDGE_FIELD_HAS_ORDINAL )
    {
        PyString_ConcatAndDel ( &string, PyString_FromFormat (
                                             "%d", self->field.ordinal ) );
        if ( ! string )
            return 0;
    }

    /* Separator between ordinal and type name */
    PyString_ConcatAndDel ( &string, PyString_FromString ( "|" ) );
    if ( ! string )
        return 0;

    /* Retrieve the type name from the lookup in the types module; if
     * unknown, display the type id */
    if ( ! ( typeobj = PyInt_FromLong ( self->field.type ) ) )
        goto clear_string_and_fail;
    pyobj = PyDict_GetItem ( s_typenamedict, typeobj );
    Py_DECREF( typeobj );
    if ( pyobj )
        PyString_Concat ( &string, pyobj );
    else
        PyString_ConcatAndDel ( &string,
                                PyString_FromFormat ( "%d", self->field.type ) );
    if ( ! string )
        return 0;

    /* Separator between type name and the value */
    PyString_ConcatAndDel ( &string, PyString_FromString ( ":" ) );
    if ( ! string )
        return 0;

    /* Output the stringized field value */
    switch ( self->field.type )
    {
        case FUDGE_TYPE_INDICATOR:
            break;

        /* For all non-byte array types, use the stringized Python value */
        case FUDGE_TYPE_BOOLEAN:
        case FUDGE_TYPE_BYTE:
        case FUDGE_TYPE_SHORT:
        case FUDGE_TYPE_INT:
        case FUDGE_TYPE_LONG:
        case FUDGE_TYPE_FLOAT:
        case FUDGE_TYPE_DOUBLE:
        case FUDGE_TYPE_SHORT_ARRAY:
        case FUDGE_TYPE_INT_ARRAY:
        case FUDGE_TYPE_LONG_ARRAY:
        case FUDGE_TYPE_FLOAT_ARRAY:
        case FUDGE_TYPE_DOUBLE_ARRAY:
        case FUDGE_TYPE_STRING:
        case FUDGE_TYPE_FUDGE_MSG:
        case FUDGE_TYPE_DATE:
        case FUDGE_TYPE_TIME:
        case FUDGE_TYPE_DATETIME:
            if ( ! ( pyobj = Field_value ( self ) ) )
                goto clear_string_and_fail;
            PyString_ConcatAndDel ( &string, PyObject_Str ( pyobj ) );
            Py_DECREF( pyobj );
            break;

        /* Assume everything else is just a bundle of (potentially
         * unprintable) bytes */
        default:
            PyString_ConcatAndDel ( &string,
                                    PyString_FromFormat ( "<%d bytes>",
                                                          self->field.numbytes ) );
            break;
    }

    if ( string )
        PyString_ConcatAndDel ( &string, PyString_FromString ( "]" ) );
    return string;

clear_string_and_fail:
    Py_XDECREF( string );
    return 0;
}

/****************************************************************************
 * Type and method list definitions
 */

static PyMethodDef Field_methods [] =
{
    { "name",            ( PyCFunction ) Field_name,         METH_NOARGS, DOC_fudgepyc_field_name },
    { "ordinal",         ( PyCFunction ) Field_ordinal,      METH_NOARGS, DOC_fudgepyc_field_ordinal },
    { "type",            ( PyCFunction ) Field_type,         METH_NOARGS, DOC_fudgepyc_field_type },
    { "numbytes",        ( PyCFunction ) Field_numbytes,     METH_NOARGS, DOC_fudgepyc_field_numbytes },
    { "bytes",           ( PyCFunction ) Field_bytes,        METH_NOARGS, DOC_fudgepyc_field_bytes },

    { "getBool",         ( PyCFunction ) Field_getBool,      METH_NOARGS, DOC_fudgepyc_field_getBool },
    { "getByte",         ( PyCFunction ) Field_getByte,      METH_NOARGS, DOC_fudgepyc_field_getByte },
    { "getInt16",        ( PyCFunction ) Field_getI16,       METH_NOARGS, DOC_fudgepyc_field_getI16 },
    { "getInt32",        ( PyCFunction ) Field_getI32,       METH_NOARGS, DOC_fudgepyc_field_getI32 },
    { "getInt64",        ( PyCFunction ) Field_getI64,       METH_NOARGS, DOC_fudgepyc_field_getI64 },
    { "getFloat32",      ( PyCFunction ) Field_getF32,       METH_NOARGS, DOC_fudgepyc_field_getF32 },
    { "getFloat64",      ( PyCFunction ) Field_getF64,       METH_NOARGS, DOC_fudgepyc_field_getF64 },

    { "getMessage",      ( PyCFunction ) Field_getMsg,       METH_NOARGS, DOC_fudgepyc_field_getMsg },
    { "getString",       ( PyCFunction ) Field_getString,    METH_NOARGS, DOC_fudgepyc_field_getString },

    { "getByteArray",    ( PyCFunction ) Field_getByteArray, METH_NOARGS, DOC_fudgepyc_field_getByteArray },
    { "getInt16Array",   ( PyCFunction ) Field_getI16Array,  METH_NOARGS, DOC_fudgepyc_field_getI16Array },
    { "getInt32Array",   ( PyCFunction ) Field_getI32Array,  METH_NOARGS, DOC_fudgepyc_field_getI32Array },
    { "getInt64Array",   ( PyCFunction ) Field_getI64Array,  METH_NOARGS, DOC_fudgepyc_field_getI64Array },
    { "getFloat32Array", ( PyCFunction ) Field_getF32Array,  METH_NOARGS, DOC_fudgepyc_field_getF32Array },
    { "getFloat64Array", ( PyCFunction ) Field_getF64Array,  METH_NOARGS, DOC_fudgepyc_field_getF64Array },

    { "getAsBool",       ( PyCFunction ) Field_getAsBool,    METH_NOARGS, DOC_fudgepyc_field_getAsBool },
    { "getAsByte",       ( PyCFunction ) Field_getAsByte,    METH_NOARGS, DOC_fudgepyc_field_getAsByte },
    { "getAsInt16",      ( PyCFunction ) Field_getAsI16,     METH_NOARGS, DOC_fudgepyc_field_getAsI16 },
    { "getAsInt32",      ( PyCFunction ) Field_getAsI32,     METH_NOARGS, DOC_fudgepyc_field_getAsI32 },
    { "getAsInt64",      ( PyCFunction ) Field_getAsI64,     METH_NOARGS, DOC_fudgepyc_field_getAsI64 },
    { "getAsFloat32",    ( PyCFunction ) Field_getAsF32,     METH_NOARGS, DOC_fudgepyc_field_getAsF32 },
    { "getAsFloat64",    ( PyCFunction ) Field_getAsF64,     METH_NOARGS, DOC_fudgepyc_field_getAsF64 },

    { "value",           ( PyCFunction ) Field_value,        METH_NOARGS, DOC_fudgepyc_field_value },
    { NULL }
};

PySequenceMethods Field_as_sequence =
{
    ( lenfunc ) Field_len,                  /* sq_length */
    0,                                      /* sq_concat */
    0,                                      /* sq_repeat */
    0,                                      /* sq_item */
    0,                                      /* sq_slice */
    0,                                      /* sq_ass_item */
    0,                                      /* sq_ass_slice */
    0,                                      /* sq_contains */
    0,                                      /* sq_inplace_concat */
    0                                       /* sq_inplace_repeat */
};

PyNumberMethods Field_as_number =
{
    0,                                      /* nb_add */
    0,                                      /* nb_subtract */
    0,                                      /* nb_multiply */
    0,                                      /* nb_divide */
    0,                                      /* nb_remainder */
    0,                                      /* nb_divmod */
    0,                                      /* nb_power */
    0,                                      /* nb_negative */
    0,                                      /* nb_positive */
    0,                                      /* nb_absolute */
    0,                                      /* nb_nonzero */
    0,                                      /* nb_invert */
    0,                                      /* nb_lshift */
    0,                                      /* nb_rshift */
    0,                                      /* nb_and */
    0,                                      /* nb_xor */
    0,                                      /* nb_or */
    0,                                      /* nb_coerce */
    ( unaryfunc ) Field_int,                /* nb_int */
    ( unaryfunc ) Field_long,               /* nb_long */
    ( unaryfunc ) Field_getAsF64,           /* nb_float */
    0,                                      /* nb_oct */
    0,                                      /* nb_hex */
    0,                                      /* nb_inplace_add */
    0,                                      /* nb_inplace_subtract */
    0,                                      /* nb_inplace_multiply */
    0,                                      /* nb_inplace_divide */
    0,                                      /* nb_inplace_remainder */
    0,                                      /* nb_inplace_power */
    0,                                      /* nb_inplace_lshift */
    0,                                      /* nb_inplace_rshift */
    0,                                      /* nb_inplace_and */
    0,                                      /* nb_inplace_xor */
    0,                                      /* nb_inplace_or */
    0,                                      /* nb_floor_divide */
    0,                                      /* nb_true_divide */
    0,                                      /* nb_inplace_floor_divide */
    0,                                      /* nb_inplace_true_divide */
    0                                       /* nb_index */
};

PyTypeObject FieldType =
{
    PyObject_HEAD_INIT( NULL )
    0,                                              /* ob_size */
    "fudgepyc.Field",                               /* tp_name */
    sizeof ( Field ),                               /* tp_basicsize */
    0,                                              /* tp_itemsize */
    ( destructor ) Field_dealloc,                   /* tp_dealloc */
    0,                                              /* tp_print */
    0,                                              /* tp_getattr */
    0,                                              /* tp_setattr */
    0,                                              /* tp_compare */
    0,                                              /* tp_repr */
    &Field_as_number,                               /* tp_as_number */
    &Field_as_sequence,                             /* tp_as_sequence */
    0,                                              /* tp_as_mapping */
    0,                                              /* tp_hash */
    0,                                              /* tp_call */
    ( reprfunc ) &Field_str,                        /* tp_str */
    0,                                              /* tp_getattro */
    0,                                              /* tp_setattro */
    0,                                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    DOC_fudgepyc_field,                             /* tp_doc */
    0,                                              /* tp_traverse */
    0,                                              /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Field_methods,                                  /* tp_methods */
    0,                                              /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    ( initproc ) Field_init,                        /* tp_init */
    PyType_GenericAlloc,                            /* tp_alloc */
    Field_new                                       /* tp_new */
};

/****************************************************************************
 * Type functions
 */

extern PyObject * Field_create ( FudgeField field, Message * parent )
{
    Field * obj = ( Field * ) FieldType.tp_new ( &FieldType, 0, 0 );
    if ( obj )
    {
        Py_INCREF( ( PyObject * ) parent );
        obj->parent = parent;
        obj->field = field;
    }
    return ( PyObject * ) obj;
}

int Field_modinit ( PyObject * module )
{
    if ( ! s_typesmodule &&
         ! ( s_typesmodule = PyImport_ImportModule ( "fudgepyc.types" ) ) )
        return -1;
    if ( ! s_typenamedict &&
         ! ( s_typenamedict = PyObject_GetAttrString ( s_typesmodule,
                                                       "TYPE_NAMES" ) ) )
        return -1;
    return 0;
}

