#include "../sg.h"
/*
	       ,   ..
	    - ,  - .
	
	  (   )       .
	      ,         .
	
	        -  
		   ,     

 :
		 
		hole - TRUE   ""   
					 FALSE   ""    
		closed - TRUE      
						 FALSE      
 :
		listh -    SEL_LIST,   - - 
		fplane - TRUE,    
						 FALSE,       
												    .
		plane -    ( fplane == TRUE!)

: G_OK - " " 
						G_KEY -     
						 -     

*/
TEST_CORRECT_PATHS_RES test_correct_paths_array_for_face_command(hOBJ out_obj,
												  hOBJ* int_objcs,
												  int int_objcs_count)
{
	lpOBJ			obj;
	OSCAN_COD		code;
	D_PLANE			out_plane; // 
	D_PLANE			plane1;    //  
	int				out_orient; //  
	int             orient;     //   

	if (int_objcs_count==0)
		return TCPR_SUCCESS;
	
	obj = (lpOBJ)out_obj;
	if (obj->type!=OCIRCLE && !(obj->status & ST_CLOSE))
	{
		//   
		return TCPR_EXIST_NO_CLOSE;
	}
	if (obj->type!=OCIRCLE && !(obj->status & ST_FLAT))
	{
		//   
		return TCPR_EXIST_NO_FLAT;
	}
	if (!set_flat_on_path(out_obj, &out_plane)) 
	{
		//   
		return TCPR_EXIST_NO_FLAT;
	}
	out_orient = test_orient_path(out_obj, &out_plane);
	if (out_orient != 1 && out_orient != -1) 
	{
		//   
		return TCPR_EXIST_WITH_BAD_ORIENT;
	}

	for (int i=0; i<int_objcs_count; i++)
	{
		obj = (lpOBJ)int_objcs[i];
		if (obj->type!=OCIRCLE && !(obj->status & ST_CLOSE))
		{
			//   
			return TCPR_EXIST_NO_CLOSE;
		}
		if (obj->type!=OCIRCLE && !(obj->status & ST_FLAT))
		{
			//   
			return TCPR_EXIST_NO_FLAT;
		}
		if (!set_flat_on_path(int_objcs[i], &plane1)) 
		{
			//   
			return TCPR_EXIST_NO_FLAT;
		}
		if (!((dpoint_eq(&out_plane.v,&plane1.v,eps_n) &&
			fabs(out_plane.d - plane1.d) < eps_d) ||
			(dpoint_eq(&out_plane.v,dpoint_inv(&plane1.v,&plane1.v),eps_n) &&
			fabs(out_plane.d + plane1.d) < eps_d))) 
		{
			//       
			return TCPR_NOT_IN_ONE_PLANE;
		}
		code = test_self_cross_path(int_objcs[i], out_obj);
		if (code == OSBREAK) 
		{
			//       
			return TCPR_EXIST_INTERSECTS;
		}
		if (code != OSTRUE) 
		{
			//  -       
			return TCPR_EXIST_INTERSECTS;
		}
		if (test_inside_path(int_objcs[i], out_obj, &out_plane) != 1) 
		{
			//     
			return TCPR_BAD_INCLUDES;
		}
		for (int j=i+1; j<int_objcs_count; j++)
		{
			code = test_self_cross_path(int_objcs[i], int_objcs[j]);
			if (code == OSBREAK) 
			{
				//       
				return TCPR_EXIST_INTERSECTS;
			}
			if (code != OSTRUE) 
			{
				//  -       
				return TCPR_EXIST_INTERSECTS;
			}
			if (test_inside_path(int_objcs[i], int_objcs[j], &out_plane) != -1) 
			{
				//    
				return TCPR_BAD_INCLUDES;
			}
			if (test_inside_path(int_objcs[j], int_objcs[i], &out_plane) != -1) 
			{
				//     
				return TCPR_BAD_INCLUDES;
			}
		}
		orient = test_orient_path(int_objcs[i], &out_plane);
		if (orient != 1 && orient != -1) 
		{
			//    
			return TCPR_EXIST_WITH_BAD_ORIENT;
		}
		if (orient * out_orient > 0) 
		{ //   
			obj = (lpOBJ)int_objcs[i];
			obj->status ^= ST_DIRECT;
		}
	}
	return TCPR_SUCCESS;
}
