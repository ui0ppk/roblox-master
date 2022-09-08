#ifndef  __sgTD__
#define  __sgTD__

class sgCText;
class sgCDimensions;
class sgFontManager;

typedef struct 
{ 
	char           name[15];     // Font name
	unsigned short table_begin;  // Table begin
	unsigned short table_end;    // Table end
	unsigned short table_size;   // Table size
	unsigned char  posit_size;  
	unsigned char  negat_size;  
	unsigned char  state;        
	sgFloat         proportion;   
	unsigned char  symbols_table[1];     // Table
} SG_FONT_DATA;

class sgCore_API   sgCFont
{
	friend class sgCText;
	friend class sgCScene;
	friend class sgFontManager;
	friend class sgCDimensions;
	
private:
	mutable void*          m_handle;
	mutable unsigned long  m_length;
	long           m_id;
	sgCFont(void*, unsigned long, long);
	~sgCFont();

public:
	const   SG_FONT_DATA*   const     GetFontData() const;

	static  sgCFont*       LoadFont(const char* path, char* comment, short commentLength);
	static  bool           UnloadFont(sgCFont*);
};


class sgCore_API sgFontManager
{
public:
	static bool			   AttachFont(sgCFont*);
	static unsigned int    GetFontsCount();
	static const sgCFont*  GetFont(unsigned int);
	static bool            SetCurrentFont(unsigned int);
	static unsigned int    GetCurrentFont();
};


/************************************************************************/
/* TEXT                                                                 */
/************************************************************************/
#define SG_TEXT_VERTICAL      0x01  

typedef struct
{
	unsigned char  state;
	sgFloat		   height;      
	sgFloat		   proportions; 
	sgFloat		   angle;        
	sgFloat		   horiz_space_proportion;  
	sgFloat		   vert_space_proportion;         
} SG_TEXT_STYLE;   


class sgCore_API   sgCText : public sgCObject
{
private:
	sgCText();
	sgCText(SG_OBJ_HANDLE);
	virtual    ~sgCText();

	SG_LINE*     m_lines;
	int          m_lines_count;
	char*        m_text;
public:
	static   sgCText*    Create(const sgCFont*, const SG_TEXT_STYLE&,const char*);

	const    SG_LINE*    GetLines() const;
	int                  GetLinesCount() const;

	SG_TEXT_STYLE        GetStyle() const;
	const char*          GetText();
	unsigned int         GetFont();
	bool                 GetWorldMatrix(sgCMatrix& mtrx) const;

	virtual  bool		 ApplyTempMatrix();

	static   bool        Draw(const sgCFont*,
         const SG_TEXT_STYLE&,sgCMatrix*,const char* ,SG_DRAW_LINE_FUNC);

	PRIVATE_ACCESS
};

#define  sgCreateText  sgCText::Create


typedef enum
{
	SG_TA_CENTER,
	SG_TA_LEFT,
	SG_TA_RIGHT
} SG_TEXT_ALIGN;

typedef enum
{
	SG_DBT_VERTICAL,
	SG_DBT_HORIZONTAL,
	SG_DBT_PARALLEL,
	SG_DBT_SLANT,
	SG_DBT_OPTIMAL
} SG_DIMENSION_BEHAVIOUR;

typedef struct  
{
	bool			dimension_line;
	bool			first_side_line;
	bool			second_side_line;
	sgFloat          lug_size;

	bool            automatic_arrows;

	bool			out_first_arrow;
	unsigned char   first_arrow_style;

	bool			out_second_arrow;
	unsigned char   second_arrow_style;

	sgFloat          arrows_size;

	SG_TEXT_ALIGN   text_align;
	SG_TEXT_STYLE   text_style;

	bool            invert;

	SG_DIMENSION_BEHAVIOUR   behaviour_type;

	unsigned short  precision;

} SG_DIMENSION_STYLE;

typedef enum
{
	SG_DT_LINEAR,
	SG_DT_ANGLE,
	SG_DT_RAD,
	SG_DT_DIAM
} SG_DIMENSION_TYPE;

/************************************************************************/
/* DIMENSIONS                                                              */
/************************************************************************/
class sgCore_API   sgCDimensions : public sgCObject
{
private:
	sgCDimensions();
	sgCDimensions(SG_OBJ_HANDLE);
	virtual    ~sgCDimensions();	

	SG_LINE*     m_lines;
	int          m_lines_count;
	SG_POINT*    m_formed_points;
	char*        m_text;
public:

	static   sgCDimensions* Create(SG_DIMENSION_TYPE, const SG_POINT*,
					const sgCFont*, const SG_DIMENSION_STYLE& ,
					const char*);

	const    SG_LINE*    GetLines() const;
	int                  GetLinesCount() const;

	const SG_POINT*      GetFormedPoints();

	SG_DIMENSION_TYPE    GetType() const;
	SG_DIMENSION_STYLE   GetStyle() const;
	const char*          GetText();
	unsigned int         GetFont();

	virtual  bool		 ApplyTempMatrix();



	static   bool        Draw(SG_DIMENSION_TYPE, const SG_POINT*,
						const sgCFont*, const SG_DIMENSION_STYLE& ,
						const char* ,SG_DRAW_LINE_FUNC);

	PRIVATE_ACCESS
};

#endif
