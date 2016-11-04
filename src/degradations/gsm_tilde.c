/**
@file gsm_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-25
@license GPLv3 or later

gsm~ encodes the signal with [GSM Full rate / GSM 6.10](http://en.wikipedia.org/wiki/Full_Rate) (8kHz).

Parameters:
  gsm10~

Inlets:
  1x Audio inlet
  also bang: lose next frame (not implemented)

Outlets:
  1x Audio outlet

*/

#include <m_pd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "ringbuffer.h"
#include "generic_codec.h"

#include <gsm.h>

static t_class *gsm_tilde_class;

typedef struct _gsm_tilde {
  t_object x_obj;

  t_generic_codec codec;

  gsm decoder;
  gsm encoder;

  t_float float_inlet;
} t_gsm_tilde;

void gsm_add_to_outbuffer (t_gsm_tilde * x);

t_int *gsm_tilde_perform (t_int * w) {
  t_gsm_tilde *x = (t_gsm_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);

  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    gsm_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void gsm_add_to_outbuffer (t_gsm_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  short raw[x->codec.frame_size];
  for (int i = 0; i < x->codec.frame_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }

  gsm_byte encoded[x->codec.frame_size];
  gsm_encode (x->encoder, raw, encoded);
  gsm_decode (x->decoder, encoded, raw);

  for (int i = 0; i < x->codec.frame_size; i++) {
    frame[i] = (float) raw[i] / SHRT_MAX;
  }
  generic_codec_resample_to_external (&x->codec, x->codec.ringbuffer_input->chunk_size, frame);

  if (free_required) {
    free (frame);
  }
}

void gsm_packet_loss (t_gsm_tilde * x) {
  x->codec.drop_next_frame = true;
  error ("gsm~: Packet loss is not implemented.");
}

void gsm_tilde_dsp (t_gsm_tilde * x, t_signal ** sp) {
  x->decoder = gsm_create ();
  x->encoder = gsm_create ();

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, gsm_tilde_perform, sp);
}

void gsm_tilde_free (t_gsm_tilde * x) {
  generic_codec_free (&x->codec);
  if (x->decoder != NULL) {
    gsm_destroy (x->decoder);
  }
  if (x->encoder != NULL) {
    gsm_destroy (x->encoder);
  }
}

void *gsm_tilde_new () {
  t_gsm_tilde *x = (t_gsm_tilde *) pd_new (gsm_tilde_class);

  generic_codec_init (&x->codec, &x->x_obj, 8000, 160);

  x->encoder = NULL;
  x->decoder = NULL;
  return (void *) x;
}

void gsm_tilde_setup (void) {
  gsm_tilde_class = class_new (gensym ("gsm~"), (t_newmethod) gsm_tilde_new, (t_method) gsm_tilde_free, sizeof (t_gsm_tilde), CLASS_DEFAULT, 0);
  class_addmethod (gsm_tilde_class, (t_method) gsm_tilde_dsp, gensym ("dsp"), 0);
  class_addbang (gsm_tilde_class, gsm_packet_loss);
  CLASS_MAINSIGNALIN (gsm_tilde_class, t_gsm_tilde, float_inlet);
  class_sethelpsymbol (gsm_tilde_class, gensym ("gsm~"));
}
