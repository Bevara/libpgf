/*
 *  Minimal C surface over libpgf.
 *
 *  libpgf is C++ and reports every error by throwing, while the filter itself
 *  is plain C against the GPAC API. Keeping the two apart means the filter
 *  never sees a C++ type and every libpgf exception is caught at this boundary
 *  and turned into a return code.
 */

#ifndef _PGF_DECODE_H_
#define _PGF_DECODE_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decodes a whole PGF file to a tightly packed 24-bit RGB buffer.
 *
 * On success returns 0, stores a malloc()'d buffer in *out (released with
 * pgf_decode_free) and its dimensions in *width and *height. */
int pgf_decode_rgb(const unsigned char *data, size_t size,
                   unsigned char **out, unsigned int *width, unsigned int *height);

void pgf_decode_free(unsigned char *buffer);

#define PGF_DEC_OK              0
#define PGF_DEC_ERR_MEMORY     -1
#define PGF_DEC_ERR_BITSTREAM  -2   /* not a PGF file, or a broken one */
#define PGF_DEC_ERR_MODE       -3   /* colour mode this filter does not handle */

#ifdef __cplusplus
}
#endif

#endif
