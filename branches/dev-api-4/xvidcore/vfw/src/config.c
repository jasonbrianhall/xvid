/**************************************************************************
 *
 *	XVID VFW FRONTEND
 *	config
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	15.06.2002	added bframes options
 *	21.04.2002	fixed custom matrix support, tried to get dll size down
 *	17.04.2002	re-enabled lumi masking in 1st pass
 *	15.04.2002	updated cbr support
 *	07.04.2002	min keyframe interval checkbox
 *				2-pass max bitrate and overflow customization
 *	04.04.2002	interlacing support
 *				hinted ME support
 *	24.03.2002	daniel smith <danielsmith@astroboymail.com>
 *				added Foxer's new CBR engine
 *				- cbr_buffer is being used as reaction delay (quick hack)
 *	23.03.2002	daniel smith <danielsmith@astroboymail.com>
 *				added load defaults button
 *				merged foxer's alternative 2-pass code (2-pass alt tab)
 *				added proper tooltips
 *				moved registry data into reg_ints/reg_strs arrays
 *				added DEBUGERR output on errors instead of returning
 *	16.03.2002	daniel smith <danielsmith@astroboymail.com>
 *				rewrote/restructured most of file
 *				added tooltips (kind of - dirty message hook method)
 *				split tabs into a main dialog / advanced prop sheet
 *				advanced controls are now enabled/disabled by mode
 *				added modulated quantization, DX50 fourcc
 *	11.03.2002  Min Chen <chenm001@163.com>
 *              now get Core Version use xvid_init()
 *	05.03.2002  Min Chen <chenm001@163.com>
 *				Add Core version display to about box
 *	01.12.2001	inital version; (c)2001 peter ross <pross@xvid.org>
 *
 *************************************************************************/


#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <prsht.h>

#include <stdio.h>  /* sprintf */
#include <xvid.h>	/* XviD API */

#include "debug.h"
#include "config.h"
#include "resource.h"


#define CONSTRAINVAL(X,Y,Z) if((X)<(Y)) X=Y; if((X)>(Z)) X=Z;
#define IsDlgChecked(hwnd,idc)	(IsDlgButtonChecked(hwnd,idc) == BST_CHECKED)
#define CheckDlg(hwnd,idc,value) CheckDlgButton(hwnd,idc, value?BST_CHECKED:BST_UNCHECKED)
#define EnableDlgWindow(hwnd,idc,state) EnableWindow(GetDlgItem(hwnd,idc),state)

HINSTANCE g_hInst;
HWND g_hTooltip;

/* enumerates child windows, assigns tooltips */
BOOL CALLBACK enum_tooltips(HWND hWnd, LPARAM lParam)
{
	char help[500];

    if (LoadString(g_hInst, GetDlgCtrlID(hWnd), help, 500))
	{
		TOOLINFO ti;
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
		ti.hwnd = GetParent(hWnd);
		ti.uId	= (LPARAM)hWnd;
		ti.lpszText = help;
		SendMessage(g_hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}

	return TRUE;
}


/* ===================================================================================== */
/* MPEG-4 PROFILES/LEVELS ============================================================== */
/* ===================================================================================== */



/* default vbv_occupancy is (64/170)*vbv_buffer_size */

const profile_t profiles[] =
{
/*    name                 p@l,    w    h  fps  obj  Tvmv  vmv    vcv   ac%     vbv      pkt   kbps  flags */
    { "Simple @ L0",       0x08,  176, 144, 15,  1,  198,   99,   1485, 100,  10*16368,  2048,   64, 0 },
	/* simple@l0: max f_code=1, intra_dc_vlc_threshold=0 */
	/* if ac preidition is used, adaptive quantization must not be used */
	/* <=qcif must be used */
	{ "Simple @ L1",       0x01,  176, 144, 15,  4,  198,   99,   1485, 100,  10*16368,  2048,   64, PROFILE_ADAPTQUANT },
	{ "Simple @ L2",       0x02,  352, 288, 15,  4,  792,  396,   5940, 100,  40*16368,  4096,  128, PROFILE_ADAPTQUANT },
	{ "Simple @ L3",       0x03,  352, 288, 15,  4,  792,  396,  11880, 100,  40*16368,  8192,  384, PROFILE_ADAPTQUANT },

	{ "ARTS @ L1",         0x91,  176, 144, 15,  4,  198,   99,   1485, 100,  10*16368,  8192,   64, PROFILE_ARTS },
	{ "ARTS @ L2",         0x92,  352, 288, 15,  4,  792,  396,   5940, 100,  40*16368, 16384,  128, PROFILE_ARTS },
	{ "ARTS @ L3",         0x93,  352, 288, 30,  4,  792,  396,  11880, 100,  40*16368, 16384,  384, PROFILE_ARTS },
	{ "ARTS @ L4",         0x94,  352, 288, 30, 16,  792,  396,  11880, 100,  80*16368, 16384, 2000, PROFILE_ARTS },

	{ "AS @ L0",	       0xf0,  176, 144, 30,  1,  297,   99,   2970, 100,  10*16368,  2048,  128, PROFILE_AS },
	{ "AS @ L1",	       0xf1,  176, 144, 30,  4,  297,   99,   2970, 100,  10*16368,  2048,  128, PROFILE_AS },
	{ "AS @ L2",	       0xf2,  352, 288, 15,  4, 1188,  396,   5940, 100,  40*16368,  4096,  384, PROFILE_AS },
	{ "AS @ L3",	       0xf3,  352, 288, 30,  4, 1188,  396,  11880, 100,  40*16368,  4096,  768, PROFILE_AS },
 /*  ISMA Profile 1, (ASP) @ L3b (CIF, 1.5 Mb/s) CIF(352x288), 30fps, 1.5Mbps max ??? */
	{ "AS @ L4",	       0xf4,  352, 576, 30,  4, 2376,  792,  23760,  50,  80*16368,  8192, 3000, PROFILE_AS },
	{ "AS @ L5",	       0xf5,  720, 576, 30,  4, 4860, 1620,  48600,  25, 112*16368, 16384, 8000, PROFILE_AS },

	{ "DXN Handheld",	   0x00,  176, 144, 15, -1,  198,   99,   1485, 100,  16*16368,    -1,  128, PROFILE_ADAPTQUANT },
	{ "DXN Portable NTSC", 0x00,  352, 240, 30, -1,  990,  330,   9900, 100,  64*16368,    -1,  768, PROFILE_ADAPTQUANT|PROFILE_BVOP },
	{ "DXN Portable PAL",  0x00,  352, 288, 25, -1, 1188,  396,   9900, 100,  64*16368,    -1,  768, PROFILE_ADAPTQUANT|PROFILE_BVOP },
	{ "DXN HT NTSC",	   0x00,  720, 480, 30, -1, 4050, 1350,  40500, 100, 192*16368,    -1, 4000, PROFILE_ADAPTQUANT|PROFILE_BVOP|PROFILE_INTERLACE },
	{ "DXN HT PAL",        0x00,  720, 576, 25, -1, 4860, 1620,  40500, 100, 192*16368,    -1, 4000, PROFILE_ADAPTQUANT|PROFILE_BVOP|PROFILE_INTERLACE },
	{ "DXN HDTV",          0x00, 1280, 720, 30, -1,10800, 3600, 108000, 100, 384*16368,    -1, 8000, PROFILE_ADAPTQUANT|PROFILE_BVOP|PROFILE_INTERLACE },
	
    { "(unrestricted)",    0x00,    0,   0,  0,  0,    0,    0,      0, 100,   0*16368,     0,    0, 0xffffffff },
};


/* ===================================================================================== */
/* REGISTRY ============================================================================ */
/* ===================================================================================== */

/* registry info structs */
CONFIG reg;

static const REG_INT reg_ints[] = {
	{"mode",					&reg.mode,						RC_MODE_1PASS},
	{"bitrate",					&reg.bitrate,					700},
    {"desired_size",			&reg.desired_size,				570000},
    {"use_2pass_bitrate",       &reg.use_2pass_bitrate,         0},

    /* profile */
    {"quant_type",				&reg.quant_type,				0},
	{"lum_masking",				&reg.lum_masking,				0},
	{"interlacing",				&reg.interlacing,				0},
	{"qpel",					&reg.qpel,						0},
	{"gmc",						&reg.gmc,						0},
	{"reduced_resolution",		&reg.reduced_resolution,		0},
    {"use_bvop",				&reg.use_bvop,	    			0},
	{"max_bframes",				&reg.max_bframes,				2},
	{"bquant_ratio",			&reg.bquant_ratio,				150},   /* 100-base float */
	{"bquant_offset",			&reg.bquant_offset,				100},   /* 100-base float */
	{"packed",					&reg.packed,					0},
	{"closed_gov",				&reg.closed_gov,				1},

    /* zones */
    {"num_zones",               &reg.num_zones,                 1},

    /* single pass */
	{"rc_reaction_delay_factor",&reg.rc_reaction_delay_factor,	16},
	{"rc_averaging_period",		&reg.rc_averaging_period,		100},
	{"rc_buffer",				&reg.rc_buffer,		    		100},

    /* 2pass1 */
    {"discard1pass",			&reg.discard1pass,				1},

    /* 2pass2 */
	{"keyframe_boost",			&reg.keyframe_boost,			0},
	{"kfreduction",				&reg.kfreduction,				20},
	{"curve_compression_high",	&reg.curve_compression_high,	0},
	{"curve_compression_low",	&reg.curve_compression_low,		0},
	{"overflow_control_strength", &reg.overflow_control_strength, 10},
	{"twopass_max_overflow_improvement", &reg.twopass_max_overflow_improvement, 60},
	{"twopass_max_overflow_degradation", &reg.twopass_max_overflow_degradation, 60},

    /* motion */
    {"motion_search",			&reg.motion_search,				6},
	{"vhq_mode",				&reg.vhq_mode,					0},
    {"chromame",				&reg.chromame,					0},
    {"cartoon_mode",			&reg.cartoon_mode,				0},
	{"max_key_interval",		&reg.max_key_interval,			300},
	{"min_key_interval",		&reg.min_key_interval,			1},
	{"frame_drop_ratio",		&reg.frame_drop_ratio,			0},	
	
    /* quant */
	{"min_iquant",				&reg.min_iquant,				2},
	{"max_iquant",				&reg.max_iquant,				31},
	{"min_pquant",				&reg.min_pquant,				2},
	{"max_pquant",				&reg.max_pquant,				31},
	{"min_bquant",				&reg.min_bquant,				2},
	{"max_bquant",				&reg.max_bquant,				31},
    {"trellis_quant",           &reg.trellis_quant,             0},

    /* debug */
    {"fourcc_used",				&reg.fourcc_used,				0},
    {"debug",					&reg.debug,						0x0},
    {"vop_debug",				&reg.vop_debug,					0},
    {"display_status",          &reg.display_status,            1},
};

static const REG_STR reg_strs[] = {
	{"profile",					reg.profile_name,				"(unrestricted)"},	
    {"stats",					reg.stats,						CONFIG_2PASS_FILE},
};


zone_t stmp;
static const REG_INT reg_zone[] = {
    {"zone%i_frame",            &stmp.frame,                     0},
    {"zone%i_mode",             &stmp.mode,                      RC_ZONE_WEIGHT},
    {"zone%i_weight",           &stmp.weight,                    100},      /* 100-base float */
    {"zone%i_quant",            &stmp.quant,                     500},      /* 100-base float */
    {"zone%i_type",             &stmp.type,                      XVID_TYPE_AUTO},
    {"zone%i_greyscale",        &stmp.greyscale,                 0},
    {"zone%i_chroma_opt",       &stmp.chroma_opt,                0},
    {"zone%i_bvop_threshold",   &stmp.bvop_threshold,            0},
};

static const BYTE default_qmatrix_intra[] = {
	8, 17,18,19,21,23,25,27,
	17,18,19,21,23,25,27,28,
	20,21,22,23,24,26,28,30,
	21,22,23,24,26,28,30,32,
	22,23,24,26,28,30,32,35,
	23,24,26,28,30,32,35,38,
	25,26,28,30,32,35,38,41,
	27,28,30,32,35,38,41,45
};

static const BYTE default_qmatrix_inter[] = {
	16,17,18,19,20,21,22,23,
	17,18,19,20,21,22,23,24,
	18,19,20,21,22,23,24,25,
	19,20,21,22,23,24,26,27,
	20,21,22,23,25,26,27,28,
	21,22,23,24,26,27,28,30,
	22,23,24,26,27,28,30,31,
	23,24,25,27,28,30,31,33
};



#define REG_GET_B(X, Y, Z) size=sizeof((Z));if(RegQueryValueEx(hKey, X, 0, 0, Y, &size) != ERROR_SUCCESS) {memcpy(Y, Z, sizeof((Z)));}

void config_reg_get(CONFIG * config)
{
    char tmp[32];
    HKEY hKey;
	DWORD size;
    int i,j;
	xvid_gbl_info_t info;

	memset(&info, 0, sizeof(info));
	info.version = XVID_VERSION;
	xvid_global(0, XVID_GBL_INFO, &info, NULL);
	reg.cpu = info.cpu_flags;
	reg.num_threads = info.num_threads;
	
	RegOpenKeyEx(XVID_REG_KEY, XVID_REG_PARENT "\\" XVID_REG_CHILD, 0, KEY_READ, &hKey);

    /* read integer values */
	for (i=0 ; i<sizeof(reg_ints)/sizeof(REG_INT); i++) {
		size = sizeof(int);
        if (RegQueryValueEx(hKey, reg_ints[i].reg_value, 0, 0, (LPBYTE)reg_ints[i].config_int, &size) != ERROR_SUCCESS) {
			*reg_ints[i].config_int = reg_ints[i].def;
        }
	}

    /* read string values */
	for (i=0 ; i<sizeof(reg_strs)/sizeof(REG_STR); i++) {
		size = MAX_PATH;
		if (RegQueryValueEx(hKey, reg_strs[i].reg_value, 0, 0, (LPBYTE)reg_strs[i].config_str, &size) != ERROR_SUCCESS) {
			memcpy(reg_strs[i].config_str, reg_strs[i].def, MAX_PATH);
		}
	}

    reg.profile = 0;
    for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++) {
        if (strcmpi(profiles[i].name, reg.profile_name) == 0) {
            reg.profile = i;
        }
    }

    memcpy(config, &reg, sizeof(CONFIG));


    /* read quant matrices */
	REG_GET_B("qmatrix_intra", config->qmatrix_intra, default_qmatrix_intra);
	REG_GET_B("qmatrix_inter", config->qmatrix_inter, default_qmatrix_inter);
    

    /* read zones */
    if (config->num_zones>MAX_ZONES) {
        config->num_zones=MAX_ZONES;
    }else if (config->num_zones<=0) {
        config->num_zones = 1;
    }

    for (i=0; i<config->num_zones; i++) {
	    for (j=0; j<sizeof(reg_zone)/sizeof(REG_INT); j++)  {
		    size = sizeof(int);
            
            wsprintf(tmp, reg_zone[j].reg_value, i);
		    if (RegQueryValueEx(hKey, tmp, 0, 0, (LPBYTE)reg_zone[j].config_int, &size) != ERROR_SUCCESS)
			    *reg_zone[j].config_int = reg_zone[j].def;
	    }

        memcpy(&config->zones[i], &stmp, sizeof(zone_t));
    }

	RegCloseKey(hKey);
}


/* put config settings in registry */

#define REG_SET_B(X, Y) RegSetValueEx(hKey, X, 0, REG_BINARY, Y, sizeof((Y)))

void config_reg_set(CONFIG * config)
{
    char tmp[64];
    HKEY hKey;
	DWORD dispo;
	int i,j;

	if (RegCreateKeyEx(
			XVID_REG_KEY,
			XVID_REG_PARENT "\\" XVID_REG_CHILD,
			0,
			XVID_REG_CLASS,
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE,
			0, 
			&hKey, 
			&dispo) != ERROR_SUCCESS)
	{
		DPRINTF("Couldn't create XVID_REG_SUBKEY - GetLastError=%i", GetLastError());
		return;
	}

	memcpy(&reg, config, sizeof(CONFIG));

    /* set integer values */
	for (i=0 ; i<sizeof(reg_ints)/sizeof(REG_INT); i++) {
		RegSetValueEx(hKey, reg_ints[i].reg_value, 0, REG_DWORD, (LPBYTE)reg_ints[i].config_int, sizeof(int));
	}

    /* set string values */
    strcpy(reg.profile_name, profiles[reg.profile].name);
	for (i=0 ; i<sizeof(reg_strs)/sizeof(REG_STR); i++) {
		RegSetValueEx(hKey, reg_strs[i].reg_value, 0, REG_SZ, reg_strs[i].config_str, lstrlen(reg_strs[i].config_str)+1);
	}

    /* set quant matrices */
	REG_SET_B("qmatrix_intra", config->qmatrix_intra);
	REG_SET_B("qmatrix_inter", config->qmatrix_inter);

    /* set seections */
    for (i=0; i<config->num_zones; i++) {
        memcpy(&stmp, &config->zones[i], sizeof(zone_t));
	    for (j=0; j<sizeof(reg_zone)/sizeof(REG_INT); j++)  {
            wsprintf(tmp, reg_zone[j].reg_value, i);
	        RegSetValueEx(hKey, tmp, 0, REG_DWORD, (LPBYTE)reg_zone[j].config_int, sizeof(int));
	    }
    }

	RegCloseKey(hKey);
}


/* clear XviD registry key, load defaults */

void config_reg_default(CONFIG * config)
{
	HKEY hKey;

	if (RegOpenKeyEx(XVID_REG_KEY, XVID_REG_PARENT, 0, KEY_ALL_ACCESS, &hKey)) {
		DPRINTF("Couldn't open registry key for deletion - GetLastError=%i", GetLastError());
		return;
	}

	if (RegDeleteKey(hKey, XVID_REG_CHILD)) {
		DPRINTF("Couldn't delete registry key - GetLastError=%i", GetLastError());
		return;
	}

	RegCloseKey(hKey);
	config_reg_get(config);
	config_reg_set(config);
}


/* leaves current config value if dialog item is empty */
int config_get_int(HWND hDlg, INT item, int config)
{
	BOOL success = FALSE;
	int tmp = GetDlgItemInt(hDlg, item, &success, TRUE);
	return (success) ? tmp : config;
}


int config_get_uint(HWND hDlg, UINT item, int config)
{
	BOOL success = FALSE;
	int tmp = GetDlgItemInt(hDlg, item, &success, FALSE);
	return (success) ? tmp : config;
}


/* we use "100 base" floats */

#define FLOAT_BUF_SZ    20
int get_dlgitem_float(HWND hDlg, UINT item, int def)
{
	char buf[FLOAT_BUF_SZ];

    if (GetDlgItemText(hDlg, item, buf, FLOAT_BUF_SZ) == 0)
        return def;

	return (int)(atof(buf)*100);
}

void set_dlgitem_float(HWND hDlg, UINT item, int value)
{
	char buf[FLOAT_BUF_SZ];
    sprintf(buf, "%.2f", (float)value/100);
    SetDlgItemText(hDlg, item, buf);
}


#define HEX_BUF_SZ  16
unsigned int get_dlgitem_hex(HWND hDlg, UINT item, unsigned int def)
{
	char buf[HEX_BUF_SZ];
    unsigned int value;

    if (GetDlgItemText(hDlg, item, buf, HEX_BUF_SZ) == 0)
        return def;

    if (sscanf(buf,"0x%x", &value)==1 || sscanf(buf,"%x", &value)==1) {
        return value;
    }

    return def;
}

void set_dlgitem_hex(HWND hDlg, UINT item, int value)
{
	char buf[HEX_BUF_SZ];
    wsprintf(buf, "0x%x", value);
    SetDlgItemText(hDlg, item, buf);
}

/* ===================================================================================== */
/* QUANT MATRIX DIALOG ================================================================= */
/* ===================================================================================== */

void quant_upload(HWND hDlg, CONFIG* config)
{
	int i;

	for (i=0 ; i<64; i++) {
		SetDlgItemInt(hDlg, IDC_QINTRA00 + i, config->qmatrix_intra[i], FALSE);
		SetDlgItemInt(hDlg, IDC_QINTER00 + i, config->qmatrix_inter[i], FALSE);
	}
}


void quant_download(HWND hDlg, CONFIG* config)
{
	int i;

	for (i=0; i<64; i++) {
		int temp;

		temp = config_get_uint(hDlg, i + IDC_QINTRA00, config->qmatrix_intra[i]);
		CONSTRAINVAL(temp, 1, 255);
		config->qmatrix_intra[i] = temp;

		temp = config_get_uint(hDlg, i + IDC_QINTER00, config->qmatrix_inter[i]);
		CONSTRAINVAL(temp, 1, 255);
		config->qmatrix_inter[i] = temp;
	}
}


void quant_loadsave(HWND hDlg, CONFIG * config, int save)
{
	char file[MAX_PATH];
    OPENFILENAME ofn;
	HANDLE hFile;
	DWORD read=128, wrote=0;
	BYTE quant_data[128];

	strcpy(file, "\\matrix");
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);

	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = "All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST;

	if (save) {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		if (GetSaveFileName(&ofn)) {
			hFile = CreateFile(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

			quant_download(hDlg, config);
			memcpy(quant_data, config->qmatrix_intra, 64);
			memcpy(quant_data+64, config->qmatrix_inter, 64);

			if (hFile == INVALID_HANDLE_VALUE) {
				DPRINTF("Couldn't save quant matrix");
			}else{
				if (!WriteFile(hFile, quant_data, 128, &wrote, 0)) {
					DPRINTF("Couldnt write quant matrix");
				}
			}
			CloseHandle(hFile);
		}
	}else{
		ofn.Flags |= OFN_FILEMUSTEXIST; 
		if (GetOpenFileName(&ofn)) {
			hFile = CreateFile(file, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

			if (hFile == INVALID_HANDLE_VALUE) {
				DPRINTF("Couldn't load quant matrix");
			} else {
				if (!ReadFile(hFile, quant_data, 128, &read, 0)) {
					DPRINTF("Couldnt read quant matrix");
				}else{
					memcpy(config->qmatrix_intra, quant_data, 64);
					memcpy(config->qmatrix_inter, quant_data+64, 64);
					quant_upload(hDlg, config);
				}
			}
			CloseHandle(hFile);
		}
    }
}

/* quantization matrix dialog proc */

BOOL CALLBACK quantmatrix_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		config = (CONFIG*)lParam;
		quant_upload(hDlg, config);

		if (g_hTooltip)
		{
			EnumChildWindows(hDlg, enum_tooltips, 0);
		}
		break;

	case WM_COMMAND :

        if (HIWORD(wParam) == BN_CLICKED) {
            switch(LOWORD(wParam)) {
            case IDOK :
			    quant_download(hDlg, config);
			    EndDialog(hDlg, IDOK);
                break;

            case IDCANCEL :
                EndDialog(hDlg, IDCANCEL);
                break;

            case IDC_SAVE :
                quant_loadsave(hDlg, config, 1);
                break;

            case IDC_LOAD :
                quant_loadsave(hDlg, config, 0);
                break;

            default :
                return FALSE;
            }
            break;
        }
        return FALSE;

	default :
		return FALSE;
	}

	return TRUE;
}


/* ===================================================================================== */
/* ADVANCED DIALOG PAGES ================================================================ */
/* ===================================================================================== */

/* initialise pages */
void adv_init(HWND hDlg, int idd, CONFIG * config)
{
	unsigned int i;

    switch(idd) {
    case IDD_PROFILE :
		for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++)
			SendDlgItemMessage(hDlg, IDC_PROFILE_PROFILE, CB_ADDSTRING, 0, (LPARAM)profiles[i].name);
		SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_ADDSTRING, 0, (LPARAM)"H.263");
		SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_ADDSTRING, 0, (LPARAM)"MPEG");
		SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_ADDSTRING, 0, (LPARAM)"MPEG-Custom");
        break;
        
    case IDD_LEVEL :
		for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++)
			SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_ADDSTRING, 0, (LPARAM)profiles[i].name);
        break;
		
    case IDD_ZONE :
        EnableDlgWindow(hDlg, IDC_ZONE_FETCH, config->ci_valid);
        break;

    case IDD_MOTION :
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"0 - None");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"1 - Very Low");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"2 - Low");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"3 - Medium");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"4 - High");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"5 - Very High");
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_ADDSTRING, 0, (LPARAM)"6 - Ultra High");

		SendDlgItemMessage(hDlg, IDC_VHQ, CB_ADDSTRING, 0, (LPARAM)"0 - Off");
		SendDlgItemMessage(hDlg, IDC_VHQ, CB_ADDSTRING, 0, (LPARAM)"1 - Mode Decision");
		SendDlgItemMessage(hDlg, IDC_VHQ, CB_ADDSTRING, 0, (LPARAM)"2 - Limited Search");
		SendDlgItemMessage(hDlg, IDC_VHQ, CB_ADDSTRING, 0, (LPARAM)"3 - Medium Search");
		SendDlgItemMessage(hDlg, IDC_VHQ, CB_ADDSTRING, 0, (LPARAM)"4 - Wide Search");
        break;

    case IDD_DEBUG :
		/* force threads disabled */
		EnableWindow(GetDlgItem(hDlg, IDC_NUMTHREADS_STATIC), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_NUMTHREADS), FALSE);

		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"XVID");
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"DIVX");
		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_ADDSTRING, 0, (LPARAM)"DX50");
        break;
    }
}


/* enable/disable controls based on encoder-mode or user selection */

void adv_mode(HWND hDlg, int idd, CONFIG * config)
{
    int profile;
    int weight_en, quant_en;
    int cpu_force;
    int custom_quant, bvops;
    
    switch(idd) {
    case IDD_PROFILE :
        profile = SendDlgItemMessage(hDlg, IDC_PROFILE_PROFILE, CB_GETCURSEL, 0, 0);
        EnableDlgWindow(hDlg, IDC_BVOP, profiles[profile].flags&PROFILE_BVOP);
        
        EnableDlgWindow(hDlg, IDC_QUANTTYPE_S, profiles[profile].flags&PROFILE_MPEGQUANT);
        EnableDlgWindow(hDlg, IDC_QUANTTYPE_S, profiles[profile].flags&PROFILE_MPEGQUANT);
        EnableDlgWindow(hDlg, IDC_QUANTTYPE, profiles[profile].flags&PROFILE_MPEGQUANT);
        custom_quant = (profiles[profile].flags&PROFILE_MPEGQUANT) && SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_GETCURSEL, 0, 0)==QUANT_MODE_CUSTOM;
        EnableDlgWindow(hDlg, IDC_QUANTMATRIX, custom_quant);
        EnableDlgWindow(hDlg, IDC_LUMMASK, profiles[profile].flags&PROFILE_ADAPTQUANT);
        EnableDlgWindow(hDlg, IDC_INTERLACING, profiles[profile].flags&PROFILE_INTERLACE);
        EnableDlgWindow(hDlg, IDC_QPEL, profiles[profile].flags&PROFILE_QPEL);
        EnableDlgWindow(hDlg, IDC_GMC, profiles[profile].flags&PROFILE_GMC);
        EnableDlgWindow(hDlg, IDC_REDUCED, profiles[profile].flags&PROFILE_REDUCED);

        bvops = (profiles[profile].flags&PROFILE_BVOP) && IsDlgChecked(hDlg, IDC_BVOP);
		EnableDlgWindow(hDlg, IDC_MAXBFRAMES,       bvops);
		EnableDlgWindow(hDlg, IDC_BQUANTRATIO,      bvops);
		EnableDlgWindow(hDlg, IDC_BQUANTOFFSET,     bvops);
		EnableDlgWindow(hDlg, IDC_MAXBFRAMES_S,     bvops);
		EnableDlgWindow(hDlg, IDC_BQUANTRATIO_S,    bvops);
		EnableDlgWindow(hDlg, IDC_BQUANTOFFSET_S,   bvops);
		EnableDlgWindow(hDlg, IDC_PACKED,           bvops);
		EnableDlgWindow(hDlg, IDC_CLOSEDGOV,        bvops);

    case IDD_LEVEL :
        profile = SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_GETCURSEL, 0, 0);
        SetDlgItemInt(hDlg, IDC_LEVEL_WIDTH, profiles[profile].width, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_HEIGHT, profiles[profile].height, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_FPS, profiles[profile].fps, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_VMV, profiles[profile].max_vmv_buffer_sz, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_VCV, profiles[profile].vcv_decoder_rate, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_VBV, profiles[profile].max_vbv_size, FALSE);
        SetDlgItemInt(hDlg, IDC_LEVEL_BITRATE, profiles[profile].max_bitrate, FALSE);
        break;

    case IDD_ZONE :
        weight_en = IsDlgChecked(hDlg, IDC_ZONE_MODE_WEIGHT);
        quant_en =   IsDlgChecked(hDlg, IDC_ZONE_MODE_QUANT);
        EnableDlgWindow(hDlg, IDC_ZONE_WEIGHT, weight_en);
        EnableDlgWindow(hDlg, IDC_ZONE_QUANT, quant_en);
        EnableDlgWindow(hDlg, IDC_ZONE_SLIDER, weight_en|quant_en);

        if (weight_en) {
        	SendDlgItemMessage(hDlg, IDC_ZONE_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(001,200));
            SendDlgItemMessage(hDlg, IDC_ZONE_SLIDER, TBM_SETPOS, TRUE, get_dlgitem_float(hDlg, IDC_ZONE_WEIGHT, 100));
            SetDlgItemText(hDlg, IDC_ZONE_MIN, "0.01");
            SetDlgItemText(hDlg, IDC_ZONE_MAX, "2.00");
        }else if (quant_en) {
            SendDlgItemMessage(hDlg, IDC_ZONE_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(100,3100));
            SendDlgItemMessage(hDlg, IDC_ZONE_SLIDER, TBM_SETPOS, TRUE, get_dlgitem_float(hDlg, IDC_ZONE_QUANT, 100));
            SetDlgItemText(hDlg, IDC_ZONE_MIN, "1");
            SetDlgItemText(hDlg, IDC_ZONE_MAX, "31");
        }

        bvops = (profiles[config->profile].flags&PROFILE_BVOP) && config->use_bvop;
        EnableDlgWindow(hDlg, IDC_ZONE_BVOPTHRESHOLD_S, bvops);
        EnableDlgWindow(hDlg, IDC_ZONE_BVOPTHRESHOLD, bvops);
        break;

    case IDD_DEBUG :
	    cpu_force			= IsDlgChecked(hDlg, IDC_CPU_FORCE);
	    EnableDlgWindow(hDlg, IDC_CPU_MMX,		cpu_force);
	    EnableDlgWindow(hDlg, IDC_CPU_MMXEXT,	cpu_force);
	    EnableDlgWindow(hDlg, IDC_CPU_SSE,		cpu_force);
	    EnableDlgWindow(hDlg, IDC_CPU_SSE2,		cpu_force);
	    EnableDlgWindow(hDlg, IDC_CPU_3DNOW,	cpu_force);
	    EnableDlgWindow(hDlg, IDC_CPU_3DNOWEXT,	cpu_force);
        break;
    }
}


/* upload config data into dialog */
void adv_upload(HWND hDlg, int idd, CONFIG * config)
{
	switch (idd)
	{
	case IDD_PROFILE :
		SendDlgItemMessage(hDlg, IDC_PROFILE_PROFILE, CB_SETCURSEL, config->profile, 0);

        SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_SETCURSEL, config->quant_type, 0);
        CheckDlg(hDlg, IDC_LUMMASK, config->lum_masking);
  		CheckDlg(hDlg, IDC_INTERLACING, config->interlacing);
        CheckDlg(hDlg, IDC_QPEL, config->qpel);
  		CheckDlg(hDlg, IDC_GMC, config->gmc);
	    CheckDlg(hDlg, IDC_REDUCED, config->reduced_resolution);
        CheckDlg(hDlg, IDC_BVOP, config->use_bvop);
		
        SetDlgItemInt(hDlg, IDC_MAXBFRAMES, config->max_bframes, FALSE);
        set_dlgitem_float(hDlg, IDC_BQUANTRATIO, config->bquant_ratio);
		set_dlgitem_float(hDlg, IDC_BQUANTOFFSET, config->bquant_offset);
        CheckDlg(hDlg, IDC_PACKED, config->packed);
		CheckDlg(hDlg, IDC_CLOSEDGOV, config->closed_gov);
		break;

    case IDD_LEVEL :
        SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_SETCURSEL, config->profile, 0);
        break;

	case IDD_RC_CBR :
		SetDlgItemInt(hDlg, IDC_CBR_REACTIONDELAY, config->rc_reaction_delay_factor, FALSE);
		SetDlgItemInt(hDlg, IDC_CBR_AVERAGINGPERIOD, config->rc_averaging_period, FALSE);
		SetDlgItemInt(hDlg, IDC_CBR_BUFFER, config->rc_buffer, FALSE);
		break;

    case IDD_RC_2PASS1 :
        SetDlgItemText(hDlg, IDC_STATS, config->stats);
        CheckDlg(hDlg, IDC_DISCARD1PASS, config->discard1pass);
        break;

	case IDD_RC_2PASS2 :
		SetDlgItemText(hDlg, IDC_STATS, config->stats);
        SetDlgItemInt(hDlg, IDC_KFBOOST, config->keyframe_boost, FALSE);
		SetDlgItemInt(hDlg, IDC_KFREDUCTION, config->kfreduction, FALSE);

		SetDlgItemInt(hDlg, IDC_OVERFLOW_CONTROL_STRENGTH, config->overflow_control_strength, FALSE);
        SetDlgItemInt(hDlg, IDC_OVERIMP, config->twopass_max_overflow_improvement, FALSE);
		SetDlgItemInt(hDlg, IDC_OVERDEG, config->twopass_max_overflow_degradation, FALSE);

		SetDlgItemInt(hDlg, IDC_CURVECOMPH, config->curve_compression_high, FALSE);
		SetDlgItemInt(hDlg, IDC_CURVECOMPL, config->curve_compression_low, FALSE);
		break;

    case IDD_ZONE :
        SetDlgItemInt(hDlg, IDC_ZONE_FRAME, config->zones[config->cur_zone].frame, FALSE);

        CheckDlgButton(hDlg, IDC_ZONE_MODE_WEIGHT,   config->zones[config->cur_zone].mode == RC_ZONE_WEIGHT);
        CheckDlgButton(hDlg, IDC_ZONE_MODE_QUANT,         config->zones[config->cur_zone].mode == RC_ZONE_QUANT);

        set_dlgitem_float(hDlg, IDC_ZONE_WEIGHT, config->zones[config->cur_zone].weight);
        set_dlgitem_float(hDlg, IDC_ZONE_QUANT, config->zones[config->cur_zone].quant);
       
        CheckDlgButton(hDlg, IDC_ZONE_FORCEIVOP, config->zones[config->cur_zone].type==XVID_TYPE_IVOP);
        CheckDlgButton(hDlg, IDC_ZONE_GREYSCALE, config->zones[config->cur_zone].greyscale);
        CheckDlgButton(hDlg, IDC_ZONE_CHROMAOPT, config->zones[config->cur_zone].chroma_opt);

        SetDlgItemInt(hDlg, IDC_ZONE_BVOPTHRESHOLD, config->zones[config->cur_zone].bvop_threshold, TRUE);
        break;
	
	case IDD_MOTION :
		SendDlgItemMessage(hDlg, IDC_MOTION, CB_SETCURSEL, config->motion_search, 0);
		SendDlgItemMessage(hDlg, IDC_VHQ, CB_SETCURSEL, config->vhq_mode, 0);
        CheckDlg(hDlg, IDC_CHROMAME, config->chromame);
        CheckDlg(hDlg, IDC_CARTOON, config->cartoon_mode);
		SetDlgItemInt(hDlg, IDC_FRAMEDROP, config->frame_drop_ratio, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXKEY, config->max_key_interval, FALSE);
		SetDlgItemInt(hDlg, IDC_MINKEY, config->min_key_interval, FALSE);
        break;
        
	case IDD_QUANT :
		SetDlgItemInt(hDlg, IDC_MINIQUANT, config->min_iquant, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXIQUANT, config->max_iquant, FALSE);
		SetDlgItemInt(hDlg, IDC_MINPQUANT, config->min_pquant, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXPQUANT, config->max_pquant, FALSE);
		SetDlgItemInt(hDlg, IDC_MINBQUANT, config->min_bquant, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXBQUANT, config->max_bquant, FALSE);
        CheckDlg(hDlg, IDC_TRELLISQUANT, config->trellis_quant);
		break;

	case IDD_DEBUG :
		CheckDlg(hDlg, IDC_CPU_MMX, (config->cpu & XVID_CPU_MMX));
		CheckDlg(hDlg, IDC_CPU_MMXEXT, (config->cpu & XVID_CPU_MMXEXT));
		CheckDlg(hDlg, IDC_CPU_SSE, (config->cpu & XVID_CPU_SSE));
		CheckDlg(hDlg, IDC_CPU_SSE2, (config->cpu & XVID_CPU_SSE2));
		CheckDlg(hDlg, IDC_CPU_3DNOW, (config->cpu & XVID_CPU_3DNOW));
		CheckDlg(hDlg, IDC_CPU_3DNOWEXT, (config->cpu & XVID_CPU_3DNOWEXT));

		CheckRadioButton(hDlg, IDC_CPU_AUTO, IDC_CPU_FORCE, 
			config->cpu & XVID_CPU_FORCE ? IDC_CPU_FORCE : IDC_CPU_AUTO );

		SetDlgItemInt(hDlg, IDC_NUMTHREADS, config->num_threads, FALSE);

		SendDlgItemMessage(hDlg, IDC_FOURCC, CB_SETCURSEL, config->fourcc_used, 0);
        set_dlgitem_hex(hDlg, IDC_DEBUG, config->debug);
		CheckDlg(hDlg, IDC_VOPDEBUG, config->vop_debug);
        CheckDlg(hDlg, IDC_DISPLAY_STATUS, config->display_status);
		break;
	}
}


/* download config data from dialog */

void adv_download(HWND hDlg, int idd, CONFIG * config)
{
	switch (idd)
	{
	case IDD_PROFILE :
		config->profile = SendDlgItemMessage(hDlg, IDC_PROFILE_PROFILE, CB_GETCURSEL, 0, 0);

        config->quant_type = SendDlgItemMessage(hDlg, IDC_QUANTTYPE, CB_GETCURSEL, 0, 0);
        config->lum_masking = IsDlgChecked(hDlg, IDC_LUMMASK);
		config->interlacing = IsDlgChecked(hDlg, IDC_INTERLACING);
        config->qpel = IsDlgChecked(hDlg, IDC_QPEL);
		config->gmc = IsDlgChecked(hDlg, IDC_GMC);
		config->reduced_resolution = IsDlgChecked(hDlg, IDC_REDUCED);

        config->use_bvop = IsDlgChecked(hDlg, IDC_BVOP);
		config->max_bframes = config_get_uint(hDlg, IDC_MAXBFRAMES, config->max_bframes);
		config->bquant_ratio = get_dlgitem_float(hDlg, IDC_BQUANTRATIO, config->bquant_ratio);
		config->bquant_offset = get_dlgitem_float(hDlg, IDC_BQUANTOFFSET, config->bquant_offset);
		config->packed = IsDlgChecked(hDlg, IDC_PACKED);
		config->closed_gov = IsDlgChecked(hDlg, IDC_CLOSEDGOV);
		break;

    case IDD_LEVEL :
        config->profile = SendDlgItemMessage(hDlg, IDC_LEVEL_PROFILE, CB_GETCURSEL, 0, 0);
        break;

	case IDD_RC_CBR :
		config->rc_reaction_delay_factor = config_get_uint(hDlg, IDC_CBR_REACTIONDELAY, config->rc_reaction_delay_factor);
		config->rc_averaging_period = config_get_uint(hDlg, IDC_CBR_AVERAGINGPERIOD, config->rc_averaging_period);
		config->rc_buffer = config_get_uint(hDlg, IDC_CBR_BUFFER, config->rc_buffer);
		break;

	case IDD_RC_2PASS1 :
        if (GetDlgItemText(hDlg, IDC_STATS, config->stats, MAX_PATH) == 0)
			lstrcpy(config->stats, CONFIG_2PASS_FILE);
        config->discard1pass = IsDlgChecked(hDlg, IDC_DISCARD1PASS);
        break;

    case IDD_RC_2PASS2 :
        if (GetDlgItemText(hDlg, IDC_STATS, config->stats, MAX_PATH) == 0)
			lstrcpy(config->stats, CONFIG_2PASS_FILE);

        config->keyframe_boost = GetDlgItemInt(hDlg, IDC_KFBOOST, NULL, FALSE);
		config->kfreduction = GetDlgItemInt(hDlg, IDC_KFREDUCTION, NULL, FALSE);
		CONSTRAINVAL(config->keyframe_boost, 0, 1000);

		config->overflow_control_strength = GetDlgItemInt(hDlg, IDC_OVERFLOW_CONTROL_STRENGTH, NULL, FALSE);
		config->twopass_max_overflow_improvement = config_get_uint(hDlg, IDC_OVERIMP, config->twopass_max_overflow_improvement);
		config->twopass_max_overflow_degradation = config_get_uint(hDlg, IDC_OVERDEG, config->twopass_max_overflow_degradation);
		CONSTRAINVAL(config->twopass_max_overflow_improvement, 1, 80);
		CONSTRAINVAL(config->twopass_max_overflow_degradation, 1, 80);
		CONSTRAINVAL(config->overflow_control_strength, 0, 100);
		
		config->curve_compression_high = GetDlgItemInt(hDlg, IDC_CURVECOMPH, NULL, FALSE);
		config->curve_compression_low = GetDlgItemInt(hDlg, IDC_CURVECOMPL, NULL, FALSE);
		CONSTRAINVAL(config->curve_compression_high, 0, 100);
		CONSTRAINVAL(config->curve_compression_low, 0, 100);

		break;

    case IDD_ZONE :
        config->zones[config->cur_zone].frame = config_get_uint(hDlg, IDC_ZONE_FRAME, config->zones[config->cur_zone].frame);

        if (IsDlgChecked(hDlg, IDC_ZONE_MODE_WEIGHT)) {
            config->zones[config->cur_zone].mode = RC_ZONE_WEIGHT;
        }else if (IsDlgChecked(hDlg, IDC_ZONE_MODE_QUANT)) {
            config->zones[config->cur_zone].mode = RC_ZONE_QUANT;
        }

        config->zones[config->cur_zone].weight = get_dlgitem_float(hDlg, IDC_ZONE_WEIGHT, config->zones[config->cur_zone].weight);
        config->zones[config->cur_zone].quant =  get_dlgitem_float(hDlg, IDC_ZONE_QUANT, config->zones[config->cur_zone].quant);

        config->zones[config->cur_zone].type = IsDlgButtonChecked(hDlg, IDC_ZONE_FORCEIVOP)?XVID_TYPE_IVOP:XVID_TYPE_AUTO;
        config->zones[config->cur_zone].greyscale = IsDlgButtonChecked(hDlg, IDC_ZONE_GREYSCALE);
        config->zones[config->cur_zone].chroma_opt = IsDlgButtonChecked(hDlg, IDC_ZONE_CHROMAOPT);

        config->zones[config->cur_zone].bvop_threshold = config_get_int(hDlg, IDC_ZONE_BVOPTHRESHOLD, config->zones[config->cur_zone].bvop_threshold);
        break;

	case IDD_MOTION :
		config->motion_search = SendDlgItemMessage(hDlg, IDC_MOTION, CB_GETCURSEL, 0, 0);
		config->vhq_mode = SendDlgItemMessage(hDlg, IDC_VHQ, CB_GETCURSEL, 0, 0);
		config->chromame = IsDlgChecked(hDlg, IDC_CHROMAME);
		config->cartoon_mode = IsDlgChecked(hDlg, IDC_CARTOON);

        config->frame_drop_ratio = config_get_uint(hDlg, IDC_FRAMEDROP, config->frame_drop_ratio);

		config->max_key_interval = config_get_uint(hDlg, IDC_MAXKEY, config->max_key_interval);
		config->min_key_interval = config_get_uint(hDlg, IDC_MINKEY, config->min_key_interval);
		break;

	case IDD_QUANT :
		config->min_iquant = config_get_uint(hDlg, IDC_MINIQUANT, config->min_iquant);
		config->max_iquant = config_get_uint(hDlg, IDC_MAXIQUANT, config->max_iquant);
		config->min_pquant = config_get_uint(hDlg, IDC_MINPQUANT, config->min_pquant);
		config->max_pquant = config_get_uint(hDlg, IDC_MAXPQUANT, config->max_pquant);
		config->min_bquant = config_get_uint(hDlg, IDC_MINBQUANT, config->min_bquant);
		config->max_bquant = config_get_uint(hDlg, IDC_MAXBQUANT, config->max_bquant);

		CONSTRAINVAL(config->min_iquant, 1, 31);
		CONSTRAINVAL(config->max_iquant, config->min_iquant, 31);
		CONSTRAINVAL(config->min_pquant, 1, 31);
		CONSTRAINVAL(config->max_pquant, config->min_pquant, 31);
		CONSTRAINVAL(config->min_bquant, 1, 31);
		CONSTRAINVAL(config->max_bquant, config->min_bquant, 31);

        config->trellis_quant = IsDlgChecked(hDlg, IDC_TRELLISQUANT);
		break;

	case IDD_DEBUG :
		config->cpu = 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_MMX)      ? XVID_CPU_MMX : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_MMXEXT)   ? XVID_CPU_MMXEXT : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_SSE)      ? XVID_CPU_SSE : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_SSE2)     ? XVID_CPU_SSE2 : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_3DNOW)    ? XVID_CPU_3DNOW : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_3DNOWEXT) ? XVID_CPU_3DNOWEXT : 0;
		config->cpu |= IsDlgChecked(hDlg, IDC_CPU_FORCE)    ? XVID_CPU_FORCE : 0;

		config->num_threads = config_get_uint(hDlg, IDC_NUMTHREADS, config->num_threads);

        config->fourcc_used = SendDlgItemMessage(hDlg, IDC_FOURCC, CB_GETCURSEL, 0, 0);
        config->debug = get_dlgitem_hex(hDlg, IDC_DEBUG, config->debug);
        config->vop_debug = IsDlgChecked(hDlg, IDC_VOPDEBUG);
        config->display_status = IsDlgChecked(hDlg, IDC_DISPLAY_STATUS);
		break;
	}
}



/* advanced dialog proc */

BOOL CALLBACK adv_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETINFO *psi;

	psi = (PROPSHEETINFO*)GetWindowLong(hDlg, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		psi = (PROPSHEETINFO*) ((LPPROPSHEETPAGE)lParam)->lParam;
		SetWindowLong(hDlg, GWL_USERDATA, (LPARAM)psi);

		if (g_hTooltip)
			EnumChildWindows(hDlg, enum_tooltips, 0);

        adv_init(hDlg, psi->idd, psi->config);
		break;

	case WM_COMMAND :
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
            case IDC_BVOP :
            case IDC_ZONE_MODE_WEIGHT :
            case IDC_ZONE_MODE_QUANT :
            case IDC_ZONE_BVOPTHRESHOLD_ENABLE :
			case IDC_CPU_AUTO :
			case IDC_CPU_FORCE :
				adv_mode(hDlg, psi->idd, psi->config);
				break;

            case IDC_QUANTMATRIX :
    			DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_QUANTMATRIX), hDlg, quantmatrix_proc, (LPARAM)psi->config);
                break;

            case IDC_STATS_BROWSE :
            {
			    OPENFILENAME ofn;
			    char tmp[MAX_PATH];

			    GetDlgItemText(hDlg, IDC_STATS, tmp, MAX_PATH);

			    memset(&ofn, 0, sizeof(OPENFILENAME));
			    ofn.lStructSize = sizeof(OPENFILENAME);

			    ofn.hwndOwner = hDlg;
			    ofn.lpstrFilter = "bitrate curve (*.stats)\0*.stats\0All files (*.*)\0*.*\0\0";
			    ofn.lpstrFile = tmp;
			    ofn.nMaxFile = MAX_PATH;
			    ofn.Flags = OFN_PATHMUSTEXIST;

                if (psi->idd == IDD_RC_2PASS1) {
                    ofn.Flags |= OFN_OVERWRITEPROMPT;
                }else{
                    ofn.Flags |= OFN_FILEMUSTEXIST; 
                }
			    
			    if (GetSaveFileName(&ofn))
			    {
				    SetDlgItemText(hDlg, IDC_STATS, tmp);
                }
            }

            case IDC_ZONE_FETCH :
                SetDlgItemInt(hDlg, IDC_ZONE_FRAME, psi->config->ci.ciActiveFrame, FALSE);
                break;

            default :
                return TRUE;
            }
		}else if (HIWORD(wParam) == LBN_SELCHANGE &&
            (LOWORD(wParam) == IDC_PROFILE_PROFILE ||
             LOWORD(wParam) == IDC_LEVEL_PROFILE ||
             LOWORD(wParam) == IDC_QUANTTYPE))
		{
            adv_mode(hDlg, psi->idd, psi->config);
        }else if (HIWORD(wParam) == EN_UPDATE && (LOWORD(wParam)==IDC_ZONE_WEIGHT || LOWORD(wParam)==IDC_ZONE_QUANT)) {

            SendDlgItemMessage(hDlg, IDC_ZONE_SLIDER, TBM_SETPOS, TRUE, 
                    get_dlgitem_float(hDlg, LOWORD(wParam), 100));
        }else {
            return 0;
        }
		break;

	case WM_HSCROLL :
		if((HWND)lParam == GetDlgItem(hDlg, IDC_ZONE_SLIDER)) {
            int idc = IsDlgChecked(hDlg, IDC_ZONE_MODE_WEIGHT) ? IDC_ZONE_WEIGHT : IDC_ZONE_QUANT;
            set_dlgitem_float(hDlg, idc, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) );
            break;
        }
        return 0;

   
	case WM_NOTIFY :
		switch (((NMHDR *)lParam)->code)
		{
        case PSN_SETACTIVE :
            OutputDebugString("PSN_SET");
            adv_upload(hDlg, psi->idd, psi->config);
		    adv_mode(hDlg, psi->idd, psi->config);
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;

        case PSN_KILLACTIVE :
            OutputDebugString("PSN_KILL");
            adv_download(hDlg, psi->idd, psi->config);
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			break;

		case PSN_APPLY :
            OutputDebugString("PSN_APPLY");
			psi->config->save = TRUE;
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			break;
		}
		break;

	default :
		return 0;
	}

	return 1;
}




/* load advanced options property sheet 
  returns true, if the user accepted the changes
  or fasle if changes were canceled.

  */
BOOL adv_dialog(HWND hParent, CONFIG * config, const int * dlgs, int size)
{
	PROPSHEETINFO psi[6];
	PROPSHEETPAGE psp[6];
	PROPSHEETHEADER psh;
	CONFIG temp;
	int i;

	config->save = FALSE;
	memcpy(&temp, config, sizeof(CONFIG));

	for (i=0; i<size; i++)
	{
		psp[i].dwSize = sizeof(PROPSHEETPAGE);
		psp[i].dwFlags = 0;
		psp[i].hInstance = g_hInst;
		psp[i].pfnDlgProc = adv_proc;
		psp[i].lParam = (LPARAM)&psi[i];
		psp[i].pfnCallback = NULL;
		psp[i].pszTemplate = MAKEINTRESOURCE(dlgs[i]);

		psi[i].idd = dlgs[i];
		psi[i].config = &temp;
	}

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hParent;
	psh.hInstance = g_hInst;
	psh.pszCaption = (LPSTR) "XviD Configuration";
	psh.nPages = size;
	psh.nStartPage = 0;
	psh.ppsp = (LPCPROPSHEETPAGE)&psp;
	psh.pfnCallback = NULL;
	PropertySheet(&psh);

    if (temp.save)
		memcpy(config, &temp, sizeof(CONFIG));

    return temp.save;
}

/* ===================================================================================== */
/* MAIN DIALOG ========================================================================= */
/* ===================================================================================== */


void main_insert_zone(HWND hDlg, zone_t * s, int i, BOOL insert)
{
    char tmp[32];

    wsprintf(tmp,"%i",s->frame);

    if (insert) {
        LVITEM lvi; 

        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE; 
        lvi.state = 0; 
        lvi.stateMask = 0; 
        lvi.iImage = 0;
        lvi.pszText = tmp;
        lvi.cchTextMax = strlen(tmp);
        lvi.iItem = i;
        lvi.iSubItem = 0;
        ListView_InsertItem(hDlg, &lvi);
    }else{
        ListView_SetItemText(hDlg, i, 0, tmp);
    }

    if (s->mode == RC_ZONE_WEIGHT) {
        sprintf(tmp,"%.2f",(float)s->weight/100);
    }else if (s->mode == RC_ZONE_QUANT) {
        sprintf(tmp,"( %.2f )",(float)s->quant/100);
    }else {
        strcpy(tmp,"EXT");
    }
    ListView_SetItemText(hDlg, i, 1, tmp);

    tmp[0] = '\0';
    if (s->type==XVID_TYPE_IVOP)
        strcat(tmp, "K ");

    if (s->greyscale)
        strcat(tmp, "G ");
    
    if (s->chroma_opt)
        strcat(tmp, "C ");

    ListView_SetItemText(hDlg, i, 2, tmp);
}

static int g_use_bitrate = 1;

void main_mode(HWND hDlg, CONFIG * config)
{
    const int profile = SendDlgItemMessage(hDlg, IDC_PROFILE, CB_GETCURSEL, 0, 0);
    const int rc_mode = SendDlgItemMessage(hDlg, IDC_MODE, CB_GETCURSEL, 0, 0);
    /* enable target rate/size control only for 1pass and 2pass  modes*/
    const int target_en = rc_mode==RC_MODE_1PASS || rc_mode==RC_MODE_2PASS2;
   
    char buf[16];
    int max;

    g_use_bitrate = rc_mode==RC_MODE_1PASS || config->use_2pass_bitrate;

    if (g_use_bitrate) {
        SetDlgItemText(hDlg, IDC_BITRATE_S, "Target bitrate (kbps):");

        wsprintf(buf, "%i kbps", DEFAULT_MIN_KBPS);
        SetDlgItemText(hDlg, IDC_BITRATE_MIN, buf);

        max = profiles[profile].max_bitrate;
        if (max == 0) max = DEFAULT_MAX_KBPS;
        wsprintf(buf, "%i kbps", max);
        SetDlgItemText(hDlg, IDC_BITRATE_MAX, buf);

  	    SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(DEFAULT_MIN_KBPS, max));
        SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE, 
                        config_get_uint(hDlg, IDC_BITRATE, DEFAULT_MIN_KBPS) );

    }else{
        SetDlgItemText(hDlg, IDC_BITRATE_S, "Target size (kbytes):");
    }

    EnableDlgWindow(hDlg, IDC_BITRATE_S, target_en);
    EnableDlgWindow(hDlg, IDC_BITRATE, target_en);

    EnableDlgWindow(hDlg, IDC_BITRATE_MIN, target_en && g_use_bitrate);
    EnableDlgWindow(hDlg, IDC_BITRATE_MAX, target_en && g_use_bitrate);
    EnableDlgWindow(hDlg, IDC_SLIDER, target_en && g_use_bitrate);
}



void main_upload(HWND hDlg, CONFIG * config)
{
    int i;

    SendDlgItemMessage(hDlg, IDC_PROFILE, CB_SETCURSEL, config->profile, 0);
	SendDlgItemMessage(hDlg, IDC_MODE, CB_SETCURSEL, config->mode, 0);

    if (g_use_bitrate) {
        SetDlgItemInt(hDlg, IDC_BITRATE, config->bitrate, FALSE);
    }else{
        SetDlgItemInt(hDlg, IDC_BITRATE, config->desired_size, FALSE);
    }

    ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_ZONES));
    for (i=0; i < config->num_zones; i++) {
        main_insert_zone(GetDlgItem(hDlg,IDC_ZONES), &config->zones[i], i, TRUE);
    }  
}


/* downloads data from main dialog */
void main_download(HWND hDlg, CONFIG * config)
{
    config->profile = SendDlgItemMessage(hDlg, IDC_PROFILE, CB_GETCURSEL, 0, 0);
	config->mode = SendDlgItemMessage(hDlg, IDC_MODE, CB_GETCURSEL, 0, 0);

    if (g_use_bitrate) {
        config->bitrate = config_get_uint(hDlg, IDC_BITRATE, config->bitrate);
    }else{
        config->desired_size = config_get_uint(hDlg, IDC_BITRATE, config->desired_size);
    }
}


/* main dialog proc */

static const int profile_dlgs[] = { IDD_PROFILE, IDD_LEVEL };
static const int single_dlgs[] = { IDD_RC_CBR };
static const int pass1_dlgs[] = { IDD_RC_2PASS1 };
static const int pass2_dlgs[] = { IDD_RC_2PASS2 };
static const int zone_dlgs[] = { IDD_ZONE };
static const int bitrate_dlgs[] = { IDD_CALC };
static const int adv_dlgs[] = { IDD_MOTION, IDD_QUANT, IDD_DEBUG};


BOOL CALLBACK main_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);
    unsigned int i;

	switch (uMsg)
	{
	case WM_INITDIALOG :
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		config = (CONFIG*)lParam;

		for (i=0; i<sizeof(profiles)/sizeof(profile_t); i++)
			SendDlgItemMessage(hDlg, IDC_PROFILE, CB_ADDSTRING, 0, (LPARAM)profiles[i].name);

        SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Single pass");
		SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Twopass - 1st pass");
		SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Twopass - 2nd pass");
#ifdef _DEBUG
        SendDlgItemMessage(hDlg, IDC_MODE, CB_ADDSTRING, 0, (LPARAM)"Null test speed");
#endif

		InitCommonControls();

		if ((g_hTooltip = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				NULL, NULL, g_hInst, NULL)))
		{
			SetWindowPos(g_hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(g_hTooltip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, MAKELONG(1500, 0));
			SendMessage(g_hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);

			EnumChildWindows(hDlg, enum_tooltips, 0);
		}

        SetClassLong(GetDlgItem(hDlg, IDC_BITRATE_S), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_HAND));

        {
            DWORD ext_style = ListView_GetExtendedListViewStyle(GetDlgItem(hDlg,IDC_ZONES));
            ext_style |= LVS_EX_FULLROWSELECT | LVS_EX_FLATSB ;
            ListView_SetExtendedListViewStyle(GetDlgItem(hDlg,IDC_ZONES), ext_style);
        }

        {
            typedef struct {
                char * name;
                int value;
            } char_int_t;

            const static char_int_t columns[] = { 
                {"Frame #",     64},
                {"Weight (Q)",  72},
                {"Modifiers",   120}};

            LVCOLUMN lvc; 
            int i;
 
            /* Initialize the LVCOLUMN structure.  */
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
            lvc.fmt = LVCFMT_LEFT; 
  
            /* Add the columns.  */
            for (i=0; i<sizeof(columns)/sizeof(char_int_t); i++) {
                lvc.pszText = (char*)columns[i].name; 
                lvc.cchTextMax = strlen(columns[i].name);
                lvc.iSubItem = i;
                lvc.cx = columns[i].value;  /* column width, pixels */
                ListView_InsertColumn(GetDlgItem(hDlg,IDC_ZONES), i, &lvc);
            }
        }

        /* XXX: main_mode needs RC_MODE_xxx, main_upload needs g_use_bitrate set correctly... */
        main_upload(hDlg, config);
        main_mode(hDlg, config);
        main_upload(hDlg, config);
		break;

    case WM_NOTIFY :
        {
            NMHDR * n = (NMHDR*)lParam;

            if (n->code == NM_DBLCLK) {
                 NMLISTVIEW * nmlv = (NMLISTVIEW*) lParam;
                 config->cur_zone = nmlv->iItem;

                 main_download(hDlg, config);
                 if (config->cur_zone >= 0 && adv_dialog(hDlg, config, zone_dlgs, sizeof(zone_dlgs)/sizeof(int))) {
                     main_insert_zone(GetDlgItem(hDlg, IDC_ZONES), &config->zones[config->cur_zone], config->cur_zone, FALSE);
                 }
                 break;
            }

            if (n->code == NM_RCLICK) {
                OutputDebugString("Right click");
            }
        break;
        }

	case WM_COMMAND :
        if (HIWORD(wParam) == BN_CLICKED) {

            switch(LOWORD(wParam)) {
            case IDC_PROFILE_ADV :
                main_download(hDlg, config);
			    adv_dialog(hDlg, config, profile_dlgs, sizeof(profile_dlgs)/sizeof(int));

                SendDlgItemMessage(hDlg, IDC_PROFILE, CB_SETCURSEL, config->profile, 0);
                main_mode(hDlg, config);
                break;

            case IDC_MODE_ADV :
                main_download(hDlg, config);
    			if (config->mode == RC_MODE_1PASS) {
				    adv_dialog(hDlg, config, single_dlgs, sizeof(single_dlgs)/sizeof(int));
			    }else if (config->mode == RC_MODE_2PASS1) {
				    adv_dialog(hDlg, config, pass1_dlgs, sizeof(pass1_dlgs)/sizeof(int));
			    }else if (config->mode == RC_MODE_2PASS2) {
				    adv_dialog(hDlg, config, pass2_dlgs, sizeof(pass2_dlgs)/sizeof(int));
			    }
                break;


            case IDC_BITRATE_S :
                /* alternate between bitrate/desired_length metrics */
                main_download(hDlg, config);
                config->use_2pass_bitrate = !config->use_2pass_bitrate;
                main_mode(hDlg, config);
                main_upload(hDlg, config);
                break;

            case IDC_BITRATE_CALC :
                main_download(hDlg, config);
                adv_dialog(hDlg, config, bitrate_dlgs, sizeof(bitrate_dlgs)/sizeof(int));
                //SetDlgItemInt(hDlg, IDC_BITRATE, config->bitrate, FALSE);
                main_mode(hDlg, config);
                break;

            case IDC_ADD :
            {
                int i, sel, new_frame;

                if (config->num_zones >= MAX_ZONES) {
                    MessageBox(hDlg, "Exceeded maximum number of zones.\nIncrease config.h:MAX_ZONES and rebuild.", "Warning", 0);
                    break;
                }

                sel = ListView_GetNextItem(GetDlgItem(hDlg, IDC_ZONES), -1, LVNI_SELECTED);

                if (sel<0) {
                    if (config->ci_valid && config->ci.ciActiveFrame>0) {
                        for(sel=0; sel<config->num_zones-1 && config->zones[sel].frame<config->ci.ciActiveFrame; sel++) ;
                        sel--;
                        new_frame = config->ci.ciActiveFrame;
                    }else{
                        sel = config->num_zones-1;
                        new_frame = sel<0 ? 0 : config->zones[sel].frame + 1;
                    }
                }else{
                    new_frame = config->zones[sel].frame + 1;
                }

                for(i=config->num_zones-1; i>sel; i--) {
                    config->zones[i+1] = config->zones[i];
                }
                config->num_zones++;
                config->zones[sel+1].frame = new_frame;
                config->zones[sel+1].mode = RC_ZONE_WEIGHT;
                config->zones[sel+1].weight = 100;
                config->zones[sel+1].quant = 500;
                config->zones[sel+1].type = XVID_TYPE_AUTO;
                config->zones[sel+1].greyscale = 0;
                config->zones[sel+1].chroma_opt = 0;
                config->zones[sel+1].bvop_threshold = 0;

                ListView_SetItemState(GetDlgItem(hDlg, IDC_ZONES), sel, 0x00000000, LVIS_SELECTED);
                main_insert_zone(GetDlgItem(hDlg, IDC_ZONES), &config->zones[sel+1], sel+1, TRUE);
                ListView_SetItemState(GetDlgItem(hDlg, IDC_ZONES), sel+1, 0xffffffff, LVIS_SELECTED);
                break;
            }

            case IDC_REMOVE :
            {
                int i, sel;
                sel = ListView_GetNextItem(GetDlgItem(hDlg, IDC_ZONES), -1, LVNI_SELECTED);

                if (sel == -1) {
                    MessageBox(hDlg, "Nothing selected", "Warning", 0);
                    break;
                }

                for (i=sel; i<config->num_zones-1; i++) {
                    config->zones[i] = config->zones[i+1];
                }
                config->num_zones--;
                ListView_DeleteItem(GetDlgItem(hDlg, IDC_ZONES), sel);

                sel--;
                if (sel==0 && config->num_zones>1) {
                    sel=1;
                }
                ListView_SetItemState(GetDlgItem(hDlg, IDC_ZONES), sel, 0xffffffff, LVIS_SELECTED);
                break;
            }

            case IDC_EDIT :
                main_download(hDlg, config);
                config->cur_zone = ListView_GetNextItem(GetDlgItem(hDlg, IDC_ZONES), -1, LVNI_SELECTED);
                if (config->cur_zone != -1 && adv_dialog(hDlg, config, zone_dlgs, sizeof(zone_dlgs)/sizeof(int))) {
                    main_insert_zone(GetDlgItem(hDlg, IDC_ZONES), &config->zones[config->cur_zone], config->cur_zone, FALSE);
                }
                break;

            case IDC_ADVANCED :
                main_download(hDlg, config);
                adv_dialog(hDlg, config, adv_dlgs, sizeof(adv_dlgs)/sizeof(int));
                break;

            case IDC_DEFAULTS : 
			    config_reg_default(config);
                main_upload(hDlg, config);
                break;

            case IDOK :
			    main_download(hDlg, config);
			    config->save = TRUE;
			    EndDialog(hDlg, IDOK);
                break;

            case IDCANCEL :
    			config->save = FALSE;
	    		EndDialog(hDlg, IDCANCEL);
                break;
            }
        }else if (HIWORD(wParam) == LBN_SELCHANGE && 
            (LOWORD(wParam)==IDC_PROFILE || LOWORD(wParam)==IDC_MODE)) {

            main_download(hDlg, config);
            main_mode(hDlg, config);
            main_upload(hDlg, config);
        
        }else if (HIWORD(wParam)==EN_UPDATE && LOWORD(wParam)==IDC_BITRATE) {

            if (g_use_bitrate) {
                SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE, 
                        config_get_uint(hDlg, IDC_BITRATE, DEFAULT_MIN_KBPS) );
            }

        }else {
            return 0;
        }
		break;

	case WM_HSCROLL :
		if((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER)) {
            SetDlgItemInt(hDlg, IDC_BITRATE, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), FALSE);
            break;
        }
        return 0;

	default :
		return 0;
	}

	return 1;
}



/* ===================================================================================== */
/* ABOUT DIALOG ======================================================================== */
/* ===================================================================================== */

BOOL CALLBACK about_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			xvid_gbl_info_t info;
			char core[100];
			HFONT hFont;
			LOGFONT lfData;

			SetDlgItemText(hDlg, IDC_BUILD, XVID_BUILD);
			SetDlgItemText(hDlg, IDC_SPECIAL_BUILD, XVID_SPECIAL_BUILD);

			memset(&info, 0, sizeof(info));
			info.version = XVID_VERSION;
			xvid_global(0, XVID_GBL_INFO, &info, NULL);
			wsprintf(core, "libxvidcore version %d.%d.%d (\"%s\")",
				XVID_VERSION_MAJOR(info.actual_version),
				XVID_VERSION_MINOR(info.actual_version),
				XVID_VERSION_PATCH(info.actual_version),
				info.build);

			SetDlgItemText(hDlg, IDC_CORE, core);

			hFont = (HFONT)SendDlgItemMessage(hDlg, IDC_WEBSITE, WM_GETFONT, 0, 0L);

			if (GetObject(hFont, sizeof(LOGFONT), &lfData)) {
				lfData.lfUnderline = 1;

				hFont = CreateFontIndirect(&lfData);
				if (hFont) {
					SendDlgItemMessage(hDlg, IDC_WEBSITE, WM_SETFONT, (WPARAM)hFont, 1L);
				}
			}

			SetClassLong(GetDlgItem(hDlg, IDC_WEBSITE), GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_HAND));
			SetDlgItemText(hDlg, IDC_WEBSITE, XVID_WEBSITE);
		}
		break;

	case WM_CTLCOLORSTATIC :
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_WEBSITE))
		{
			SetBkMode((HDC)wParam, TRANSPARENT) ;             
			SetTextColor((HDC)wParam, RGB(0x00,0x00,0xc0));
			return (BOOL)GetStockObject(NULL_BRUSH); 
		}
		return 0;

	case WM_COMMAND :
		if (LOWORD(wParam) == IDC_WEBSITE && HIWORD(wParam) == STN_CLICKED) 
		{
			ShellExecute(hDlg, "open", XVID_WEBSITE, NULL, NULL, SW_SHOWNORMAL);
		}
		else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
		}
		break;

	default :
		return 0;
	}

	return 1;
}