/**
@file g711_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-24
@license GPLv3 or later

g722~ encodes the signal with [G.722](http://en.wikipedia.org/wiki/G.722) (16kHz).
The signal temporary sampled to 16kHz.

Parameters:
  g722~ FRAME_SIZE COMPRESSION_MODE PACKET_LOSS_CONCEALMENT

  FRAME_SIZE in samples: 160, 320
  COMPRESSION_MODE: 0 (64kbit/s) [default], 1 (56kbit/s), 2 (48kbit/s)
  PACKET_LOSS_CONCEALMENT: 0 (zero insertion) [default], 1 (zero insertion, decoder reset)

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
#include "g722.h"
#include "ringbuffer.h"
#include "generic_codec.h"

static t_class *g722_tilde_class;

typedef struct _g722_tilde {
  t_object x_obj;

  t_generic_codec codec;

  g722_encode_state_t *encoder;
  g722_decode_state_t *decoder;

  int g722_decoding_mode;       //The decoder modes [internal] (8 = 64kbit/s; 7 = 56kbit/s; 6 = 48kbit/s)

  unsigned int packet_loss_concealment_mode;

  t_float float_inlet_unused;
} t_g722_tilde;

void g722_add_to_outbuffer (t_g722_tilde * x);

t_int *g722_tilde_perform (t_int * w) {
  t_g722_tilde *x = (t_g722_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);

  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    g722_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void g722_add_to_outbuffer (t_g722_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_read_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  //Encode
  short raw[x->codec.frame_size];
  for (int i = 0; i < x->codec.frame_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }
  uint8_t encoded[x->codec.frame_size];
  int encoded_length = g722_encode (x->encoder, encoded, raw, x->codec.frame_size);

  //Decode ATTENTION: decoded_length varies
  if (x->codec.drop_next_frame) {
    switch (x->packet_loss_concealment_mode) {
    case 0:
      memset (raw, 0, x->codec.ringbuffer_input->chunk_size * sizeof (short));  //zero insertion
      break;
    case 1:
      g722_decode_release (x->decoder);
      g722_decode_init (x->decoder, x->codec.sample_rate_internal, x->g722_decoding_mode);
      memset (raw, 0, x->codec.frame_size * sizeof (short));    //zero insertion
      break;
    }
    x->codec.drop_next_frame = false;

    //Copy to outbuffer
    for (int i = 0; i < x->codec.frame_size; i++) {
      frame[i] = (float) raw[i] / SHRT_MAX;
    }
    generic_codec_resample_to_external (&x->codec, x->codec.frame_size, frame);

  } else {
    int16_t decoded[x->codec.frame_size];
    int decoded_length = g722_decode (x->decoder, decoded, encoded, encoded_length);

    //Copy to outbuffer
    for (int i = 0; i < decoded_length; i++) {
      frame[i] = (float) decoded[i] / SHRT_MAX;
    }
    generic_codec_resample_to_external (&x->codec, decoded_length, frame);
  }

  if (free_required) {
    free (frame);
  }
}


void g722_packet_loss (t_g722_tilde * x) {
  x->codec.drop_next_frame = true;
}

void g722_tilde_dsp (t_g722_tilde * x, t_signal ** sp) {
  x->encoder = (g722_encode_state_t *) malloc (sizeof (g722_encode_state_t));
  g722_encode_init (x->encoder, x->codec.sample_rate_internal, x->g722_decoding_mode);

  x->decoder = (g722_decode_state_t *) malloc (sizeof (g722_decode_state_t));
  g722_decode_init (x->decoder, x->codec.sample_rate_internal, x->g722_decoding_mode);

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, g722_tilde_perform, sp);
}

void g722_tilde_free (t_g722_tilde * x) {
  generic_codec_free (&x->codec);
  if (x->encoder != NULL) {
    g722_encode_release (x->encoder);
    x->encoder = NULL;
  }
  if (x->decoder != NULL) {
    g722_decode_release (x->decoder);
    x->decoder = NULL;
  }
}

void *g722_tilde_new (t_floatarg frame_size, t_floatarg packet_loss_concealment_mode, t_floatarg g722_decoding_mode) {
  t_g722_tilde *x = (t_g722_tilde *) pd_new (g722_tilde_class);

  if ((int) frame_size != 160 && frame_size != 320) {
    error ("g722~: invalid frame size specified (%i). Using 160.", (int) packet_loss_concealment_mode);
    frame_size = 160;
  }

  if (packet_loss_concealment_mode < 0 && packet_loss_concealment_mode > 1) {
    error ("g722~: invalid packet loss concealment mode specified (%d). Using mode 0.", (int) packet_loss_concealment_mode);
    packet_loss_concealment_mode = 0;
  }
  x->packet_loss_concealment_mode = packet_loss_concealment_mode;

  if ((int) g722_decoding_mode != 0 && (int) g722_decoding_mode != 1 && (int) g722_decoding_mode != 2) {
    error ("g722~: invalid g722 decoding mode specified (%d). Using mode 0.", (int) g722_decoding_mode);
    g722_decoding_mode = 0;
  }
  x->g722_decoding_mode = g722_decoding_mode;

  post ("g722~: Created with frame size (%d), packet-loss concealment mode (%d), and decoding mode (%d).", frame_size, x->packet_loss_concealment_mode, x->g722_decoding_mode);

  x->g722_decoding_mode = 8 - g722_decoding_mode;       //Decoding mode transformed for g722.h

  generic_codec_init (&x->codec, &x->x_obj, 16000, frame_size);

  x->encoder = NULL;
  x->decoder = NULL;
  return (void *) x;
}

void g722_tilde_setup (void) {
  g722_tilde_class = class_new (gensym ("g722~"), (t_newmethod) g722_tilde_new, (t_method) g722_tilde_free, sizeof (t_g722_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addmethod (g722_tilde_class, (t_method) g722_tilde_dsp, gensym ("dsp"), 0);
  class_addbang (g722_tilde_class, g722_packet_loss);
  CLASS_MAINSIGNALIN (g722_tilde_class, t_g722_tilde, float_inlet_unused);
}
