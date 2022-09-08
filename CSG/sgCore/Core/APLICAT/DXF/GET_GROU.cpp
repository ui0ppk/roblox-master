#include "../../sg.h"

#define GR_SHIFT 10000

static  BOOL dxf_str(void);
static  BOOL dxf_gr(void);

extern BOOL ACAD_R13;

static sgFloat dxf_round_coord(char *str)
{
	if (static_data->RoundOn)
		 return round(atof(str), export_floating_precision);
	return atof(str);
}

BOOL get_group(lpDXF_GROUP dxfg)
{
	int						len, gr1;
	static int		g1 = 0;
	static sgFloat dxf_38 = 0.;

//	step_grad(dxfg->num_proc , dxfg->bd->p_beg + dxfg->bd->cur_buf);
	if (application_interface && application_interface->GetProgresser() && dxfg->bd->len_file>0)  
		application_interface->GetProgresser()->Progress(100*(dxfg->bd->p_beg + dxfg->bd->cur_buf)/dxfg->bd->len_file);

	if (ctrl_c_press) {
		put_message(CTRL_C_PRESS, NULL, 0);
		return FALSE;
	}
	if (g1 >= GR_SHIFT) {
		dxfg->gr  = g1 - GR_SHIFT;
		g1 = 0;
	} else
		if (!dxf_gr()) goto err;

	if (0 > dxfg->gr/* || dxfg->gr > 1079*/) {
		dxf_err(BAD_DXF_GROUP);
		return FALSE;
	}

	if (!dxfg->binary) {
		if ( !load_str(dxfg->bd,sizeof(dxfg->str),dxfg->str) )		goto err;
		
		len = strlen(dxfg->str);
		while (len > 0) {
			if (dxfg->str[len - 1] == ' ') len--;
			else break;
		}
	}
	if (dxfg->gr <= 9 || (999 <= dxfg->gr && dxfg->gr <= 1009)) {
		if ( !dxfg->gr ) dxf_38 = 0.;
		if (dxfg->binary) return dxf_str();
		return TRUE;
	}
	if (38 <= dxfg->gr && dxfg->gr <= 49) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->v)) goto err;
		} else dxfg->v = atof(dxfg->str);
		if (dxfg->gr == 38) dxf_38 = dxfg->v;
		return TRUE;
	}
	if (50 <= dxfg->gr && dxfg->gr <= 59) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->a)) goto err;
		} else dxfg->a = atof(dxfg->str);
		return TRUE;
	}
	if (60 <= dxfg->gr && dxfg->gr <= 69) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(short), &dxfg->p)) goto err;
		} else dxfg->p = atoi(dxfg->str);
		return TRUE;
	}
	if (70 <= dxfg->gr && dxfg->gr <= 79) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(short), &dxfg->f)) goto err;
		} else dxfg->f = atoi(dxfg->str);
		return TRUE;
	}
	if (140 <= dxfg->gr && dxfg->gr <= 147) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->a)) goto err;
		} else dxfg->a = atof(dxfg->str);
		return TRUE;
	}
	if ((170 <= dxfg->gr && dxfg->gr <= 179) ||
			(270 <= dxfg->gr && dxfg->gr <= 289)) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(short), &dxfg->p)) goto err;
		} else dxfg->p = atoi(dxfg->str);
		return TRUE;
	}
	if (1010 <= dxfg->gr && dxfg->gr <= 1059) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->a)) goto err;
		} else dxfg->a = atof(dxfg->str);
		return TRUE;
	}
	if (1060 <= dxfg->gr && dxfg->gr <= 1079) {
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(short), &dxfg->p)) goto err;
		} else dxfg->p = atoi(dxfg->str);
		return TRUE;
	}
	if (dxfg->gr >= 20 && (dxfg->gr < 210 || dxfg->gr > 219)) {
		if (dxfg->binary) return dxf_str();
		return TRUE;
//		dxf_err(BAD_DXF_GROUP);
//		return FALSE;
	}
	if (dxfg->binary) {
		if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->xyz.x)) goto err;
	} else dxfg->xyz.x = dxf_round_coord(dxfg->str);
	gr1 = dxfg->gr;
	if (!dxf_gr()) goto err;
	if (gr1 + 10 != dxfg->gr) {
		dxf_err(BAD_DXF_GROUP);
		return FALSE;
	}
	if (dxfg->binary) {
		if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->xyz.y)) goto err;
	} else {
		if ( !load_str(dxfg->bd,sizeof(dxfg->str),dxfg->str) )	goto err;
		dxfg->xyz.y = dxf_round_coord(dxfg->str);
	}
	if (!dxf_gr()) goto err;
	if (gr1 + 20 != dxfg->gr) {
		g1 = dxfg->gr + GR_SHIFT;
		dxfg->gr = gr1;
		dxfg->xyz.z = dxf_38;
	} else {
		dxfg->gr = gr1;
		if (dxfg->binary) {
			if (!load_data(dxfg->bd, sizeof(sgFloat), &dxfg->xyz.z)) goto err;
		} else {
			if ( !load_str(dxfg->bd,sizeof(dxfg->str),dxfg->str) )	goto err;
			dxfg->xyz.z = dxf_round_coord(dxfg->str);
		}
	}
	return TRUE;
err:
	dxf_err(DXF_ERROR);
	return FALSE;
}

static  BOOL dxf_str(void)
{
	short i;

	for (i = 0; i < sizeof(dxfg->str); i++) {
		if (!load_data(dxfg->bd, sizeof(char), &dxfg->str[i])) return FALSE;
		if (!dxfg->str[i]) break;
	}
	if (i == sizeof(dxfg->str)) return FALSE;
	return TRUE;
}

static  BOOL dxf_gr(void)
{
	UCHAR	gr;

	if (dxfg->binary) {
		if (ACAD_R13) {
			
			if (!load_data(dxfg->bd, sizeof(short), &dxfg->gr)) return FALSE;
		} else {
			if (!load_data(dxfg->bd, sizeof(UCHAR), &gr)) return FALSE;
			if (gr == 255) {
				if (!load_data(dxfg->bd, sizeof(short), &dxfg->gr)) return FALSE;
			} else dxfg->gr = gr;
		}
	} else {
		if (!load_str(dxfg->bd,sizeof(dxfg->str),dxfg->str)) return FALSE;
		dxfg->gr = atoi(dxfg->str);
	}
	return TRUE;
}
