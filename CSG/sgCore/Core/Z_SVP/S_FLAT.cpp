#include "../sg.h"


bool is_path_on_one_line(hOBJ hobj)
{
	lpOBJ pobj = (lpOBJ)hobj;

	if (pobj->type!=OPATH)
		return false;

	VDIM   points_vdim;
	init_vdim(&points_vdim,sizeof(D_POINT));

	VADR   cur_adr = ((lpGEO_PATH)(pobj->geo_data))->listh.hhead;

	while (cur_adr)
	{
		lpOBJ cur_obj = (lpOBJ)cur_adr;

		if (cur_obj->type==OLINE)
		{
			lpGEO_LINE lineG = (lpGEO_LINE)cur_obj->geo_data;
			add_elem(&points_vdim, &lineG->v1);
			add_elem(&points_vdim, &lineG->v2);
		}
		else
			if (cur_obj->type==OARC)
			{
				free_vdim(&points_vdim);
				return false;
			}
			else
				if (cur_obj->type==OPATH)
				{
					if (!is_path_on_one_line(cur_adr))
					{
						free_vdim(&points_vdim);
						return false;
					}
					D_POINT tmpP;
					get_endpoint_on_path(cur_adr,&tmpP,(OSTATUS)0);
					add_elem(&points_vdim, &tmpP);
					get_endpoint_on_path(cur_adr,&tmpP,ST_DIRECT);
					add_elem(&points_vdim, &tmpP);
				}


		get_next_item(cur_adr,&cur_adr);
	}

	const int       pntsCnt = points_vdim.num_elem;

	if (pntsCnt<=2)
	{
		free_vdim(&points_vdim);
		return true;
	}

	SG_POINT* p1 = (SG_POINT*)get_elem(&points_vdim,0);
	SG_POINT* p2 = (SG_POINT*)get_elem(&points_vdim,1);

	for (int i=2;i<pntsCnt;i++)
	{
		SG_POINT* p3 = (SG_POINT*)get_elem(&points_vdim,i);
		if (!sgSpaceMath::IsPointsOnOneLine(*p1,*p2,*p3))
		{
			free_vdim(&points_vdim);
			return false;
		}
	}

	free_vdim(&points_vdim);
	return true;
}


/*
		       
						calc_plane == NULL
								: FALSE - -  (.)
														TRUE - /  ST_FLAT
		
		  
						calc_plane != NULL
								: FALSE -   
																		(.   )
														TRUE  -  
*/
BOOL set_flat_on_path(hOBJ hobj, lpD_PLANE calc_plane)
{
	VDIM	  vdim;
	D_POINT	root, first, second, a, b, /*d,*/ c = {0., 0., 0.};
	lpMNODE mnode;
	D_PLANE	plane;
	BOOL 		ip, flat;
	lpOBJ		obj;

	//  ""  
	if (!apr_path(&vdim, hobj)) return FALSE;
	if (vdim.num_elem < 3) {
		if (calc_plane) return FALSE;
		obj = (lpOBJ)hobj;
		obj->status |= ST_FLAT;
		return TRUE;
	}
	if (!begin_rw(&vdim, 0)) {
err1:
		free_vdim(&vdim);
		return FALSE;
	}
	if ((mnode = (MNODE *)get_next_elem(&vdim)) == NULL) {
err2:
		end_rw(&vdim);
		goto err1;
	}
	root = mnode->p;
	if ((mnode = (MNODE *)get_next_elem(&vdim)) == NULL) goto err2;
	first = mnode->p;
	/*while ((mnode = (MNODE *)get_next_elem(&vdim)) != NULL) {
		second = mnode->p;
		dpoint_sub(&first, &root, &a);
		dpoint_sub(&second, &root, &b);
		dvector_product(&a, &b, &d);
		c.x = c.x + fabs(d.x);
		c.y = c.y + fabs(d.y);
		c.z = c.z + fabs(d.z);
		first = second;
	}*/

	/**RA*/while ((mnode = (MNODE *)get_next_elem(&vdim)) != NULL) {
	/**RA*/	second = mnode->p;
	/**RA*/	if (!sgSpaceMath::IsPointsOnOneLine(*((SG_POINT*)&root),
	/**RA*/									*((SG_POINT*)&first),
	/**RA*/									*((SG_POINT*)&second)) )
	/**RA*/	{
	/**RA*/		dpoint_sub(&first, &root, &a);
	/**RA*/		dpoint_sub(&second, &root, &b);
	/**RA*/		dvector_product(&a, &b, &c);
	/**RA*/		goto RALab;
	/**RA*/	}
	/**RA*/	else
	/**RA*/		continue;
	/**RA*/}
//         ,    ,
//  ,        - .   .
	/**RA*/assert(0);
	/**RA*/goto err2;


/**RA*/RALab:		  
	end_rw(&vdim);
	ip = dnormal_vector (&c);
	if (ip == FALSE) {	//     
		if (calc_plane) return FALSE;
		obj = (lpOBJ)hobj;
		obj->status |= ST_FLAT;
		return TRUE;
	}
	plane.v = c;
	plane.d = - dskalar_product(&root, &c);
	// ===       
	flat = TRUE;
	if (!begin_rw(&vdim, 0)) goto err1;
	while ((mnode = (MNODE *)get_next_elem(&vdim)) != NULL) {

		D_POINT  ppp1 = plane.v;
		D_POINT  ppp2 = mnode->p;

		dnormal_vector(&ppp1);
		dnormal_vector(&ppp2);

		sgFloat tV = dskalar_product(&ppp1, &ppp2);
        //sgFloat tV = dskalar_product(&plane.v, &mnode->p);
		sgFloat tmpVal = fabs(tV + plane.d);
		if (tmpVal >= eps_d) {
			flat = FALSE;
			break;
		}
	}
	end_rw(&vdim);
	free_vdim(&vdim);
	if (calc_plane) {
		if (flat) {
			*calc_plane = plane;
			return TRUE;
		}
		return FALSE;
	}
	obj = (lpOBJ)hobj;
	if (flat) obj->status |= ST_FLAT;
	else      obj->status &= ~ST_FLAT;
	return TRUE;
}

