#include "../sg.h"

/*
	Най поек ок p на пм, поод еез ок l1,l2

	Возваае: NULL - ел ок l1,l2 овпада
										 Сообене не вдае!
							лк на ком ок - в номалном лае
	Меод:
		кома ока наод з еен ем з 4- лн.-нй:
			pp = l1 + alpha * (l2 - l1)   - pp леж на пмой
			(p - pp, l2 - l1) = 0         - калное позв. = 0
*/
lpD_POINT projection_on_line(lpD_POINT p,
														 lpD_POINT l1, lpD_POINT l2,
														 lpD_POINT pp)
{
	D_POINT ll;
	sgFloat 	alpha;

	dpoint_sub(l2, l1, &ll);
	alpha = dskalar_product(&ll, &ll);
	if (alpha < eps_d) return NULL;
	alpha = (dskalar_product(&ll, p) - dskalar_product(&ll, l1)) / alpha;
	return dpoint_parametr(l1, l2, alpha, pp);
}

/*
	Вл вен p2  p3 пмоголнка по заданнм венам
	p0, p1  по вене, лежаей на пмой p2,p3.
	Возможна вожденно пмоголнка в оезок p0,p1.

	Возваае: TRUE  - вен p2,p3 влен
							FALSE - пмоголнк вожден в оезок p0,p1 л
											оезок p0,p1 мее нлев длн
											Сообене не вдае!
*/
BOOL calc_face4(lpD_POINT p0, lpD_POINT p1, lpD_POINT p,
								lpD_POINT p2, lpD_POINT p3)
{
	D_POINT pp, a;

	if (!projection_on_line(p, p0, p1, &pp)) return FALSE;
	dpoint_sub(p, &pp, &a);
	dpoint_add(p0, &a, p2);
	dpoint_add(p1, &a, p3);
	return TRUE;
}
