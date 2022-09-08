#include "..//Core//sg.h"

#include <vector>

static   std::vector<SG_LINE>    lines_buffer;

static BOOL  lines_collector_func(lpD_POINT p1, lpD_POINT p2, void* us_d)
{
	SG_LINE  tmpL;
	assert(p1);
	assert(p2);
	memcpy(&tmpL.p1,p1,sizeof(D_POINT));
	memcpy(&tmpL.p2,p2,sizeof(D_POINT));
	lines_buffer.push_back(tmpL);
	return TRUE;
}

static  void  FillLinesArrayfromGeoDim(lpGEO_DIM geoD)
{
	lines_buffer.clear();
	assert(geoD);
	draw_geo_dim(geoD,lines_collector_func,NULL);
}


sgCDimensions::sgCDimensions():sgCObject()
{
	m_lines = NULL;
	m_lines_count=0;
	m_text = NULL;
	m_formed_points = NULL;
}

sgCDimensions::~sgCDimensions()
{
	if (m_text)
	{
		SGFree(m_text);
		m_text = NULL;
	}
	if (m_lines)
	{
		delete [] m_lines;
		m_lines = NULL;
		m_lines_count=0;
	}
	if (m_formed_points)
	{
		delete [] m_formed_points;
		m_formed_points = NULL;
	}
	lines_buffer.clear();
}

sgCDimensions::sgCDimensions(SG_OBJ_HANDLE objH):sgCObject(objH)
{
	m_lines = NULL;
	m_lines_count=0;
	m_text = NULL;
	m_formed_points = NULL;

	char* typeID = "{0000000000000-0000-0000-000000000008}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	lpOBJ pobj = (lpOBJ)objH;
	
	FillLinesArrayfromGeoDim(reinterpret_cast<lpGEO_DIM>(pobj->geo_data));

	size_t lbs = lines_buffer.size();
	if (lbs>0)
	{
		m_lines_count = lbs;
		m_lines = new SG_LINE[lbs];
		memcpy(m_lines, &lines_buffer[0], sizeof(SG_LINE)*m_lines_count);
	}
	lines_buffer.clear();

	PostCreate();

}


bool	sgCDimensions::ApplyTempMatrix()
{
	if (!sgCObject::ApplyTempMatrix())
		return false;
	if (m_lines)
	{
		delete [] m_lines;
		m_lines = NULL;
		m_lines_count=0;
	}
	lpOBJ pobj = (lpOBJ)GetObjectHandle(this);

	FillLinesArrayfromGeoDim(reinterpret_cast<lpGEO_DIM>(pobj->geo_data));

	size_t lbs = lines_buffer.size();
	if (lbs>0)
	{
		m_lines_count = lbs;
		m_lines = new SG_LINE[lbs];
		memcpy(m_lines, &lines_buffer[0], sizeof(SG_LINE)*m_lines_count);
	}
	lines_buffer.clear();

	if (m_formed_points)
	{
		GEO_DIM  geom;
		get_simple_obj_geo(GetObjectHandle(this),&geom);

			switch(geom.type) 
			{
			case DM_LIN:
			case DM_RAD:
			case DM_DIAM:
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[0]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[1]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[2]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[3]);
				break;
			case DM_ANG:
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[0]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[1]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[2]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[3]);
				GetTempMatrix()->ApplyMatrixToPoint(m_formed_points[4]);
				break;
			default:
				assert(0);
				return false;
			}
	}

	return true;
}

const    SG_LINE*   sgCDimensions::GetLines() const
{
	return  m_lines;
}

int  sgCDimensions::GetLinesCount() const
{
	return m_lines_count;
}

const SG_POINT*  sgCDimensions::GetFormedPoints()
{
	if (m_formed_points)
		return m_formed_points;


	GEO_DIM  geom;
	get_simple_obj_geo(GetObjectHandle(this),&geom);

	sgCMatrix mmm(geom.matr);
	switch(geom.type) 
	{
	case DM_LIN:
	case DM_RAD:
	case DM_DIAM:
		m_formed_points = new SG_POINT[4];
		memcpy(m_formed_points, geom.p, 4*sizeof(SG_POINT));
		mmm.ApplyMatrixToPoint(m_formed_points[0]);
		mmm.ApplyMatrixToPoint(m_formed_points[1]);
		mmm.ApplyMatrixToPoint(m_formed_points[2]);
		mmm.ApplyMatrixToPoint(m_formed_points[3]);
		return m_formed_points;
	case DM_ANG:
		m_formed_points = new SG_POINT[5];
		memcpy(m_formed_points, geom.p, 5*sizeof(SG_POINT));
		mmm.ApplyMatrixToPoint(m_formed_points[0]);
		mmm.ApplyMatrixToPoint(m_formed_points[1]);
		mmm.ApplyMatrixToPoint(m_formed_points[2]);
		mmm.ApplyMatrixToPoint(m_formed_points[3]);
		mmm.ApplyMatrixToPoint(m_formed_points[4]);
		return m_formed_points;
	default:
		assert(0);
		return NULL;
	}
}

SG_DIMENSION_TYPE  sgCDimensions::GetType() const
{
	GEO_DIM  geom;
	get_simple_obj_geo(GetObjectHandle(this),&geom);

	switch(geom.type) 
	{
	case DM_LIN:
		return SG_DT_LINEAR;
	case DM_ANG:
		return SG_DT_ANGLE;
	case DM_RAD:
		return SG_DT_RAD;
	case DM_DIAM:
		return SG_DT_DIAM;
	default:
		assert(0);
		return SG_DT_LINEAR;
	}
}

SG_DIMENSION_STYLE sgCDimensions::GetStyle() const
{
	SG_DIMENSION_STYLE retRes;
	GEO_DIM  geom;
	get_simple_obj_geo(GetObjectHandle(this),&geom);

    // was - RA
	//memcpy(&(retRes.text_style), &geom.dtstyle.tstyle, sizeof(TSTYLE));
    
    // new - RA
    retRes.text_style.state = geom.dtstyle.tstyle.status;
    retRes.text_style.height = geom.dtstyle.tstyle.height;
    retRes.text_style.proportions = geom.dtstyle.tstyle.scale;
    retRes.text_style.angle = geom.dtstyle.tstyle.angle;
    retRes.text_style.horiz_space_proportion = geom.dtstyle.tstyle.dhor;
    retRes.text_style.vert_space_proportion = geom.dtstyle.tstyle.dver;

	retRes.arrows_size = geom.dgstyle.arrlen;
	retRes.automatic_arrows = (geom.dgstyle.flags & ARR_AUTO)?true:false;
	retRes.dimension_line = (geom.dgstyle.flags & DIM_LN_NONE)?false:true;
	retRes.first_arrow_style = geom.dgstyle.arrtype[0];
	retRes.first_side_line = (geom.dgstyle.flags & EXT1_NONE)?false:true;
	retRes.invert = (geom.neg==1);
	retRes.lug_size  = geom.dgstyle.extln_out;
	retRes.out_first_arrow = (retRes.automatic_arrows)?false:((geom.dgstyle.flags & ARR1_OUT)?true:false);
	retRes.out_second_arrow = (retRes.automatic_arrows)?false:((geom.dgstyle.flags & ARR2_OUT)?true:false);

	retRes.precision = 8;
	
	if (geom.dtstyle.snap==0.1)
		retRes.precision = 1;
	if (geom.dtstyle.snap==0.01)
		retRes.precision = 2;
	if (geom.dtstyle.snap==0.001)
		retRes.precision = 3;
	if (geom.dtstyle.snap==0.0001)
		retRes.precision = 4;
	if (geom.dtstyle.snap==0.00001)
		retRes.precision = 5;
	if (geom.dtstyle.snap==0.000001)
		retRes.precision = 6;
	if (geom.dtstyle.snap==0.00000001)
		retRes.precision = 7;
	

	retRes.second_arrow_style = geom.dgstyle.arrtype[1];
	retRes.second_side_line = (geom.dgstyle.flags & EXT2_NONE)?false:true;

	switch(geom.dgstyle.tplace) 
	{
		case DTP_CENTER:
			retRes.text_align = SG_TA_CENTER;
			break;
		case DTP_LEFT:
			retRes.text_align = SG_TA_LEFT;
			break;
		case DTP_RIGHT:
			retRes.text_align = SG_TA_RIGHT;
			break;
		default:
			retRes.text_align = SG_TA_CENTER;
	}
	
	return retRes;
}

const char*  sgCDimensions::GetText()
{
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);

	if (m_text!=NULL)
		return m_text;

	GEO_DIM  geom;
	get_simple_obj_geo(GetObjectHandle(this),&geom);

	IDENT_V    id = geom.dtstyle.text;
	ULONG      len;

	if(0 ==(m_text = (char*)alloc_and_get_ft_value(id, &len))) 
	{
		assert(0);
		return NULL;
	}
	return m_text;
}

unsigned int sgCDimensions::GetFont()
{
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);

	GEO_DIM  geom;
	get_simple_obj_geo(GetObjectHandle(this),&geom);

	IDENT_V    id;
	//    
	id = ft_bd->ft_listh[FTFONT].head;
	unsigned int res = 0;
	while(id)
	{
		if (id==geom.dtstyle.font)
			return res;
		if(!il_get_next_item(&ft_bd->vd, id, &id))
		{
			assert(0);
			return 0;
		}
		res++;
	}
	return res;
}


static  SG_DRAW_LINE_FUNC   drLF = NULL;
static  sgCMatrix*  mmm=NULL;

static BOOL  lineFnkForCore(lpD_POINT pb, lpD_POINT pe, void *usrD)
{
	assert(drLF);
	SG_POINT begP = *(SG_POINT*)(pb);
	SG_POINT endP = *(SG_POINT*)(pe);
	if (mmm)
	{
		mmm->ApplyMatrixToPoint(begP);
		mmm->ApplyMatrixToPoint(endP);
	}
	drLF(&begP,&endP);
	return TRUE;
}

bool    sgCDimensions::Draw(SG_DIMENSION_TYPE dimType, const SG_POINT* points,
									 const sgCFont* fnt,
									 const SG_DIMENSION_STYLE& dim_st,
									 const char* strng,
									 SG_DRAW_LINE_FUNC dlf)
{
	if (points==NULL || fnt==NULL || dlf==NULL)
		return false;

	GEO_DIM  geo;
	memset(&geo, 0, sizeof(GEO_DIM));
	switch(dimType) 
	{
	case SG_DT_LINEAR:
		geo.type = DM_LIN;
		break;
	case SG_DT_ANGLE:
		geo.type = DM_ANG;
		break;
	case SG_DT_RAD:
		geo.type = DM_RAD;
		break;
	case SG_DT_DIAM:
		geo.type = DM_DIAM;
		break;
	default:
		assert(0);
		return false;
	}
	geo.dtstyle = dim_info->dts[geo.type];
	geo.dgstyle = dim_info->dgs;
	
	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());
	drLF = dlf;
	short errr = 0;

	short ldtype=1;

	if (dimType==SG_DT_LINEAR)
	{
			switch(dim_st.behaviour_type) 
			{
			case SG_DBT_VERTICAL:
				ldtype = LDIM_VERT;
				break;
			case SG_DBT_HORIZONTAL:
				ldtype = LDIM_HOR;
				break;
			case SG_DBT_PARALLEL:
				ldtype = LDIM_PAR;
				break;
			case SG_DBT_SLANT:
				ldtype = LDIM_ANG;
				break;
			case SG_DBT_OPTIMAL:
				ldtype = LDIM_FREE;
				break;
			default:
				ldtype = LDIM_PAR;
			}
	}

	//geo.dtstyle = dim_info->dts[DM_LIN];
	memcpy(&geo.dtstyle.tstyle,&dim_st.text_style,sizeof(TSTYLE));
	geo.dtstyle.font = fnt->m_id;

	//geo.dgstyle = dim_info->dgs;
	geo.dgstyle.extln_out = dim_st.lug_size;
	if (dim_st.dimension_line)
		geo.dgstyle.flags &= ~DIM_LN_NONE;
	else
		geo.dgstyle.flags |= DIM_LN_NONE;
	if (dim_st.first_side_line)
		geo.dgstyle.flags &= ~EXT1_NONE;
	else
		geo.dgstyle.flags |= EXT1_NONE;
	if (dim_st.second_side_line)
		geo.dgstyle.flags &= ~EXT2_NONE;
	else
		geo.dgstyle.flags |= EXT2_NONE;

	if (!dim_st.automatic_arrows)
	{
		geo.dgstyle.flags &= ~ARR_AUTO;

		if (!dim_st.out_first_arrow)
			geo.dgstyle.flags &= ~ARR1_OUT;
		else
			geo.dgstyle.flags |= ARR1_OUT;

		if (!dim_st.out_second_arrow)
			geo.dgstyle.flags &= ~ARR2_OUT;
		else
			geo.dgstyle.flags |= ARR2_OUT;
	}
	else
		geo.dgstyle.flags |= ARR_AUTO;

	geo.dgstyle.arrtype[0] = dim_st.first_arrow_style;
	geo.dgstyle.arrtype[1] = dim_st.second_arrow_style;

	geo.dgstyle.arrlen = dim_st.arrows_size;

	geo.dgstyle.flags &= (~DTP_AUTO);
	geo.dgstyle.flags |= ARR_DEFINE;

	switch(dim_st.text_align) 
	{
	case SG_TA_CENTER:
		geo.dgstyle.tplace = DTP_CENTER;
		break;
	case SG_TA_LEFT:
		geo.dgstyle.tplace = DTP_LEFT;
		break;
	case SG_TA_RIGHT:
		geo.dgstyle.tplace = DTP_RIGHT;
		break;
	default:
		geo.dgstyle.tplace = DTP_CENTER;
	}

	if (dim_st.precision<1)
		geo.dtstyle.snap = 0.1;
	else
		if (dim_st.precision>8)
			geo.dtstyle.snap = 0.00001;
		else
		{
			switch(dim_st.precision) 
			{
			case 1:
				geo.dtstyle.snap = 0.1;
				break;
			case 2:
				geo.dtstyle.snap = 0.01;
				break;
			case 3:
				geo.dtstyle.snap = 0.001;
				break;
			case 4:
				geo.dtstyle.snap = 0.0001;
				break;
			case 5:
				geo.dtstyle.snap = 0.00001;
				break;
			case 6:
				geo.dtstyle.snap = 0.000001;
				break;
			case 7:
				geo.dtstyle.snap = 0.00000001;
				break;
			default:
				geo.dtstyle.snap = 0.000000001;
			}
		}

	if (dim_st.invert && dimType==SG_DT_ANGLE)
		geo.neg = 1;
	else
		geo.neg = 0;


	unsigned char  My_shablon[] = {'<','>',0};
	unsigned char  My_R_shablon[] = {'R','<','>',0};
	unsigned char  My_D_shablon[] = {'%','%','c','<','>',0};

	bool   isR = (dimType==SG_DT_RAD);
	bool   isD = (dimType==SG_DT_DIAM);

	set_dim_geo(&geo,ldtype,fntHndl, 
		(strng==NULL)?((isR)?My_R_shablon:((isD)?My_D_shablon:My_shablon)):const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
			(lpD_POINT)(points),(lpD_POINT)(points+1),
			(dimType!=SG_DT_ANGLE)?NULL:((lpD_POINT)(points+2)), 
			(dimType!=SG_DT_ANGLE)?NULL:((lpD_POINT)(points+3)),
			(dimType!=SG_DT_ANGLE)?((lpD_POINT)(points+2)):((lpD_POINT)(points+4)),
			&errr);
	
	mmm = new sgCMatrix(geo.matr);

	draw_dim1(&geo,fntHndl,	
		(strng==NULL)?((isR)?My_R_shablon:((isD)?My_D_shablon:My_shablon)):const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)), 
		lineFnkForCore, NULL);
	delete mmm;
	mmm = NULL;
	drLF = NULL;
	return true;
}

sgCDimensions* sgCDimensions::Create(SG_DIMENSION_TYPE dimType, const SG_POINT* points,
									 const sgCFont* fnt,
									 const SG_DIMENSION_STYLE& dim_st,
									 const char* strng)
{
	if (points==NULL || fnt==NULL )
		return NULL;

	GEO_DIM  geo;
	memset(&geo, 0, sizeof(GEO_DIM));
	switch(dimType) 
	{
	case SG_DT_LINEAR:
		geo.type = DM_LIN;
		break;
	case SG_DT_ANGLE:
		geo.type = DM_ANG;
		break;
	case SG_DT_RAD:
		geo.type = DM_RAD;
		break;
	case SG_DT_DIAM:
		geo.type = DM_DIAM;
		break;
	default:
		assert(0);
		return NULL;
	}
	geo.dtstyle = dim_info->dts[geo.type];

	geo.dgstyle = dim_info->dgs;

	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());
	
	short errr = 0;

	short ldtype=1;

	if (dimType==SG_DT_LINEAR)
	{
			switch(dim_st.behaviour_type) 
			{
			case SG_DBT_VERTICAL:
				ldtype = LDIM_VERT;
				break;
			case SG_DBT_HORIZONTAL:
				ldtype = LDIM_HOR;
				break;
			case SG_DBT_PARALLEL:
				ldtype = LDIM_PAR;
				break;
			case SG_DBT_SLANT:
				ldtype = LDIM_ANG;
				break;
			case SG_DBT_OPTIMAL:
				ldtype = LDIM_FREE;
				break;
			default:
				ldtype = LDIM_PAR;
			}
	}

	//geo.dtstyle = dim_info->dts[DM_LIN];
    // RA - was
	//memcpy(&geo.dtstyle.tstyle,&dim_st.text_style,sizeof(TSTYLE));
    
    // RA - new
    geo.dtstyle.tstyle.status = dim_st.text_style.state;
    geo.dtstyle.tstyle.height = dim_st.text_style.height;
    geo.dtstyle.tstyle.scale = dim_st.text_style.proportions;
    geo.dtstyle.tstyle.angle = dim_st.text_style.angle;
    geo.dtstyle.tstyle.dhor = dim_st.text_style.horiz_space_proportion;
    geo.dtstyle.tstyle.dver = dim_st.text_style.vert_space_proportion;
    
	geo.dtstyle.font = fnt->m_id;

	if (strng!=NULL)
	{
		IDENT_V    id;
		ULONG      len;
		UCHAR      *exist_txt=NULL;
		bool       exist_this_text = false;

		id = ft_bd->ft_listh[FTTEXT].head;

		while(id){
			if(0 ==(exist_txt = (UCHAR *)alloc_and_get_ft_value(id, &len))) 
				return NULL;
			if(!strcmp((char*)exist_txt, strng))
			{
				SGFree(exist_txt);
				geo.dtstyle.text = id;
				exist_this_text = true;
				break;
			}
			SGFree(exist_txt); exist_txt = NULL;
			if(!il_get_next_item(&ft_bd->vd, id, &id)) 
				return NULL;
		}

		if (!exist_this_text)
			geo.dtstyle.text = add_ft_value(FTTEXT, const_cast<char*>(strng), strlen(strng)+1);
		if(!geo.dtstyle.text) 
			return NULL;
	}

	//geo.dgstyle = dim_info->dgs;
	geo.dgstyle.extln_out = dim_st.lug_size;
	if (dim_st.dimension_line)
		geo.dgstyle.flags &= ~DIM_LN_NONE;
	else
		geo.dgstyle.flags |= DIM_LN_NONE;
	if (dim_st.first_side_line)
		geo.dgstyle.flags &= ~EXT1_NONE;
	else
		geo.dgstyle.flags |= EXT1_NONE;
	if (dim_st.second_side_line)
		geo.dgstyle.flags &= ~EXT2_NONE;
	else
		geo.dgstyle.flags |= EXT2_NONE;

	if (!dim_st.automatic_arrows)
	{
		geo.dgstyle.flags &= ~ARR_AUTO;

		if (!dim_st.out_first_arrow)
			geo.dgstyle.flags &= ~ARR1_OUT;
		else
			geo.dgstyle.flags |= ARR1_OUT;

		if (!dim_st.out_second_arrow)
			geo.dgstyle.flags &= ~ARR2_OUT;
		else
			geo.dgstyle.flags |= ARR2_OUT;
	}
	else
		geo.dgstyle.flags |= ARR_AUTO;

	geo.dgstyle.arrtype[0] = dim_st.first_arrow_style;
	geo.dgstyle.arrtype[1] = dim_st.second_arrow_style;

	geo.dgstyle.arrlen = dim_st.arrows_size;

	geo.dgstyle.flags &= (~DTP_AUTO);
	geo.dgstyle.flags |= ARR_DEFINE;
	switch(dim_st.text_align) 
	{
	case SG_TA_CENTER:
		geo.dgstyle.tplace = DTP_CENTER;
		break;
	case SG_TA_LEFT:
		geo.dgstyle.tplace = DTP_LEFT;
		break;
	case SG_TA_RIGHT:
		geo.dgstyle.tplace = DTP_RIGHT;
		break;
	default:
		geo.dgstyle.tplace = DTP_CENTER;
	}

	if (dim_st.precision<1)
		geo.dtstyle.snap = 0.1;
	else
		if (dim_st.precision>8)
			geo.dtstyle.snap = 0.00001;
		else
		{
			switch(dim_st.precision) 
			{
			case 1:
				geo.dtstyle.snap = 0.1;
				break;
			case 2:
				geo.dtstyle.snap = 0.01;
				break;
			case 3:
				geo.dtstyle.snap = 0.001;
				break;
			case 4:
				geo.dtstyle.snap = 0.0001;
				break;
			case 5:
				geo.dtstyle.snap = 0.00001;
				break;
			case 6:
				geo.dtstyle.snap = 0.000001;
				break;
			case 7:
				geo.dtstyle.snap = 0.00000001;
				break;
			default:
				geo.dtstyle.snap = 0.000000001;
			}
		}

		if (dim_st.invert && dimType==SG_DT_ANGLE)
			geo.neg = 1;
		else
			geo.neg = 0;

	unsigned char  My_shablon[] = {'<','>',0};

	unsigned char  My_R_shablon[] = {'R','<','>',0};
	unsigned char  My_D_shablon[] = {'%','%','c','<','>',0};

	bool   isR = (dimType==SG_DT_RAD);
	bool   isD = (dimType==SG_DT_DIAM);

	set_dim_geo(&geo,ldtype,
		fntHndl, 
		(strng==NULL)?((isR)?My_R_shablon:((isD)?My_D_shablon:My_shablon)):const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
		(lpD_POINT)(points),(lpD_POINT)(points+1),
		(dimType!=SG_DT_ANGLE)?NULL:((lpD_POINT)(points+2)), 
		(dimType!=SG_DT_ANGLE)?NULL:((lpD_POINT)(points+3)),
		(dimType!=SG_DT_ANGLE)?((lpD_POINT)(points+2)):((lpD_POINT)(points+4)),
		&errr);

	hOBJ hO = create_simple_obj(ODIM, &geo); 
	if (hO==NULL)
		return NULL;

	sgCDimensions*   newD=new sgCDimensions(hO);

	char* typeID = "{0000000000000-0000-0000-000000000008}";
	set_hobj_attr_value(id_TypeID, hO, typeID);
	
	newD->PostCreate();

	return  newD;
}
/*
void    sgCDimensions::DrawArcDimension(const SG_POINT& f_pnt,
									 const SG_POINT& s_pnt,
									 const SG_POINT& th_pnt,
									 const SG_POINT& four_pnt,
									 const SG_POINT& cur_pnt,
									 const sgCFont* fnt,
									 const SG_DIMENSION_STYLE& dim_st,
									 const char* strng,
									 SG_DRAW_LINE_FUNC dlf)
{
	GEO_DIM  geo;
	memset(&geo, 0, sizeof(GEO_DIM));
	geo.type    = DM_ANG;
	geo.dtstyle = dim_info->dts[DM_ANG];
	geo.dgstyle = dim_info->dgs;

	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());
	drLF = dlf;
	short errr = 0;

	//geo.dtstyle = dim_info->dts[DM_LIN];
	memcpy(&geo.dtstyle.tstyle,&dim_st.text_style,sizeof(TSTYLE));
	geo.dtstyle.font = fnt->m_id;

	//geo.dgstyle = dim_info->dgs;
	geo.dgstyle.extln_out = dim_st.lug_size;
	if (dim_st.dimension_line)
		geo.dgstyle.flags &= ~DIM_LN_NONE;
	else
		geo.dgstyle.flags |= DIM_LN_NONE;
	if (dim_st.first_side_line)
		geo.dgstyle.flags &= ~EXT1_NONE;
	else
		geo.dgstyle.flags |= EXT1_NONE;
	if (dim_st.second_side_line)
		geo.dgstyle.flags &= ~EXT2_NONE;
	else
		geo.dgstyle.flags |= EXT2_NONE;

	if (!dim_st.automatic_arrows)
	{
		geo.dgstyle.flags &= ~ARR_AUTO;

		if (!dim_st.out_first_arrow)
			geo.dgstyle.flags &= ~ARR1_OUT;
		else
			geo.dgstyle.flags |= ARR1_OUT;

		if (!dim_st.out_second_arrow)
			geo.dgstyle.flags &= ~ARR2_OUT;
		else
			geo.dgstyle.flags |= ARR2_OUT;
	}
	else
		geo.dgstyle.flags |= ARR_AUTO;

	geo.dgstyle.arrtype[0] = dim_st.first_arrow_style;
	geo.dgstyle.arrtype[1] = dim_st.second_arrow_style;

	geo.dgstyle.arrlen = dim_st.arrows_size;

	geo.dgstyle.flags &= (~DTP_AUTO);
	geo.dgstyle.flags |= ARR_DEFINE;

	switch(dim_st.text_align) 
	{
	case SG_TA_CENTER:
		geo.dgstyle.tplace = DTP_CENTER;
		break;
	case SG_TA_LEFT:
		geo.dgstyle.tplace = DTP_LEFT;
		break;
	case SG_TA_RIGHT:
		geo.dgstyle.tplace = DTP_RIGHT;
		break;
	default:
		geo.dgstyle.tplace = DTP_CENTER;
	}

	if (dim_st.precision<1)
		geo.dtstyle.snap = 0.1;
	else
		if (dim_st.precision>8)
			geo.dtstyle.snap = 0.00001;
		else
		{
			switch(dim_st.precision) 
			{
			case 1:
				geo.dtstyle.snap = 0.1;
				break;
			case 2:
				geo.dtstyle.snap = 0.01;
				break;
			case 3:
				geo.dtstyle.snap = 0.001;
				break;
			case 4:
				geo.dtstyle.snap = 0.0001;
				break;
			case 5:
				geo.dtstyle.snap = 0.00001;
				break;
			case 6:
				geo.dtstyle.snap = 0.000001;
				break;
			case 7:
				geo.dtstyle.snap = 0.00000001;
				break;
			default:
				geo.dtstyle.snap = 0.000000001;
			}
		}


		unsigned char  My_shablon[] = {'<','>',0};

	set_dim_geo(&geo,1,
		fntHndl, 
		(strng==NULL)?My_shablon:const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
		(lpD_POINT)&f_pnt,
		(lpD_POINT)&s_pnt,
		(lpD_POINT)&th_pnt,
		(lpD_POINT)&four_pnt,
		(lpD_POINT)&cur_pnt,
		&errr);
	mmm = new sgCMatrix(geo.matr);
	draw_dim1(&geo,fntHndl,	
		(strng==NULL)?My_shablon:const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)), 
		lineFnkForCore, NULL);
	delete mmm;
	mmm = NULL;
	drLF = NULL;
}

sgCDimensions*   sgCDimensions::CreateArcDimension(const SG_POINT& f_pnt,
												   const SG_POINT& s_pnt,
												   const SG_POINT& th_pnt,
												   const SG_POINT& four_pnt,
												   const SG_POINT& cur_pnt,
												   const sgCFont* fnt,
												   const SG_DIMENSION_STYLE& dim_st,
												   const char* strng)
{
	GEO_DIM  geo;
	memset(&geo, 0, sizeof(GEO_DIM));
	geo.type    = DM_ANG;
	geo.dtstyle = dim_info->dts[DM_ANG];
	geo.dgstyle = dim_info->dgs;

	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());

	short errr = 0;

	//geo.dtstyle = dim_info->dts[DM_LIN];
	memcpy(&geo.dtstyle.tstyle,&dim_st.text_style,sizeof(TSTYLE));
	geo.dtstyle.font = fnt->m_id;

	if (strng!=NULL)
	{
		IDENT_V    id;
		ULONG      len;
		UCHAR      *exist_txt=NULL;
		bool       exist_this_text = false;

		id = ft_bd->ft_listh[FTTEXT].head;

		while(id){
			if(0 ==(exist_txt = (UCHAR *)alloc_and_get_ft_value(id, &len))) 
				return NULL;
			if(!strcmp((char*)exist_txt, strng))
			{
				SGFree(exist_txt);
				geo.dtstyle.text = id;
				exist_this_text = true;
				break;
			}
			SGFree(exist_txt); exist_txt = NULL;
			if(!il_get_next_item(&ft_bd->vd, id, &id)) 
				return NULL;
		}

		if (!exist_this_text)
			geo.dtstyle.text = add_ft_value(FTTEXT, const_cast<char*>(strng), strlen(strng)+1);
		if(!geo.dtstyle.text) 
			return NULL;
	}

	//geo.dgstyle = dim_info->dgs;
	geo.dgstyle.extln_out = dim_st.lug_size;
	if (dim_st.dimension_line)
		geo.dgstyle.flags &= ~DIM_LN_NONE;
	else
		geo.dgstyle.flags |= DIM_LN_NONE;
	if (dim_st.first_side_line)
		geo.dgstyle.flags &= ~EXT1_NONE;
	else
		geo.dgstyle.flags |= EXT1_NONE;
	if (dim_st.second_side_line)
		geo.dgstyle.flags &= ~EXT2_NONE;
	else
		geo.dgstyle.flags |= EXT2_NONE;

	if (!dim_st.automatic_arrows)
	{
		geo.dgstyle.flags &= ~ARR_AUTO;

		if (!dim_st.out_first_arrow)
			geo.dgstyle.flags &= ~ARR1_OUT;
		else
			geo.dgstyle.flags |= ARR1_OUT;

		if (!dim_st.out_second_arrow)
			geo.dgstyle.flags &= ~ARR2_OUT;
		else
			geo.dgstyle.flags |= ARR2_OUT;
	}
	else
		geo.dgstyle.flags |= ARR_AUTO;

	geo.dgstyle.arrtype[0] = dim_st.first_arrow_style;
	geo.dgstyle.arrtype[1] = dim_st.second_arrow_style;

	geo.dgstyle.arrlen = dim_st.arrows_size;

	geo.dgstyle.flags &= (~DTP_AUTO);
	geo.dgstyle.flags |= ARR_DEFINE;
	switch(dim_st.text_align) 
	{
	case SG_TA_CENTER:
		geo.dgstyle.tplace = DTP_CENTER;
		break;
	case SG_TA_LEFT:
		geo.dgstyle.tplace = DTP_LEFT;
		break;
	case SG_TA_RIGHT:
		geo.dgstyle.tplace = DTP_RIGHT;
		break;
	default:
		geo.dgstyle.tplace = DTP_CENTER;
	}

	if (dim_st.precision<1)
		geo.dtstyle.snap = 0.1;
	else
		if (dim_st.precision>8)
			geo.dtstyle.snap = 0.00001;
		else
		{
			switch(dim_st.precision) 
			{
			case 1:
				geo.dtstyle.snap = 0.1;
				break;
			case 2:
				geo.dtstyle.snap = 0.01;
				break;
			case 3:
				geo.dtstyle.snap = 0.001;
				break;
			case 4:
				geo.dtstyle.snap = 0.0001;
				break;
			case 5:
				geo.dtstyle.snap = 0.00001;
				break;
			case 6:
				geo.dtstyle.snap = 0.000001;
				break;
			case 7:
				geo.dtstyle.snap = 0.00000001;
				break;
			default:
				geo.dtstyle.snap = 0.000000001;
			}
		}

		unsigned char  My_shablon[] = {'<','>',0};
		set_dim_geo(&geo,1,
			fntHndl, 
			(strng==NULL)?My_shablon:const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
			(lpD_POINT)&f_pnt,
			(lpD_POINT)&s_pnt,
			(lpD_POINT)&th_pnt,
			(lpD_POINT)&four_pnt,
			(lpD_POINT)&cur_pnt,
			&errr);

		hOBJ hO = create_simple_obj(ODIM, &geo); 
		if (hO==NULL)
			return NULL;

		sgCDimensions*   newD=new sgCDimensions(hO);

		char* typeID = "{0000000000000-0000-0000-000000000008}";
		set_hobj_attr_value(id_TypeID, hO, typeID);

		newD->PostCreate();

		return  newD;
}

void     sgCDimensions::DrawRadianDimension(const SG_POINT& f_pnt,
								   const SG_POINT& s_pnt,
								   const SG_POINT& cur_pnt,
								   const sgCFont* fnt,
								   const SG_DIMENSION_STYLE& dim_st,
								   const char* strng,
								   SG_DRAW_LINE_FUNC dlf)
{
	GEO_DIM  geo;
	memset(&geo, 0, sizeof(GEO_DIM));
	geo.type    = DM_DIAM;
	geo.dtstyle = dim_info->dts[DM_DIAM];
	geo.dgstyle = dim_info->dgs;

	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());
	drLF = dlf;
	short errr = 0;


	//geo.dtstyle = dim_info->dts[DM_LIN];
	memcpy(&geo.dtstyle.tstyle,&dim_st.text_style,sizeof(TSTYLE));
	geo.dtstyle.font = fnt->m_id;

	//geo.dgstyle = dim_info->dgs;
	geo.dgstyle.extln_out = dim_st.lug_size;
	if (dim_st.dimension_line)
		geo.dgstyle.flags &= ~DIM_LN_NONE;
	else
		geo.dgstyle.flags |= DIM_LN_NONE;
	if (dim_st.first_side_line)
		geo.dgstyle.flags &= ~EXT1_NONE;
	else
		geo.dgstyle.flags |= EXT1_NONE;
	if (dim_st.second_side_line)
		geo.dgstyle.flags &= ~EXT2_NONE;
	else
		geo.dgstyle.flags |= EXT2_NONE;

	if (!dim_st.automatic_arrows)
	{
		geo.dgstyle.flags &= ~ARR_AUTO;

		if (!dim_st.out_first_arrow)
			geo.dgstyle.flags &= ~ARR1_OUT;
		else
			geo.dgstyle.flags |= ARR1_OUT;

		if (!dim_st.out_second_arrow)
			geo.dgstyle.flags &= ~ARR2_OUT;
		else
			geo.dgstyle.flags |= ARR2_OUT;
	}
	else
		geo.dgstyle.flags |= ARR_AUTO;

	geo.dgstyle.arrtype[0] = dim_st.first_arrow_style;
	geo.dgstyle.arrtype[1] = dim_st.second_arrow_style;

	geo.dgstyle.arrlen = dim_st.arrows_size;

	geo.dgstyle.flags &= (~DTP_AUTO);
	geo.dgstyle.flags |= ARR_DEFINE;

	switch(dim_st.text_align) 
	{
	case SG_TA_CENTER:
		geo.dgstyle.tplace = DTP_CENTER;
		break;
	case SG_TA_LEFT:
		geo.dgstyle.tplace = DTP_LEFT;
		break;
	case SG_TA_RIGHT:
		geo.dgstyle.tplace = DTP_RIGHT;
		break;
	default:
		geo.dgstyle.tplace = DTP_CENTER;
	}

	if (dim_st.precision<1)
		geo.dtstyle.snap = 0.1;
	else
		if (dim_st.precision>8)
			geo.dtstyle.snap = 0.00001;
		else
		{
			switch(dim_st.precision) 
			{
			case 1:
				geo.dtstyle.snap = 0.1;
				break;
			case 2:
				geo.dtstyle.snap = 0.01;
				break;
			case 3:
				geo.dtstyle.snap = 0.001;
				break;
			case 4:
				geo.dtstyle.snap = 0.0001;
				break;
			case 5:
				geo.dtstyle.snap = 0.00001;
				break;
			case 6:
				geo.dtstyle.snap = 0.000001;
				break;
			case 7:
				geo.dtstyle.snap = 0.00000001;
				break;
			default:
				geo.dtstyle.snap = 0.000000001;
			}
		}


		unsigned char  My_shablon[] = {'<','>',0};

		set_dim_geo(&geo,1,fntHndl, 
			(strng==NULL)?My_shablon:const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
			(lpD_POINT)&f_pnt,(lpD_POINT)&s_pnt,
			NULL, NULL,(lpD_POINT)&cur_pnt,	&errr);

		mmm = new sgCMatrix(geo.matr);

		draw_dim1(&geo,fntHndl,	
			(strng==NULL)?My_shablon:const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)), 
			lineFnkForCore, NULL);
		delete mmm;
		mmm = NULL;
		drLF = NULL;
}

*/