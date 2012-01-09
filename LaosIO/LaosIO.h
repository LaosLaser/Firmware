/**
 * LaosIO.h
 * input / output system
 *
 * Copyright (c) 2011 Peter Brier
 *
 *   This file is part of the LaOS project (see: http://wiki.protospace.nl/index.php/LaOS)
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
 * set and get inputs/outputs for the peripherals
 *
 @code 
 --code--
 
 @endcode
 */
#ifndef _LAOSIO_H_
#define _LAOSIO_H_
#include "global.h"

    /** IO System
      * Get and Set IO lines
      */
class LaosIO {
public:
    /** Make new LaosIO object. 
      */
  LaosIO();

  ~LaosIO();

  void set(int line, bool state);
  bool get(int line);

private:
  unsigned int state;
};

#endif
