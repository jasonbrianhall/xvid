/*****************************************************************************
*
*  XVID MPEG-4 VIDEO CODEC
*  - XviD Main header file -
*
*  This program is free software ; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation ; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY ; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program ; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
*****************************************************************************/
/*****************************************************************************
*
*  History
*
*  - 2002/06/13 Added legal header, ANSI C comment style (only for this header
*               as it can be included in a ANSI C project).
*
*               ToDo ? : when BFRAMES is defined, the API_VERSION should not
*                        be the same (3.0 ?)
*
*  $Id: xvid.h,v 1.17.2.19 2003-01-04 06:14:32 suxen_drol Exp $
*
*****************************************************************************/


#ifndef _XVID_H_
#define _XVID_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Global constants
 ****************************************************************************/

/* API Version : 2.1 */
#define API_VERSION ((2 << 16) | (1))
#define XVID_API_UNSTABLE

/* Bitstream Version 
 * this will be writen into the bitstream to allow easy detection of xvid 
 * encoder bugs in the decoder, without this it might not possible to 
 * automatically distinquish between a file which has been encoded with an 
 * old & buggy XVID from a file which has been encoded with a bugfree version
 * see the infamous interlacing bug ...
 *
 * this MUST be increased if an encoder bug is fixed, increasing it too often
 * doesnt hurt but not increasing it could cause difficulty for decoders in the
 * future
 */
#define XVID_BS_VERSION "0003"


/* Error codes */
#define XVID_ERR_FAIL		-1
#define XVID_ERR_OK			0
#define	XVID_ERR_MEMORY		1
#define XVID_ERR_FORMAT		2


/* Colorspaces */
#define XVID_CSP_RGB24 	0		/* [b|g|r] */
#define XVID_CSP_YV12	1
#define XVID_CSP_YUY2	2
#define XVID_CSP_UYVY	3
#define XVID_CSP_I420	4
#define XVID_CSP_RGB555	10
#define XVID_CSP_RGB565	11
#define XVID_CSP_USER	12
#define XVID_CSP_EXTERN      1004  // per slice rendering
#define XVID_CSP_YVYU	1002
#define XVID_CSP_RGB32 	1000	/* [b|g|r|a] */
#define XVID_CSP_ABGR	1006	/* [a|b|g|r] */
#define XVID_CSP_RGBA	1005	/* [r|g|b|a] */



#define XVID_CSP_NULL 	9999

#define XVID_CSP_VFLIP	0x80000000	// flip mask


/*****************************************************************************
 *  Initialization constants
 ****************************************************************************/

/* CPU flags for XVID_INIT_PARAM.cpu_flags */
#define XVID_CPU_FORCE		0x80000000
#define XVID_CPU_CHKONLY	0x40000000		/* check cpu only; dont init globals */

#define XVID_CPU_ASM		0x00000080		/* native assembly */
/* ARCH_X86 */
#define XVID_CPU_MMX		0x00000001		/* mmx: pentiumMMX,k6 */
#define XVID_CPU_MMXEXT		0x00000002		/* mmxx-ext: pentium2,athlon */
#define XVID_CPU_SSE		0x00000004		/* sse: pentium3,athlonXP */
#define XVID_CPU_SSE2		0x00000008		/* sse2: pentium4,athlon64 */
#define XVID_CPU_3DNOW		0x00000010		/* 3dnow: k6-2 */
#define XVID_CPU_3DNOWEXT	0x00000020		/* 3dnow-ext: athlon */
#define XVID_CPU_TSC		0x00000040		/* timestamp counter */
/* ARCH_IA64 */
#define XVID_CPU_IA64		XVID_CPU_ASM	/* defined for backward compatibility */
/* ARCH_PPC */
#define XVID_CPU_ALTIVEC	0x00000001		/* altivec */


	typedef struct
	{
		int colorspace;
		void * y;
		void * u;
		void * v;
		int y_stride;
		int uv_stride;
	} XVID_IMAGE;		/* from yv12 */

#define XVID_INIT_INIT		0
#define XVID_INIT_CONVERT	1
#define XVID_INIT_TEST		2

/*****************************************************************************
 *  Initialization structures
 ****************************************************************************/

	typedef struct
	{
		int cpu_flags;
		int api_version;
		int core_build;
	}
	XVID_INIT_PARAM;

	typedef struct
	{
		XVID_IMAGE input;
		XVID_IMAGE output;
		int width;
		int height;
		int interlacing;
	} XVID_INIT_CONVERTINFO;

/*****************************************************************************
 *  Initialization entry point
 ****************************************************************************/

	int xvid_init(void *handle,
				  int opt,
				  void *param1,
				  void *param2);


/*****************************************************************************
 * Decoder constants
 ****************************************************************************/

/* Flags for XVID_DEC_FRAME.general */
#define XVID_DEC_LOWDELAY		0x00000001	/* decode lowdelay mode (ie. video-for-windows) */
#define XVID_DEC_DEBLOCKY		0x00000002	/* luma deblocking */
#define XVID_DEC_DEBLOCKUV		0x00000008	/* chroma deblocking */
#define XVID_DEC_DISCONTINUITY	0x00000004	/* indicates break in stream; instructs 
											decoder to ignore any previous reference frames */
#define XVID_QUICK_DECODE		0x00000010

/*****************************************************************************
 * Decoder structures
 ****************************************************************************/

	typedef struct
	{
		int width;
		int height;
		void *handle;
	}
	XVID_DEC_PARAM;


#define XVID_DEC_VOP	0
#define XVID_DEC_VOL	1
#define XVID_DEC_NOTHING	2	/* nothing was decoded */

	typedef struct
	{
		int notify;			/* [out]	output 'mode' */
		union
		{
			struct	/* XVID_DEC_VOP */
			{
				int time_base;		/* [out]	time base */
				int time_increment;	/* [out]	time increment */
			} vop;
			struct	/* XVID_DEC_VOL */
			{
				int general;		/* [out]	flags: eg. frames are interlaced */
				int width;			/* [out]	width */
				int height;			/* [out]	height */
				int aspect_ratio;	/* [out]	aspect ratio */
				int par_width;		/* [out]	aspect ratio width */
				int par_height;		/* [out]	aspect ratio height */
			} vol;
		} data;
	} XVID_DEC_STATS;


	typedef struct
	{
		int general;
		void *bitstream;
		int length;

		void *image;
		int stride;
		int colorspace;
	}
	XVID_DEC_FRAME;


	// This struct is used for per slice rendering
	typedef struct 
	{
		void *y,*u,*v;
		int stride_y, stride_u,stride_v;
	} XVID_DEC_PICTURE;


/*****************************************************************************
 * Decoder entry point
 ****************************************************************************/

/* decoder options */
#define XVID_DEC_DECODE		0
#define XVID_DEC_CREATE		1
#define XVID_DEC_DESTROY	2

	int xvid_decore(void *handle,
					int opt,
					void *param1,
					void *param2);


/*****************************************************************************
 * Encoder constants
 ****************************************************************************/

/* Flags for XVID_ENC_PARAM.global */
#define XVID_GLOBAL_PACKED		0x00000001	/* packed bitstream */
#define XVID_GLOBAL_DX50BVOP	0x00000002	/* dx50 bvop compatibility */
#define XVID_GLOBAL_DEBUG		0x00000004	/* print debug info on each frame */
#define XVID_GLOBAL_REDUCED		0x04000000	/* reduced resolution vop enable */

/* Flags for XVID_ENC_FRAME.general */
#define XVID_VALID_FLAGS		0x80000000

#define XVID_CUSTOM_QMATRIX		0x00000004	/* use custom quant matrix */
#define XVID_H263QUANT			0x00000010
#define XVID_MPEGQUANT			0x00000020
#define XVID_HALFPEL			0x00000040	/* use halfpel interpolation */
#define XVID_QUARTERPEL			0x02000000
#define XVID_ADAPTIVEQUANT		0x00000080
#define XVID_LUMIMASKING		0x00000100
#define XVID_LATEINTRA			0x00000200

#define XVID_INTERLACING		0x00000400	/* enable interlaced encoding */
#define XVID_TOPFIELDFIRST		0x00000800	/* set top-field-first flag  */
#define XVID_ALTERNATESCAN		0x00001000	/* set alternate vertical scan flag */

#define XVID_HINTEDME_GET		0x00002000	/* receive mv hint data from core (1st pass) */
#define XVID_HINTEDME_SET		0x00004000	/* send mv hint data to core (2nd pass) */

#define XVID_INTER4V			0x00008000

#define XVID_ME_ZERO			0x00010000
#define XVID_ME_LOGARITHMIC		0x00020000
#define XVID_ME_FULLSEARCH		0x00040000
#define XVID_ME_PMVFAST			0x00080000
#define XVID_ME_EPZS			0x00100000

#define XVID_GREYSCALE			0x01000000	/* enable greyscale only mode (even for */
#define XVID_GRAYSCALE			0x01000000  /* color input material chroma is ignored) */

#define XVID_GMC				0x20000000
#define XVID_ME_COLOUR			0x40000000

#define XVID_REDUCED			0x04000000	/* reduced resolution vop */

/* Flags for XVID_ENC_FRAME.motion */
#define PMV_ADVANCEDDIAMOND8	0x00004000
#define PMV_ADVANCEDDIAMOND16   0x00008000

#define PMV_HALFPELDIAMOND16 	0x00010000
#define PMV_HALFPELREFINE16 	0x00020000
#define PMV_QUARTERPELREFINE16	0x00040000
#define PMV_EXTSEARCH16 		0x00080000	/* extend PMV by more searches */
#define PMV_QUICKSTOP16	   		0x00100000	/* like early, but without any more refinement */
#define PMV_UNRESTRICTED16   	0x00200000	/* unrestricted ME, not implemented */
#define PMV_OVERLAPPING16   	0x00400000	/* overlapping ME, not implemented */
#define PMV_USESQUARES16		0x00800000

#define PMV_HALFPELDIAMOND8 	0x01000000
#define PMV_HALFPELREFINE8 		0x02000000
#define PMV_QUARTERPELREFINE8	0x04000000
#define PMV_EXTSEARCH8 			0x08000000	/* extend PMV by more searches */
#define PMV_QUICKSTOP8	   		0x10000000	/* like early, but without any more refinement */
#define PMV_UNRESTRICTED8   	0x20000000	/* unrestricted ME, not implemented */
#define PMV_OVERLAPPING8   		0x40000000	/* overlapping ME, not implemented */
#define PMV_USESQUARES8			0x80000000



/*****************************************************************************
 * Encoder structures
 ****************************************************************************/

	typedef struct
	{
		int width, height;
		int fincr, fbase;		/* frame increment, fbase. each frame = "fincr/fbase" seconds */
		int rc_bitrate;			/* the bitrate of the target encoded stream, in bits/second */
		int rc_reaction_delay_factor;	/* how fast the rate control reacts - lower values are faster */
		int rc_averaging_period;	/* as above */
		int rc_buffer;			/* as above */
		int max_quantizer;		/* the upper limit of the quantizer */
		int min_quantizer;		/* the lower limit of the quantizer */
		int max_key_interval;	/* the maximum interval between key frames */
#ifdef _SMP
		int num_threads;		/* number of threads */
#endif
		int global;				/* global/debug options */
		int max_bframes;		/* max sequential bframes (0=disable bframes) */
		int bquant_ratio;		/* bframe quantizer multipier (percentage).
								 * used only when bquant < 1
								 * eg. 200 = x2 multiplier
								 * quant = ((past_quant + future_quant) * bquant_ratio)/200
								 */
		int bquant_offset;		/* bquant += bquant_offset */
		int frame_drop_ratio;   /* frame dropping: 0=drop none... 100=drop all */
		void *handle;			/* [out] encoder instance handle */
	}
	XVID_ENC_PARAM;

	typedef struct
	{
		int x;
		int y;
	}
	VECTOR;

	typedef struct
	{
		int mode;				/* macroblock mode */
		VECTOR mvs[4];
	}
	MVBLOCKHINT;

	typedef struct
	{
		int intra;				/* frame intra choice */
		int fcode;				/* frame fcode */
		MVBLOCKHINT *block;		/* caller-allocated array of block hints (mb_width * mb_height) */
	}
	MVFRAMEHINT;

	typedef struct
	{
		int rawhints;			/* if set, use MVFRAMEHINT, else use compressed buffer */

		MVFRAMEHINT mvhint;
		void *hintstream;		/* compressed hint buffer */
		int hintlength;			/* length of buffer (bytes) */
	}
	HINTINFO;

	typedef struct
	{
		int general;			/* [in] general options */
		int motion;				/* [in] ME options */
		void *bitstream;		/* [in] bitstream ptr */
		int length;				/* [out] bitstream length (bytes) */

		void *image;			/* [in] image ptr */
		int stride;
		int colorspace;			/* [in] source colorspace */

		unsigned char *quant_intra_matrix;	// [in] custom intra qmatrix */
		unsigned char *quant_inter_matrix;	// [in] custom inter qmatrix */
		int quant;				/* [in] frame quantizer (vbr) */
		int intra;				/* [in] force intra frame (vbr only)
								 * [out] intra state
								 */
		HINTINFO hint;			/* [in/out] mv hint information */

		int bquant;				/* [in] bframe quantizer */

	}
	XVID_ENC_FRAME;


	typedef struct
	{
		int quant;				/* [out] frame quantizer */
		int hlength;			/* [out] header length (bytes) */
		int kblks, mblks, ublks;	/* [out] */
	}
	XVID_ENC_STATS;


/*****************************************************************************
 * Encoder entry point
 ****************************************************************************/

/* Encoder options */
#define XVID_ENC_ENCODE		0
#define XVID_ENC_CREATE		1
#define XVID_ENC_DESTROY	2

	int xvid_encore(void *handle,
					int opt,
					void *param1,
					void *param2);


#ifdef __cplusplus
}
#endif

#endif
