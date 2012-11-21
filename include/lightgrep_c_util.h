/*
  liblightgrep: not the worst forensics regexp engine
  Copyright (C) 2012 Lightbox Technologies, Inc

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIGHTGREP_C_UTIL_H
#define LIGHTGREP_C_UTIL_H

#include "lightgrep_c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char* const name;
  unsigned int idx;
} LG_TRANS;

int lg_get_char_char_transformation_id(const char* const name);

int lg_get_char_byte_transformation_id(const char* const name);

int lg_get_byte_byte_transformation_id(const char* const name);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTGREP_C_UTIL_H */