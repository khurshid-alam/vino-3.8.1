/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * vncauth.c - Functions for VNC password management and authentication.
 */

#include "rfb/rfbproto.h"
#include "d3des.h"

#ifdef VINO_HAVE_GCRYPT
#include <gcrypt.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#define srandom srand
#define random rand
#endif

/*
 * Generate CHALLENGESIZE random bytes for use in challenge-response
 * authentication.
 */

void
vncRandomBytes(unsigned char *bytes)
{
#ifdef VINO_HAVE_GCRYPT
    static rfbBool gcrypt_init = FALSE;

    if (!gcrypt_init) {
      if (!gcry_check_version(NULL)) /* version mismatch */
        exit(1);
      gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
      gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
      gcrypt_init = TRUE;
    }

    gcry_randomize(bytes, CHALLENGESIZE, GCRY_STRONG_RANDOM);
#else
    int i;
    static rfbBool s_srandom_called = FALSE;

    if (!s_srandom_called) {
      srandom((unsigned int)time(0) ^ (unsigned int)getpid());
      s_srandom_called = TRUE;
    }

    for (i = 0; i < CHALLENGESIZE; i++) {
      bytes[i] = (unsigned char)(random() & 255);
    }
#endif /* VINO_HAVE_GCRYPT */
}


/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */

void
vncEncryptBytes(unsigned char *bytes, char *passwd)
{
    unsigned char key[8];
    unsigned int i;

    /* key is simply password padded with nulls */

    for (i = 0; i < 8; i++) {
	if (i < strlen(passwd)) {
	    key[i] = passwd[i];
	} else {
	    key[i] = 0;
	}
    }

    deskey(key, EN0);

    for (i = 0; i < CHALLENGESIZE; i += 8) {
	des(bytes+i, bytes+i);
    }
}
