/**
@file vad_speex_tilde.c
@author Dennis Guse, Frank Haase
@date 2016-08-26
@license GPLv3 or later

vad_speex~ applies the _voice activity detection_ algorithm of [Speex](http://www.speex.org/).

Parameters:
  vad_speex~ FRAME_SIZE SAMPLE_RATE
  FRAME_SIZE in samples: 80, 160, 240, 320
  SAMPLE_RATE in  Hz: 8000, 16000, 32000

Inlets:
  1x Audio inlet
  
Outlets:
  1x Audio inlet (routing incoming signal through)
  1x Bang (bang on vocie activity in current frame)

Developer note:
resampler_output and ringbuffer_output provided by generic_codec are not used.

@see denoise_speex_tilde.c
*/

#include <m_pd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "ringbuffer.h"
#include "generic_codec.h"

#include "speex/speex.h"
#include "speex/speex_preprocess.h"

static t_class *vad_speex_tilde_class;

typedef struct _vad_speex_tilde {
  t_object x_obj;

  t_float float_inlet_unused;

  t_generic_codec codec;        //Outbuffer is not unsed!

  void *speex_preprocess_state;

  t_outlet *outlet_bang_vad;

} t_vad_speex_tilde;

void vad_speex_add_to_outbuffer (t_vad_speex_tilde * x);

t_int *vad_speex_tilde_perform (t_int * w) {
  t_vad_speex_tilde *x = (t_vad_speex_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);
  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    vad_speex_add_to_outbuffer (x);
  }
  //Passthrough incoming signal
  for (int i = 0; i < n; i++) {
    out[i] = in[i];
  }
  return (w + 5);
}

void vad_speex_add_to_outbuffer (t_vad_speex_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  short raw[x->codec.ringbuffer_input->chunk_size];
  for (int i = 0; i < x->codec.ringbuffer_input->chunk_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }

  if (speex_preprocess_run (x->speex_preprocess_state, raw)) {
    outlet_bang (x->outlet_bang_vad);
  }

  if (free_required) {
    free (frame);
  }
}

void vad_speex_tilde_dsp (t_vad_speex_tilde * x, t_signal ** sp) {
  if (x->speex_preprocess_state != NULL) {
    speex_preprocess_state_destroy (x->speex_preprocess_state);
  }

  x->speex_preprocess_state = speex_preprocess_state_init (x->codec.frame_size, x->codec.sample_rate_internal);

  int vad_enabled = 1;
  speex_preprocess_ctl (x->speex_preprocess_state, SPEEX_PREPROCESS_SET_VAD, &vad_enabled);

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, vad_speex_tilde_perform, sp);
}

void vad_speex_tilde_free (t_vad_speex_tilde * x) {
  generic_codec_free (&x->codec);

  outlet_free (x->outlet_bang_vad);
}

void *vad_speex_tilde_new (t_floatarg frame_size, t_floatarg sample_rate) {
  t_vad_speex_tilde *x = (t_vad_speex_tilde *) pd_new (vad_speex_tilde_class);

  if ((int) frame_size != 80 && (int) frame_size != 160 && (int) frame_size != 240 && (int) frame_size != 320) {
    error ("vad_speex~: invalid frame size specified (%d). Using 80.", (int) frame_size);
    frame_size = 80;
  }

  if ((int) sample_rate != 8000 && (int) sample_rate != 16000 && (int) sample_rate != 32000) {
    error ("vad_speex~: invalid sample rate specified (%d). Using 8000.", (int) sample_rate);
    sample_rate = 8000;
  }

  x->speex_preprocess_state = NULL;

  generic_codec_init (&x->codec, &x->x_obj, sample_rate, frame_size);

  x->outlet_bang_vad = outlet_new (&x->x_obj, &s_bang);

  post ("vad_speex~: Created with frame size (%d) and sampling rate (%f).", x->codec.frame_size, x->codec.sample_rate_internal);

  return (void *) x;
}

void vad_speex_tilde_setup (void) {
  vad_speex_tilde_class = class_new (gensym ("vad_speex~"), (t_newmethod) vad_speex_tilde_new, (t_method) vad_speex_tilde_free, sizeof (t_vad_speex_tilde), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod (vad_speex_tilde_class, (t_method) vad_speex_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (vad_speex_tilde_class, t_vad_speex_tilde, float_inlet_unused);
  class_sethelpsymbol (vad_speex_tilde_class, gensym ("vad_speex~"));
}
