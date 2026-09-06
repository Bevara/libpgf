/*
 *  libpgf-facing half of the PGF filter - see pgf_decode.h for why it is a
 *  translation unit of its own.
 */

/* PGFplatform.h works out its target from __linux__/__APPLE__/__GLIBC__, none
 * of which emscripten defines, and falls back to the Win32 branch; __POSIX__
 * picks the right one. This matches how libpgf.a itself was built. */
#define __POSIX__ 1

#include <stdlib.h>
#include <string.h>
#include <new>

#include <PGFimage.h>

#include "pgf_decode.h"

extern "C" void pgf_decode_free(unsigned char *buffer)
{
	free(buffer);
}

extern "C" int pgf_decode_rgb(const unsigned char *data, size_t size,
                              unsigned char **out, unsigned int *width, unsigned int *height)
{
	unsigned char *rgb = NULL;
	unsigned char *scratch = NULL;
	int ret = PGF_DEC_ERR_BITSTREAM;

	/* Reject anything that is not a PGF stream up front. libpgf reports a bad
	 * header by throwing, and __cxa_throw is fatal here (see pgf_stubs.cpp), so
	 * the cheap magic check is what keeps a wrong or truncated file from taking
	 * the whole module down. */
	if (size < 8 || memcmp(data, "PGF", 3) != 0)
		return PGF_DEC_ERR_BITSTREAM;

	try
	{
		/* CPGFMemoryStream only borrows the buffer, but its constructor takes a
		 * non-const pointer; nothing in the read path writes to it. */
		CPGFMemoryStream stream(const_cast<UINT8 *>((const UINT8 *)data), size);
		CPGFImage pgf;

		pgf.Open(&stream);

		const UINT32 w = pgf.Width();
		const UINT32 h = pgf.Height();
		const BYTE channels = pgf.Channels();
		const BYTE mode = pgf.Mode();
		if (!w || !h)
			return PGF_DEC_ERR_BITSTREAM;

		/* Only the 8-bit-per-channel modes are handled. The 16- and 32-bit ones
		 * (RGB48, Gray16/32, CMYK64...) would need a depth reduction that has no
		 * single right answer, and none of them appears in the test corpus. */
		if (mode != ImageModeRGBColor && mode != ImageModeGrayScale && mode != ImageModeRGBA)
			return PGF_DEC_ERR_MODE;
		/* Not ChannelDepth(): that one reports the internal wavelet coefficient
		 * width (16 or 32 bits), not the image's. UsedBitsPerChannel is the
		 * per-channel depth of the picture itself. */
		if (pgf.UsedBitsPerChannel() != 8 || pgf.BPP() != channels * 8)
			return PGF_DEC_ERR_MODE;

		pgf.Read();

		rgb = (unsigned char *)malloc((size_t)w * h * 3);
		if (!rgb)
			return PGF_DEC_ERR_MEMORY;

		if (channels == 3)
		{
			/* GetBitmap writes channel 0 at byte channelMap[0], and after the
			 * YUV->RGB step channel 0 holds blue - so the default map {0,1,2}
			 * produces BGR. Reversing it gives the RGB the pid expects. */
			int map[3] = {2, 1, 0};
			pgf.GetBitmap((int)(w * 3), (UINT8 *)rgb, 24, map);
		}
		else if (channels == 1)
		{
			/* Grey is replicated over the three channels: a GREYSCALE pid has no
			 * adaptation path to writegen in this build (see the note in
			 * test-player/libpng.js). */
			scratch = (unsigned char *)malloc((size_t)w * h);
			if (!scratch)
			{
				free(rgb);
				return PGF_DEC_ERR_MEMORY;
			}
			pgf.GetBitmap((int)w, (UINT8 *)scratch, 8);
			for (size_t i = 0; i < (size_t)w * h; i++)
			{
				rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = scratch[i];
			}
			free(scratch);
			scratch = NULL;
		}
		else if (channels == 4)
		{
			/* Decode as BGRA, then drop alpha - same reason as above. */
			int map[4] = {2, 1, 0, 3};
			scratch = (unsigned char *)malloc((size_t)w * h * 4);
			if (!scratch)
			{
				free(rgb);
				return PGF_DEC_ERR_MEMORY;
			}
			pgf.GetBitmap((int)(w * 4), (UINT8 *)scratch, 32, map);
			for (size_t i = 0; i < (size_t)w * h; i++)
				memcpy(rgb + i * 3, scratch + i * 4, 3);
			free(scratch);
			scratch = NULL;
		}
		else
		{
			free(rgb);
			return PGF_DEC_ERR_MODE;
		}

		*out = rgb;
		*width = (unsigned int)w;
		*height = (unsigned int)h;
		return PGF_DEC_OK;
	}
	catch (...)
	{
		/* libpgf signals failures by throwing IOException. Since no solver
		 * exports __cxa_throw, pgf_stubs.cpp defines it locally and it aborts,
		 * so in this build a throw does not actually reach here - the magic
		 * check above is what handles the realistic failure. The handler is
		 * kept so the same source still behaves correctly if it is ever linked
		 * against a solver with working exceptions. */
		free(scratch);
		free(rgb);
		return ret;
	}
}
