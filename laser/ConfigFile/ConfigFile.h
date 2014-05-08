/**
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
 * Reads a setting, based on the key. (case sensitive)
 * If the key is not found, the default value is returned.
 *
 * Simple, not (time) efficient. For every request, the complete file is reset and read again
 *
 @code 
 file format
 
 ; comment
 key value [newline || comment]

 For example:
 
 ; This is a test config file
 ip 192.168.1.1
 port 1234 ; the key is "port" the value is "1234". The rest is comment
 ; EOF
 @endcode
 */
#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include "global.h"
#include "laosfilesystem.h"

    /** Simple config file object
      * Only supports reading config files. Tries to limit memory usage.
      * Note: the file handle is kept open during the lifetime of this object.
      * To close the file: destroy this ConfigFile object! A simple way is to enclose the creation
      * of this object inside a code block
      * Example:
      * @code 
      * char ip[16];
      * int port;
      * {
      *   ConfigFile cfg("/local/config.txt");
      *   cfg.Value("ip", ip, sizeof(ip), "192.168.1.10");
      *   cfg.Value("port", &port, 80);
      * }
      * @endcode
      */
class ConfigFile {
public:
    /** Make new ConfigFile object. Open config file.
      * Note: the file handle is kept open during the lifetime of this object.
      * To close the file: destroy this ConfigFile object!
      * @param file Filename of the configuration file.
      */
    ConfigFile(char *name);

    ~ConfigFile();

/** Read value. If file is not open, or key does not exist: copy default value (return false)
  * @param key name of the key in the file
  * @param value pointer to buffer that receives the value
  * @param maxlen the maximum length of the value. If the actual value is longer, it is truncated
  * @param def Default value. If the key is not found in the file, this value is copied. 
  * @return "true" if the key is found "false" is key is not found (default value is returned)
  */ 
    bool Value(const std::string& key, char *value,  size_t maxlen, const std::string& def);

/** Read Integer value. If file is not open, or key does not exist: copy default value (return false)
  * @param key name of the key in the file
  * @param value pointer to integer that receives the value
  * @param def Default value. If the key is not found in the file, this value is copied. 
  * @return "true" if the key is found "false" is key is not found (default value is returned)
  */ 
    bool Value(const std::string& key, int *value, int def);
    
  /** See if file was present
  * @return "true" if file is open, "false" file is not found 
  */ 
  bool IsOpen(void) { return fp != NULL; }
  
private:
    FILE *fp;

};


#endif
