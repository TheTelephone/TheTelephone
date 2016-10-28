/**
@file denoise_speex_tilde.c
@author Dennis Guse, Frank Haase, Michal Soloducha
@date 2016-10-21
@license GPLv3 or later

denoise_speex~ applies the _noise suppression_ algorithm of [Speex](http://www.speex.org/).

Parameters:
  denoise_speex~ FRAME_SIZE SAMPLE_RATE MAX_NOISE_ATTENUATION
  FRAME_SIZE: 80, 160, 240, 320, 640 [samples]
  SAMPLE_RATE: 8000, 16000, 32000 [Hz]
  MAX_NOISE_ATTENUATION: <-100,-1> [dB] (default: -15)

Inlets:
  1x Audio inlet
  
Outlets:
  1x Audio outlet

*/

#include <m_pd.h>
#include <limits.h>
#include "generic_codec.h"

#include "speex/speex.h"
#include "speex/speex_preprocess.h"

static t_class *denoise_speex_tilde_class;

typedef struct _denoise_speex_tilde {
  t_object x_obj;

  t_int max_noise_attenuation;

  t_float float_inlet;
  t_outlet *audio_out;

  t_generic_codec codec;

  void *speex_preprocess_state;

} t_denoise_speex_tilde;

void denoise_speex_add_to_outbuffer (t_denoise_speex_tilde * x);

t_int *denoise_speex_tilde_perform (t_int * w) {
  t_denoise_speex_tilde *x = (t_denoise_speex_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);

  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    denoise_speex_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void denoise_speex_add_to_outbuffer (t_denoise_speex_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  short raw[x->codec.ringbuffer_input->chunk_size];

  for (int i = 0; i < x->codec.ringbuffer_input->chunk_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }

  speex_preprocess_run (x->speex_preprocess_state, raw);

  for (int i = 0; i < x->codec.frame_size; i++) {
    frame[i] = (float) raw[i] / SHRT_MAX;
  }

  generic_codec_resample_to_external (&x->codec, x->codec.ringbuffer_input->chunk_size, frame);

  if (free_required) {
    free (frame);
  }
}

void denoise_speex_tilde_dsp (t_denoise_speex_tilde * x, t_signal ** sp) {
  if (x->speex_preprocess_state != NULL) {
    speex_preprocess_state_destroy (x->speex_preprocess_state);
  }

  x->speex_preprocess_state = speex_preprocess_state_init (x->codec.frame_size, x->codec.sample_rate_internal);

  int denoise_enabled = 1;
  speex_preprocess_ctl (x->speex_preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise_enabled);

  speex_preprocess_ctl (x->speex_preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &x->max_noise_attenuation);

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, denoise_speex_tilde_perform, sp);
}

void denoise_speex_tilde_free (t_denoise_speex_tilde * x) {
  generic_codec_free (&x->codec);

  outlet_free (x->audio_out);
}

void *denoise_speex_tilde_new (t_floatarg frame_size, t_floatarg sample_rate, t_floatarg max_noise_attenuation) {
  t_denoise_speex_tilde *x = (t_denoise_speex_tilde *) pd_new (denoise_speex_tilde_class);

  if ((int) frame_size != 80 && (int) frame_size != 160 && (int) frame_size != 240 && (int) frame_size != 320 && (int) frame_size != 640) {
    error ("denoise_speex~: invalid frame size specified (%d). Using 80.", (int) frame_size);
    frame_size = 80;
  }

  if ((int) sample_rate != 8000 && (int) sample_rate != 16000 && (int) sample_rate != 32000) {
    error ("denoise_speex~: invalid sample rate specified (%d). Using 8000.", (int) sample_rate);
    sample_rate = 8000;
  }

  if ((int) max_noise_attenuation > -1 && (int) max_noise_attenuation < -100) {
    error ("denoise_speex~: max. noise attenuation not specified or not in range <-100,-1>. Using -15.");
    max_noise_attenuation = -15;
  }

  x->speex_preprocess_state = NULL;
  x->max_noise_attenuation = max_noise_attenuation;

  generic_codec_init (&x->codec, &x->x_obj, sample_rate, frame_size);

  x->audio_out = outlet_new (&x->x_obj, &s_signal);

  post ("denoise_speex~: Created with frame size (%d), sampling rate (%f) and max. noise attenuation (%d).", x->codec.frame_size, x->codec.sample_rate_internal, x->max_noise_attenuation);

  return (void *) x;
}

void denoise_speex_tilde_setup (void) {
  denoise_speex_tilde_class = class_new (gensym ("denoise_speex~"), (t_newmethod) denoise_speex_tilde_new, (t_method) denoise_speex_tilde_free, sizeof (t_denoise_speex_tilde), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod (denoise_speex_tilde_class, (t_method) denoise_speex_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (denoise_speex_tilde_class, t_denoise_speex_tilde, float_inlet);
  class_sethelpsymbol (denoise_speex_tilde_class, gensym ("denoise_speex~"));
}
