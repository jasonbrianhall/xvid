/**************************************************************************
 *
 *	XVID VFW FRONTEND
 *	codec
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
 *	12.07.2002	num_threads
 *	23.06.2002	XVID_CPU_CHKONLY; loading speed up
 *	25.04.2002	ICDECOMPRESS_PREROLL
 *	17.04.2002	re-enabled lumi masking for 1st pass
 *	15.04.2002	updated cbr support
 *	04.04.2002	separated 2-pass code to 2pass.c
 *				interlacing support
 *				hinted ME support
 *	23.03.2002	daniel smith <danielsmith@astroboymail.com>
 *				changed inter4v to only be in modes 5 or 6
 *				fixed null mode crash ?
 *				merged foxer's alternative 2-pass code
 *				added DEBUGERR output on errors instead of returning
 *	16.03.2002	daniel smith <danielsmith@astroboymail.com>
 *				changed BITMAPV4HEADER to BITMAPINFOHEADER
 *					- prevents memcpy crash in compress_get_format()
 *				credits are processed in external 2pass mode
 *				motion search precision = 0 now effective in 2-pass
 *				modulated quantization
 *				added DX50 fourcc
 *	01.12.2001	inital version; (c)2001 peter ross <pross@xvid.org>
 *
 *************************************************************************/

#include <windows.h>
#include <vfw.h>

#include "codec.h"
#include "2pass.h"

int pmvfast_presets[7] = {
	0, 0, 0, 0,
	0 | PMV_HALFPELREFINE16 | 0,
	0 | PMV_HALFPELREFINE16 | 0 |
	PMV_ADVANCEDDIAMOND16, PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 |
	PMV_HALFPELREFINE8 | 0 | PMV_USESQUARES16
};

/*	return xvid compatbile colorspace,
	or XVID_CSP_NULL if failure
*/

int get_colorspace(BITMAPINFOHEADER * hdr)
{
	/* rgb only: negative height specifies top down image */
	int rgb_flip = (hdr->biHeight < 0 ? 0 : XVID_CSP_VFLIP);

	switch(hdr->biCompression)
	{
	case BI_RGB :
		if (hdr->biBitCount == 16)
		{
			DEBUG("RGB16 (RGB555)");
			return rgb_flip | XVID_CSP_RGB555;
		}
		if (hdr->biBitCount == 24) 
		{
			DEBUG("RGB24");
			return rgb_flip | XVID_CSP_BGR;
		}
		if (hdr->biBitCount == 32) 
		{
			DEBUG("RGB32");
			return rgb_flip | XVID_CSP_BGRA;
		}

		DEBUG1("unsupported BI_RGB biBitCount", hdr->biBitCount);
		return XVID_CSP_NULL;

	case BI_BITFIELDS :
		if (hdr->biSize >= sizeof(BITMAPV4HEADER))
		{
			BITMAPV4HEADER * hdr4 = (BITMAPV4HEADER *)hdr;

			if (hdr4->bV4BitCount == 16 &&
				hdr4->bV4RedMask == 0x7c00 &&
				hdr4->bV4GreenMask == 0x3e0 &&
				hdr4->bV4BlueMask == 0x1f)
			{
				DEBUG("RGB555");
				return rgb_flip | XVID_CSP_RGB555;
			}

			if(hdr4->bV4BitCount == 16 &&
				hdr4->bV4RedMask == 0xf800 &&
				hdr4->bV4GreenMask == 0x7e0 &&
				hdr4->bV4BlueMask == 0x1f)
			{
				DEBUG("RGB565");
				return rgb_flip | XVID_CSP_RGB565;
			}

			DEBUG("unsupported BI_BITFIELDS mode");
			return XVID_CSP_NULL;
		}
		
		DEBUG("unsupported BI_BITFIELDS/BITMAPHEADER combination");
		return XVID_CSP_NULL;

	case FOURCC_I420 :
	case FOURCC_IYUV :
		DEBUG("IYUY");
		return XVID_CSP_I420;

	case FOURCC_YV12 :
		DEBUG("YV12");
		return XVID_CSP_YV12;
			
	case FOURCC_YUYV :
	case FOURCC_YUY2 :
		DEBUG("YUY2");
		return XVID_CSP_YUY2;

	case FOURCC_YVYU :
		DEBUG("YVYU");
		return XVID_CSP_YVYU;

	case FOURCC_UYVY :
		DEBUG("UYVY");
		return XVID_CSP_UYVY;

	default :
		DEBUGFOURCC("unsupported colorspace", hdr->biCompression);
		return XVID_CSP_NULL;
	}
}


/* compressor */


/* test the output format */

LRESULT compress_query(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
	BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;

	if (get_colorspace(inhdr) == XVID_CSP_NULL) 
	{
		return ICERR_BADFORMAT;
	}

	if (lpbiOutput == NULL) 
	{
		return ICERR_OK;
	}

	if (inhdr->biWidth != outhdr->biWidth || inhdr->biHeight != outhdr->biHeight ||
		(outhdr->biCompression != FOURCC_XVID && outhdr->biCompression != FOURCC_DIVX && outhdr->biCompression != FOURCC_DX50))
	{
		return ICERR_BADFORMAT;
	}

	return ICERR_OK;
}


LRESULT compress_get_format(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
	BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;

	if (get_colorspace(inhdr) == XVID_CSP_NULL)
	{
		return ICERR_BADFORMAT;
	}

	if (lpbiOutput == NULL) 
	{
		return sizeof(BITMAPV4HEADER);
	}

	memcpy(outhdr, inhdr, sizeof(BITMAPINFOHEADER));
	outhdr->biSize = sizeof(BITMAPINFOHEADER);
	outhdr->biSizeImage = compress_get_size(codec, lpbiInput, lpbiOutput);
	outhdr->biXPelsPerMeter = 0;
	outhdr->biYPelsPerMeter = 0;
	outhdr->biClrUsed = 0;
	outhdr->biClrImportant = 0;

	if (codec->config.fourcc_used == 0)
	{
		outhdr->biCompression = FOURCC_XVID;
	}
	else if (codec->config.fourcc_used == 1)
	{
		outhdr->biCompression = FOURCC_DIVX;
	}
	else
	{
		outhdr->biCompression = FOURCC_DX50;
	}

	return ICERR_OK;
}


LRESULT compress_get_size(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	return 2 * lpbiOutput->bmiHeader.biWidth * lpbiOutput->bmiHeader.biHeight * 3;
}


LRESULT compress_frames_info(CODEC * codec, ICCOMPRESSFRAMES * icf)
{
	// DEBUG2("frate fscale", codec->frate, codec->fscale);
	codec->fincr = icf->dwScale;
	codec->fbase = icf->dwRate;
	return ICERR_OK;
}


LRESULT compress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	xvid_gbl_init_t init;
	xvid_enc_create_t create;
	xvid_enc_rc_t rc;
    xvid_enc_plugin_t plugins[1];

	memset(&rc, 0, sizeof(rc));
	rc.version = XVID_VERSION;

	switch (codec->config.mode) 
	{
	case DLG_MODE_CBR :
		//rc.type = XVID_RC_CBR;
		rc.min_iquant = codec->config.min_iquant;
		rc.max_iquant = codec->config.max_iquant;
		rc.min_pquant = codec->config.min_pquant;
		rc.max_pquant = codec->config.max_pquant;
		rc.min_bquant = 0;	/* XXX: todo */
		rc.max_bquant = 0;	/* XXX: todo */
		rc.data.cbr.bitrate = codec->config.rc_bitrate;
        rc.data.cbr.reaction_delay_factor = codec->config.rc_reaction_delay_factor;
		rc.data.cbr.averaging_period = codec->config.rc_averaging_period;
		rc.data.cbr.buffer = codec->config.rc_buffer;
		break;

	case DLG_MODE_VBR_QUAL :
		codec->config.fquant = 0;
		//param.rc_bitrate = 0;
		break;

	case DLG_MODE_VBR_QUANT :
		codec->config.fquant = (float) codec->config.quant;
		//param.rc_bitrate = 0;
		break;

	case DLG_MODE_2PASS_1 :
	case DLG_MODE_2PASS_2_INT :
	case DLG_MODE_2PASS_2_EXT :
		//param.rc_bitrate = 0;
		codec->twopass.max_framesize = (int)((double)codec->config.twopass_max_bitrate / 8.0 / ((double)codec->fbase / (double)codec->fincr));
		break;

	case DLG_MODE_NULL :
		return ICERR_OK;

	default :
		break;
	}

    /* destroy previously created codec */
	if(codec->ehandle)
	{
		xvid_encore(codec->ehandle, XVID_ENC_DESTROY, NULL, NULL);
		codec->ehandle = NULL;
	}
	
    memset(&init, 0, sizeof(init));
	init.version = XVID_VERSION;
	init.cpu_flags = codec->config.cpu;
	xvid_global(0, XVID_GBL_INIT, &init, NULL);

	memset(&create, 0, sizeof(create));
	create.version = XVID_VERSION;

	create.width = lpbiInput->bmiHeader.biWidth;
	create.height = lpbiInput->bmiHeader.biHeight;
	create.fincr = codec->fincr;
	create.fbase = codec->fbase;

    create.plugins = plugins;
    create.num_plugins = 0;
   	if (codec->config.lum_masking) {
        plugins[create.num_plugins].func = xvid_plugin_lumimasking;
        plugins[create.num_plugins].param = NULL;
        create.num_plugins++; 
    }

    if (codec->config.packed) 
        create.global |= XVID_PACKED;
	if (codec->config.dx50bvop) 
		create.global |= XVID_CLOSED_GOP;

    create.max_key_interval = codec->config.max_key_interval;
	/* XXX: param.min_quantizer = codec->config.min_pquant;
	param.max_quantizer = codec->config.max_pquant; */

	create.max_bframes = codec->config.max_bframes;
	create.frame_drop_ratio = codec->config.frame_drop_ratio;

	create.bquant_ratio = codec->config.bquant_ratio;
	create.bquant_offset = codec->config.bquant_offset;

	create.num_threads = codec->config.num_threads;

	switch(xvid_encore(0, XVID_ENC_CREATE, &create, (codec->config.mode==DLG_MODE_CBR)?&rc:NULL )) 
	{
	case XVID_ERR_FAIL :	
		return ICERR_ERROR;

	case XVID_ERR_MEMORY :
		return ICERR_MEMORY;

	case XVID_ERR_FORMAT :
		return ICERR_BADFORMAT;

	case XVID_ERR_VERSION :
		return ICERR_UNSUPPORTED;
	}

	codec->ehandle = create.handle;
	codec->framenum = 0;
	codec->keyspacing = 0;

	codec->twopass.hints = codec->twopass.stats1 = codec->twopass.stats2 = INVALID_HANDLE_VALUE;
	codec->twopass.hintstream = NULL;

	return ICERR_OK;
}


LRESULT compress_end(CODEC * codec)
{
	if (codec->ehandle != NULL)
	{
		xvid_encore(codec->ehandle, XVID_ENC_DESTROY, NULL, NULL);

		if (codec->twopass.hints != INVALID_HANDLE_VALUE)
		{
			CloseHandle(codec->twopass.hints);
		}
		if (codec->twopass.stats1 != INVALID_HANDLE_VALUE)
		{
			CloseHandle(codec->twopass.stats1);
		}
		if (codec->twopass.stats2 != INVALID_HANDLE_VALUE)
		{
			CloseHandle(codec->twopass.stats2);
		}
		if (codec->twopass.hintstream != NULL)
		{
			free(codec->twopass.hintstream);
		}

		codec->ehandle = NULL;

		codec_2pass_finish(codec);
	}

	return ICERR_OK;
}

LRESULT compress(CODEC * codec, ICCOMPRESS * icc)
{
	BITMAPINFOHEADER * inhdr = icc->lpbiInput;
	BITMAPINFOHEADER * outhdr = icc->lpbiOutput;
	xvid_enc_frame_t frame;
	xvid_enc_stats_t stats;
	int length;
	
	// mpeg2avi yuv bug workaround (2 instances of CODEC)
	if (codec->twopass.stats1 == INVALID_HANDLE_VALUE)
	{
		if (codec_2pass_init(codec) == ICERR_ERROR)
		{
			return ICERR_ERROR;
		}
	}

	memset(&frame, 0, sizeof(frame));
	frame.version = XVID_VERSION;

    frame.type = XVID_TYPE_AUTO;

	/* vol stuff */

	if (codec->config.quant_type != QUANT_MODE_H263)
	{
		frame.vol_flags |= XVID_MPEGQUANT;

		// we actually need "default/custom" selectbox for both inter/intra
		// this will do for now
		if (codec->config.quant_type == QUANT_MODE_CUSTOM)
		{
			frame.quant_intra_matrix = codec->config.qmatrix_intra;
			frame.quant_inter_matrix = codec->config.qmatrix_inter;
		}
		else
		{
			frame.quant_intra_matrix = NULL;
			frame.quant_inter_matrix = NULL;
		}
	}

	if (codec->config.reduced_resolution) {
		frame.vol_flags |= XVID_REDUCED_ENABLE;
		frame.vop_flags |= XVID_REDUCED;	/* XXX: need auto decion mode */
	}

	if (codec->config.qpel) {
		frame.vol_flags |= XVID_QUARTERPEL;
		frame.motion |= PMV_QUARTERPELREFINE16 | PMV_QUARTERPELREFINE8;
	}

	if (codec->config.gmc)
		frame.vol_flags |= XVID_GMC;

	if (codec->config.interlacing)
		frame.vol_flags |= XVID_INTERLACING;

    /* vop stuff */

	frame.vop_flags |= XVID_HALFPEL;
	frame.vop_flags |= XVID_HQACPRED;

	if (codec->config.debug) 
		frame.vop_flags |= XVID_DEBUG;

	if (codec->config.motion_search > 4)
		frame.vop_flags |= XVID_INTER4V;

	if (codec->config.chromame)
		frame.vop_flags |= PMV_CHROMA16 + PMV_CHROMA8;

	if (codec->config.chroma_opt)
		frame.vop_flags |= XVID_CHROMAOPT;

	check_greyscale_mode(&codec->config, &frame, codec->framenum);

/* XXX: hinted me

// fix 1pass modes/hinted MV by koepi
	if (codec->config.hinted_me && (codec->config.mode == DLG_MODE_CBR || codec->config.mode == DLG_MODE_VBR_QUAL || codec->config.mode == DLG_MODE_VBR_QUANT))
	{
		codec->config.hinted_me = 0;
	}
// end of ugly hack

	if (codec->config.hinted_me && codec->config.mode == DLG_MODE_2PASS_1)
	{
		frame.hint.hintstream = codec->twopass.hintstream;
		frame.hint.rawhints = 0;
		frame.general |= XVID_HINTEDME_GET;
	}
	else if (codec->config.hinted_me && (codec->config.mode == DLG_MODE_2PASS_2_EXT || codec->config.mode == DLG_MODE_2PASS_2_INT))
	{
		DWORD read;
		DWORD blocksize;

		frame.hint.hintstream = codec->twopass.hintstream;
		frame.hint.rawhints = 0;
		frame.general |= XVID_HINTEDME_SET;

		if (codec->twopass.hints == INVALID_HANDLE_VALUE)
		{
			codec->twopass.hints = CreateFile(codec->config.hintfile, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (codec->twopass.hints == INVALID_HANDLE_VALUE)
			{
				DEBUGERR("couldn't open hints file");
				return ICERR_ERROR;
			}
		}
		if (!ReadFile(codec->twopass.hints, &blocksize, sizeof(DWORD), &read, 0) || read != sizeof(DWORD) ||
			!ReadFile(codec->twopass.hints, frame.hint.hintstream, blocksize, &read, 0) || read != blocksize)
		{
			DEBUGERR("couldn't read from hints file");
			return ICERR_ERROR;
		}
	} */

	frame.motion |= pmvfast_presets[codec->config.motion_search];

	switch (codec->config.vhq_mode)
	{
	case VHQ_MODE_DECISION :
		frame.vop_flags |= XVID_MODEDECISION_BITS;
		break;

	case VHQ_LIMITED_SEARCH :
		frame.vop_flags |= XVID_MODEDECISION_BITS;
		frame.motion |= HALFPELREFINE16_BITS;
		frame.motion |= QUARTERPELREFINE16_BITS;
		break;

	case VHQ_MEDIUM_SEARCH :
		frame.vop_flags |= XVID_MODEDECISION_BITS;
		frame.motion |= HALFPELREFINE16_BITS;
		frame.motion |= HALFPELREFINE8_BITS;
		frame.motion |= QUARTERPELREFINE16_BITS;
		frame.motion |= QUARTERPELREFINE8_BITS;
		frame.motion |= CHECKPREDICTION_BITS;
		break;

	case VHQ_WIDE_SEARCH :
		frame.vop_flags |= XVID_MODEDECISION_BITS;
		frame.motion |= HALFPELREFINE16_BITS;
		frame.motion |= HALFPELREFINE8_BITS;
		frame.motion |= QUARTERPELREFINE16_BITS;
		frame.motion |= QUARTERPELREFINE8_BITS;
		frame.motion |= CHECKPREDICTION_BITS;
		frame.motion |= EXTSEARCH_BITS;
		break;

	default :
		break;
	}

	frame.input.plane[0] = icc->lpInput;
	frame.input.stride[0] = (((icc->lpbiInput->biWidth * icc->lpbiInput->biBitCount) + 31) & ~31) >> 3;

	if ((frame.input.csp = get_colorspace(inhdr)) == XVID_CSP_NULL)
		return ICERR_BADFORMAT;

	if (frame.input.csp == XVID_CSP_I420 || frame.input.csp == XVID_CSP_YV12)
		frame.input.stride[0] = (frame.input.stride[0]*2)/3;

	frame.bitstream = icc->lpOutput;
	frame.length = icc->lpbiOutput->biSizeImage;

	switch (codec->config.mode) 
	{
	case DLG_MODE_CBR :
		frame.quant = 0;  /* use xvidcore cbr rate control */
		break;

	case DLG_MODE_VBR_QUAL :
	case DLG_MODE_VBR_QUANT :
	case DLG_MODE_2PASS_1 :
		if (codec_get_quant(codec, &frame) == ICERR_ERROR)
		{
			return ICERR_ERROR;
		}
		break;

	case DLG_MODE_2PASS_2_EXT :
	case DLG_MODE_2PASS_2_INT :
		if (codec_2pass_get_quant(codec, &frame) == ICERR_ERROR)
		{
			return ICERR_ERROR;
		}
		if (codec->config.dummy2pass)
		{
			outhdr->biSizeImage = codec->twopass.bytes2;
			*icc->lpdwFlags = (codec->twopass.nns1.quant & NNSTATS_KEYFRAME) ? AVIIF_KEYFRAME : 0;
			return ICERR_OK;
		}
		break;

	case DLG_MODE_NULL :
		outhdr->biSizeImage = 0;
		*icc->lpdwFlags = AVIIF_KEYFRAME;
		return ICERR_OK;

	default :
		DEBUGERR("Invalid encoding mode");
		return ICERR_ERROR;
	}


	// force keyframe spacing in 2-pass 1st pass
	if (codec->config.motion_search == 0)
	{
		frame.type = XVID_TYPE_IVOP;
	}
	else if (codec->keyspacing < codec->config.min_key_interval && codec->framenum)
	{
		DEBUG("current frame forced to p-frame");
		frame.type = XVID_TYPE_PVOP;
	}

	memset(&stats, 0, sizeof(stats));
	stats.version = XVID_VERSION;

    length = xvid_encore(codec->ehandle, XVID_ENC_ENCODE, &frame, &stats);
	switch (length) 
	{
	case XVID_ERR_FAIL :	
		return ICERR_ERROR;

	case XVID_ERR_MEMORY :
		return ICERR_MEMORY;

	case XVID_ERR_FORMAT :
		return ICERR_BADFORMAT;	

	case XVID_ERR_VERSION :
		return ICERR_UNSUPPORTED;
	}

	{  /* XXX: debug text */
		char tmp[100];
		wsprintf(tmp, " {type=%i len=%i} length=%i", stats.type, stats.length, length);
		OutputDebugString(tmp);
	}

    if (length == 0)	/* no encoder output */
	{
		*icc->lpdwFlags = 0;
		((char*)icc->lpOutput)[0] = 0x7f;	/* virtual dub skip frame */
		outhdr->biSizeImage = 1;
		
	}else{
		if (frame.out_flags & XVID_KEYFRAME)
	    {
		    codec->keyspacing = 0;
		    *icc->lpdwFlags = AVIIF_KEYFRAME;
	    }
	    else
	    {
		     *icc->lpdwFlags = 0;
	    }

    	outhdr->biSizeImage = length;

    	if (codec->config.mode == DLG_MODE_2PASS_1 && codec->config.discard1pass)
	    {
		    outhdr->biSizeImage = 0;
	    }

        /* XXX: hinted me 
	    if (frame.general & XVID_HINTEDME_GET)
    	{
	    	DWORD wrote;
	    	DWORD blocksize = frame.hint.hintlength;

    		if (codec->twopass.hints == INVALID_HANDLE_VALUE)
	    	{
		    	codec->twopass.hints = CreateFile(codec->config.hintfile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
			    if (codec->twopass.hints	== INVALID_HANDLE_VALUE)
    			{
	    			DEBUGERR("couldn't create hints file");
		    		return ICERR_ERROR;
    			}
	    	}
		    if (!WriteFile(codec->twopass.hints, &frame.hint.hintlength, sizeof(int), &wrote, 0) || wrote != sizeof(int) ||
			    !WriteFile(codec->twopass.hints, frame.hint.hintstream, blocksize, &wrote, 0) || wrote != blocksize)
    		{
	    		DEBUGERR("couldn't write to hints file");
		    	return ICERR_ERROR;
    		}
	    }
        */

        if (stats.type > 0)
	    {   
		  codec_2pass_update(codec, &stats);
	    }
    }

	++codec->framenum;
	++codec->keyspacing;

	return ICERR_OK;
}


/* decompressor */


LRESULT decompress_query(CODEC * codec, BITMAPINFO *lpbiInput, BITMAPINFO *lpbiOutput)
{
	BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
	BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;

	if (lpbiInput == NULL) 
	{
		return ICERR_ERROR;
	}

	if (inhdr->biCompression != FOURCC_XVID && inhdr->biCompression != FOURCC_DIVX && inhdr->biCompression != FOURCC_DX50 && get_colorspace(inhdr) == XVID_CSP_NULL)
	{
		return ICERR_BADFORMAT;
	}

	if (lpbiOutput == NULL) 
	{
		return ICERR_OK;
	}

	if (inhdr->biWidth != outhdr->biWidth ||
		inhdr->biHeight != outhdr->biHeight ||
		get_colorspace(outhdr) == XVID_CSP_NULL) 
	{
		return ICERR_BADFORMAT;
	}

	return ICERR_OK;
}


LRESULT decompress_get_format(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	BITMAPINFOHEADER * inhdr = &lpbiInput->bmiHeader;
	BITMAPINFOHEADER * outhdr = &lpbiOutput->bmiHeader;
	LRESULT result;

	if (lpbiOutput == NULL) 
	{
		return sizeof(BITMAPINFOHEADER);
	}

	/* --- yv12 --- */

	if (get_colorspace(inhdr) != XVID_CSP_NULL) {
		memcpy(outhdr, inhdr, sizeof(BITMAPINFOHEADER));
		// XXX: should we set outhdr->biSize ??
		return ICERR_OK;
	}
	/* --- yv12 --- */

	result = decompress_query(codec, lpbiInput, lpbiOutput);
	if (result != ICERR_OK) 
	{
		return result;
	}

	outhdr->biSize = sizeof(BITMAPINFOHEADER);
	outhdr->biWidth = inhdr->biWidth;
	outhdr->biHeight = inhdr->biHeight;
	outhdr->biPlanes = 1;
	outhdr->biBitCount = 24;
	outhdr->biCompression = BI_RGB;	/* sonic foundry vegas video v3 only supports BI_RGB */
	outhdr->biSizeImage = outhdr->biWidth * outhdr->biHeight * outhdr->biBitCount;
	outhdr->biXPelsPerMeter = 0;
	outhdr->biYPelsPerMeter = 0;
	outhdr->biClrUsed = 0;
	outhdr->biClrImportant = 0;

	return ICERR_OK;
}


LRESULT decompress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput)
{
	xvid_gbl_init_t init;
	xvid_dec_create_t create;

	memset(&init, 0, sizeof(init));
	init.version = XVID_VERSION;
	init.cpu_flags = codec->config.cpu;
	xvid_global(0, XVID_GBL_INIT, &init, NULL);

	memset(&create, 0, sizeof(create));
	create.version = XVID_VERSION;
	create.width = lpbiInput->bmiHeader.biWidth;
	create.height = lpbiInput->bmiHeader.biHeight;

	switch(xvid_decore(0, XVID_DEC_CREATE, &create, NULL)) 
	{
	case XVID_ERR_FAIL :	
		return ICERR_ERROR;

	case XVID_ERR_MEMORY :
		return ICERR_MEMORY;

	case XVID_ERR_FORMAT :
		return ICERR_BADFORMAT;

	case XVID_ERR_VERSION :
		return ICERR_UNSUPPORTED;
	}

	codec->dhandle = create.handle;

	return ICERR_OK;
}


LRESULT decompress_end(CODEC * codec)
{
	if (codec->dhandle != NULL)
	{
		xvid_decore(codec->dhandle, XVID_DEC_DESTROY, NULL, NULL);
		codec->dhandle = NULL;
	}
	return ICERR_OK;
}


LRESULT decompress(CODEC * codec, ICDECOMPRESS * icd)
{
	xvid_dec_frame_t frame;
	
	/* --- yv12 --- */	
	if (icd->lpbiInput->biCompression != FOURCC_XVID &&
		 icd->lpbiInput->biCompression != FOURCC_DIVX &&
		 icd->lpbiInput->biCompression != FOURCC_DX50)
	{
		xvid_gbl_convert_t convert;

		DEBUGFOURCC("input", icd->lpbiInput->biCompression);
		DEBUGFOURCC("output", icd->lpbiOutput->biCompression);

		memset(&convert, 0, sizeof(convert));
		convert.version = XVID_VERSION;

		convert.input.csp = get_colorspace(icd->lpbiInput);
		convert.input.plane[0] = icd->lpInput;
		convert.input.stride[0] = (((icd->lpbiInput->biWidth *icd->lpbiInput->biBitCount) + 31) & ~31) >> 3;  
		if (convert.input.csp == XVID_CSP_I420 || convert.input.csp == XVID_CSP_YV12)
			convert.input.stride[0] = (convert.input.stride[0]*2)/3;

		convert.output.csp = get_colorspace(icd->lpbiOutput);
		convert.output.plane[0] = icd->lpOutput;
		convert.output.stride[0] = (((icd->lpbiOutput->biWidth *icd->lpbiOutput->biBitCount) + 31) & ~31) >> 3;
		if (convert.output.csp == XVID_CSP_I420 || convert.output.csp == XVID_CSP_YV12)
			convert.output.stride[0] = (convert.output.stride[0]*2)/3;

		convert.width = icd->lpbiInput->biWidth;
		convert.height = icd->lpbiInput->biHeight;
		convert.interlacing = 0;
		if (convert.input.csp == XVID_CSP_NULL ||
			convert.output.csp == XVID_CSP_NULL ||
			xvid_global(0, XVID_GBL_CONVERT, &convert, NULL) < 0)
		{
			 return ICERR_BADFORMAT;
		}
		return ICERR_OK;
	}
	/* --- yv12 --- */
	
    memset(&frame, 0, sizeof(frame));
    frame.version = XVID_VERSION;
    frame.general = XVID_LOWDELAY;	/* force low_delay_default mode */
	frame.bitstream = icd->lpInput;
	frame.length = icd->lpbiInput->biSizeImage;
	
	if (~((icd->dwFlags & ICDECOMPRESS_HURRYUP) | (icd->dwFlags & ICDECOMPRESS_UPDATE) | (icd->dwFlags & ICDECOMPRESS_PREROLL)))
	{
		if ((frame.output.csp = get_colorspace(icd->lpbiOutput)) == XVID_CSP_NULL) 
		{
			return ICERR_BADFORMAT;
		}
		frame.output.plane[0] = icd->lpOutput;
		frame.output.stride[0] = (((icd->lpbiOutput->biWidth * icd->lpbiOutput->biBitCount) + 31) & ~31) >> 3;
		if (frame.output.csp == XVID_CSP_I420 || frame.output.csp == XVID_CSP_YV12)
			frame.output.stride[0] = (frame.output.stride[0]*2)/3;
	}
	else
	{
		frame.output.csp = XVID_CSP_NULL;
	}

	switch (xvid_decore(codec->dhandle, XVID_DEC_DECODE, &frame, NULL)) 
	{
	case XVID_ERR_FAIL :	
		return ICERR_ERROR;

	case XVID_ERR_MEMORY :
		return ICERR_MEMORY;

	case XVID_ERR_FORMAT :
		return ICERR_BADFORMAT;	

	case XVID_ERR_VERSION :
		return ICERR_UNSUPPORTED;
	}

	return ICERR_OK;
}

int codec_get_quant(CODEC* codec, xvid_enc_frame_t* frame)
{
	switch (codec->config.mode)
	{
	case DLG_MODE_VBR_QUAL :
		if (codec_is_in_credits(&codec->config, codec->framenum))
		{
// added by koepi for credits greyscale

			check_greyscale_mode(&codec->config, frame, codec->framenum);

// end of koepi's addition

			switch (codec->config.credits_mode)
			{
			case CREDITS_MODE_RATE :
				frame->quant = codec_get_vbr_quant(&codec->config, codec->config.quality * codec->config.credits_rate / 100);
				break;

			case CREDITS_MODE_QUANT :
				frame->quant = codec->config.credits_quant_p;
				break;

			default :
				DEBUGERR("Can't use credits size mode in quality mode");
				return ICERR_ERROR;
			}
		}
		else
		{
// added by koepi for credits greyscale

			check_greyscale_mode(&codec->config, frame, codec->framenum);

// end of koepi's addition

			frame->quant = codec_get_vbr_quant(&codec->config, codec->config.quality);
		}
		return ICERR_OK;

	case DLG_MODE_VBR_QUANT :
		if (codec_is_in_credits(&codec->config, codec->framenum))
		{
// added by koepi for credits greyscale

			check_greyscale_mode(&codec->config, frame, codec->framenum);

// end of koepi's addition

			switch (codec->config.credits_mode)
			{
			case CREDITS_MODE_RATE :
				frame->quant =
					codec->config.max_pquant -
					((codec->config.max_pquant - codec->config.quant) * codec->config.credits_rate / 100);
				break;

			case CREDITS_MODE_QUANT :
				frame->quant = codec->config.credits_quant_p;
				break;

			default :
				DEBUGERR("Can't use credits size mode in quantizer mode");
				return ICERR_ERROR;
			}
		}
		else
		{
// added by koepi for credits greyscale

			check_greyscale_mode(&codec->config, frame, codec->framenum);

// end of koepi's addition

			frame->quant = codec->config.quant;
		}
		return ICERR_OK;

	case DLG_MODE_2PASS_1 :
// added by koepi for credits greyscale

		check_greyscale_mode(&codec->config, frame, codec->framenum);

// end of koepi's addition

		if (codec->config.credits_mode == CREDITS_MODE_QUANT)
		{
			if (codec_is_in_credits(&codec->config, codec->framenum))
			{
				frame->quant = codec->config.credits_quant_p;
			}
			else
			{
				frame->quant = 2;
			}
		}
		else
		{
			frame->quant = 2;
		}
		return ICERR_OK;

	default:
		DEBUGERR("get quant: invalid mode");
		return ICERR_ERROR;
	}
}


int codec_is_in_credits(CONFIG* config, int framenum)
{
	if (config->credits_start)
	{
		if (framenum >= config->credits_start_begin &&
			framenum <= config->credits_start_end)
		{
			return CREDITS_START;
		}
	}

	if (config->credits_end)
	{
		if (framenum >= config->credits_end_begin &&
			framenum <= config->credits_end_end)
		{
			return CREDITS_END;
		}
	}

	return 0;
}


int codec_get_vbr_quant(CONFIG* config, int quality)
{
	static float fquant_running = 0;
	static int my_quality = -1;
	int	quant;

	// if quality changes, recalculate fquant (credits)
	if (quality != my_quality)
	{
		config->fquant = 0;
	}

	my_quality = quality;

	// desired quantiser = (maxQ-minQ)/100 * (100-qual) + minQ
	if (!config->fquant)
	{
		config->fquant =
			((float) (config->max_pquant - config->min_pquant) / 100) *
			(100 - quality) +
			(float) config->min_pquant;

		fquant_running = config->fquant;
	}

	if (fquant_running < config->min_pquant)
	{
		fquant_running = (float) config->min_pquant;
	}
	else if(fquant_running > config->max_pquant)
	{
		fquant_running = (float) config->max_pquant;
	}

	quant = (int) fquant_running;

	// add error between fquant and quant to fquant_running
	fquant_running += config->fquant - quant;

	return quant;
}

// added by koepi for credits greyscale

int check_greyscale_mode(CONFIG* config, xvid_enc_frame_t* frame, int framenum)

{

	if ((codec_is_in_credits(config, framenum)) && (config->mode!=DLG_MODE_CBR))

	{

		if (config->credits_greyscale)

		{

			if ((frame->vop_flags && XVID_GREYSCALE))  // use only if not already in greyscale

				frame->vop_flags |= XVID_GREYSCALE;

		} else {

			if (!(frame->vop_flags && XVID_GREYSCALE))  // if movie is in greyscale, switch back

				frame->vop_flags |= XVID_GREYSCALE;

		}

	} else {

		if (config->greyscale)

		{

			if ((frame->vop_flags && XVID_GREYSCALE))  // use only if not already in greyscale

				frame->vop_flags |= XVID_GREYSCALE;

		} else {

			if (!(frame->vop_flags && XVID_GREYSCALE))  // if credits is in greyscale, switch back

				frame->vop_flags |= XVID_GREYSCALE;

		}

	}

	return 0;

}

// end of koepi's addition

