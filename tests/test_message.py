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

import datetime, unittest
import fudgepyc
import fudgepyc.types
from fudgepyc import Field, Message

class TestTimeZone ( datetime.tzinfo ):
    def utcoffset ( self, dt ): return datetime.timedelta ( minutes = -300 )
    def dst ( self, dt ): return False
    def tzname ( self, dt ): return 'EST'


class MessageTestCase ( unittest.TestCase ):
    def setUp ( self ):
        fudgepyc.init ( )

    def testFieldFunctions ( self ):
        # Set up the source data
        rawBytesSeq = [ -0x00, -0x11, -0x23, -0x34, -0x45, -0x56, -0x67, -0x77, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 ]
        rawShortsSeq = ( -32767, 32767, 0, 1, -1, 100, -100, 0, 16385 )
        rawIntsSeq = [ 2147483647, 0, -2147483647, 0 ]
        rawLongsSeq = ( -9223372036854775807l, 0, 9223372036854775807l, -1, 2, -3, 5, -8, 13, -21, 34, -55 )
        rawFloatsSeq = [ 0.0, 2147483647.0, 214748364.7, 21474836.47, 2147483.647, 2.147483647, 21.47483647, 214.7483647 ]
        rawDoublesSeq = ( 9223372036854775807.0, 0.0, 0.0000000123456, 1234560000000.0, -9223372036854775807.0 )
        rawDate = datetime.date ( 1962, 2, 20 )
        rawTime = datetime.time ( 11, 41, 19, 123456, TestTimeZone ( ) )
        rawDateTime = datetime.datetime ( 2012, 2, 19, 11, 43, 16 )

        # Will need a variety of large byte strings and arrays - bit of
        # hackery required to support Fudge's signed bytes and Python's
        # unsigned characters
        fixedByteArrays = ( ( 1, 4,   fudgepyc.types.BYTE_ARRAY_4 ),
                            ( 2, 8,   fudgepyc.types.BYTE_ARRAY_8 ),
                            ( 3, 16,  fudgepyc.types.BYTE_ARRAY_16 ),
                            ( 4, 20,  fudgepyc.types.BYTE_ARRAY_20 ),
                            ( 5, 32,  fudgepyc.types.BYTE_ARRAY_32 ),
                            ( 6, 64,  fudgepyc.types.BYTE_ARRAY_64 ),
                            ( 7, 128, fudgepyc.types.BYTE_ARRAY_128 ),
                            ( 8, 256, fudgepyc.types.BYTE_ARRAY_256 ),
                            ( 9, 512, fudgepyc.types.BYTE_ARRAY_512 ) )

        largeByteArrays = dict ( ( sz, self.__generateByteArray ( sz ) ) for _, sz, _ in fixedByteArrays )
        largeByteStrings = dict ( ( sz, ''.join ( chr ( i % 256 ) for i in ar ) ) for sz, ar in largeByteArrays.iteritems ( ) )

        # Construct an empty message
        message1 = Message ( )
        self.assertEqual ( len ( message1 ), 0 )

        # Test some failure cases
        self.assertRaises ( fudgepyc.Exception, message1.getFieldAtIndex, 0 )
        self.assertEqual ( message1.getFieldByOrdinal ( 0 ), None )
        self.assertEqual ( message1.getFieldByName ( 'Field Name' ), None )

        self.assertRaises ( fudgepyc.Exception,
                            message1.addFieldBool,
                            True,
                            "This message field name is too long. The limit for message field names is 255 characters; "
                            "this is because the name's length must be stored in a single leading byte (a bit like a "
                            "length limited version Pascal). An exception should be thrown containing the "
                            "FUDGE_NAME_TOO_LONG code." )

        # Add indicator, boolean, integer and float fields
        message1.addFieldIndicator ( )
        message1.addFieldBool ( True )
        message1.addFieldByte ( 127 )
        message1.addFieldI16 ( 32767 )
        message1.addFieldI32 ( -2147483647 )
        message1.addFieldI64 ( 9223372036854775807l )
        message1.addFieldF32 ( 2147483647.0 )
        message1.addFieldF64 ( -9223372036854775807.0 )

        self.assertEqual ( len ( message1 ), 8 )

        # Add date, time and datetime fields
        message1.addFieldDate ( rawDate )
        message1.addFieldTime ( rawTime )
        message1.addField ( rawDateTime )

        self.assertEqual ( len ( message1 ), 11 )

        # Add fixed width byte arrays in a sub message
        message2 = Message ( )
        for ordinal, size, _ in fixedByteArrays:
            getattr ( message2, 'addField%dByteArray' % size ) ( largeByteStrings [ size ], ordinal = ordinal )

        self.assertEqual ( len ( message2 ), 9 )
        message1.addFieldMsg ( message2, name = 'Byte Array SubMessage' )

        # Add empty and populate and unicode strings
        message1.addFieldString ( '', name = 'Empty string' )
        message1.addFieldString ( 'This is a string', name = 'String' )
        message1.addFieldString ( u'This is unicode', name = 'Unicode' )

        self.assertEqual ( len ( message1 ), 15 )

        # Add empty and populated arrays in a sub message
        message2 = Message ( )
        self.assertEqual ( len ( message2 ), 0 )
        message2.addFieldByteArray ( [ ], name = 'No Bytes' )
        message2.addFieldByteArray ( rawBytesSeq, name = 'Bytes' )
        message2.addFieldI16Array ( rawShortsSeq, name = 'Shorts' )
        message2.addFieldI32Array ( rawIntsSeq, name = 'Ints' )
        message2.addFieldI64Array ( rawLongsSeq, name = 'Longs' )
        message2.addFieldF32Array ( rawFloatsSeq, name = 'Floats' )
        message2.addFieldF64Array ( rawDoublesSeq, name = 'Doubles' )

        self.assertEqual ( len ( message2 ), 7 )
        message1.addFieldMsg ( message2, name = 'Array SubMessage' )

        # Add an empty sub message
        message2 = Message ( )
        message1.addFieldMsg ( message2, name = 'Empty SubMessage' )

        self.assertEqual ( len ( message1 ), 17 )

        # Retrieve the top-level fields and check their contents
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 17 )
        self.assert_ ( all ( isinstance ( fld, Field ) for fld in fields ) )

        self.assertEqual ( fields [ 0 ].type ( ), fudgepyc.types.INDICATOR )
        self.assertEqual ( fields [ 0 ].value ( ), None )
        self.assertEqual ( fields [ 1 ].type ( ), fudgepyc.types.BOOLEAN )
        self.assertEqual ( fields [ 1 ].getBool ( ), True )
        self.assertEqual ( fields [ 1 ].value ( ), True )
        self.assertEqual ( fields [ 2 ].type ( ), fudgepyc.types.BYTE )
        self.assertEqual ( fields [ 2 ].getByte ( ), 127 )
        self.assertEqual ( fields [ 2 ].value ( ), 127 )
        self.assertEqual ( fields [ 3 ].type ( ), fudgepyc.types.SHORT )
        self.assertEqual ( fields [ 3 ].getInt16 ( ), 32767 )
        self.assertEqual ( fields [ 3 ].value ( ), 32767 )
        self.assertEqual ( fields [ 4 ].type ( ), fudgepyc.types.INT )
        self.assertEqual ( fields [ 4 ].getInt32 ( ), -2147483647 )
        self.assertEqual ( fields [ 4 ].value ( ), -2147483647 )
        self.assertEqual ( fields [ 5 ].type ( ), fudgepyc.types.LONG )
        self.assertEqual ( fields [ 5 ].getInt64 ( ), 9223372036854775807l )
        self.assertEqual ( fields [ 5 ].value ( ), 9223372036854775807l )
        self.assertEqual ( fields [ 6 ].type ( ), fudgepyc.types.FLOAT )
        self.assertAlmostEqual ( fields [ 6 ].getFloat32 ( ), 2147483647.0, -1 )
        self.assertAlmostEqual ( fields [ 6 ].value ( ), 2147483647.0, -1 )
        self.assertEqual ( fields [ 7 ].type ( ), fudgepyc.types.DOUBLE )
        self.assertAlmostEqual ( fields [ 7 ].getFloat64 ( ), -9223372036854775807.0, -1 )
        self.assertAlmostEqual ( fields [ 7 ].value ( ), -9223372036854775807.0, -1 )
        self.assertEqual ( fields [ 8 ].getDate ( ), rawDate )
        self.assertEqual ( fields [ 8 ].value ( ), rawDate )
        self.assertEqual ( fields [ 9 ].getTime ( ), rawTime )
        self.assertEqual ( fields [ 9 ].value ( ), rawTime )
        self.assert_ ( isinstance ( fields [ 9 ].value ( ).tzinfo, fudgepyc.timezone.Timezone ) )
        self.assertEqual ( fields [ 10 ].value ( ), rawDateTime )
        self.assertEqual ( fields [ 10 ].getDateTime ( ), rawDateTime )
        self.assertEqual ( fields [ 10 ].value ( ).tzinfo, None )
        self.assertEqual ( fields [ 11 ].type ( ), fudgepyc.types.MESSAGE )
        self.assert_ ( isinstance ( fields [ 11 ].getMessage ( ), Message ) )
        self.assert_ ( isinstance ( fields [ 11 ].value ( ), Message ) )
        self.assertEqual ( fields [ 12 ].type ( ), fudgepyc.types.STRING )
        self.assertEqual ( fields [ 12 ].getString ( ), '' )
        self.assertEqual ( fields [ 12 ].value ( ), '' )
        self.assertEqual ( fields [ 13 ].type ( ), fudgepyc.types.STRING )
        self.assertEqual ( fields [ 13 ].getString ( ), 'This is a string' )
        self.assertEqual ( fields [ 13 ].value ( ), 'This is a string' )
        self.assertEqual ( fields [ 14 ].type ( ), fudgepyc.types.STRING )
        self.assertEqual ( fields [ 14 ].getString ( ), 'This is unicode' )
        self.assertEqual ( fields [ 14 ].value ( ), 'This is unicode' )
        self.assertEqual ( fields [ 15 ].type ( ), fudgepyc.types.MESSAGE )
        self.assert_ ( isinstance ( fields [ 15 ].getMessage ( ), Message ) )
        self.assert_ ( isinstance ( fields [ 15 ].value ( ), Message ) )
        self.assertEqual ( fields [ 16 ].type ( ), fudgepyc.types.MESSAGE )
        self.assert_ ( isinstance ( fields [ 16 ].getMessage ( ), Message ) )
        self.assert_ ( isinstance ( fields [ 16 ].value ( ), Message ) )

        field1 = message1.getFieldByName ( 'Empty string' )
        self.assertEqual ( field1.type ( ), fudgepyc.types.STRING )
        self.assertEqual ( field1.value ( ), u'' )
        field1 = message1 [ 'Empty SubMessage' ]
        self.assertEqual ( field1.type ( ), fudgepyc.types.MESSAGE )
        self.assertEqual ( len ( field1.value ( ) ), 0 )

        self.assertEqual ( message1.getFieldByName ( '' ), None )
        self.assertEqual ( message1.getFieldByName ( 'Bytes' ), None )
        self.assertRaises ( LookupError, message1.__getitem__, 'null string' )

        self.assertEqual ( len ( fields [ 0 ] ), 0 )
        self.assertEqual ( len ( fields [ 2 ] ), 1 )
        self.assertEqual ( len ( fields [ 15 ] ), 7 )

        self.assertEqual ( fields [ 2 ].ordinal ( ), None )
        self.assertEqual ( fields [ 2 ].name ( ), None )
        self.assertEqual ( fields [ 13 ].ordinal ( ), None )
        self.assertEqual ( fields [ 13 ].name ( ), 'String' )

        # Check the fixed array message
        message2 = fields [ 11 ].getMessage ( )
        self.assertEqual ( len ( message2 ), 9 )
        fields = message2.getFields ( )

        for idx in range ( len ( message2 ) ):
            ordinal, size, fudgetype = fixedByteArrays [ idx ]
            self.assertEqual ( fields [ idx ].type ( ), fudgetype )
            self.assertEqual ( fields [ idx ].numbytes ( ), size )
            self.assertEqual ( len ( fields [ idx ] ), size )
            self.assertEqual ( fields [ idx ].bytes ( ), largeByteStrings [ size ] )
            self.assertEqual ( fields [ idx ].value ( ), largeByteStrings [ size ] )
            self.assertEqual ( fields [ idx ].getByteArray ( ), largeByteArrays [ size ] )
            self.assertEqual ( fields [ idx ].name ( ), None )
            self.assertEqual ( fields [ idx ].ordinal ( ), ordinal )

        field1 = message2.getFieldByOrdinal ( 1 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.BYTE_ARRAY_4 )
        field1 = message2.getFieldByOrdinal ( 4 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.BYTE_ARRAY_20 )

        self.assertEqual ( message2.getFieldByOrdinal ( 0 ), None )
        self.assertRaises ( LookupError, message2.__getitem__, 10 )
        self.assertRaises ( OverflowError, message2.__getitem__, -1 )

        # Check the variable array message
        field1 = message1.getFieldAtIndex ( 15 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.MESSAGE )
        self.assertEqual ( len ( field1 ), 7 )
        fields = field1.value ( ).getFields ( )
        self.assertEqual ( len ( fields ), 7 )

        self.assertEqual ( fields [ 0 ].type ( ), fudgepyc.types.BYTE_ARRAY )
        self.assertEqual ( fields [ 0 ].getByteArray ( ), [ ] )
        self.assertEqual ( fields [ 0 ].value ( ), '' )
        self.assertEqual ( fields [ 1 ].type ( ), fudgepyc.types.BYTE_ARRAY )
        self.assertEqual ( fields [ 1 ].getByteArray ( ), rawBytesSeq )
        self.assertEqual ( fields [ 1 ].value ( ), ''.join ( chr ( i % 256 ) for i in rawBytesSeq ) )
        self.assertEqual ( fields [ 2 ].type ( ), fudgepyc.types.SHORT_ARRAY )
        self.assertEqual ( fields [ 2 ].getInt16Array ( ), list ( rawShortsSeq ) )
        self.assertEqual ( fields [ 2 ].value ( ), list ( rawShortsSeq ) )
        self.assertEqual ( fields [ 3 ].type ( ), fudgepyc.types.INT_ARRAY )
        self.assertEqual ( fields [ 3 ].getInt32Array ( ), rawIntsSeq )
        self.assertEqual ( fields [ 3 ].value ( ), rawIntsSeq )
        self.assertEqual ( fields [ 4 ].type ( ), fudgepyc.types.LONG_ARRAY )
        self.assertEqual ( fields [ 4 ].getInt64Array ( ), list ( rawLongsSeq ) )
        self.assertEqual ( fields [ 4 ].value ( ), list ( rawLongsSeq ) )
        self.assertEqual ( fields [ 5 ].type ( ), fudgepyc.types.FLOAT_ARRAY )
        self.__compareFloatArray ( fields [ 5 ].getFloat32Array ( ), rawFloatsSeq, 8  )
        self.__compareFloatArray ( fields [ 5 ].value ( ), rawFloatsSeq, 8  )
        self.assertEqual ( fields [ 6 ].type ( ), fudgepyc.types.DOUBLE_ARRAY )
        self.__compareFloatArray ( fields [ 6 ].getFloat64Array ( ), rawDoublesSeq, 12 )
        self.__compareFloatArray ( fields [ 6 ].value ( ), rawDoublesSeq, 12 )

        message2 = message1.getFieldAtIndex ( 15 ).value ( )
        field1 = message2.getFieldByName ( 'Bytes' )
        self.assertEqual ( field1.type ( ), fudgepyc.types.BYTE_ARRAY )
        self.assertRaises ( fudgepyc.Exception, field1.getInt16Array )
        field1 = message2 [ 'Floats' ]
        self.assertEqual ( field1.type ( ), fudgepyc.types.FLOAT_ARRAY )
        self.assertRaises ( fudgepyc.Exception, field1.getByteArray )

        self.assertEqual ( message2.getFieldByName ( 'Empty SubMessage' ), None )
        self.assertEqual ( message2.getFieldByOrdinal ( 1 ), None )
        self.assertRaises ( LookupError, message2.__getitem__, 'Empty SubMessage' )
        self.assertRaises ( LookupError, message2.__getitem__, 1 )

        # Check the empty message
        field1 = message1.getFieldAtIndex ( 16 )
        self.assertEqual ( field1.type ( ), fudgepyc.types.MESSAGE )
        self.assertEqual ( len ( field1 ), 0 )
        self.assertEqual ( field1.name ( ), u'Empty SubMessage' )
        self.assertEqual ( field1.ordinal ( ), None )
        message2 = field1.value ( )
        self.assertEqual ( len ( message2 ), 0 )

    def testIntegerFieldDowncasting ( self ):
        # Create the test message
        message1 = Message ( )
        message1.addFieldByte ( -127 )
        message1.addFieldI16 ( -127 )
        message1.addFieldI16 ( 32767 )
        message1.addFieldI32 ( -127 )
        message1.addFieldI32 ( 32767 )
        message1.addFieldI32 ( -2147483647 )
        message1.addFieldI64 ( -127 )
        message1.addFieldI64 ( 32767 )
        message1.addFieldI64 ( -2147483647 )
        message1.addFieldI64 ( 2147483648l )

        # Check that the fields are using the smallest integer type possible
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 10 )

        for idx in ( 0, 1, 3, 6 ):
            self.assertEqual ( fields [ idx ].type ( ), fudgepyc.types.BYTE )
            self.assertEqual ( fields [ idx ].getByte ( ), -127 )
        for idx in ( 2, 4, 7 ):
            self.assertEqual ( fields [ idx ].type ( ), fudgepyc.types.SHORT )
            self.assertEqual ( fields [ idx ].getInt16 ( ), 32767 )
        for idx in ( 5, 8 ):
            self.assertEqual ( fields [ idx ].type ( ), fudgepyc.types.INT )
            self.assertEqual ( fields [ idx ].getInt32 ( ), -2147483647 )
        self.assertEqual ( fields [ 9 ].type ( ), fudgepyc.types.LONG )
        self.assertEqual ( fields [ 9 ].getInt64 ( ), 2147483648l )

    def testFieldCoercion ( self ):
        # Create the test message
        message1 = Message ( )
        message1.addFieldIndicator ( 'Indicator' )
        message1.addFieldBool ( True, 'True Bool' )
        message1.addFieldBool ( False, 'False Bool' )
        message1.addFieldByte ( 0, 'Zero Byte' )
        message1.addFieldByte ( -42, 'Non-zero Byte' )
        message1.addFieldI16 ( 256, 'Non-zero Short' )
        message1.addFieldI32 ( -40000, 'Non-zero Int' )
        message1.addFieldI64 ( 10000000000l, 'Non-zero Long' )
        message1.addFieldF32 ( 0.0, 'Zero Float' )
        message1.addFieldF32 ( -1.234, 'Non-zero Float' )
        message1.addFieldF64 ( 0.0, 'Zero Double' )
        message1.addFieldF64 ( 123.4567, 'Non-zero Double' )
        message1.addFieldString ( 'This is a string', 'String' )

        # Retrieve test fields and check coercion
        fields = message1.getFields ( )
        self.assertEqual ( len ( fields ), 13 )

        self.assertRaises( fudgepyc.Exception, fields [ 0 ].getAsBool )     # Indicator
        self.assertEqual ( fields [ 1 ].getAsBool ( ), True )               # True bool
        self.assertEqual ( fields [ 2 ].getAsBool ( ), False )              # False bool
        self.assertEqual ( fields [ 3 ].getAsBool ( ), False )              # Zero byte
        self.assertEqual ( fields [ 4 ].getAsBool ( ), True )               # Non-zero byte
        self.assertEqual ( fields [ 5 ].getAsBool ( ), True )               # Non-zero short
        self.assertEqual ( fields [ 6 ].getAsBool ( ), True )               # Non-zero int
        self.assertEqual ( fields [ 7 ].getAsBool ( ), True )               # Non-zero long
        for idx in range ( 8, len ( fields ) ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsBool )

        self.assertRaises( fudgepyc.Exception, fields [ 0 ].getAsByte )     # Indicator
        self.assertEqual ( fields [ 1 ].getAsByte ( ), 1 )                  # True bool
        self.assertEqual ( fields [ 2 ].getAsByte ( ), 0 )                  # False bool
        self.assertEqual ( fields [ 3 ].getAsByte ( ), 0 )                  # Zero byte
        self.assertEqual ( fields [ 4 ].getAsByte ( ), -42 )                # Non-zero byte
        for idx in range ( 5, len ( fields ) ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsByte )

        self.assertRaises( fudgepyc.Exception, fields [ 0 ].getAsInt16 )    # Indicator
        self.assertEqual ( fields [ 1 ].getAsInt16 ( ), 1 )                 # True bool
        self.assertEqual ( fields [ 2 ].getAsInt16 ( ), 0 )                 # False bool
        self.assertEqual ( fields [ 3 ].getAsInt16 ( ), 0 )                 # Zero byte
        self.assertEqual ( fields [ 4 ].getAsInt16 ( ), -42 )               # Non-zero byte
        self.assertEqual ( fields [ 5 ].getAsInt16 ( ), 256 )               # Non-zero short
        for idx in range ( 6, len ( fields ) ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsInt16 )

        self.assertRaises( fudgepyc.Exception, fields [ 0 ].getAsInt32 )    # Indicator
        self.assertEqual ( fields [ 1 ].getAsInt32 ( ), 1 )                 # True bool
        self.assertEqual ( fields [ 2 ].getAsInt32 ( ), 0 )                 # False bool
        self.assertEqual ( fields [ 3 ].getAsInt32 ( ), 0 )                 # Zero byte
        self.assertEqual ( fields [ 4 ].getAsInt32 ( ), -42 )               # Non-zero byte
        self.assertEqual ( fields [ 5 ].getAsInt32 ( ), 256 )               # Non-zero short
        self.assertEqual ( fields [ 6 ].getAsInt32 ( ), -40000 )            # Non-zero int
        for idx in range ( 7, len ( fields ) ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsInt32 )

        self.assertRaises( fudgepyc.Exception, fields [ 0 ].getAsInt64 )    # Indicator
        self.assertEqual ( fields [ 1 ].getAsInt64 ( ), 1 )                 # True bool
        self.assertEqual ( fields [ 2 ].getAsInt64 ( ), 0 )                 # False bool
        self.assertEqual ( fields [ 3 ].getAsInt64 ( ), 0 )                 # Zero byte
        self.assertEqual ( fields [ 4 ].getAsInt64 ( ), -42 )               # Non-zero byte
        self.assertEqual ( fields [ 5 ].getAsInt64 ( ), 256 )               # Non-zero short
        self.assertEqual ( fields [ 6 ].getAsInt64 ( ), -40000 )            # Non-zero int
        self.assertEqual ( fields [ 7 ].getAsInt64 ( ), 10000000000l )      # Non-zero long
        for idx in range ( 8, len ( fields ) ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsInt64 )

        for idx in range ( 0, 3 ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsFloat32 )
        self.assertAlmostEqual ( fields [  3 ].getAsFloat32 ( ), 0.0, 1 )           # Zero byte
        self.assertAlmostEqual ( fields [  4 ].getAsFloat32 ( ), -42.0, 1 )         # Non-zero byte
        self.assertAlmostEqual ( fields [  5 ].getAsFloat32 ( ), 256.0, 1 )         # Non-zero short
        self.assertAlmostEqual ( fields [  6 ].getAsFloat32 ( ), -40000.0, 1 )      # Non-zero int
        self.assertAlmostEqual ( fields [  7 ].getAsFloat32 ( ), 10000000000.0, 1 ) # Non-zero long
        self.assertAlmostEqual ( fields [  8 ].getAsFloat32 ( ), 0.0, 1 )           # Zero float
        self.assertAlmostEqual ( fields [  9 ].getAsFloat32 ( ), -1.234, 3 )        # Non-zero float
        self.assertAlmostEqual ( fields [ 10 ].getAsFloat32 ( ), 0.0, 1 )           # Zero double
        self.assertAlmostEqual ( fields [ 11 ].getAsFloat32 ( ), 123.4567, 4 )      # Non-zero double
        self.assertRaises( fudgepyc.Exception, fields [ 12 ].getAsFloat32 )

        for idx in range ( 0, 3 ):
            self.assertRaises( fudgepyc.Exception, fields [ idx ].getAsFloat64 )
        self.assertAlmostEqual ( fields [  3 ].getAsFloat64 ( ), 0.0, 1 )           # Zero byte
        self.assertAlmostEqual ( fields [  4 ].getAsFloat64 ( ), -42.0, 1 )         # Non-zero byte
        self.assertAlmostEqual ( fields [  5 ].getAsFloat64 ( ), 256.0, 1 )         # Non-zero short
        self.assertAlmostEqual ( fields [  6 ].getAsFloat64 ( ), -40000.0, 1 )      # Non-zero int
        self.assertAlmostEqual ( fields [  7 ].getAsFloat64 ( ), 10000000000.0, 1 ) # Non-zero long
        self.assertAlmostEqual ( fields [  8 ].getAsFloat64 ( ), 0.0, 1 )           # Zero float
        self.assertAlmostEqual ( fields [  9 ].getAsFloat64 ( ), -1.234, 3 )        # Non-zero float
        self.assertAlmostEqual ( fields [ 10 ].getAsFloat64 ( ), 0.0, 1 )           # Zero double
        self.assertAlmostEqual ( fields [ 11 ].getAsFloat64 ( ), 123.4567, 4 )      # Non-zero double
        self.assertRaises( fudgepyc.Exception, fields [ 12 ].getAsFloat64 )

    def testDateTimeFields ( self ):
        message1 = Message ( )

        # Populate message using raw datetime methods
        message1.addFieldRawDate (               name = 'nodate' )
        message1.addFieldRawDate ( 1689,         name = 'year' )
        message1.addFieldRawDate ( 1689, 12,     name = 'month' )
        message1.addFieldRawDate ( 1689, 12, 16, name = 'day' )

        message1.addFieldRawTime ( fudgepyc.types.PRECISION_MILLENNIUM,                       name = 'notime' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_HOUR,       18,                   name = 'hour' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_MINUTE,     18, 4,                name = 'minute' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_SECOND,     18, 4, 40,            name = 'second' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_NANOSECOND, 18, 4, 40, 237363123, name = 'nanosecond' )

        message1.addFieldRawTime ( fudgepyc.types.PRECISION_MILLENNIUM,                       offset = 0,   name = 'notime_utc' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_HOUR,       18,                   offset = 8,   name = 'hour_+2h' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_MINUTE,     18, 4,                offset = -4,  name = 'minute_-1h' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_SECOND,     18, 4, 40,            offset = -20, name = 'second_-5h' )
        message1.addFieldRawTime ( fudgepyc.types.PRECISION_NANOSECOND, 18, 4, 40, 237363123, offset = 0,   name = 'nanosecond_utc' )

        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_CENTURY,                                        name = 'nodatetime' )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_YEAR,       1689,                               name = 'dt_year' )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_MONTH,      1689, 12,                           name = 'dt_month',      offset = -6 )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_DAY,        1689, 12, 16,                       name = 'dt_day' )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_HOUR,       1689, 12, 16, 18,                   name = 'dt_hour' )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_MINUTE,     1689, 12, 16, 18, 4,                name = 'dt_minute',     offset = 0 )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_SECOND,     1689, 12, 16, 18, 4, 40,            name = 'dt_second' )
        message1.addFieldRawDateTime ( fudgepyc.types.PRECISION_NANOSECOND, 1689, 12, 16, 18, 4, 40, 237363123, name = 'dt_nanosecond', offset = 1 )

        # Access the fields as raw tuples
        self.assertEqual ( message1 [ 'nodate' ].getRawDate ( ), ( 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'year' ].getRawDate ( ),   ( 1689, 0, 0 ) )
        self.assertEqual ( message1 [ 'month' ].getRawDate ( ),  ( 1689, 12, 0 ) )
        self.assertEqual ( message1 [ 'day' ].getRawDate ( ),    ( 1689, 12, 16 ) )

        self.assertEqual ( message1 [ 'notime' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_MILLENNIUM, 0, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'hour' ].getRawTime ( ),       ( fudgepyc.types.PRECISION_HOUR,       18, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'minute' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_MINUTE,     18, 4, 0, 0, None ) )
        self.assertEqual ( message1 [ 'second' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_SECOND,     18, 4, 40, 0, None ) )
        self.assertEqual ( message1 [ 'nanosecond' ].getRawTime ( ), ( fudgepyc.types.PRECISION_NANOSECOND, 18, 4, 40, 237363123, None ) )

        self.assertEqual ( message1 [ 'notime_utc' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_MILLENNIUM, 0, 0, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'hour_+2h' ].getRawTime ( ),       ( fudgepyc.types.PRECISION_HOUR,       18, 0, 0, 0, 8 ) )
        self.assertEqual ( message1 [ 'minute_-1h' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_MINUTE,     18, 4, 0, 0, -4 ) )
        self.assertEqual ( message1 [ 'second_-5h' ].getRawTime ( ),     ( fudgepyc.types.PRECISION_SECOND,     18, 4, 40, 0, -20 ) )
        self.assertEqual ( message1 [ 'nanosecond_utc' ].getRawTime ( ), ( fudgepyc.types.PRECISION_NANOSECOND, 18, 4, 40, 237363123, 0 ) )

        self.assertEqual ( message1 [ 'nodatetime' ].getRawDateTime ( ),    ( fudgepyc.types.PRECISION_CENTURY,    0, 0, 0, 0, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'dt_year' ].getRawDateTime ( ),       ( fudgepyc.types.PRECISION_YEAR,       1689, 0, 0, 0, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'dt_month' ].getRawDateTime ( ),      ( fudgepyc.types.PRECISION_MONTH,      1689, 12, 0, 0, 0, 0, 0, -6 ) )
        self.assertEqual ( message1 [ 'dt_day' ].getRawDateTime ( ),        ( fudgepyc.types.PRECISION_DAY,        1689, 12, 16, 0, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'dt_hour' ].getRawDateTime ( ),       ( fudgepyc.types.PRECISION_HOUR,       1689, 12, 16, 18, 0, 0, 0, None ) )
        self.assertEqual ( message1 [ 'dt_minute' ].getRawDateTime ( ),     ( fudgepyc.types.PRECISION_MINUTE,     1689, 12, 16, 18, 4, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'dt_second' ].getRawDateTime ( ),     ( fudgepyc.types.PRECISION_SECOND,     1689, 12, 16, 18, 4, 40, 0, None ) )
        self.assertEqual ( message1 [ 'dt_nanosecond' ].getRawDateTime ( ), ( fudgepyc.types.PRECISION_NANOSECOND, 1689, 12, 16, 18, 4, 40, 237363123, 1 ) )

        # Access the fields as datetime objects
        self.assertEqual ( message1 [ 'nodate' ].value ( ), datetime.date ( 1, 1, 1 ) )
        self.assertEqual ( message1 [ 'year' ].value ( ),   datetime.date ( 1689, 1, 1 ) )
        self.assertEqual ( message1 [ 'month' ].value ( ),  datetime.date ( 1689, 12, 1 ) )
        self.assertEqual ( message1 [ 'day' ].value ( ),    datetime.date ( 1689, 12, 16 ) )

        self.assertEqual ( message1 [ 'notime' ].value ( ),     datetime.time ( 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'hour' ].value ( ),       datetime.time ( 18, 0, 0 ) )
        self.assertEqual ( message1 [ 'minute' ].value ( ),     datetime.time ( 18, 4, 0 ) )
        self.assertEqual ( message1 [ 'second' ].value ( ),     datetime.time ( 18, 4, 40 ) )
        self.assertEqual ( message1 [ 'nanosecond' ].value ( ), datetime.time ( 18, 4, 40, 237363 ) )

        self.assertEqual ( message1 [ 'notime_utc' ].value ( ),     datetime.time ( 0, 0, 0, 0, fudgepyc.timezone.Timezone ( 0 ) ) )
        self.assertEqual ( message1 [ 'hour_+2h' ].value ( ),       datetime.time ( 18, 0, 0, 0, fudgepyc.timezone.Timezone ( 8 ) ) )
        self.assertEqual ( message1 [ 'minute_-1h' ].value ( ),     datetime.time ( 18, 4, 0, 0, fudgepyc.timezone.Timezone ( -4 ) ) )
        self.assertEqual ( message1 [ 'second_-5h' ].value ( ),     datetime.time ( 18, 4, 40, 0, fudgepyc.timezone.Timezone ( -20 ) ) )
        self.assertEqual ( message1 [ 'nanosecond_utc' ].value ( ), datetime.time ( 18, 4, 40, 237363, fudgepyc.timezone.Timezone ( 0 ) ) )

        self.assertEqual ( message1 [ 'nodatetime' ].value ( ),    datetime.datetime ( 1, 1, 1, 0, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'dt_year' ].value ( ),       datetime.datetime ( 1689, 1, 1, 0, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'dt_month' ].value ( ),      datetime.datetime ( 1689, 12, 1, 0, 0, 0, 0, fudgepyc.timezone.Timezone ( -6 ) ) )
        self.assertEqual ( message1 [ 'dt_day' ].value ( ),        datetime.datetime ( 1689, 12, 16, 0, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'dt_hour' ].value ( ),       datetime.datetime ( 1689, 12, 16, 18, 0, 0, 0 ) )
        self.assertEqual ( message1 [ 'dt_minute' ].value ( ),     datetime.datetime ( 1689, 12, 16, 18, 4, 0, 0, fudgepyc.timezone.Timezone ( 0 ) ) )
        self.assertEqual ( message1 [ 'dt_second' ].value ( ),     datetime.datetime ( 1689, 12, 16, 18, 4, 40, 0 ) )
        self.assertEqual ( message1 [ 'dt_nanosecond' ].value ( ), datetime.datetime ( 1689, 12, 16, 18, 4, 40, 237363, fudgepyc.timezone.Timezone ( 1 ) ) )


    def testIntegerFields ( self ):
        message1 = Message ( )

        self.assertRaises ( OverflowError, message1.addFieldByte, -129 )
        self.assertRaises ( OverflowError, message1.addFieldByte, 128 )
        self.assertRaises ( OverflowError, message1.addFieldI16, -32769 )
        self.assertRaises ( OverflowError, message1.addFieldI16, 32768 )
        self.assertRaises ( OverflowError, message1.addFieldI32, -2147483649 )
        self.assertRaises ( OverflowError, message1.addFieldI32, 2147483648 )
        self.assertRaises ( OverflowError, message1.addFieldI64, 9223372036854775808 )
        self.assertRaises ( OverflowError, message1.addFieldI64, -9223372036854775809 )

        self.assertRaises ( ValueError, message1.addFieldByte, None )
        self.assertRaises ( ValueError, message1.addFieldI16,  '' )
        self.assertRaises ( ValueError, message1.addFieldI32,  u'' )
        self.assertRaises ( ValueError, message1.addFieldI64,  lambda x: x )

        message1.addFieldByte ( 127,                'byte_fromint' )
        message1.addFieldByte ( -128.4,             'byte_fromfloat' )
        message1.addFieldI16 ( 32767,               'short_fromint' )
        message1.addFieldI16 ( -32768.1,            'short_fromfloat' )
        message1.addFieldI32 ( 2147483647,          'int_fromint' )
        message1.addFieldI32 ( -2147483648.9,       'int_fromfloat' )
        message1.addFieldI64 ( 9223372036854775807, 'long_fromint' )
        message1.addFieldI64 ( 9.22e+18,            'long_fromfloat' )

        self.assertEquals ( message1 [ 'byte_fromint' ].value ( ),    127 )
        self.assertEquals ( message1 [ 'byte_fromfloat' ].value ( ),  -128 )
        self.assertEquals ( message1 [ 'short_fromint' ].value ( ),   32767 )
        self.assertEquals ( message1 [ 'short_fromfloat' ].value ( ), -32768 )
        self.assertEquals ( message1 [ 'int_fromint' ].value ( ),     2147483647 )
        self.assertEquals ( message1 [ 'int_fromfloat' ].value ( ),   -2147483648 )
        self.assertEquals ( message1 [ 'long_fromint' ].value ( ),    9223372036854775807 )
        self.assertEquals ( message1 [ 'long_fromfloat' ].value ( ),  9220000000000000000 )


    def __generateByteArray ( self, size ):
        return [ idx % 256 - 128 for idx in range ( 0, size ) ]

    def __compareFloatArray ( self, x, y, sig ):
        self.assertEqual ( len ( x ), len ( y ) )
        for xe, ye in zip ( x, y ):
            self.assertEqual ( str ( xe ) [ : sig ], str ( ye ) [ : sig ] )


def suite ( ):
    tests = [ 'testFieldFunctions',
              'testIntegerFieldDowncasting',
              'testFieldCoercion',
              'testDateTimeFields',
              'testIntegerFields' ]
    return unittest.TestSuite ( map ( MessageTestCase, tests ) )
