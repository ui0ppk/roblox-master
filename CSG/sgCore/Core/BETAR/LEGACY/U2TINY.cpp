
#include "../../sg.h"

#define ROTORSZ	256
#define MASK	0377

static BYTE	t1[ROTORSZ];
static BYTE	t2[ROTORSZ];
static BYTE	t3[ROTORSZ];

static BOOL		generate				(LPSTR pw, int lenpw);

//
//
//
static BOOL generate(LPSTR	pw, int	lenpw)
{
	int			ic, i, k;
	BYTE temp;
	unsigned short int 	random;
	char		buf[13];
	long		seed;

	lenpw = min(8, lenpw);
	memset(buf, 0, 13);                                          
	memset(t1, 0, ROTORSZ);                                          
	memset(t2, 0, ROTORSZ);                                          
	memset(t3, 0, ROTORSZ); 
	for (i = 0; i < lenpw; i++)                                         
		buf[i] = (BYTE) pw[i];
	buf[8] = buf[0];
	buf[9] = buf[1];

	seed = 123;
	for (i = 0; i < 13; i++)
		seed = seed * buf[i] + i;
	for (i = 0; i < ROTORSZ; i++)
		t1[i] = (BYTE) i;
	for (i = 0; i < ROTORSZ; i++)
	{
		seed = 5 * seed + buf[i%13];
		random = (unsigned short int) (seed % 65521L);
		k = ROTORSZ - 1 - i;
		ic = (random & MASK) % (k + 1);
		random >>= 8;
		temp = t1[k];	t1[k] = t1[ic];		t1[ic] = temp;
		if (t3[k] != 0)
			continue;
		ic = (random & MASK) % k;
		while (t3[ic] != 0)
			ic = (ic + 1) % k;
		t3[k] = (BYTE) ic;
		t3[ic] = (BYTE) k;
	}
	for (i = 0; i < ROTORSZ; i++)
		t2[t1[i]&MASK] = (BYTE) i;
	return 1;
}


BOOL f17(LPSTR	buffer, int	buflen, LPSTR 	password, int 	passlen)
{
	int		i, n1, n2;
	int		j;

	generate(password, passlen);
// enscrypt
	if (buflen <= 0)
		return FALSE;
	n1 = n2 = 0;
	for (j = 0; j < buflen; j++)
	{
		i = buffer[j];
		i = t2[(t3[(t1[(i+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1;
		buffer[j] = (BYTE) i;
		n1++;
		if (n1 == ROTORSZ)
		{
			n1 = 0;
			n2++;
			if (n2 == ROTORSZ)
				n2 = 0;
	
		}
	}
	return TRUE;
}
