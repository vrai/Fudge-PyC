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
#ifndef INC_FUDGEPYC_FIELD_H
#define INC_FUDGEPYC_FIELD_H

#include "message.h"

typedef struct
{
    PyObject_HEAD
    FudgeField field;
    Message * parent;
} Field;

extern PyTypeObject FieldType;

extern PyObject * Field_create ( FudgeField field, Message * parent );

extern int Field_modinit ( PyObject * module );

#endif

