/**
@file resample.h
@author Frank Haase, Dennis Guse
@date 2016-08-24
@license GPLv3 or later

Helper function for libresample to resample the complete signal.
Resample the source signal by individual blocks (src_blocksize).

*/

#ifndef RESAMPLE_H_
#define RESAMPLE_H_

#include <libresample.h>

#define MIN(A, B) (A) < (B)? (A) : (B)

/**
 * Resamples the input signal by the desired resampling factor.
 *
 * @param src_size Sample count of the input signal.
 * @param src The input signal.
 * @param resample_handle Handle of the resampler to be used (resample_open(...)).
 * @param resample_factor The resampling factor.
 * @param dst_size_max Will contain the sample count of the resampled signal.
 *
 * @return dst The resampled signal (must be freed later).
 *
 * @warning dst must be freed.
 *
 * @note Uses libresample.
 */
static inline float *do_resample (unsigned int src_size, float *src, void *resample_handle, double resample_factor, unsigned int *dst_size) {
  int dst_size_max = (int) (src_size * resample_factor) + 1000; //Maximal sample count for the resampled signal.
  unsigned int dst_blocksize = (int) (src_size * resample_factor + 10); //Maximal sample count per resampled block.

  float *dst = (float *) calloc (dst_size_max, sizeof (float));

  unsigned int dst_idx = 0;
  int dst_samplecount_current; //Might be negative if resampling fails.
  unsigned int src_idx = 0, src_processed, src_blocksize = 512;
  do {
    int src_blocksize_current = MIN (src_size - src_idx, src_blocksize);
    int dst_blocksize_current = MIN (dst_size_max - dst_idx, dst_blocksize);

    dst_samplecount_current = resample_process (resample_handle, resample_factor, &src[src_idx], src_blocksize_current, 0, &src_processed, &dst[dst_idx], dst_blocksize_current);

    src_idx += src_processed;

    if (dst_samplecount_current >= 0) {
      dst_idx += dst_samplecount_current;
    }
  } while (dst_samplecount_current > 0 && src_idx < src_size);

  *dst_size = dst_idx;
  return dst;
}
#endif
