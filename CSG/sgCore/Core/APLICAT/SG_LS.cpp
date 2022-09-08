#include "../sg.h"


BOOL SGLoadModel(char* name, lpD_POINT p_min, lpD_POINT p_max,
								  MATR matr, lpLISTH listh)
{
	BOOL				cod = FALSE;
	short				count;
	BUFFER_DAT	bd;
	hOBJ				hobj;
  UCHAR       Header_Version;


	init_listh(&selected);
  /*if(pProp)
    ScenePropSetPath(pProp, name);*/

	if (nb_access(name,4)) {
		put_message(NOT_ACCESS_FILE, name, 0);
		return FALSE;
	}
	if ( !init_buf(&bd,name,BUF_OLD) )	return FALSE;
	if ( !SGLoadHeader(&bd, p_min, p_max, /*pProp,*/ &Header_Version) )	goto err;
	if (matr) f3d_gabar_project1(matr, p_min, p_max, p_min, p_max);
	if ( !o_load_list(&bd, &selected, SEL_LIST, &count, Header_Version,
                    /*((Header_Version>=VER_HEAD_1_4) ?*/ NULL /*: pProp)*/))	goto err;
	if ( file_unit != sgw_unit ) {
		sgFloat	tmp;
		MATR		matr;
		tmp = file_unit / sgw_unit;
		o_hcunit(matr);
		o_scale(matr,tmp);
		f3d_gabar_project1(matr, p_min, p_max, p_min, p_max);
	}
	cod = TRUE;
err:
	if ( !close_buf(&bd) ) cod = FALSE;
	hobj = selected.hhead;
	while (hobj) {
		if (matr) cod = o_trans(hobj, matr);
		attach_item_tail(listh, hobj);
		get_next_item_z(SEL_LIST, hobj, &hobj);
	}
	init_listh(&selected);	
	return cod;
}

//
//
//
BOOL SGLoadHeader(lpBUFFER_DAT bd, lpD_POINT p_min, lpD_POINT p_max, UCHAR* Header_Version)
{
//WORD      	 len;
//SCENE_ACCESS Access;

	{

			sgFileManager::SG_FILE_HEADER tmpFH;
			
			if (load_data(bd,sizeof(sgFileManager::SG_FILE_HEADER),&tmpFH)!=sizeof(sgFileManager::SG_FILE_HEADER) )  
				goto err;

			if (strcmp(tmpFH.signature,SG_FILE_SIGNATURE)!=0)
				goto err;

			if (!seek_buf(bd,sizeof(sgFileManager::SG_FILE_HEADER)+tmpFH.userBlockSize))
				goto err;

		}
		
	if ( load_data(bd, sizeof(*p_min), p_min) != sizeof(*p_min) ) goto err;
	if ( load_data(bd, sizeof(*p_max), p_max) != sizeof(*p_max) ) goto err;
	if (p_min->x > p_max->x) {
		p_min->x = p_min->y = p_min->z = -100;
		p_max->x = p_max->y = p_max->z =  100;
	}
	return TRUE;
err:
  /*if(!SPropErrMsgCode)
    SPropErrMsgCode = UNKNOWN_FILE; 
	put_message(SPropErrMsgCode,	NULL, 0);*/
	return FALSE;
}

//
//
//
bool sg_save_model(char *name, BOOL sel_flag, int *count, 
				   MATR matr, MATR matr_inv, 
				   const void* userData, unsigned long userDataSize)
{
	bool				ret = false;
	BUFFER_DAT 	bd;
	D_POINT			mn, mx, mn1, mx1;
	hOBJ				hobj;
	NUM_LIST		num_list;
	lpLISTH			listh;
//	short				len;

	mn.x = mn.y = mn.z = 	 GAB_NO_DEF;
	mx.x = mx.y = mx.z = - GAB_NO_DEF;
	if (sel_flag) {
		listh = &selected;
		num_list = SEL_LIST;
	} else {
		listh = &objects;
		num_list = OBJ_LIST;
	}
	hobj = listh->hhead;
	while (hobj) {
		if ( matr ) o_trans(hobj, matr);
		if (!get_object_limits(hobj, &mn1, &mx1)) return FALSE;
		if ( matr ) o_trans(hobj, matr_inv);
		union_limits(&mn1, &mx1, &mn, &mx);
		get_next_item_z(num_list, hobj, &hobj);
	}
	if (mn.x > mx.x) {
		mn.x = mn.y = mn.z = -100.;
		mx.x = mx.y = mx.z =  100.;
	}

	if (!init_buf(&bd,name,BUF_NEW)) return FALSE;
	{

		sgFileManager::SG_FILE_HEADER tmpFH;
		strcpy(tmpFH.signature,SG_FILE_SIGNATURE);
		tmpFH.major_ver = KERNEL_VERSION_MAJOR;
		tmpFH.minor_ver = KERNEL_VERSION_MINOR;
		if (userData==NULL)
			tmpFH.userBlockSize = 0;
		else
			tmpFH.userBlockSize = userDataSize;
		if (!story_data(&bd,sizeof(sgFileManager::SG_FILE_HEADER),&tmpFH))  goto err;

		if (userData && (userDataSize>0))
			if (!story_data(&bd,userDataSize,const_cast<void*>(userData)))  goto err;

	}
	
	if (!story_data(&bd,sizeof(D_POINT),&mn))  goto err;
	if (!story_data(&bd,sizeof(D_POINT),&mx))  goto err;

	if (!o_save_list(&bd, listh, num_list, count, matr, matr_inv)) goto err;

	ret = TRUE;
err:
	if (!close_buf(&bd)) ret = FALSE;
 	return ret;
}

