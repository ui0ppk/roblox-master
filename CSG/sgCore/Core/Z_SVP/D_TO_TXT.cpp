#include "../sg.h"

short floating_precision        = 3;      //     
short export_floating_precision = 7;      //     


char *sgFloat_to_text(sgFloat d, BOOL delspace, char *txt){
//  sgFloat     
//  .  delspace = TRUE  -  
// 

  return sgFloat_output(d, LEN_sgFloat, floating_precision, delspace, TRUE, txt);
}

char *sgFloat_to_textf(sgFloat d, short size, short precision, char *txt){
//  sgFloat     .
  return sgFloat_output(d, size, precision, FALSE, TRUE, txt);
}

char *dummy_to_text(char *txt){
short l = LEN_sgFloat;
//       -
	memset(txt, '*', l);
	txt[l] = 0;
	if(l > 2) txt[0] = txt[1] = ' ';
	if(l > 5) txt[l - 1] = txt[l - 2] = ' ';
	return txt;
}

void sgFloat_to_text_for_export(char *buf, sgFloat num)
//      DXF  
{
	sgFloat_output1(num, -1, export_floating_precision, TRUE, FALSE,TRUE, buf);
}

char *sgFloat_to_textp(sgFloat num, char *buf){
//        ,
//    ,  ,    
//  
//  buf     MAX_sgFloat_STRING
  return sgFloat_output(num, -1, floating_precision, TRUE, TRUE, buf);
}
