#include "..//Core//sg.h"
#include "..//Core//common_hdr.h"

sgCFont::sgCFont(void* hndl, unsigned long lngth, long iden)
{
	m_handle = hndl;
	m_length = lngth;
	m_id = iden;
}

sgCFont::~sgCFont()
{
	if (m_handle)
	{
		SGFree(m_handle);
	}
	m_handle = NULL;
	m_length = 0;
	m_id = 0;
}

const SG_FONT_DATA* const sgCFont::GetFontData() const
{
	if (!m_handle)
	{
		if (m_id==0)
		{
			assert(0);
			return NULL;
		}
		if(0 ==(m_handle = alloc_and_get_ft_value(m_id, &m_length))) 
			return NULL;
	}
	
	return reinterpret_cast<SG_FONT_DATA*>(m_handle);
}

sgCFont*    sgCFont::LoadFont(const char* path, char* comment, short commentLength)
{
	ULONG lenF;
	lpFONT font=NULL;
	short errcode=-1;

	char       drive [MAXDRIVE];
	char       dir   [MAXDIR];
	char       file  [MAXFILE];
	char       ext   [MAXEXT];
	
	fnsplit(path, drive, dir, file, ext);

	if ((strlen(file)+strlen(ext))>15)
		return NULL;
	
	if(0 ==(font = load_font(const_cast<char*>(path),&lenF,comment,commentLength, &errcode))) 
		return NULL;
	
	sgCFont* resF = new sgCFont(font,lenF,0);

	return resF;
}

bool        sgCFont::UnloadFont(sgCFont* fnt)
{
	if (!fnt || fnt->m_id!=0)
		return false;

	delete fnt;

	return true;
}