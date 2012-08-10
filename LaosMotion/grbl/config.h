/**
 * config.h
 * Configuration for the GRBL motion setpoint generator
 *
 * Copyright (c) 2011 Peter Brier
 *
 *   This file is part of the LaOS project (see:  http://laoslaser.org)
 *
 *   LaOS is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   LaOS is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with LaOS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */
#ifndef config_h
#define config_h
#include "stdint.h"


typedef struct config_s
{
  float steps_per_mm_x;
  float steps_per_mm_y;
  float steps_per_mm_z;
  float steps_per_mm_e;
  int32_t maximum_feedrate_x;
  int32_t maximum_feedrate_y;
  int32_t maximum_feedrate_z;
  int32_t maximum_feedrate_e;
  float  acceleration;
  float  junction_deviation; 
} config_t;

#endif