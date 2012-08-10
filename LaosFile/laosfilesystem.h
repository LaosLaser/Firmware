/*
 *
 * LaosFilesystem.h
 * Simple Long Filename system on top of SDFilesystem.h
 *
 * Copyright (c) 2011 Jaap Vermaas
 *
 *   This file is part of the LaOS project (see: http://wiki.laoslaser.org
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
 */
#ifndef _LAOSFILESYSTEM_
#define _LAOSFILESYSTEM_

#include "SDFileSystem.h"
#include "FATFileSystem.h"
#include <string>
#include <ctype.h>

#define _LAOSFILE_TRANSTABLE "longname.sys"
#define MAXFILESIZE 21
#define SHORTFILESIZE 13

class LaosFileSystem : public SDFileSystem {
    public:
        LaosFileSystem(PinName mosi, PinName miso, PinName sclk, PinName cs, 
                const char* name);        // Create the filesystem on SD
        virtual ~LaosFileSystem();                // destructor
        FILE* openfile(char* fname, char* iom);    // open a file
        void getlongname(char *result, char *searchname);   // return long names
        void getshortname(char* shortname, char* name); //get a short name
        char pathname[MAXFILESIZE+2];
        void cleanlist();
        void shorten(char* name, int max);
        
    private:
        int islegalname(char* name);
        int isshortname(char* name);
        void removespaces(char* name);
        void makeshortname(char* shortname, char* name);
        size_t dirread(char* longname, char* shortname, FILE *fp);
        size_t dirwrite(char* longname, char* shortname, FILE* fp);
        char tablename[MAXFILESIZE + SHORTFILESIZE + 1];
};

void showfile();        // debug: list contents of long filesytem file
void cleandir();        // delete all files in directory
void printdir();        // list all files in directory (with long names)
//void getfilename(char *name, int filenr); // get name of the #filenr file
//int getfilenum(char *name); // get number of this filename
void getprevjob(char *name);     // previous job
void getnextjob(char *name);     // next job
void writefile(char *name); // example code to open a file
void removefile(char *name);    // example code to remove a file
int readint(FILE *fp);      // read integers from open file
void strtolower(char *name);    // change characters to lowercase
int isFirmware(char *name);     // check if it's firmware
void installFirmware(char *filename); // put firmware in place
void removeFirmware(); // remove old firmware
int SDcheckFirmware();  // check for firmware
int isLaosFile(char *filename);   // check extension for LaOS compatibility
#endif
