#include "../../sg.h"

void np_gab_init(lpREGION_3D gab)
{
	gab->min.x = gab->min.y = gab->min.z =  1.e35;
	gab->max.x = gab->max.y = gab->max.z = -1.e35;
}

BOOL np_gab_inter(lpREGION_3D m1, lpREGION_3D m2)
{
  if (m1->min.x > m2->max.x+eps_d || m2->min.x > m1->max.x+eps_d) return TRUE;
  if (m1->min.y > m2->max.y+eps_d || m2->min.y > m1->max.y+eps_d) return TRUE;
  if (m1->min.z > m2->max.z+eps_d || m2->min.z > m1->max.z+eps_d) return TRUE;
  return FALSE;
}
void np_gab_un(lpREGION_3D g1, lpREGION_3D g2, lpREGION_3D g3)
{

	if (g1->min.x < g2->min.x) g3->min.x = g1->min.x;
	else                       g3->min.x = g2->min.x;
	if (g1->min.y < g2->min.y) g3->min.y = g1->min.y;
	else                       g3->min.y = g2->min.y;
	if (g1->min.z < g2->min.z) g3->min.z = g1->min.z;
	else                       g3->min.z = g2->min.z;
	if (g1->max.x > g2->max.x) g3->max.x = g1->max.x;
	else                       g3->max.x = g2->max.x;
	if (g1->max.y > g2->max.y) g3->max.y = g1->max.y;
	else                       g3->max.y = g2->max.y;
	if (g1->max.z > g2->max.z) g3->max.z = g1->max.z;
	else                       g3->max.z = g2->max.z;
}

short np_gab_pl(lpREGION_3D gab, lpD_PLANE pl, sgFloat eps)
{
#define DIST(a,b,c) (a*pl->v.x + b*pl->v.y + c*pl->v.z + pl->d)

	short p;

	p = sg(DIST(gab->min.x,gab->min.y,gab->min.z),eps);
	if (sg(DIST(gab->min.x,gab->min.y,gab->max.z),eps) != p) return 0;
	if (sg(DIST(gab->min.x,gab->max.y,gab->min.z),eps) != p) return 0;
	if (sg(DIST(gab->max.x,gab->min.y,gab->min.z),eps) != p) return 0;
	if (sg(DIST(gab->min.x,gab->max.y,gab->max.z),eps) != p) return 0;
	if (sg(DIST(gab->max.x,gab->min.y,gab->max.z),eps) != p) return 0;
	if (sg(DIST(gab->max.x,gab->max.y,gab->min.z),eps) != p) return 0;
	if (sg(DIST(gab->max.x,gab->max.y,gab->max.z),eps) != p) return 0;
	return p;
}

