/*
 *
 * LaosFilesystem.cpp
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
 *
 */
#include "laosfilesystem.h"

LaosFileSystem::LaosFileSystem(PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name)
        : SDFileSystem(mosi, miso, sclk, cs, name) {
    sprintf(tablename, "/%s/%s", name, _LAOSFILE_TRANSTABLE);
    sprintf(pathname, "/%s/", name);
}

LaosFileSystem::~LaosFileSystem() {
}

FILE* LaosFileSystem::openfile(char *name, const std::string& iom) {
    if (islegalname(name)) {  // check length and chars in name

        char shortname[SHORTFILESIZE] = "";
        getshortname(shortname, name);
        char fullname[MAXFILESIZE+SHORTFILESIZE+1];
        if (strstr(iom.c_str(), "r")) {        // open file for reading
            if (strlen(shortname)!=0) {
                sprintf(fullname, "%s%s", pathname, shortname);
                return fopen(fullname, iom.c_str());
            } else {                // no file exists
                return NULL;
            }
        } else {                    // open file for writng
            if (strlen(shortname)!=0) {
                sprintf(fullname, "%s%s", pathname, shortname);
                return fopen(fullname, iom.c_str());
            } else {                    // create a new file
                makeshortname(shortname, name);
                sprintf(fullname, "%s%s", pathname, shortname);
                return fopen(fullname, iom.c_str());
            }
        }
    } else {    // (islegalname(name))
        return NULL;
    }
}

void LaosFileSystem::getlongname(char *result, char *searchname) {
    FILE *fp = fopen(tablename, "r");
    if (fp) {
        char longname[MAXFILESIZE];
        char shortname[SHORTFILESIZE];
        while (dirread(longname, shortname, fp))
            if (! strcmp(shortname, searchname))
                break;
        if (strcmp(shortname, searchname))
            strcpy(result, searchname);
        else
            strcpy(result, longname);
        fclose(fp);
    } else {
        strcpy(result, searchname);
    }
}

int LaosFileSystem::islegalname(char* name) {
    if (( strlen(name) > MAXFILESIZE-1 ) || (strlen(name) == 0))
        return 0;
    int legal = 1;
    char nochar[] = "?*,;=+#>|[]/\\";
    int x = 0;
    while (legal && (name[x] != 0)) {
        legal *= (name[x] > 32) && (name[x] < 126);
        int y = 0;
        while (legal && (nochar[y] != 0))
            legal *= name[x] != nochar[y++];
        x++;
    }
    return legal;
}

int LaosFileSystem::isshortname(char* name) {
    unsigned int len = strlen(name);
    for (unsigned int x=0; x<len; x++)
        if (name[x] == ' ') return 0; // spaces not allowed in shortname

    char myname[MAXFILESIZE];
    strcpy(myname, name);
    char *basename = NULL;
    if (len <= SHORTFILESIZE-1) {
        basename = strtok(myname, ".");
        if (strlen(basename) > 8) {
            return 0;    // basename was too long
        } else {
            if (char *ext_name = strtok(NULL, ".")) {  //
                if ((strlen(basename)+strlen(ext_name)+1) == len) {
                    if (strlen(ext_name)<4) {
                        return 1; // filename is in 8.3 format
                    } else {
                        return 0; // extension too long
                    }
                } else {
                    return 0; // there was more then one dot
                }
            } else {
                return 1; // filename of max 8 chars, no extension (OK)
            }
        }
    } else {
        return 0; // total filename too long
    }
}

void LaosFileSystem::removespaces(char* name) {
    unsigned int spaces = 1;
    while (spaces) {
        spaces = 0;
        unsigned int x = 0;
        while ((name[x] != ' ') && (x<strlen(name))) x++;
        if (name[x] == ' ') {
            spaces = 1;
            for (int y = x; x<(strlen(name)-1); y++)
                name[y] = name[y+1];
        }
    }
}

void LaosFileSystem::getshortname(char* shortname, char* name) {
    // * open filename translation table
    // * and see if a file with that name exists
    if (isshortname(name)) {
        strcpy(shortname, name);
    } else {
        int found = 0;
        char longname[MAXFILESIZE];
        FILE *fp = fopen(tablename, "r");
        if (fp) {
            while (dirread(longname, shortname, fp)) {
                if (!strcmp(longname, name)) {
                    found = 1;
                    break;
                }
            }
            if (! found) strcpy(shortname, "");
            fclose(fp);
        }
    }
}

void LaosFileSystem::makeshortname(char* shortname, char* name) {
    char *tmpname = new char[MAXFILESIZE];
    strcpy(tmpname, name);
    removespaces(tmpname);
    shorten(tmpname, SHORTFILESIZE);
    char *basename = strtok(tmpname, ".");
    char *ext_name = strtok(NULL, ".");
    strtolower(basename);
    strtolower(ext_name);
    int cnt = 1;
    char fullname[MAXFILESIZE+SHORTFILESIZE+2];
    FILE *fp = NULL;
    do {
        if (fp != NULL) fclose(fp);
        while ((cnt/10+strlen(basename)+2) > 8)
            basename[strlen(basename)-1] = 0;
        if (strlen(ext_name) > 0) {
            sprintf(shortname, "%s~%d.%s", basename, cnt++, ext_name);
        } else {
            sprintf(shortname, "%s~%d", basename, cnt++);
        }
        sprintf(fullname, "%s%s", pathname, shortname);
        fp = fopen(fullname, "rb");
    } while (fp!=NULL);
    
    FILE *tfp = fopen(tablename, "ab");
    dirwrite(name, shortname, tfp);
    fclose(tfp);

    delete(tmpname);
}

void LaosFileSystem::cleanlist() {
    // * open filename translation table
    char longname[MAXFILESIZE];
    char shortname[SHORTFILESIZE];
    char tabletmpname[MAXFILESIZE+SHORTFILESIZE+1];
    strcpy (tabletmpname, tablename);
    tabletmpname[strlen(tabletmpname)-1] = '~';
    
    // make a full copy of the table
    FILE* fp1 = fopen(tablename, "rb");
    if (fp1 == NULL) return;
    FILE* fp2 = fopen(tabletmpname, "wb");
    if (fp2 == NULL) return;
    while (dirread(longname, shortname, fp1))
        dirwrite(longname, shortname, fp2);
    fclose(fp1);
    fclose(fp2);

    fp1 = fopen(tablename, "wb");
    if (fp1 == NULL) return;
    fp2 = fopen(tabletmpname, "rb");
    if (fp2 == NULL) return;
    FILE* fp;
    while (dirread(longname, shortname, fp2)) {
        char fullname[MAXFILESIZE+SHORTFILESIZE+1];
        sprintf(fullname, "%s%s", pathname, shortname);
        fp = fopen(fullname, "rb");
        if (fp != NULL) {
            fclose(fp);
            dirwrite(longname, shortname, fp1);
        } 
     }
     fclose(fp1);
     fclose(fp2);
}

void LaosFileSystem::shorten(char* name, int max) {
    int len = 0;
    while (name[len++] != 0);   /* end of string */
    len--;
    if (len > max-1) {
        int ext = len;
        while (name[--ext] != '.'); /* begin of extension */
        int baselen, extlen;
        baselen = ext;
        extlen = len-ext-1;
        while ((baselen > max-5) && (baselen+extlen+1 > max-1))
            baselen--;
        name[baselen++] = '.';
        while ((ext<len) && (baselen < max-1))
            name[baselen++] = name[++ext];
        name[baselen] = 0;
    }
}

size_t LaosFileSystem::dirread(char* longname, char* shortname, FILE *fp) {
    char buff[MAXFILESIZE+SHORTFILESIZE];
    size_t result = fread(buff, 1, MAXFILESIZE+SHORTFILESIZE, fp);
    if (result) {       
        strncpy(longname, buff, MAXFILESIZE);
        longname[MAXFILESIZE-1] = 0;
        int cnt = MAXFILESIZE-2;
        while (longname[cnt]==' ') longname[cnt--] = 0;
        
        strncpy(shortname, &buff[MAXFILESIZE], SHORTFILESIZE);
        shortname[SHORTFILESIZE-1] = 0;
        cnt = SHORTFILESIZE-2;
        while (shortname[cnt]==' ') shortname[cnt--] = 0;
    }
    return result;
}

size_t LaosFileSystem::dirwrite(char* longname, char* shortname, FILE* fp) {
    char buff[MAXFILESIZE+SHORTFILESIZE];
    unsigned int x = 0; 
    while (longname[x] != 0) {
        buff[x] = longname[x];
        x++;
    }
    while (x<MAXFILESIZE-1) buff[x++] = ' ';
    buff[x++] = '\t';
    while (shortname[x-MAXFILESIZE] != 0) {
        buff[x] = shortname[x-MAXFILESIZE];
        x++;
    }
    while (x<MAXFILESIZE+SHORTFILESIZE-1) buff[x++] = ' ';
    buff[x++] = '\n';
    
    return fwrite(buff, 1, MAXFILESIZE+SHORTFILESIZE, fp);
}

void showfile() {
    char buff[35];
    FILE *fp = fopen("/sd/longname.sys","rb");
    if (fp) {
        printf("Contents of longname.sys\n\r");
        while (fread(buff, 1, MAXFILESIZE+SHORTFILESIZE, fp))
            printf("> %s\r", buff);
        fclose(fp);
        printf("\n\r");
    }
}

void cleandir() {
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            char fullname[MAXFILESIZE+SHORTFILESIZE+2];
            sprintf(fullname, "/sd/%s", p->d_name);
            remove(fullname);
        }
    } else {
        error("Could not open directory!\n\r");
    }
}

void printdir() {
    extern LaosFileSystem sd;
    printf("List of files in /sd\n\r");
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        // printf("...\n\r");
        while((p = readdir(d)) != NULL) {
            // printf("Short %s\n\r", p->d_name);
            if (strncmp(p->d_name, "longname.sy",11)) {
                char longname[MAXFILESIZE];
                // printf("Getlongname\n\r");
                sd.getlongname(longname, p->d_name);
                printf(" - %s (short: %s)\n\r", longname, p->d_name);
            }
        }
    } else {
        printf("Could not open directory!\n\r");
    }
}

void getprevjob(char *name) {
    extern LaosFileSystem sd;
    char shortname[SHORTFILESIZE], last[SHORTFILESIZE];
    strcpy(last, "");
    sd.getshortname(shortname, name);
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (strncmp(p->d_name, "longname.sy",11)) { // skip longname.sy*
                if (! strcmp(shortname, p->d_name)) {   // shortname = current entry
                    if (strcmp(last, "")) {
                        sd.getlongname(name, last);     // return entry before
                    } else {
                        sd.getlongname(name, p->d_name);    // last="", so current = first
                    }
                    closedir(d);
                    return;
                }
                strcpy(last, p->d_name);
            }
        } // while
        closedir(d);
    } else {
        printf("Getfilename: Could not open directory!\n\r");
    }
    sd.getlongname(name, last); // name not found (return last) 
                                // or no file found (return "") 
}

void getnextjob(char *name) {
    extern LaosFileSystem sd;
    char shortname[SHORTFILESIZE], last[SHORTFILESIZE];
    strcpy(last, "");
    sd.getshortname(shortname, name);
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (strncmp(p->d_name, "longname.sy",11)) { // skip longname.sy*
                if (! strcmp(shortname, last)) {        // if last was shortname
                    sd.getlongname(name, p->d_name);    //    return current
                    closedir(d);
                    return;
                }
                strcpy(last, p->d_name);
            }
        } // while
        closedir(d);
    } else {
        printf("Getfilename: Could not open directory!\n\r");
    }
    sd.getlongname(name, last);     // if last file was match, return the last
                                    // if filename not found, return last
                                    // if no file in directory, return ""
}

/*
void getfilename(char *name, int filenr) {
    extern LaosFileSystem sd;
    int cnt=0;
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (strncmp(p->d_name, "longname.sy",11)) {
                if (cnt++ == filenr) {
                    sd.getlongname(name, p->d_name);
                    closedir(d);
                    return;
                }
            }
        } // while
        strcpy(name, "");
        closedir(d);
    } else {
        printf("Getfilename: Could not open directory!\n\r");
    }
}

int getfilenum(char *name) {
    extern LaosFileSystem sd;
    char shortname[SHORTFILESIZE];
    getshortname(shortname, name);
    int cnt=0;
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (strncmp(p->d_name, "longname.sy",11)) {
                if (!strcmp(shortname, name)) {
                    closedir(d);
                    return cnt;
                }
                cnt++;
            }
        } // while
        closedir(d);
        return 0;
    } else {
        printf("Getfilename: Could not open directory!\n\r");
    }
    return 0;
}
*/

void writefile(char *myfile) {
    extern LaosFileSystem sd;
    printf("Writing file %s\n\r", myfile);
    FILE *fp = sd.openfile(myfile, "wb");
    if (fp) {
        fclose(fp);
    } 
}

void removefile(char *name) {
    extern LaosFileSystem sd;
    char shortname[SHORTFILESIZE] = "";
    sd.getshortname(shortname, name);
    if (strlen(shortname) != 0) {
        char fullname[MAXFILESIZE+SHORTFILESIZE+1];
        sprintf(fullname, "%s%s", sd.pathname, shortname);
        if (remove(fullname) < 0)
            printf("Error while removing file %s\n\r", fullname);
        sd.cleanlist();           
    } 
}

// Read an integer from file
int readint(FILE *fp)
{
  unsigned short int i=0;
  int sign=1;
  char c, str[16];
  
  while( !feof(fp)  )
  {
    fread(&c, sizeof(c),1,fp);   
    
    switch(c)
    {
      case '0': case '1': case '2':  case '3':  case '4': 
      case '5': case '6': case '7':  case '8':  case '9':  
        if ( i < sizeof(str)) 
          str[i++] = (char)c;
        break;
      case '-': sign = -1; break;
      case ';': while ((!feof(fp)) && (c != '\n')) {
            fread(&c, sizeof(c),1,fp);
        }
        break; 
      case ' ': case '\t': case '\r': case '\n':
        if ( i )
        {
          int val=0, d=1;
          while(i) 
          {
            if ( str[i-1] == '-' ) 
              d *= -1;
            else
              val += (str[i-1]-'0') * d;
            d *= 10;
            i--;
          }
          val *= sign;
          return val;
        }
        break;
    } // Switch
  } // while
  return 0;
} // read integer

void strtolower(char *name) {
    for(unsigned int i = 0; i < strlen(name); i++)
        name[i] = tolower(name[i]);
}

int isFirmware(char *filename) {
    char name[MAXFILESIZE];
    strcpy(name, filename);
    strtolower(name);
    int x = strlen(name);
    if ((tolower(filename[--x])=='n') && (tolower(filename[--x])=='i') && (tolower(filename[--x])=='b') && (filename[--x]=='.'))
        return 1;
    else
        return 0;
} 

void installFirmware(char *filename) {
    removeFirmware();
    char buff[512];
    extern LaosFileSystem sd;
    //printf("Copy firmware file %s\n\r", filename);
    FILE *fp = sd.openfile(filename, "rb");
    if (fp) {
        FILE *fp2 = fopen("/local/firmware.bin", "wb");
        while (!feof(fp)) {
            int size = fread(buff, 1, 512, fp);
            fwrite(buff, 1, size, fp2);
        }
        fclose(fp);
        fclose(fp2);
    }
    removefile(filename);
}

void removeFirmware() { // remove old firmware from SD
    DIR *d;
    struct dirent *p;
    d = opendir("/local");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (isFirmware(p->d_name)) {
                char name[32];
                sprintf(name, "/local/%s", p->d_name);
                remove(name);
            } 
        }
    } else {
        printf("removeFirmware: Could not open directory!\n\r");
    }
}

int SDcheckFirmware() {
    extern LaosFileSystem sd;
    DIR *d;
    struct dirent *p;
    d = opendir("/sd");
    if(d != NULL) {
        while((p = readdir(d)) != NULL) {
            if (strncmp(p->d_name, "longname.sy",11)) {
                if (isFirmware(p->d_name)) {
                    installFirmware(p->d_name);
                    return 1;
                }
            }
        }
    } else {
        printf("SDcheckFirmware: Could not open directory!\n\r");
    }
    return 0;
}

int isLaosFile(char *filename) {
    char name[MAXFILESIZE];
    strcpy(name, filename);
    strtolower(name);
    int x = strlen(name);
    if ((tolower(filename[--x])=='c') && (tolower(filename[--x])=='g') && (tolower(filename[--x])=='l') && (filename[--x]=='.'))
        return 1;
    else
        return 0;
}
