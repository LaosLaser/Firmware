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
  printf("ConfigFile:ConfigFile (%s)\n\r", file);
  extern LaosFileSystem sd;
  fp =sd.openfile(file, "r");
  if (fp==NULL) {
    char tmpname[32];
    sprintf(tmpname, "/local/%s", file);
    fp = fopen(tmpname,"r");
  } 
}

// Destroy a config file (closes the file handle)
ConfigFile::~ConfigFile() 
{
  if ( fp != NULL )
    fclose(fp);
}

// Read value
bool ConfigFile::Value(const std::string& key, char *value,  size_t maxlen, const std::string& def)
{
  unsigned int m=0,n=0,c,s=0;
  char *v = value;
  char *newkey = new char[key.size()+1];
  strcpy(newkey, key.c_str());
  if (fp == NULL)
  {
    strncpy(value, def.c_str(), maxlen);
    return false;
  }

  n = strlen(newkey);
  fseek(fp, 0L, SEEK_SET);
  while( s != 99  ) 
  {
    c = fgetc(fp);
    if ( c == EOF ) 
      break;
    // printf("%d(%d): '%c'\n\r", s, m, c);
    switch( s  )// sate machine
    { 
      case 0: // (re) start: note: no break; fall through to case 1
        m=0; 
        s=1;
      case 1: // read newkey, skip spaces
        if ( c == newkey[m] ) 
          m++; 
        else 
          s = 0;
        if ( c == ';' ) 
          s = 10; 
        else if ( c == ' ' || c == '\t' || c == '\n' || c == '\r') 
        {
          if ( n == m ) // newkey found
          {
            s = 2;
            m = 0; 
          }
          else
            s = 0;
        }
        break;
      case 2: // newkey matched, skip whitepaces upto the first char
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
  if ( s == 99 && m > 0) // newkey found, and value assigned
  {
    *value = 0; // terminate string
    printf("'%s'='%s'\n\r", newkey,v);
    return true; 
  }
  else
  {
    strncpy(value, def.c_str(), maxlen);
    printf("'%s'='%s' (default)\n\r", newkey,v);
    return false;
  } 
}

// Read int value
bool ConfigFile::Value(const std::string& key, int *value, int def)
{
  char *newkey = new char[key.size()+1];
  strcpy(newkey, key.c_str());
  char val[32];
  bool b = Value(newkey, val, 31, "");
  if ( b )
  {
    *value = atoi(val);
     // printf("%s: numeric value=%d\n\r", newkey, *value);
  }
  else
  {
    *value = def;
     // printf("%s default (%d)\n\r", newkey, *value);
  }
  return b;
}
