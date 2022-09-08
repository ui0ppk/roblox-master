#include "Core/sg.h"

ICoreAppInterface*    application_interface = NULL;

LPSTR    ids;


/**************************************************/
/**************************************************/
/**************************************************/
/**************************************************/
/**************************************************/
/**************************************************/

//sgCScene::SCENE_SETUPS   scene_setups;

bool  sgInitKernel()
{
	InitsBeforeMain();

	ids  = (LPSTR) SGMalloc(256);

	InitSysAttrs();
	if (!GeoObjectsInit())
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}
	if (!create_np_mem_tmp())
	{
		global_sg_error = SG_ER_INTERNAL;
		return false;
	}

	init_draft_info(/*applpath,applname*/);

	//memset(&scene_setups,0,sizeof(sgCScene::SCENE_SETUPS));

	sgGetScene();
    
    sgGetScene()->Clear();

    global_sg_error = SG_ER_SUCCESS;
	return true;
}

void  sgFreeKernel(bool show_memleaks/*=true*/)
{
	show_memory_leaks = show_memleaks; 
	sgPrivateAccess(1982,NULL,NULL);// was sgDestroyScene();
	free_all_sg_mem();
	SGFree(ids);
	vReport();
}

void   sgGetVersion(int& major, int& minor, int& release, int& build)
{
	major = KERNEL_VERSION_MAJOR;
	minor = KERNEL_VERSION_MINOR;
	release = KERNEL_VERSION_RELEASE;
	build = KERNEL_VERSION_BUILD;
}

void*    sgPrivateAccess(int Ftype, void* fVoid, void* sVoid)
{
	switch(Ftype) 
	{
	case 1980:  // SG_OBJ_HANDLE   GetObjectHandle(const sgCObject* obj)
		// fVoid - obj, return SG_OBJ_HANDLE
		{
			if (fVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCObject* obj = reinterpret_cast<sgCObject*>(fVoid);
			return obj->m_object_handle;
		}
		break;
	case 1981: // void  SetObjectHandle(sgCObject* ob,SG_OBJ_HANDLE hndl)
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCObject* obj = reinterpret_cast<sgCObject*>(fVoid);
			obj->m_object_handle = sVoid;
			return NULL;
		}
		break;
	case 1982: // sgDestroyScene()
		{
			if (scene!=NULL)
			{
				delete scene;
				scene = NULL;
			}
			return NULL;
		}
		break;
	case 1983: //sgCObject*   ObjectFromHandle(SG_OBJ_HANDLE objH)
		{
			if (fVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}

			lpOBJ pobj = reinterpret_cast<lpOBJ>(fVoid);
			SG_OBJ_HANDLE objH = fVoid;

			switch (pobj->type)
			{
			case OPOINT:
				return new sgCPoint(objH);
			case OLINE:
				return new sgCLine(objH);
			case OCIRCLE:
				return new sgCCircle(objH);
			case OARC:
				return new sgCArc(objH);
			case OSPLINE:
				return new sgCSpline(objH);
			case OTEXT:
				return new sgCText(objH);
			case ODIM:
				return new sgCDimensions(objH);
			case OPATH:
				{
					short	 pathtype;
					lpCSG	 csg;

					if (pobj->hcsg) 
					{
						csg = (lpCSG)(pobj->hcsg);
						pathtype = *((short*)(LPSTR)csg->data);
						if (pathtype==RECTANGLE)
							return new sgCContour(objH);
						else if (pathtype==POLYGON)
							return new sgCContour(objH);
						else
							return new sgCContour(objH);//  
					} 
					else 
						return new sgCContour(objH);//  
				}
			case OBREP:
				{
					lpGEO_BREP 	geo;
					LNP		  	lnp;
					int		  	num;

					geo = (lpGEO_BREP)(pobj->geo_data);
					num = geo->num;
					if ( !(read_elem(&vd_brep,num,&lnp)) )
						return new sgC3DObject(objH);
					switch ( lnp.kind ) 
					{
					case COMMON:
						return new sgC3DObject(objH); //  BREP
					case BOX:
						return new sgCBox(objH);
					case SPHERE:
						return new sgCSphere(objH);
					case CYL:
						return new sgCCylinder(objH);
					case CONE:
						return new sgCCone(objH);
					case TOR:
						return new sgCTorus(objH);
					case PRISM:
					case PYRAMID:
						return new sgC3DObject(objH);
					case ELIPSOID:
						return new sgCEllipsoid(objH);
					case SPHERIC_BAND:
						return new sgCSphericBand(objH);
					default:
						return new sgC3DObject(objH); //  BREP
					}
				}
			case OGROUP:
				return new sgCGroup(objH);
			default:
				return NULL;
			}
			return NULL;
		}
		break;
	case 1984: // void  DeleteExtendedClass(SG_OBJ_HANDLE objH)
		{
			if (fVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			lpOBJ obj = (lpOBJ)fVoid;
			if (obj->extendedClass)
			{
				delete  obj->extendedClass;
				obj->extendedClass = NULL;
			}
			return NULL;
		}
		break;
	case 1985: // void  AllocMemoryForBRepPiecesInBRep
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCBRep* brep = (sgCBRep*)fVoid;
			int      number_of_pieces = *((int*)(sVoid));
			if (brep->m_pieces!=NULL || brep->m_pieces_count>0 || number_of_pieces<=0)
			{
				assert(0);
				return NULL;
			}
			brep->m_pieces = (sgCBRepPiece**)SGMalloc(number_of_pieces*sizeof(sgCBRepPiece*));
			brep->m_pieces_count = (unsigned int)(number_of_pieces);
			memset(brep->m_pieces,NULL,number_of_pieces*sizeof(sgCBRepPiece*));
			for (int ii=0;ii<number_of_pieces;ii++)
			{
				brep->m_pieces[ii] = new sgCBRepPiece;
			}
			return NULL;
		}
		break;
	case 1986: // void  Set_i_BRepPiece
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			TMP_STRUCT_FOR_BREP_PIECES_SETTING* i_brep_piece = (TMP_STRUCT_FOR_BREP_PIECES_SETTING*)fVoid;
			lpNPW tmpNPW = (lpNPW)(sVoid);
			if (i_brep_piece->br->m_pieces[i_brep_piece->ii]==NULL)
			{
				assert(0);
				return NULL;
			}
			memcpy(&(i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_min),&(tmpNPW->gab.min),sizeof(SG_POINT));
			memcpy(&(i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_max),&(tmpNPW->gab.max),sizeof(SG_POINT));

			i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_vertexes_count = tmpNPW->nov;
			i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_vertexes = (SG_POINT*)SGMalloc(tmpNPW->nov*sizeof(SG_POINT));
			memcpy(i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_vertexes,tmpNPW->v+1,tmpNPW->nov*sizeof(SG_POINT));

			i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_edges_count = tmpNPW->noe;
			i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_edges = (SG_EDGE*)SGMalloc(tmpNPW->noe*sizeof(SG_EDGE));
			for (short i=0;i<tmpNPW->noe;i++)
			{
				i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_edges[i].edge_type = tmpNPW->efr[i+1].el;
				i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_edges[i].begin_vertex_index = tmpNPW->efr[i+1].bv-1;
				i_brep_piece->br->m_pieces[i_brep_piece->ii]->m_edges[i].end_vertex_index = tmpNPW->efr[i+1].ev-1;
			}
			return NULL;
		}
		break;
	case 1987:  // for GetNPWFromBRepPiece
		{
			if (fVoid==NULL || sVoid!=NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCBRepPiece* brP = (sgCBRepPiece*)fVoid;
			return brP->m_brep_piece_handle;
		}
		break;
	case 1988:  // for Set3DObjectType
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgC3DObject* obP = (sgC3DObject*)fVoid;
			SG_3DOBJECT_TYPE obT = *((SG_3DOBJECT_TYPE*)sVoid);
			obP->m_objectType = obT;
			return NULL;
		}
		break;
	case 1989:  // for SetBRepPieceMinTriangleNumber
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCBRepPiece* br_p = (sgCBRepPiece*)fVoid;
			int           min_n = *((int*)sVoid);
			br_p->m_min_triangle_number = min_n;
			return NULL;
		}
		break;
	case 1990:  // for SetBRepPieceMaxTriangleNumber
		{
			if (fVoid==NULL || sVoid==NULL)
			{
				global_sg_error = SG_ER_INTERNAL;
				return NULL;
			}
			sgCBRepPiece* br_p = (sgCBRepPiece*)fVoid;
			int           max_n = *((int*)sVoid);
			br_p->m_max_triangle_number = max_n;
			return NULL;
		}
		break;
	default:
		assert(0);
	}
	global_sg_error = SG_ER_INTERNAL;
	return NULL;
}

SG_OBJ_HANDLE   GetObjectHandle(const sgCObject* obj)
{
	return sgPrivateAccess(1980,const_cast<sgCObject*>(obj),NULL);
}

void         SetObjectHandle(sgCObject* ob,SG_OBJ_HANDLE hndl)
{
	sgPrivateAccess(1981,ob,hndl);
}

sgCObject*   ObjectFromHandle(SG_OBJ_HANDLE objH)
{
	return reinterpret_cast<sgCObject*>(sgPrivateAccess(1983,objH,NULL));
}

void  DeleteExtendedClass(void* obj)
{
	sgPrivateAccess(1984,obj,NULL);
}

void  AllocMemoryForBRepPiecesInBRep(sgCBRep* br,int number_all)
{
	int nA = number_all;
	assert(br);
	assert(number_all>0);
	sgPrivateAccess(1985,br,&nA);
}

void  Set_i_BRepPiece(TMP_STRUCT_FOR_BREP_PIECES_SETTING* tmpSFBPS,lpNPW tmpNPW)
{
	if (tmpSFBPS==NULL || tmpSFBPS->br==NULL || tmpNPW==NULL)
	{
		assert(0);
		return;
	}
	sgPrivateAccess(1986,tmpSFBPS,tmpNPW);
}

lpNPW        GetNPWFromBRepPiece(sgCBRepPiece* brP)
{
	if (brP==NULL)
	{
		assert(0);
		return NULL;
	}
	return (lpNPW)(sgPrivateAccess(1987, brP, NULL));
}

void         Set3DObjectType(sgC3DObject* obj_3D, SG_3DOBJECT_TYPE obj_type)
{
	if (obj_3D==NULL)
	{
		assert(0);
		return;
	}
	sgPrivateAccess(1988,obj_3D,&obj_type);
}

void         SetBRepPieceMinTriangleNumber(sgCBRepPiece* br_p, int min_num)
{
	if (br_p==NULL || min_num<0)
	{
		assert(0);
		return;
	}
	sgPrivateAccess(1989,br_p,&min_num);
}

void         SetBRepPieceMaxTriangleNumber(sgCBRepPiece* br_p, int max_num)
{
	if (br_p==NULL || max_num<0)
	{
		assert(0);
		return;
	}
	sgPrivateAccess(1990,br_p,&max_num);
}


bool		 sgSetApplicationInterface(ICoreAppInterface* app_int)
{
	if (application_interface==NULL)
	{
		application_interface = app_int;
		return true;
	}
	return false;
}

void  sgSetToObjectTreeNodeHandle(sgCObject* obj,  ISceneTreeControl::TREENODEHANDLE tnh)
{
	((lpOBJ)(GetObjectHandle(obj)))->handle_of_tree_node = tnh;
}

ISceneTreeControl::TREENODEHANDLE  sgGetFromObjectTreeNodeHandle(sgCObject* obj)
{
	return ((lpOBJ)(GetObjectHandle(obj)))->handle_of_tree_node;
}

