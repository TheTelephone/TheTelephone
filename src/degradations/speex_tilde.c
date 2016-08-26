/**
@file speex_tilde.c
@author Dennis Guse, Frank Haase
@date 2016-08-26
@license GPLv3 or later

speex~ encodes the signal with [SPEEX](https://en.wikipedia.org/wiki/Speex) in narrowband mode.

Parameters:
  speex~

Inlets:
  1x Audio inlet
  also bang: lose next frame

Outlets:
  1x Audio outlet

*/

#include <m_pd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "ringbuffer.h"
#include "generic_codec.h"

#include <speex/speex.h>

static t_class *speex_tilde_class;

typedef struct _speex_tilde {
  t_object x_obj;

  t_generic_codec codec;

  SpeexMode speex_mode;
  SpeexBits speex_bits_encoder;
  SpeexBits speex_bits_decoder;

  void *encoder;
  void *decoder;

  t_float float_inlet_unused;
} t_speex_tilde;

void speex_add_to_outbuffer (t_speex_tilde * x);

t_int *speex_tilde_perform (t_int * w) {
  t_speex_tilde *x = (t_speex_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);

  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    speex_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void speex_add_to_outbuffer (t_speex_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_read_chunk (x->codec.ringbuffer_input, &frame, x->codec.frame_size, &free_required);

  //Encode
  short raw[x->codec.frame_size];
  for (int i = 0; i < x->codec.frame_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }

  unsigned int frame_size;

  speex_bits_reset (&x->speex_bits_encoder);
  speex_encode_int (x->encoder, raw, &x->speex_bits_encoder);
  char encoded[speex_bits_nbytes (&x->speex_bits_encoder)];
  unsigned int encoded_length = speex_bits_write (&x->speex_bits_encoder, encoded, speex_bits_nbytes (&x->speex_bits_encoder));

  //Decode
  speex_bits_read_from (&x->speex_bits_decoder, encoded, encoded_length);

  if (x->codec.drop_next_frame) {
    speex_bits_read_from (&x->speex_bits_decoder, NULL, 0);
    speex_decode_int (x->decoder, &x->speex_bits_decoder, raw);
    x->codec.drop_next_frame = false;
  } else {
    speex_bits_read_from (&x->speex_bits_decoder, encoded, encoded_length);
    speex_decode_int (x->decoder, &x->speex_bits_decoder, raw);
  }

  for (int i = 0; i < x->codec.frame_size; i++) {
    frame[i] = (float) raw[i] / SHRT_MAX;
  }

  generic_codec_resample_to_external (&x->codec, x->codec.frame_size, frame);

  if (free_required) {
    free (frame);
  }
}

void speex_packet_loss (t_speex_tilde * x) {
  x->codec.drop_next_frame = true;
}

void speex_tilde_dsp (t_speex_tilde * x, t_signal ** sp) {
  speex_bits_init (&x->speex_bits_encoder);
  x->encoder = speex_encoder_init (&x->speex_mode);

  speex_bits_init (&x->speex_bits_decoder);
  x->decoder = speex_decoder_init (&x->speex_mode);

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, speex_tilde_perform, sp);
}

void speex_tilde_free (t_speex_tilde * x) {
  if (x->encoder != NULL) {
    speex_decoder_destroy (x->encoder);
  }
  if (x->decoder != NULL) {
    speex_decoder_destroy (x->decoder);
  }
  generic_codec_free (&x->codec);
}

void *speex_tilde_new () {
  t_speex_tilde *x = (t_speex_tilde *) pd_new (speex_tilde_class);

  x->speex_mode = speex_nb_mode;

  //Get frame size: create an encoder and destroy it afterwards
  unsigned int frame_size;
  speex_bits_init (&x->speex_bits_encoder);
  x->encoder = speex_encoder_init (&x->speex_mode);
  speex_encoder_ctl (x->encoder, SPEEX_GET_FRAME_SIZE, &frame_size);
  speex_encoder_destroy (x->encoder);
  speex_bits_destroy (&x->speex_bits_encoder);

  generic_codec_init (&x->codec, &x->x_obj, 8000, frame_size);

  post ("speex~: Created with frame size (%d) and sample rate (%f).", x->codec.frame_size, x->codec.sample_rate_internal);

  x->decoder = NULL;
  x->encoder = NULL;
  return (void *) x;
}

void speex_tilde_setup (void) {
  speex_tilde_class = class_new (gensym ("speex~"), (t_newmethod) speex_tilde_new, (t_method) speex_tilde_free, sizeof (t_speex_tilde), CLASS_DEFAULT, 0);
  class_addmethod (speex_tilde_class, (t_method) speex_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (speex_tilde_class, t_speex_tilde, float_inlet_unused);
  class_addbang (speex_tilde_class, speex_packet_loss);
  class_sethelpsymbol (speex_tilde_class, gensym ("speex~"));
}
