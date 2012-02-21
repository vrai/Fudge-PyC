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
#ifndef INC_FUDGEPYC_CONVERTERS_H
#define INC_FUDGEPYC_CONVERTERS_H

#include "exception.h"
#include <fudge/types.h>

extern int fudgepyc_initialiseConverters ( PyObject * module );

extern int fudgepyc_convertPythonToBool ( fudge_bool * target, PyObject * source );
extern int fudgepyc_convertPythonToByte ( fudge_byte * target, PyObject * source );
extern int fudgepyc_convertPythonToI16 ( fudge_i16 * target, PyObject * source );
extern int fudgepyc_convertPythonToI32 ( fudge_i32 * target, PyObject * source );
extern int fudgepyc_convertPythonToI64 ( fudge_i64 * target, PyObject * source );
extern int fudgepyc_convertPythonToF32 ( fudge_f32 * target, PyObject * source );
extern int fudgepyc_convertPythonToF64 ( fudge_f64 * target, PyObject * source );

extern int fudgepyc_convertPythonToMsg ( FudgeMsg * target, PyObject * source );
extern int fudgepyc_convertPythonToString ( FudgeString * target, PyObject * source );

extern int fudgepyc_convertPythonToDate ( FudgeDate * target, PyObject * source );
extern int fudgepyc_convertPythonToTime ( FudgeTime * target, PyObject * source );
extern int fudgepyc_convertPythonToDateTime ( FudgeDateTime * target, PyObject * source );

extern int fudgepyc_convertPythonToDateEx ( FudgeDate * target,
                                            PyObject * yearobj,
                                            PyObject * monthobj,
                                            PyObject * dayobj );
extern int fudgepyc_convertPythonToTimeEx ( FudgeTime * target,
                                            unsigned int precision,
                                            PyObject * hourobj,
                                            PyObject * minuteobj,
                                            PyObject * secondobj,
                                            PyObject * nanoobj,
                                            PyObject * offsetobj );
extern int fudgepyc_convertPythonToDateTimeEx ( FudgeDateTime * target,
                                                unsigned int precision,
                                                PyObject * yearobj,
                                                PyObject * monthobj,
                                                PyObject * dayobj,
                                                PyObject * hourobj,
                                                PyObject * minuteobj,
                                                PyObject * secondobj,
                                                PyObject * nanoobj,
                                                PyObject * offsetobj );

extern int fudgepyc_convertPythonToByteArray ( fudge_byte * * target,
                                               fudge_i32 * size,
                                               PyObject * source );
extern int fudgepyc_convertPythonToI16Array ( fudge_i16 * * target,
                                              fudge_i32 * targetsize,
                                              PyObject * source );
extern int fudgepyc_convertPythonToI32Array ( fudge_i32 * * target,
                                              fudge_i32 * targetsize,
                                              PyObject * source );
extern int fudgepyc_convertPythonToI64Array ( fudge_i64 * * target,
                                              fudge_i32 * targetsize,
                                              PyObject * source );
extern int fudgepyc_convertPythonToF32Array ( fudge_f32 * * target,
                                              fudge_i32 * targetsize,
                                              PyObject * source );
extern int fudgepyc_convertPythonToF64Array ( fudge_f64 * * target,
                                              fudge_i32 * targetsize,
                                              PyObject * source );

extern int fudgepyc_convertPythonToFixedByteArray ( fudge_byte * target,
                                                    fudge_i32 size,
                                                    PyObject * source );

extern PyObject * fudgepyc_convertBoolToPython ( fudge_bool source );
extern PyObject * fudgepyc_convertByteToPython ( fudge_byte source );
extern PyObject * fudgepyc_convertI16ToPython ( fudge_i16 source );
extern PyObject * fudgepyc_convertI32ToPython ( fudge_i32 source );
extern PyObject * fudgepyc_convertI64ToPython ( fudge_i64 source );
extern PyObject * fudgepyc_convertF32ToPython ( fudge_f32 source );
extern PyObject * fudgepyc_convertF64ToPython ( fudge_f64 source );

extern PyObject * fudgepyc_convertStringToPython ( FudgeString source );

extern PyObject * fudgepyc_convertDateToPython ( FudgeDate * source );
extern PyObject * fudgepyc_convertTimeToPython ( FudgeTime * source );
extern PyObject * fudgepyc_convertDateTimeToPython ( FudgeDateTime * source );

extern PyObject * fudgepyc_convertDateToPythonEx ( FudgeDate * source );
extern PyObject * fudgepyc_convertTimeToPythonEx ( FudgeTime * source );
extern PyObject * fudgepyc_convertDateTimeToPythonEx ( FudgeDateTime * source );

extern PyObject * fudgepyc_convertByteArrayToPython ( const fudge_byte * bytes,
                                                      fudge_i32 numbytes );
extern PyObject * fudgepyc_convertI16ArrayToPython ( const fudge_byte * bytes,
                                                     fudge_i32 numbytes );
extern PyObject * fudgepyc_convertI32ArrayToPython ( const fudge_byte * bytes,
                                                     fudge_i32 numbytes );
extern PyObject * fudgepyc_convertI64ArrayToPython ( const fudge_byte * bytes,
                                                     fudge_i32 numbytes );
extern PyObject * fudgepyc_convertF32ArrayToPython ( const fudge_byte * bytes,
                                                     fudge_i32 numbytes );
extern PyObject * fudgepyc_convertF64ArrayToPython ( const fudge_byte * bytes,
                                                     fudge_i32 numbytes );

extern PyObject * fudgepyc_convertByteStringToPython ( const fudge_byte * bytes,
                                                       fudge_i32 numbytes );

#endif

