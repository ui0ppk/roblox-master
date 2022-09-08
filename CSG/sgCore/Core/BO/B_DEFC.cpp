#include "../sg.h"

/************************************************
 *            --- short b_defc ---
 * ************************************************/

short b_defc(void)
{
	D_POINT c;

	dvector_product(&(npa->p[facea].v), &(npb->p[faceb].v),&c);
	c.x = fabs(c.x);
	c.y = fabs(c.y);
	c.z = fabs(c.z);
	if (c.x <= c.y)
		if (c.y <= c.z)  return 2;
		else             return 1;
	else
		if (c.x <= c.z)  return 2;
		else             return 0;
}
