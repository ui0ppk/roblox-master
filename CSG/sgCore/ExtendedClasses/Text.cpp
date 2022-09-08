#include "..//Core//sg.h"

#include <vector>


sgCFont*                      temp_font = NULL;

bool sgFontManager::AttachFont(sgCFont* fnt)
{
	if (!fnt || !fnt->m_handle || fnt->m_length==0)
		return false;

	lpFONT     font = NULL;
	IDENT_V    id;
	ULONG      len;
	//    
	id = ft_bd->ft_listh[FTFONT].head;
	while(id){
		if(0 ==(font = (FONT *)alloc_and_get_ft_value(id, &len))) 
		{
			assert(0);
			sgCFont::UnloadFont(fnt);
			return false;
		}
		if(!strcmp(fnt->GetFontData()->name, font->name))
		{
			SGFree(font);
			sgCFont::UnloadFont(fnt);
			return false;
		}
		SGFree(font); 
		font = NULL;
		if(!il_get_next_item(&ft_bd->vd, id, &id))
		{
			assert(0);
			return false;
		}
	}



	id = add_ft_value(FTFONT, fnt->m_handle, fnt->m_length);

	if(!id) 
		return false;

	fnt->m_id = id;
	SGFree(fnt->m_handle);
	fnt->m_handle = NULL;
	fnt->m_length = 0;

	if (ft_bd->curr_font==0)
		ft_bd->def_font = ft_bd->curr_font = id;

	delete fnt;
	fnt = NULL;

	return true;
}

unsigned int  sgFontManager::GetFontsCount()
{
	return ft_bd->ft_listh[FTFONT].num;
}

const sgCFont* sgFontManager::GetFont(unsigned int numb)
{
	lpFONT     fnt = NULL;
	IDENT_V    id;

	if ((long)numb>=ft_bd->ft_listh[FTFONT].num)
		return NULL;
	//    
	id = ft_bd->ft_listh[FTFONT].head;

	for (unsigned int i=0;i<numb;i++)
		if(!il_get_next_item(&ft_bd->vd, id, &id)) 
			return NULL;
	if (temp_font)
	{
		delete temp_font;
		temp_font = NULL;
	}

	temp_font = new sgCFont(NULL,0,id);
	return temp_font;
}

bool sgFontManager::SetCurrentFont(unsigned int numb)
{
	IDENT_V    id;

	if ((long)numb>=ft_bd->ft_listh[FTFONT].num)
		return false;
	//    
	id = ft_bd->ft_listh[FTFONT].head;
	for (unsigned int i=0;i<numb;i++)
		if(!il_get_next_item(&ft_bd->vd, id, &id)) 
			return NULL;
	ft_bd->curr_font = id;
	return true;
}

unsigned int sgFontManager::GetCurrentFont()
{
	IDENT_V    id;
	//    
	id = ft_bd->ft_listh[FTFONT].head;
	unsigned int res = 0;
	while(id)
	{
		if (id==ft_bd->curr_font)
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

static  void  FillLinesArrayfromGeoText(lpGEO_TEXT geoT)
{
	lines_buffer.clear();
	assert(geoT);
	draw_geo_text(geoT,lines_collector_func,NULL);
}


sgCText::sgCText():sgCObject()
{
	m_lines = NULL;
	m_lines_count=0;
	m_text = NULL;
}

sgCText::~sgCText()
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
}

sgCText::sgCText(SG_OBJ_HANDLE objH):sgCObject(objH)
{
	m_lines = NULL;
	m_lines_count=0;
	m_text = NULL;

	char* typeID = "{0000000000000-0000-0000-000000000006}";
	set_hobj_attr_value(id_TypeID, objH, typeID);

	lpOBJ pobj = (lpOBJ)objH;
	
	FillLinesArrayfromGeoText(reinterpret_cast<lpGEO_TEXT>(pobj->geo_data));

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


bool	sgCText::ApplyTempMatrix()
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

	FillLinesArrayfromGeoText(reinterpret_cast<lpGEO_TEXT>(pobj->geo_data));

	size_t lbs = lines_buffer.size();
	if (lbs>0)
	{
		m_lines_count = lbs;
		m_lines = new SG_LINE[lbs];
		memcpy(m_lines, &lines_buffer[0], sizeof(SG_LINE)*m_lines_count);
	}
	lines_buffer.clear();

	return true;
}

sgCText*   sgCText::Create(const sgCFont* fnt, const SG_TEXT_STYLE& text_stl,const char* textStr)
{

	if (fnt==NULL || textStr==NULL )
		return NULL;

	GEO_TEXT gtext;
    	
    // was - RA
    //memcpy(&gtext.style,&text_stl,sizeof(SG_TEXT_STYLE));//ft_bd->curr_style;
	
    // new - RA
    gtext.style.status = text_stl.state;
    gtext.style.height = text_stl.height;
    gtext.style.scale = text_stl.proportions;
    gtext.style.angle = text_stl.angle;
    gtext.style.dhor = text_stl.horiz_space_proportion;
    gtext.style.dver = text_stl.vert_space_proportion;
    
    gtext.font_id = fnt->m_id;//ft_bd->curr_font;

	IDENT_V    id;
	ULONG      len;
	UCHAR      *exist_txt=NULL;
	bool       exist_this_text = false;

	id = ft_bd->ft_listh[FTTEXT].head;

	while(id){
		if(0 ==(exist_txt = (UCHAR *)alloc_and_get_ft_value(id, &len))) 
			return NULL;
		if(!strcmp((char*)exist_txt, textStr))
		{
			SGFree(exist_txt);
			gtext.text_id = id;
			exist_this_text = true;
			break;
		}
		SGFree(exist_txt); exist_txt = NULL;
		if(!il_get_next_item(&ft_bd->vd, id, &id)) 
			return NULL;
	}

	if (!exist_this_text)
		gtext.text_id = add_ft_value(FTTEXT, const_cast<char*>(textStr), strlen(textStr)+1);
	if(!gtext.text_id) 
		return NULL;

	if(get_text_geo_par(&gtext) != G_OK)
	{
		if (!exist_this_text)
			delete_ft_item(FTTEXT, gtext.text_id);
		return NULL;
	}

	hOBJ obj = create_simple_obj_loc(OTEXT, &gtext);

	if (!obj)
	{
		assert(0);
		return NULL;
	}

	sgCText*   newT=new sgCText(obj);

	char* typeID = "{0000000000000-0000-0000-000000000006}";
	set_hobj_attr_value(id_TypeID, obj, typeID);


	newT->PostCreate();

	return  newT;
}


const    SG_LINE*   sgCText::GetLines() const
{
	return  m_lines;
}

int  sgCText::GetLinesCount() const
{
	return m_lines_count;
}

SG_TEXT_STYLE   sgCText::GetStyle() const
{
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);
	
	SG_TEXT_STYLE retStl;
	memcpy(&retStl, &((lpGEO_TEXT)(obj->geo_data))->style, sizeof(SG_TEXT_STYLE));

	return retStl;
}

const char*  sgCText::GetText()
{
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);

	if (m_text!=NULL)
		return m_text;

	IDENT_V    id = ((lpGEO_TEXT)(obj->geo_data))->text_id;
	ULONG      len;
	
	if(0 ==(m_text = (char*)alloc_and_get_ft_value(id, &len))) 
	{
		assert(0);
		return NULL;
	}
	return m_text;
}

unsigned int  sgCText::GetFont()
{
	lpOBJ obj = (lpOBJ)GetObjectHandle(this);

	IDENT_V    id;
	//    
	id = ft_bd->ft_listh[FTFONT].head;
	unsigned int res = 0;
	while(id)
	{
		if (id==((lpGEO_TEXT)(obj->geo_data))->font_id)
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

bool   sgCText::GetWorldMatrix(sgCMatrix& mtrx) const
{
	GEO_TEXT oldGeo;
	get_simple_obj_geo(GetObjectHandle(this), &oldGeo);
	sgCMatrix* newM = new sgCMatrix(oldGeo.matr);
	mtrx.SetMatrix(newM);
	delete newM;
	return true;
}


static  SG_DRAW_LINE_FUNC   drLF = NULL;

static  sgCMatrix*           drMatr = NULL;

static BOOL  lineFnkForCore(lpD_POINT pb, lpD_POINT pe, void *usrD)
{
	assert(drLF);
	SG_POINT* pBeg = (SG_POINT*)(pb);
	SG_POINT* pEnd = (SG_POINT*)(pe);
	if (drMatr)
	{
		drMatr->ApplyMatrixToPoint(*pBeg);
		drMatr->ApplyMatrixToPoint(*pEnd);
	}
	drLF(pBeg,pEnd);
	return TRUE;
}

bool  sgCText::Draw(const sgCFont* fnt,
                        const SG_TEXT_STYLE& stle,
						sgCMatrix* mtrx,
						const char* strng, 
						SG_DRAW_LINE_FUNC dlf)
{
	if (fnt==NULL || strng==NULL || dlf==NULL)
		return false;

	drLF = dlf;
	drMatr  = mtrx;
	lpFONT fntHndl = (lpFONT)const_cast<SG_FONT_DATA*>(fnt->GetFontData());
    TSTYLE tmpTSt;
    memcpy(&tmpTSt, &stle, sizeof(SG_TEXT_STYLE));
    draw_text(fntHndl, &tmpTSt,
        const_cast<UCHAR*>(reinterpret_cast<const UCHAR*>(strng)),
        lineFnkForCore, NULL);
	drLF = NULL;
	drMatr = NULL;
	return true;
}
