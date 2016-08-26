/**
@file lpc10_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-25
@license GPLv3 or later

lpc10~ encodes the signal with [LPC-10](https://en.wikipedia.org/wiki/FS-1015) aka FS-1015 aka STANAG 4198 (8kHz).

Parameters:
  lpc10~

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

#include "lpc10.h"

static t_class *lpc10_tilde_class;

typedef struct _lpc10_tilde {
  t_object x_obj;

  t_generic_codec codec;

  t_float float_inlet_unused;

  struct lpc10_encoder_state *lpc10_encode_state;
  struct lpc10_decoder_state *lpc10_decode_state;
} t_lpc10_tilde;


void lpc10_add_to_outbuffer (t_lpc10_tilde * x);

t_int *lpc10_tilde_perform (t_int * w) {
  t_lpc10_tilde *x = (t_lpc10_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);


  generic_codec_resample_to_internal (&x->codec, n, in);
  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    lpc10_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void lpc10_add_to_outbuffer (t_lpc10_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  int compressed[LPC10_BITS_IN_COMPRESSED_FRAME];
  lpc10_encode (frame, compressed, x->lpc10_encode_state);
  lpc10_decode (compressed, frame, x->lpc10_decode_state);

  generic_codec_resample_to_external (&x->codec, x->codec.ringbuffer_input->chunk_size, frame);

  if (free_required) {
    free (frame);
  }
}

void lpc10_packet_loss (t_lpc10_tilde * x) {
  x->codec.drop_next_frame = true;
  error ("lpc10~: Packet-loss is not implemented.");
}

void lpc10_tilde_dsp (t_lpc10_tilde * x, t_signal ** sp) {
  x->lpc10_encode_state = create_lpc10_encoder_state ();
  init_lpc10_encoder_state (x->lpc10_encode_state);
  x->lpc10_decode_state = create_lpc10_decoder_state ();
  init_lpc10_decoder_state (x->lpc10_decode_state);

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, lpc10_tilde_perform, sp);
}

void lpc10_tilde_free (t_lpc10_tilde * x) {
  generic_codec_free (&x->codec);

  free (x->lpc10_encode_state);
  free (x->lpc10_decode_state);
}

void *lpc10_tilde_new () {
  t_lpc10_tilde *x = (t_lpc10_tilde *) pd_new (lpc10_tilde_class);

  generic_codec_init (&x->codec, &x->x_obj, 8000, LPC10_SAMPLES_PER_FRAME);

  x->lpc10_encode_state = NULL;
  x->lpc10_decode_state = NULL;

  return (void *) x;
}

void lpc10_tilde_setup (void) {
  lpc10_tilde_class = class_new (gensym ("lpc10~"), (t_newmethod) lpc10_tilde_new, (t_method) lpc10_tilde_free, sizeof (t_lpc10_tilde), CLASS_DEFAULT, 0);
  class_addmethod (lpc10_tilde_class, (t_method) lpc10_tilde_dsp, gensym ("dsp"), 0);
  class_addbang (lpc10_tilde_class, lpc10_packet_loss);
  CLASS_MAINSIGNALIN (lpc10_tilde_class, t_lpc10_tilde, float_inlet_unused);
  class_sethelpsymbol (lpc10_tilde_class, gensym ("lpc10~"));
}
