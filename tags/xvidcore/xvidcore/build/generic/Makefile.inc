LIB=libcore.a
LIBSO=libxvidcore.so


SRCDIR = ../../src

SRC = \
 	$(SRCDIR)/decoder.c \
	$(SRCDIR)/divx4.c \
	$(SRCDIR)/encoder.c \
	$(SRCDIR)/xvid.c \
	$(SRCDIR)/bitstream/bitstream.c \
	$(SRCDIR)/bitstream/cbp.c \
	$(SRCDIR)/bitstream/mbcoding.c \
	$(SRCDIR)/dct/fdct.c \
	$(SRCDIR)/dct/idct.c \
	$(SRCDIR)/image/colorspace.c \
	$(SRCDIR)/image/image.c \
	$(SRCDIR)/image/interpolate8x8.c \
	$(SRCDIR)/motion/motion_comp.c \
	$(SRCDIR)/motion/motion_est.c \
	$(SRCDIR)/motion/sad.c \
	$(SRCDIR)/prediction/mbprediction.c \
	$(SRCDIR)/quant/adapt_quant.c \
	$(SRCDIR)/quant/quant_h263.c \
	$(SRCDIR)/quant/quant_matrix.c \
	$(SRCDIR)/quant/quant_mpeg4.c \
	$(SRCDIR)/utils/emms.c \
	$(SRCDIR)/utils/mbtransquant.c \
	$(SRCDIR)/utils/mem_align.c \
	$(SRCDIR)/utils/mem_transfer.c \
	$(SRCDIR)/utils/ratecontrol.c \
	$(SRCDIR)/utils/timer.c


DIRS = \
	$(SRCDIR)/*.o \
	$(SRCDIR)/bitstream/*.o \
	$(SRCDIR)/dct/*.o \
	$(SRCDIR)/image/*.o \
	$(SRCDIR)/motion/*.o \
	$(SRCDIR)/prediction/*.o \
	$(SRCDIR)/quant/*.o \
	$(SRCDIR)/utils/*.o


SRC_INTEL = \
	$(SRCDIR)/bitstream/x86_asm/cbp_mmx.asm \
	$(SRCDIR)/dct/x86_asm/fdct_mmx.asm \
	$(SRCDIR)/dct/x86_asm/idct_mmx.asm \
	$(SRCDIR)/image/x86_asm/interpolate8x8_mmx.asm \
	$(SRCDIR)/image/x86_asm/rgb_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yuv_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yuyv_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_rgb24_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_rgb32_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_yuyv_mmx.asm \
	$(SRCDIR)/motion/x86_asm/sad_mmx.asm \
	$(SRCDIR)/quant/x86_asm/quantize4_mmx.asm \
	$(SRCDIR)/quant/x86_asm/quantize_mmx.asm \
	$(SRCDIR)/utils/x86_asm/cpuid.asm \
	$(SRCDIR)/utils/x86_asm/mem_transfer_mmx.asm


DIRS_INTEL = \
	$(SRCDIR)/bitstream/x86_asm/*.o \
	$(SRCDIR)/dct/x86_asm/*.o \
	$(SRCDIR)/image/x86_asm/*.o \
	$(SRCDIR)/motion/x86_asm/*.o \
	$(SRCDIR)/quant/x86_asm/*.o \
	$(SRCDIR)/utils/x86_asm/*.o

SRC_PPC = \
	$(SRCDIR)/bitstream/ppc_asm/cbp_ppc.s

SRC_ALTIVEC = \
	$(SRCDIR)/bitstream/ppc_asm/cbp_altivec.s \
	$(SRCDIR)/dct/ppc_asm/fdct_altivec.s \
	$(SRCDIR)/dct/ppc_asm/idct_altivec.s \
	$(SRCDIR)/motion/ppc_asm/sad_altivec.s


DIRS_PPC = \
	$(SRCDIR)/bitstream/ppc_asm/*.o \
	$(SRCDIR)/dct/ppc_asm/*.o