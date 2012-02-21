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

"""
The timezone module of Fudge-Pyc provides a basic datetime.tzinfo
implementation for use with time and datetime fields. Any tzinfo
implementation may be used when adding a field to a message; but all time and
datetime values returned from Fudge-Pyc will use the
fudgepyc.timezone.Timezone class to provide timezone information.
"""

import datetime

class Timezone ( datetime.tzinfo ):
    """
    Timezone(quarters) -> Timezone

    Basic datetime.tzinfo implementation for use by fudgepyc.Message and
    fudgepyc.Field types. All datetime.time and datetime.datetime objects
    returned from Fudge-Pyc (that have timezone information) will use this
    class.

    To match the FudgeTime/FudgeDateTime implementation, the UTC offset must
    be specified in fifteen minute intervals. For example, UTC+1H would be
    Timezone(4) while UTC-8H would be Timezone(-32). The number of quarters
    must be an integer.

    @param quarters: number of fifteen minute intervals before (negative) or
                     after (positive) UTC
    @return: Timezone instance
    """

    def __init__ ( self, quarters ):
        self.__delta = datetime.timedelta ( minutes = quarters * 15 )

    def utcoffset ( self, dt ):
        """
        Return offset of localtime from UTC, in minutes east of UTC; if local
        time is west of UTC the return value is negative.

        @param dt: localtime as a datetime.time object
        @return: utcoffset as a datetime.timedelta object
        """
        dst = self.dst ( dt )
        if dst:
            return self.__delta + dst
        else:
            return self.__delta

    def dst ( self, dt ):
        """
        Return the DST adjustment, if any.

        @param dt: localtime as a datetime.time object
        @return: dst adjustment as a datetime.timedelta object, or None
        """
        return None

    def tzname ( self, dt ):
        """
        Return the name of the timezone, using the format: -?H+MM. For
        example, UTC+1H would be "100" while UTC-11H would be "-1100".

        @param dt: localtime as a datetime.time object
        @return: string containing timezone name
        """
        offset = self.utcoffset ( dt )
        hours = offset.seconds / 3600
        minutes = ( offset.seconds - hours * 3600 ) / 60
        return '%d%02d' % ( offset.days * 24 + hours, minutes )
