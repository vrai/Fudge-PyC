# Copyright (C) 2012 - 2012, Vrai Stacey.
#
# Part of the Fudge-PyC distribution.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os.path
from functools import partial
from unittest import TestCase, TestSuite
import fudgepyc
import fudgepyc.types
from fudgepyc import Envelope, Field, Message

DATA_DIR = 'data'
DATA_FILES = { 'ALLNAMES'      : 'allNames.dat',
               'ALLORDINALS'   : 'allOrdinals.dat',
               'FIXEDWIDTH'    : 'fixedWidthByteArrays.dat',
               'SUBMSG'        : 'subMsg.dat',
               'UNKNOWN'       : 'unknown.dat',
               'VARIABLEWIDTH' : 'variableWidthColumnSizes.dat',
               'DATETIMES'     : 'dateTimes.dat',
               'DEEPERTREE'    : 'deeper_fudge_msg.dat' }

class CodecTestCase ( TestCase ):
    def setUp ( self ):
        fudgepyc.init ( )
        datadir = os.path.join ( os.path.split ( __file__ ) [ 0 ], DATA_DIR )
        self.__datafiles = dict ( ( k, os.path.join ( datadir, v ) )
                                   for k, v, in DATA_FILES.iteritems ( ) )

    def testDecodeAllNames ( self ):
        message1 = self.__loadMessage ( 'ALLNAMES' )
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 21 )

        # None of the fields should have ordinals
        for field in fields:
            self.assertEqual ( field.ordinal ( ), None )

        # Check the scalar field names and contents
        self.__checkField ( fields [  0 ], fudgepyc.types.BOOLEAN,   True,         'boolean',   None, Field.value )
        self.__checkField ( fields [  1 ], fudgepyc.types.BOOLEAN,   False,        'Boolean',   None, Field.getBool )
        self.__checkField ( fields [  2 ], fudgepyc.types.BYTE,      5,            'byte',      None, Field.value )
        self.__checkField ( fields [  3 ], fudgepyc.types.BYTE,      5,            'Byte',      None, Field.getByte )
        self.__checkField ( fields [  4 ], fudgepyc.types.SHORT,     132,          'short',     None, Field.value )
        self.__checkField ( fields [  5 ], fudgepyc.types.SHORT,     132,          'Short',     None, Field.getInt16 )
        self.__checkField ( fields [  6 ], fudgepyc.types.INT,       32772,        'int',       None, Field.value )
        self.__checkField ( fields [  7 ], fudgepyc.types.INT,       32772,        'Integer',   None, Field.getInt32 )
        self.__checkField ( fields [  8 ], fudgepyc.types.LONG,      2147483652l,  'long',      None, Field.value )
        self.__checkField ( fields [  9 ], fudgepyc.types.LONG,      2147483652l,  'Long',      None, Field.getInt64 )
        self.__checkField ( fields [ 10 ], fudgepyc.types.FLOAT,     0.5,          'float',     None, Field.value,      partial ( TestCase.assertAlmostEqual, places = 5 ) )
        self.__checkField ( fields [ 11 ], fudgepyc.types.FLOAT,     0.5,          'Float',     None, Field.getFloat32, partial ( TestCase.assertAlmostEqual, places = 5 ) )
        self.__checkField ( fields [ 12 ], fudgepyc.types.DOUBLE,    0.27362,      'double',    None, Field.value,      partial ( TestCase.assertAlmostEqual, places = 7 ) )
        self.__checkField ( fields [ 13 ], fudgepyc.types.DOUBLE,    0.27362,      'Double',    None, Field.getFloat64, partial ( TestCase.assertAlmostEqual, places = 7 ) )
        self.__checkField ( fields [ 14 ], fudgepyc.types.STRING,    'Kirk Wylie', 'String',    None, Field.value )
        self.__checkField ( fields [ 20 ], fudgepyc.types.INDICATOR, None,         'indicator', None, Field.value )

        # Check the array field names and contents (should all have zero values)
        self.__checkField ( fields [ 15 ], fudgepyc.types.FLOAT_ARRAY,  [ 0.0 ] * 24,  'float array',  None, Field.value )
        self.__checkField ( fields [ 16 ], fudgepyc.types.DOUBLE_ARRAY, [ 0.0 ] * 273, 'double array', None, Field.value )
        self.__checkField ( fields [ 17 ], fudgepyc.types.SHORT_ARRAY,  [ 0 ] * 32,    'short array',  None, Field.value )
        self.__checkField ( fields [ 18 ], fudgepyc.types.INT_ARRAY,    [ 0 ] * 83,    'int array',    None, Field.value )
        self.__checkField ( fields [ 19 ], fudgepyc.types.LONG_ARRAY,   [ 0l ] * 837,  'long array',   None, Field.value )


    def testDecodeAllOrdinals ( self ):
        message1 = self.__loadMessage ( 'ALLORDINALS' )
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 17 )

        # None of the fields should have names
        for field in fields:
            self.assertEqual ( field.name ( ), None )

        # Check the scalar field ordinals and contents
        self.__checkField ( fields [  0 ], fudgepyc.types.BOOLEAN,   True,         None,  1, Field.value )
        self.__checkField ( fields [  1 ], fudgepyc.types.BOOLEAN,   False,        None,  2, Field.getBool )
        self.__checkField ( fields [  2 ], fudgepyc.types.BYTE,      5,            None,  3, Field.value )
        self.__checkField ( fields [  3 ], fudgepyc.types.BYTE,      5,            None,  4, Field.getByte )
        self.__checkField ( fields [  4 ], fudgepyc.types.SHORT,     132,          None,  5, Field.value )
        self.__checkField ( fields [  5 ], fudgepyc.types.SHORT,     132,          None,  6, Field.getInt16 )
        self.__checkField ( fields [  6 ], fudgepyc.types.INT,       32772,        None,  7, Field.value )
        self.__checkField ( fields [  7 ], fudgepyc.types.INT,       32772,        None,  8, Field.getInt32 )
        self.__checkField ( fields [  8 ], fudgepyc.types.LONG,      2147483652l,  None,  9, Field.value )
        self.__checkField ( fields [  9 ], fudgepyc.types.LONG,      2147483652l,  None, 10, Field.getInt64 )
        self.__checkField ( fields [ 10 ], fudgepyc.types.FLOAT,     0.5,          None, 11, Field.value,      partial ( TestCase.assertAlmostEqual, places = 5 ) )
        self.__checkField ( fields [ 11 ], fudgepyc.types.FLOAT,     0.5,          None, 12, Field.getFloat32, partial ( TestCase.assertAlmostEqual, places = 5 ) )
        self.__checkField ( fields [ 12 ], fudgepyc.types.DOUBLE,    0.27362,      None, 13, Field.value,      partial ( TestCase.assertAlmostEqual, places = 7 ) )
        self.__checkField ( fields [ 13 ], fudgepyc.types.DOUBLE,    0.27362,      None, 14, Field.getFloat64, partial ( TestCase.assertAlmostEqual, places = 7 ) )
        self.__checkField ( fields [ 14 ], fudgepyc.types.STRING,    'Kirk Wylie', None, 15, Field.value )

        # Check the array field ordinals and contents (should all have zero values)
        self.__checkField ( fields [ 15 ], fudgepyc.types.FLOAT_ARRAY,  [ 0.0 ] * 24,  None, 16, Field.value )
        self.__checkField ( fields [ 16 ], fudgepyc.types.DOUBLE_ARRAY, [ 0.0 ] * 273, None, 17, Field.value )


    def testDecodeFixedWidth ( self ):
        fields = self.__loadMessage ( 'FIXEDWIDTH' ).getFields ( )
        self.assertEqual ( len ( fields ), 10 )

        # Create string and sequence versions of the largest byte array (compensating for
        # Python's use of unsigned characters)
        rawByteSeq = self.__createIntSeq ( 512, 8 )
        rawByteStr = ''.join ( chr ( b % 256 ) for b in rawByteSeq )

        # Check the field contents
        self.__checkField ( fields [ 0 ], fudgepyc.types.BYTE_ARRAY_4,   rawByteSeq [ :   4 ], 'byte[4]',   None, Field.getByteArray )
        self.__checkField ( fields [ 0 ], fudgepyc.types.BYTE_ARRAY_4,   rawByteStr [ :   4 ], 'byte[4]',   None, Field.value )
        self.__checkField ( fields [ 1 ], fudgepyc.types.BYTE_ARRAY_8,   rawByteSeq [ :   8 ], 'byte[8]',   None, Field.getByteArray )
        self.__checkField ( fields [ 1 ], fudgepyc.types.BYTE_ARRAY_8,   rawByteStr [ :   8 ], 'byte[8]',   None, Field.bytes )
        self.__checkField ( fields [ 2 ], fudgepyc.types.BYTE_ARRAY_16,  rawByteSeq [ :  16 ], 'byte[16]',  None, Field.getByteArray )
        self.__checkField ( fields [ 2 ], fudgepyc.types.BYTE_ARRAY_16,  rawByteStr [ :  16 ], 'byte[16]',  None, Field.value )
        self.__checkField ( fields [ 3 ], fudgepyc.types.BYTE_ARRAY_20,  rawByteSeq [ :  20 ], 'byte[20]',  None, Field.getByteArray )
        self.__checkField ( fields [ 3 ], fudgepyc.types.BYTE_ARRAY_20,  rawByteStr [ :  20 ], 'byte[20]',  None, Field.bytes )
        self.__checkField ( fields [ 4 ], fudgepyc.types.BYTE_ARRAY_32,  rawByteSeq [ :  32 ], 'byte[32]',  None, Field.getByteArray )
        self.__checkField ( fields [ 4 ], fudgepyc.types.BYTE_ARRAY_32,  rawByteStr [ :  32 ], 'byte[32]',  None, Field.value )
        self.__checkField ( fields [ 5 ], fudgepyc.types.BYTE_ARRAY_64,  rawByteSeq [ :  64 ], 'byte[64]',  None, Field.getByteArray )
        self.__checkField ( fields [ 5 ], fudgepyc.types.BYTE_ARRAY_64,  rawByteStr [ :  64 ], 'byte[64]',  None, Field.bytes )
        self.__checkField ( fields [ 6 ], fudgepyc.types.BYTE_ARRAY_128, rawByteSeq [ : 128 ], 'byte[128]', None, Field.getByteArray )
        self.__checkField ( fields [ 6 ], fudgepyc.types.BYTE_ARRAY_128, rawByteStr [ : 128 ], 'byte[128]', None, Field.value )
        self.__checkField ( fields [ 7 ], fudgepyc.types.BYTE_ARRAY_256, rawByteSeq [ : 256 ], 'byte[256]', None, Field.getByteArray )
        self.__checkField ( fields [ 7 ], fudgepyc.types.BYTE_ARRAY_256, rawByteStr [ : 256 ], 'byte[256]', None, Field.bytes )
        self.__checkField ( fields [ 8 ], fudgepyc.types.BYTE_ARRAY_512, rawByteSeq [ : 512 ], 'byte[512]', None, Field.getByteArray )
        self.__checkField ( fields [ 8 ], fudgepyc.types.BYTE_ARRAY_512, rawByteStr [ : 512 ], 'byte[512]', None, Field.value )

        self.__checkField ( fields [ 9 ], fudgepyc.types.BYTE_ARRAY, rawByteSeq [ : 28 ], 'byte[28]', None, Field.getByteArray )
        self.__checkField ( fields [ 9 ], fudgepyc.types.BYTE_ARRAY, rawByteStr [ : 28 ], 'byte[28]', None, Field.value )
        self.__checkField ( fields [ 9 ], fudgepyc.types.BYTE_ARRAY, rawByteStr [ : 28 ], 'byte[28]', None, Field.bytes )


    def testDecodeSubMsgs ( self ):
        message1 = self.__loadMessage ( 'SUBMSG' )
        self.assertEqual ( len ( message1 ), 2 )

        field1 = message1.getFieldAtIndex ( 0 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.MESSAGE )
        self.assertEqual ( field1.name ( ), 'sub1' )
        message2 = field1.getMessage ( )
        self.assertEqual ( len ( message2 ), 2 )

        self.__checkField ( message2.getFieldAtIndex ( 0 ), fudgepyc.types.STRING, 'fibble',  'bibble', None, Field.value )
        self.__checkField ( message2.getFieldAtIndex ( 1 ), fudgepyc.types.STRING, 'Blibble', None,     827,  Field.getString )

        field1 = message1.getFieldAtIndex ( 1 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.MESSAGE )
        self.assertEqual ( field1.name ( ), 'sub2' )
        message2 = field1.getMessage ( )
        self.assertEqual ( len ( message2 ), 2 )

        self.__checkField ( message2.getFieldAtIndex ( 0 ), fudgepyc.types.INT,   9837438,  'bibble9', None, Field.value )
        self.__checkField ( message2.getFieldAtIndex ( 1 ), fudgepyc.types.FLOAT, 82.77,    None,      828,  Field.value, partial ( TestCase.assertAlmostEqual, places = 5 ) )


    def testDecodeUnknown ( self ):
        message1 = self.__loadMessage ( 'UNKNOWN' )
        self.assertEqual ( len ( message1 ), 1 )

        # Unknown type is just 10 zero'd bytes
        rawByteStr = ''.join ( [ chr ( 0 ) ] * 10 )

        self.__checkField ( message1.getFieldAtIndex ( 0 ), 200, rawByteStr, 'unknown', None, Field.bytes )
        self.__checkField ( message1 [ 'unknown' ],         200, rawByteStr, 'unknown', None, Field.value )


    def testDecodeVariableWidths ( self ):
        fields = self.__loadMessage ( 'VARIABLEWIDTH' ).getFields ( )
        self.assertEqual ( len ( fields ), 3 )

        # All the arrays are full of zero'd bytes
        rawByteSeq = [ 0 ] * 100000
        rawByteStr = ''.join ( chr ( b % 256 ) for b in rawByteSeq )

        for field, name, size in zip ( fields,
                                       ( '100', '1000', '10000' ),
                                       ( 100,   1000,   100000 ) ):
            self.__checkField ( field, fudgepyc.types.BYTE_ARRAY, rawByteSeq [ : size ], name, None, Field.getByteArray )
            self.__checkField ( field, fudgepyc.types.BYTE_ARRAY, rawByteStr [ : size ], name, None, Field.value )
            self.__checkField ( field, fudgepyc.types.BYTE_ARRAY, rawByteStr [ : size ], name, None, Field.bytes )


    def testDecodeDateTimes ( self ):
        message1 = self.__loadMessage ( 'DATETIMES' )
        # TODO Add when date/time/datetime functionality is added to Python wrapper


    def testDecodeDeepTree ( self ):
        rawByteStr = ''.join ( chr ( idx % 256 ) for idx in range ( 512 ) )
        rawShortSeq = self.__createIntSeq ( 15, 16 )
        rawIntSeq = [ 0l ] * 15
        rawLongSeq = [ 0l ] * 15
        rawFloatSeq = [ 0.0 ] * 15
        rawDoubleSeq = [ float ( i ) / 10.0 for i in self.__createIntSeq ( 15, 64 ) ]

        # Check the top-level fields
        message1 = self.__loadMessage ( 'DEEPERTREE' )
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 13 )

        self.__checkField ( fields [  0 ], fudgepyc.types.INDICATOR, None,                 'Indicator',    None, Field.value )
        self.__checkField ( fields [  1 ], fudgepyc.types.BOOLEAN,   True,                 'Boolean',      None, Field.value )
        self.__checkField ( fields [  2 ], fudgepyc.types.BYTE,      -128,                 'Byte',         None, Field.value )
        self.__checkField ( fields [  3 ], fudgepyc.types.SHORT,     -32768,               'Short',        None, Field.value )
        self.__checkField ( fields [  4 ], fudgepyc.types.INT,       2147483647,           'Int',          None, Field.value )
        self.__checkField ( fields [  5 ], fudgepyc.types.LONG,      9223372036854775807l, 'Long',         None, Field.value )
        self.__checkField ( fields [  6 ], fudgepyc.types.FLOAT,     1.23456,              'Float',        None, Field.value, partial ( TestCase.assertAlmostEqual, places = 5 ) )
        self.__checkField ( fields [  7 ], fudgepyc.types.DOUBLE,    1.2345678,            'Double',       None, Field.value, partial ( TestCase.assertAlmostEqual, places = 7 ) )
        self.__checkField ( fields [  8 ], fudgepyc.types.MESSAGE,   True,                 'ByteArrays',   None, Field.value, lambda *x: True )
        self.__checkField ( fields [  9 ], fudgepyc.types.STRING,    '',                   'Empty String', None, Field.value )
        self.__checkField ( fields [ 10 ], fudgepyc.types.STRING,    'This is a string.',  'String',       None, Field.value )
        self.__checkField ( fields [ 11 ], fudgepyc.types.MESSAGE,   True,                 'Arrays',       None, Field.value, lambda *x: True )
        self.__checkField ( fields [ 12 ], fudgepyc.types.MESSAGE,   True,                 'Null Message', None, Field.value, lambda *x: True )

        # Check the fixed byte array message
        sizes = ( 4, 8, 16, 20, 32, 64, 128, 256, 512 )

        fields = message1 [ 'ByteArrays' ].value ( ).getFields ( )
        self.assertEqual ( len ( fields ), len ( sizes ) )
        for idx in range ( len ( fields ) ):
            self.__checkField ( fields [ idx ], fudgepyc.types.BYTE_ARRAY_4 + idx, rawByteStr [ : sizes [ idx ] ], None, sizes [ idx ], Field.value )

        # Check the arrays message
        fields = message1 [ 'Arrays' ].value ( ).getFields ( )
        self.assertEqual ( len ( fields ), 9 )

        self.__checkField ( fields [ 0 ], fudgepyc.types.BYTE_ARRAY,  '',                  u'Byte[0]',   None, Field.value )
        self.__checkField ( fields [ 1 ], fudgepyc.types.BYTE_ARRAY,  rawByteStr [ : 15 ], u'Byte[15]',  None, Field.value )
        self.__checkField ( fields [ 2 ], fudgepyc.types.MESSAGE,     True,                u'FP Arrays', None, Field.value, lambda *x: True )
        self.__checkField ( fields [ 3 ], fudgepyc.types.SHORT_ARRAY, [ ],                 u'Short[0]',  None, Field.value )
        self.__checkField ( fields [ 4 ], fudgepyc.types.SHORT_ARRAY, rawShortSeq,         u'Short[15]', None, Field.value )
        self.__checkField ( fields [ 5 ], fudgepyc.types.INT_ARRAY,   [ ],                 u'Int[0]',    None, Field.value )
        self.__checkField ( fields [ 6 ], fudgepyc.types.INT_ARRAY,   rawIntSeq,           u'Int[15]',   None, Field.value )
        self.__checkField ( fields [ 7 ], fudgepyc.types.LONG_ARRAY,  [ ],                 u'Long[0]',   None, Field.value )
        self.__checkField ( fields [ 8 ], fudgepyc.types.LONG_ARRAY,  rawLongSeq,          u'Long[15]',  None, Field.value )

        # Check the FP arrays message
        fields = fields [ 2 ].getMessage ( ).getFields ( )
        self.assertEqual ( len ( fields ), 4 )

        self.__checkField ( fields [ 0 ], fudgepyc.types.FLOAT_ARRAY,  [ ],          'Float[0]',    None, Field.value )
        self.__checkField ( fields [ 1 ], fudgepyc.types.FLOAT_ARRAY,  rawFloatSeq,  'Float[15]',   None, Field.value )
        self.__checkField ( fields [ 2 ], fudgepyc.types.DOUBLE_ARRAY, [ ],          'Double[0]',   None, Field.value )
        self.__checkField ( fields [ 3 ], fudgepyc.types.DOUBLE_ARRAY, rawDoubleSeq, 'Double[15]',  None, Field.value )

        # Check the empty message
        nullmessage = message1 [ 'Null Message' ].getMessage ( )
        self.assertEqual ( len ( nullmessage ), 0 )


    def testEncodeAllNames ( self ):
        # Construct the message
        message1 = Message ( )
        message1.addField ( True,         'boolean' )
        message1.addField ( False,        'Boolean' )
        message1.addField ( 5,            'byte' )
        message1.addField ( 5,            'Byte' )
        message1.addField ( 132,          'short' )
        message1.addField ( 132,          'Short' )
        message1.addField ( 32772,        'int' )
        message1.addField ( 32772,        'Integer' )
        message1.addField ( 2147483652l,  'long' )
        message1.addField ( 2147483652l,  'Long' )
        message1.addFieldF32 ( 0.5,       'float' )
        message1.addFieldF32 ( 0.5,       'Float' )
        message1.addField ( 0.27362,      'double' )
        message1.addField ( 0.27362,      'Double' )
        message1.addField ( 'Kirk Wylie', 'String' )

        message1.addFieldF32Array ( [ 0.0 ] * 24,  'float array' )
        message1.addFieldF64Array ( [ 0.0 ] * 273, 'double array' )
        message1.addFieldI16Array ( [ 0 ] * 32,    'short array' )
        message1.addFieldI32Array ( [ 0 ] * 83,    'int array' )
        message1.addFieldI64Array ( [ 0 ] * 837,   'long array' )

        message1.addField ( None, 'indicator' )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'ALLNAMES' )
        self.assertEqual ( encoded, reference )


    def testEncodeAllOrdinals ( self ):
        # Construct the message
        message1 = Message ( )
        message1.addField ( True,         ordinal = 1,  type = fudgepyc.types.BOOLEAN )
        message1.addField ( False,        ordinal = 2 )
        message1.addField ( 5,            ordinal = 3,  type = fudgepyc.types.BYTE )
        message1.addField ( 5,            ordinal = 4 )
        message1.addField ( 132,          ordinal = 5,  type = fudgepyc.types.SHORT )
        message1.addField ( 132,          ordinal = 6 )
        message1.addField ( 32772,        ordinal = 7,  type = fudgepyc.types.INT )
        message1.addField ( 32772,        ordinal = 8 )
        message1.addField ( 2147483652l,  ordinal = 9,  type = fudgepyc.types.LONG )
        message1.addField ( 2147483652l,  ordinal = 10 )
        message1.addField ( 0.5,          ordinal = 11, type = fudgepyc.types.FLOAT )
        message1.addField ( 0.5,          ordinal = 12, type = fudgepyc.types.FLOAT )
        message1.addField ( 0.27362,      ordinal = 13, type = fudgepyc.types.DOUBLE )
        message1.addField ( 0.27362,      ordinal = 14 )
        message1.addField ( 'Kirk Wylie', ordinal = 15, type = fudgepyc.types.STRING )

        message1.addField ( [ 0.0 ] * 24,  ordinal = 16, type = fudgepyc.types.FLOAT_ARRAY )
        message1.addField ( [ 0.0 ] * 273, ordinal = 17, type = fudgepyc.types.DOUBLE_ARRAY )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'ALLORDINALS' )
        self.assertEqual ( encoded, reference )


    def testEncodeFixedWidths ( self ):
        rawByteSeq = self.__createIntSeq ( 512, 8 )

        # Construct the message
        message1 = Message ( )
        message1.addField4ByteArray   ( rawByteSeq [ :   4 ], 'byte[4]' )
        message1.addField8ByteArray   ( rawByteSeq [ :   8 ], 'byte[8]' )
        message1.addField16ByteArray  ( rawByteSeq [ :  16 ], 'byte[16]' )
        message1.addField20ByteArray  ( rawByteSeq [ :  20 ], 'byte[20]' )
        message1.addField32ByteArray  ( rawByteSeq [ :  32 ], 'byte[32]' )
        message1.addField64ByteArray  ( rawByteSeq [ :  64 ], 'byte[64]' )
        message1.addField128ByteArray ( rawByteSeq [ : 128 ], 'byte[128]' )
        message1.addField256ByteArray ( rawByteSeq [ : 256 ], 'byte[256]' )
        message1.addField512ByteArray ( rawByteSeq [ : 512 ], 'byte[512]' )
        message1.addFieldByteArray    ( rawByteSeq [ :  28 ], 'byte[28]' )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'FIXEDWIDTH' )
        self.assertEqual ( encoded, reference )

    def testEncodeSubMsgs ( self ):
        # Construct the top-level message and add the first submessage
        message1, submessage = Message ( ), Message ( )
        submessage.addField ( 'fibble',  name = 'bibble' )
        submessage.addField ( 'Blibble', ordinal = 827 )
        message1.addField ( submessage,  name = 'sub1' )

        # Create and add the second submessage
        submessage = Message ( )
        submessage.addField ( 9837438,  name = 'bibble9' )
        submessage.addFieldF32 ( 82.77, ordinal = 828 )
        message1.addField ( submessage, name = 'sub2', type = fudgepyc.types.MESSAGE )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'SUBMSG' )
        self.assertEqual ( encoded, reference )


    def testEncodeVariableWidths ( self ):
        # Construct the message
        message1 = Message ( )
        message1.addFieldByteArray ( [ 0 ] * 100,    '100' )
        message1.addFieldByteArray ( [ 0 ] * 1000,   '1000' )
        message1.addFieldByteArray ( [ 0 ] * 100000, '10000' )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'VARIABLEWIDTH' )
        self.assertEqual ( encoded, reference )


    def testEncodeDateTimes ( self ):
        pass # TODO Add when date/time/datetime functionality is added to Python wrapper


    def testEncodeDeepTree ( self ):
        rawByteStr = ''.join ( chr ( idx % 256 ) for idx in range ( 512 ) )
        rawShortSeq = self.__createIntSeq ( 15, 16 )
        rawIntSeq = [ 0l ] * 15
        rawLongSeq = [ 0l ] * 15
        rawFloatSeq = [ 0.0 ] * 15
        rawDoubleSeq = [ float ( i ) / 10.0 for i in self.__createIntSeq ( 15, 64 ) ]

        # Construct the message
        message1 = Message ( )
        message1.addField ( None,                'Indicator' )
        message1.addField ( True,                'Boolean' )
        message1.addField ( -128,                'Byte' )
        message1.addField ( -32768,              'Short' )
        message1.addField ( 2147483647,          'Int' )
        message1.addField ( 9223372036854775807, 'Long' )
        message1.addFieldF32 ( 1.23456,          'Float' )
        message1.addField ( 1.2345678,           'Double' )

        submessage = Message ( )
        submessage.addField4ByteArray   ( rawByteStr [ :   4 ], ordinal = 4 )
        submessage.addField8ByteArray   ( rawByteStr [ :   8 ], ordinal = 8 )
        submessage.addField16ByteArray  ( rawByteStr [ :  16 ], ordinal = 16 )
        submessage.addField20ByteArray  ( rawByteStr [ :  20 ], ordinal = 20 )
        submessage.addField32ByteArray  ( rawByteStr [ :  32 ], ordinal = 32 )
        submessage.addField64ByteArray  ( rawByteStr [ :  64 ], ordinal = 64 )
        submessage.addField128ByteArray ( rawByteStr [ : 128 ], ordinal = 128 )
        submessage.addField256ByteArray ( rawByteStr [ : 256 ], ordinal = 256 )
        submessage.addField512ByteArray ( rawByteStr [ : 512 ], ordinal = 512 )
        message1.addField ( submessage, 'ByteArrays' )

        message1.addField ( '',                  u'Empty String' )
        message1.addField ( 'This is a string.', u'String' )

        submessage = Message ( )
        subsubmessage = Message ( )
        submessage.addFieldByteArray ( [ ],                 'Byte[0]' )
        submessage.addFieldByteArray ( rawByteStr [ : 15 ], 'Byte[15]' )
        submessage.addField ( subsubmessage, 'FP Arrays' )
        submessage.addFieldI16Array  ( [] ,                 'Short[0]' )
        submessage.addFieldI16Array  ( rawShortSeq,         'Short[15]' )
        submessage.addFieldI32Array  ( [] ,                 'Int[0]' )
        submessage.addFieldI32Array  ( rawIntSeq,           'Int[15]' )
        submessage.addFieldI64Array  ( [] ,                 'Long[0]' )
        submessage.addFieldI64Array  ( rawLongSeq,          'Long[15]' )
        message1.addField ( submessage, 'Arrays' )

        subsubmessage.addFieldF32Array ( [ ],          'Float[0]' )
        subsubmessage.addFieldF32Array ( rawFloatSeq,  'Float[15]' )
        subsubmessage.addFieldF64Array ( [ ],          'Double[0]' )
        subsubmessage.addFieldF64Array ( rawDoubleSeq, 'Double[15]' )

        message1.addField ( Message ( ), 'Null Message' )

        # Encode it and compare against the test file
        encoded = Envelope ( message1 ).encode ( )
        reference = self.__loadFile ( 'DEEPERTREE' )
        self.assertEqual ( encoded, reference )


    def __loadFile ( self, name ):
        infile = open ( self.__datafiles [ name ], 'rb' )
        try:
            return infile.read ( )
        finally:
            infile.close ( )

    def __loadMessage ( self, name ):
        return Envelope.decode ( self.__loadFile ( name ) ).message ( )

    def __createIntSeq ( self, length, bits ):
        maxval = pow ( 2, bits )
        midval = maxval / 2
        return [ idx % maxval - midval for idx in range ( midval, midval + length ) ]

    def __checkField ( self, field, fudgetype, value, name, ordinal, accessor, compfunc = TestCase.assertEqual ):
        self.assertEqual ( field.type ( ), fudgetype )
        self.assertEqual ( field.name ( ), name )
        self.assertEqual ( field.ordinal ( ), ordinal )
        compfunc ( self, accessor ( field ), value )


def suite ( ):
    tests = [ 'testDecodeAllNames',
              'testDecodeAllOrdinals',
              'testDecodeFixedWidth',
              'testDecodeSubMsgs',
              'testDecodeUnknown',
              'testDecodeVariableWidths',
              'testDecodeDateTimes',
              'testDecodeDeepTree',
              'testEncodeAllNames',
              'testEncodeAllOrdinals',
              'testEncodeFixedWidths',
              'testEncodeSubMsgs',
              'testEncodeVariableWidths',
              'testEncodeDateTimes',
              'testEncodeDeepTree' ]
    return TestSuite ( map ( CodecTestCase, tests ) )
