/**
@file generic_codec.h
@author Frank Haase, Dennis Guse
@date 2016-08-24
@license GPLv3 or later

Contains generic data structure and functions required to implement codecs.
This includes resampler (input, output), ringbuffer (input, output) etc.

Developer note: both ringbuffers have a similar maximum size.

Audio signal flow:
  inlet -> resampler_input -> ringbuffer_input -> CODEC -> resampler_output -> ringbuffer_output -> outlet

*/

#ifndef GENERIC_CODEC_H_
#define GENERIC_CODEC_H_

#include <m_pd.h>
#include <stdbool.h>
#include "ringbuffer.h"
#include "resample.h"

typedef struct _generic_codec {
  float sample_rate_external;   //PureData's sample rate
  float sample_rate_internal;

  unsigned int frame_size;      //Number of samples per frame (sample_rate_internal)

  void *resampler_input;
  float_buffer *ringbuffer_input;

  void *resampler_output;
  float_buffer *ringbuffer_output;

  bool drop_next_frame;

  float *frame_last_decoded;    //Contains the last encoded and decoded frame (sample_rate_internal); used for packet loss concealment

  t_outlet *outlet;
} t_generic_codec;

static inline void generic_codec_init (t_generic_codec * codec, t_object * obj, float sample_rate_internal, unsigned int frame_size) {
  codec->sample_rate_internal = sample_rate_internal;
  codec->frame_size = frame_size;

  codec->resampler_input = NULL;
  codec->ringbuffer_input = NULL;

  codec->resampler_output = NULL;
  codec->ringbuffer_output = NULL;

  codec->outlet = outlet_new (obj, &s_signal);
}

static inline void generic_codec_free_internal (t_generic_codec * codec) {
  if (codec->resampler_input != NULL) {
    resample_close (codec->resampler_input);
  }
  if (codec->ringbuffer_input != NULL) {
    float_buffer_free (codec->ringbuffer_input);
  }
  free (codec->ringbuffer_input);

  if (codec->resampler_output != NULL) {
    resample_close (codec->resampler_output);
  }
  if (codec->ringbuffer_output != NULL) {
    float_buffer_free (codec->ringbuffer_output);
  }
  free (codec->ringbuffer_output);

  free (codec->frame_last_decoded);
}

static inline void generic_codec_free (t_generic_codec * codec) {
  outlet_free (codec->outlet);
  generic_codec_free_internal (codec);
}

static inline void generic_codec_dsp_add (t_generic_codec * codec, unsigned int block_size, void *x, t_perfroutine f, t_signal ** sp) {
  generic_codec_free_internal (codec);

  codec->sample_rate_external = sys_getsr ();

  double factor_in = (double) (codec->sample_rate_internal / codec->sample_rate_external);
  codec->resampler_input = resample_open (1, factor_in, factor_in);

  double factor_out = (double) (codec->sample_rate_external / codec->sample_rate_internal);
  codec->resampler_output = resample_open (1, factor_out, factor_out);

  //Buffers are allocated with a maximum of three times the INPUT block size
  codec->ringbuffer_input = float_buffer_alloc (codec->frame_size * 3, codec->frame_size);
  int output_size = (codec->frame_size * factor_out + .5) * 3;
  codec->ringbuffer_output = float_buffer_alloc (output_size, block_size);

  codec->drop_next_frame = false;

  codec->frame_last_decoded = calloc (block_size, sizeof (codec->frame_last_decoded));

  t_int signal_ref[4];
  signal_ref[0] = (t_int) x;
  signal_ref[1] = (t_int) sp[0]->s_vec;
  signal_ref[2] = (t_int) sp[1]->s_vec;
  signal_ref[3] = (t_int) sp[0]->s_n;

  dsp_addv (f, 4, signal_ref);
}

static inline void generic_codec_resample_to_internal (t_generic_codec * codec, unsigned int n, t_sample * in) {
  float buffer[n];
  for (int i = 0; i < n; i++) {
    buffer[i] = in[i];
  }

  unsigned int input_size;
  float *input = do_resample (n, buffer, codec->resampler_input, (double) (codec->sample_rate_internal / codec->sample_rate_external), &input_size);
  float_buffer_add_chunk (codec->ringbuffer_input, input, input_size);

  free (input);
}

static inline void generic_codec_resample_to_external (t_generic_codec * codec, unsigned int n, float *out_chunk) {
  unsigned int output_size;
  float *output = do_resample (n, out_chunk, codec->resampler_output, (double) (codec->sample_rate_external / codec->sample_rate_internal), &output_size);
  float_buffer_add_chunk (codec->ringbuffer_output, output, output_size);
  free (output);
}

static inline void generic_codec_to_outbuffer (t_generic_codec * codec, t_sample * out) {
  bool manual = false;
  float *out_chunk;
  float_buffer_pop_chunk (codec->ringbuffer_output, &out_chunk, codec->ringbuffer_output->chunk_size, &manual);

  for (int i = 0; i < codec->ringbuffer_output->chunk_size; i++) {
    out[i] = out_chunk[i];
  }

  if (manual) {
    free (out_chunk);
  }
}
#endif
