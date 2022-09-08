#include "../sg.h"

EL_OBJTYPE  EPOINT,
            ELINE,
            ECIRCLE,
            EARC,
            EBREP,
            ESPLINE;

void RegCoreObjects(void)
{
  RegCoreObjType(OTEXT,       /*"Text"*/     "1GFR",       sizeof(GEO_TEXT),    OLD_OTEXT,       NULL,     TRUE);
	RegCoreObjType(OPOINT,     /* "Point"*/  "GRQF4",      sizeof(GEO_POINT),   OLD_OPOINT,      &EPOINT,  TRUE);
	RegCoreObjType(OLINE,      /* "Line"*/   "RYEI",       sizeof(GEO_LINE),    OLD_OLINE,       &ELINE,   TRUE);
	RegCoreObjType(OCIRCLE,    /* "Circle"*/ "ETIWVK",     sizeof(GEO_CIRCLE),  OLD_OCIRCLE,     &ECIRCLE, TRUE);
	RegCoreObjType(OARC,       /* "Arc"*/    "OQP",        sizeof(GEO_ARC),     OLD_OARC,        &EARC,    TRUE);
	RegCoreObjType(OPATH,     /*  "Path"*/   "JRYE",       sizeof(GEO_PATH),    OLD_OPATH,       NULL,     FALSE);
	RegCoreObjType(OBREP,     /*  "BRep"*/   "QNRE",       sizeof(GEO_BREP),    OLD_OBREP,       &EBREP,   FALSE);
	RegCoreObjType(OGROUP,    /*  "Group"*/  "PPEOR",      sizeof(GEO_GROUP),   OLD_OGROUP,      NULL,     FALSE);
	RegCoreObjType(ODIM,      /*  "Dim"*/    "EMT",        sizeof(GEO_DIM),     OLD_ODIM,        NULL,     TRUE);
	RegCoreObjType(OINSERT,   /*  "Insert"*/ "NTYREW",     sizeof(GEO_INSERT),  OLD_OINSERT,     NULL,     FALSE);
	RegCoreObjType(OSPLINE,   /*  "Spline"*/ "QOREEE",     sizeof(GEO_SPLINE),  OLD_OSPLINE,     &ESPLINE, TRUE);
	RegCoreObjType(OFRAME,   /*   "Frame"*/  "*H4F46",      sizeof(GEO_FRAME),   OLD_OFRAME,      NULL,     FALSE);
}


