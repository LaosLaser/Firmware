/*
 * ConfigFile.cpp
 * Simple Config file reader class
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
 */
#include "ConfigFile.h"

// Make new config file object
ConfigFile::ConfigFile(char *file) 
{
 // printf("ConfigFile(%s)\n\r", file);
  fp = fopen(file,"rb");
}

// Destroy a config file (closes the file handle)
ConfigFile::~ConfigFile() 
{
   printf("~ConfigFile()\n\r");
  if ( fp != NULL )
    fclose(fp);
}

// Read value
bool ConfigFile::Value(char *key, char *value,  size_t maxlen, char *def)
{
  int m=0,n=0,c,s=0;
  char *v = value;
  if (fp == NULL)
  {
    strncpy(value, def, maxlen);
    return false;
  }

  n = strlen(key);
  fseek(fp, 0L, SEEK_SET);
  while( s != 99  ) 
  {
    c = fgetc(fp);
    if ( c == EOF ) 
      break;
  //  printf("%d(%d): '%c'\n\r", s, m, c);
    switch( s  )// sate machine
    { 
      case 0: // (re) start: note: no break; fall through to case 1
        m=0; 
        s=1;
      case 1: // read key, skip spaces
        if ( c == key[m] ) 
          m++; 
        else 
          s = 0;
        if ( c == ';' ) 
          s = 10; 
        else if ( c == ' ' || c == '\t' || c == '\n' || c == '\r') 
        {
          if ( n == m ) // key found
          {
            s = 2;
            m = 0; 
          }
          else
            s = 0;
        }
        break;
      case 2: // key matched, skip whitepaces upto the first char
        if ( c == ';' ) s = 99;
        else if ( c != ' ' && c != '\t' ) 
        { 
          s = 3;
          m = 1;
          if ( m < maxlen) 
            *value++ = c;
        }
        break;
        
      case 3: // copy value content, upto eol or comment
        if ( m == maxlen || c == '\n' || c == '\r' || c == ';' ) 
          s = 99;
        else
        {
          m++;
          *value++ = c;
        }
        break;         
      case 10: // skip comments, upto eol or eof
        if ( c == '\n' || c == '\r' ) s = 0;
        break;
     }
  }
  if ( s == 99 && m > 0) // key found, and value assigned
  {
    *value = 0; // terminate string
    printf("'%s'='%s'\n", key,v);
    return true; 
  }
  else
  {
    strncpy(value, def, maxlen);
    printf("'%s'='%s' (default)\n", key,v);
    return false;
  } 
}


// Read int value
bool ConfigFile::Value(char *key, int *value, int def)
{
  char val[32];
  bool b = Value(key, val, 31, "");
  if ( b )
  {
    *value = atoi(val);
     printf("%s: numeric value=%d\n", key, *value);
  }
  else
  {
    *value = def;
     printf("%s default (%d)\n", key, *value);
  }
  return b;
}
