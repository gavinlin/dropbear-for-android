/* From: http://www.freebsd.org/cgi/cvsweb.cgi/src/lib/libcrypt/crypt-md5.c?rev=1.12
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.org> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

#include <sys/cdefs.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>
#include <err.h>

#define MD5_SIZE 16
static char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void
_crypt_to64(char *s, u_long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}


/*
 * UNIX password
 */
char *
crypt(const char *pw, const char *salt_orig)
{
	hash_state ctx,ctx1;
	unsigned long l;
	int sl, pl;
	u_int i;
	u_char final[MD5_SIZE];
	static const char *sp, *ep;
	static char passwd[120], *p;
	static const char *magic = "$1$";	/*
						 * This string is magic for
						 * this algorithm.  Having
						 * it this way, we can get
						 * better later on.
						 */
	char *salt = m_strdup(salt_orig);

	/* Refine the Salt first */
	sp = salt;

	/* If it starts with the magic string, then skip that */
	if(!strncmp(sp, magic, strlen(magic)))
		sp += strlen(magic);

	/* It stops at the first '$', max 8 chars */
	for(ep = sp; *ep && *ep != '$' && ep < (sp + 8); ep++)
		continue;

	/* get the length of the true salt */
	sl = ep - sp;

	md5_init(&ctx);

	/* The password first, since that is what is most unknown */
	md5_process(&ctx, (const u_char *)pw, strlen(pw));

	/* Then our magic string */
	md5_process(&ctx, (const u_char *)magic, strlen(magic));

	/* Then the raw salt */
	md5_process(&ctx, (const u_char *)sp, (u_int)sl);

	/* Then just as many characters of the MD5(pw,salt,pw) */
	md5_init(&ctx1);
	md5_process(&ctx1, (const u_char *)pw, strlen(pw));
	md5_process(&ctx1, (const u_char *)sp, (u_int)sl);
	md5_process(&ctx1, (const u_char *)pw, strlen(pw));
	md5_done(&ctx1, final);
	for(pl = (int)strlen(pw); pl > 0; pl -= MD5_SIZE)
		md5_process(&ctx, (const u_char *)final,
		    (u_int)(pl > MD5_SIZE ? MD5_SIZE : pl));

	/* Don't leave anything around in vm they could use. */
	memset(final, 0, sizeof(final));

	/* Then something really weird... */
	for (i = strlen(pw); i; i >>= 1)
		if(i & 1)
		    md5_process(&ctx, (const u_char *)final, 1);
		else
		    md5_process(&ctx, (const u_char *)pw, 1);

	/* Now make the output string */
	strcpy(passwd, magic);
	strncat(passwd, sp, (u_int)sl);
	strcat(passwd, "$");

	md5_done(&ctx, final);

	/*
	 * and now, just to make sure things don't run too fast
	 * On a 60 Mhz Pentium this takes 34 msec, so you would
	 * need 30 seconds to build a 1000 entry dictionary...
	 */
	for(i = 0; i < 1000; i++) {
		md5_init(&ctx1);
		if(i & 1)
			md5_process(&ctx1, (const u_char *)pw, strlen(pw));
		else
			md5_process(&ctx1, (const u_char *)final, MD5_SIZE);

		if(i % 3)
			md5_process(&ctx1, (const u_char *)sp, (u_int)sl);

		if(i % 7)
			md5_process(&ctx1, (const u_char *)pw, strlen(pw));

		if(i & 1)
			md5_process(&ctx1, (const u_char *)final, MD5_SIZE);
		else
			md5_process(&ctx1, (const u_char *)pw, strlen(pw));
		md5_done(&ctx1, final);
	}

	p = passwd + strlen(passwd);

	l = (final[ 0]<<16) | (final[ 6]<<8) | final[12];
	_crypt_to64(p, l, 4); p += 4;
	l = (final[ 1]<<16) | (final[ 7]<<8) | final[13];
	_crypt_to64(p, l, 4); p += 4;
	l = (final[ 2]<<16) | (final[ 8]<<8) | final[14];
	_crypt_to64(p, l, 4); p += 4;
	l = (final[ 3]<<16) | (final[ 9]<<8) | final[15];
	_crypt_to64(p, l, 4); p += 4;
	l = (final[ 4]<<16) | (final[10]<<8) | final[ 5];
	_crypt_to64(p, l, 4); p += 4;
	l =                    final[11]                ;
	_crypt_to64(p, l, 2); p += 2;
	*p = '\0';

	/* Don't leave anything around in vm they could use. */
	memset(final, 0, sizeof(final));

	return passwd;
}


