/* Copyright (C) 2007 MySQL AB & Michael Widenius

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#define SCRAMBLE_LENGTH_323 8

#include <string.h>
#include <math.h>

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char uchar;

struct my_rnd_struct {
  unsigned long seed1,seed2,max_value;
  double max_value_dbl;
};

static void my_rnd_init(struct my_rnd_struct *rand_st, ulong seed1, ulong seed2);
static double my_rnd(struct my_rnd_struct *rand_st);
static void hash_password(ulong *result, const char *password, uint password_len);

/*
  Initialize random generator

  NOTES
    MySQL's password checks depends on this, so don't do any changes
    that changes the random numbers that are generated!
*/

static void my_rnd_init(struct my_rnd_struct *rand_st, ulong seed1, ulong seed2)
{
  rand_st->max_value= 0x3FFFFFFFL;
  rand_st->max_value_dbl=(double) rand_st->max_value;
  rand_st->seed1=seed1%rand_st->max_value ;
  rand_st->seed2=seed2%rand_st->max_value;
}

/*
  Generate random number.

  SYNOPSIS
    my_rnd()
    rand_st    INOUT  Structure used for number generation
    
  RETURN VALUE
    generated pseudo random number
*/

static double my_rnd(struct my_rnd_struct *rand_st)
{
  rand_st->seed1=(rand_st->seed1*3+rand_st->seed2) % rand_st->max_value;
  rand_st->seed2=(rand_st->seed1+rand_st->seed2+33) % rand_st->max_value;
  return (((double) rand_st->seed1)/rand_st->max_value_dbl);
}

/*
    Generate binary hash from raw text string 
    Used for Pre-4.1 password handling
  SYNOPSIS
    hash_password()
    result       OUT store hash in this location
    password     IN  plain text password to build hash
    password_len IN  password length (password may be not null-terminated)
*/

static void hash_password(ulong *result, const char *password, uint password_len)
{
  register ulong nr=1345345333L, add=7, nr2=0x12345671L;
  ulong tmp;
  const char *password_end= password + password_len;
  for (; password < password_end; password++)
  {
    if (*password == ' ' || *password == '\t')
      continue;                                 /* skip space in password */
    tmp= (ulong) (uchar) *password;
    nr^= (((nr & 63)+add)*tmp)+ (nr << 8);
    nr2+=(nr2 << 8) ^ nr;
    add+=tmp;
  }
  result[0]=nr & (((ulong) 1L << 31) -1L); /* Don't use sign bit (str2int) */;
  result[1]=nr2 & (((ulong) 1L << 31) -1L);
}



/*
    Scramble string with password.
    Used in pre 4.1 authentication phase.
  SYNOPSIS
    scramble_323()
    to       OUT Store scrambled message here. Buffer must be at least
                 SCRAMBLE_LENGTH_323+1 bytes long
    message  IN  Message to scramble. Message must be at least
                 SRAMBLE_LENGTH_323 bytes long.
    password IN  Password to use while scrambling
*/

void scramble_323(char *to, const char *message, const char *password)
{
  struct my_rnd_struct rand_st;
  ulong hash_pass[2], hash_message[2];

  if (password && password[0])
  {
    char extra, *to_start=to;
    const char *message_end= message + SCRAMBLE_LENGTH_323;
    hash_password(hash_pass,password, (uint) strlen(password));
    hash_password(hash_message, message, SCRAMBLE_LENGTH_323);
    my_rnd_init(&rand_st,hash_pass[0] ^ hash_message[0],
               hash_pass[1] ^ hash_message[1]);
    for (; message < message_end; message++)
      *to++= (char) (floor(my_rnd(&rand_st)*31)+64);
    extra=(char) (floor(my_rnd(&rand_st)*31));
    while (to_start != to)
      *(to_start++)^=extra;
  }
  *to= 0;
}

