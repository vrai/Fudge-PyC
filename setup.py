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

import os, os.path, sys
from cStringIO import StringIO
from distutils.core import setup, Extension
from distutils.cmd import Command
from distutils.util import get_platform
from glob import glob
from unittest import TextTestRunner

def _makeExtension ( name ):
    return Extension ( name = 'fudgepyc.' + name,
                       sources = [ _srcdir + n for n in _sources [ name ] ],
                       depends = [ _srcdir + n for n in _depends [ name ] ],
                       libraries = [ 'fudgec' ] )


class TestStreamOutput ( object ):
    # Implements basic stream functionality, passing completed lines
    # to a closure that accepts a single string argument (e.g.
    # sys.stdout.write or TestCommand.announce).
    def __init__ ( self, writer ):
        self.__writer = writer
        self.__buffer = StringIO ( )

    def write ( self, output ):
        while True:
            index = output.find ( '\n' )
            if index < 0:
                self.__buffer.write ( output )
                return
            else:
                self.__buffer.write ( output [ : index ] )
                self.commit ( )
                output = output [ index + 1 : ]

    def flush ( self ): pass

    def commit ( self ):
        if self.__buffer.getvalue ( ):
            self.__writer ( self.__buffer.getvalue ( ) )
            self.__buffer.truncate ( 0 )



class TestCommand ( Command ):
    # Implements a distutils command for running Python unittests. By
    # default searches the "tests" directory for any Python file whose
    # name starts with "test_" and which contains a "suite" function.
    # The unit test output is passed to "announce" and so is visible if
    # setup.py is run with the verbose flag.
    user_options = [ ]
    description = 'run package unit tests'

    def initialize_options ( self ):
        self.__testdirs = None

    def finalize_options ( self ):
        self.__testdirs = self.__testdirs or [ 'tests' ]

    def run ( self ):
        builddir = 'lib.%s-%d.%d' % ( get_platform ( ),
                                      sys.version_info [ 0 ],
                                      sys.version_info [ 1 ] )
        buildpath = os.path.join ( 'build', builddir )
        sys.path.insert ( 0, buildpath )

        for dirname in self.__testdirs:
            for filename in glob ( os.path.join ( dirname, 'test_*.py' ) ):
                self.announce ( 'running test from %s' % filename )
                try:
                    results = self.__runfiletest ( filename )
                except ImportError, error:
                    self.warn ( str ( error ) )

    def __runfiletest ( self, filename ):
        dirname, basename = os.path.split ( filename )
        sys.path.insert ( 0, dirname )
        stream = TestStreamOutput ( self.announce )
        try:
            modname = os.path.splitext ( basename ) [ 0 ]
            module = __import__ ( modname )
            if not hasattr ( module, 'suite' ):
                raise ImportError, \
                      'Module %s does not implement "suite"' % modname
            suite = module.suite ( )
            runner = TextTestRunner ( stream = stream )
            return runner.run ( suite )

        finally:
            stream.commit ( )
            del sys.path [ sys.path.index ( dirname ) ]

_srcdir = 'src/'

_sources = { 'impl'  : [ 'converters.c',
                         'envelope.c',
                         'exception.c',
                         'field.c',
                         'message.c',
                         'implmodule.c',
                         'modulemethods.c' ],
             'types' : [ 'typesmodule.c' ] }

_depends = { 'impl' : [ 'converters.h',
                        'envelope.h',
                        'exception.h',
                        'field.h',
                        'message.h',
                        'modulemethods.h',
                        'version.h' ],
             'types' : [ ] }

setup ( name = 'Fudge-PyC',
        version = '0.1.0',
        description = 'Python wrapper around the Fudge-C library',
        packages = [ 'fudgepyc' ],
        package_dir = { 'fudgepyc' : 'fudgepyc' },
        ext_modules = [ _makeExtension ( 'impl' ),
                        _makeExtension ( 'types' ) ],
        cmdclass = { 'test' : TestCommand },
        author = 'Vrai Stacey',
        author_email = 'vrai.stacey@gmail.com',
        url = 'http://github.com/vrai/Fudge-PyC',
        license = 'APL2',
        long_description = """\
Python wrapper around the Fudge-C implementation of the Fudge message
encoding specification (see http://fudgemsg.org/ for details).

Presents an interface to the Fudge library that's similar to Fudge-C, but
which fits the Python vernacular (exceptions rather than return codes,
implicit use of unicode, etc). """ )

