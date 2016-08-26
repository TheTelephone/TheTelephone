/**
@file g711_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-24
@license GPLv3 or later

g711~ encodes the signal with [G.711](http://en.wikipedia.org/wiki/G.711) (8kHz).
Packet-loss concealment is available: UGST/ITU-T G711 Appendix I PLC MODULE.

Parameters:
  g711~ FRAME_SIZE PACKET_LOSS_CONCEALMENT

  FRAME_SIZE in  samples: 80, 160, 240
  PACKET_LOSS_CONCEALMENT: 0 (zero insertion) [default] and 1 (UGST/ITU-T G711 Appendix I)

Inlets:
  1x Audio inlet
  also bang: lose next frame

Outlets:
  1x Audio outlet

*/

#include <m_pd.h>
#include <limits.h>
#include <string.h>
#include "ringbuffer.h"
#include "generic_codec.h"

#include "g711.h"
#include "lowcfe.h"             //Packet loss concealment

static t_class *g711_tilde_class;

typedef struct _g711_tilde {
  t_object x_obj;

  t_generic_codec codec;

  t_float float_inlet_unused;

  LowcFE_c lc;                  //G.711 packet loss concealment

  unsigned int packet_loss_concealment_mode;
} t_g711_tilde;

void g711_add_to_outbuffer (t_g711_tilde * x);

t_int *g711_tilde_perform (t_int * w) {
  t_g711_tilde *x = (t_g711_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);
  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    g711_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void g711_add_to_outbuffer (t_g711_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  //Encode
  short raw[x->codec.ringbuffer_input->chunk_size];
  for (int i = 0; i < x->codec.ringbuffer_input->chunk_size; i++) {
    raw[i] = SHRT_MAX * frame[i];
  }
  short compressed[x->codec.ringbuffer_input->chunk_size];
  alaw_compress (x->codec.ringbuffer_input->chunk_size, raw, compressed);

  //Decode
  if (x->codec.drop_next_frame) {
    switch (x->packet_loss_concealment_mode) {
    case 1:
      g711plc_dofe (&x->lc, raw);
      break;
    default:
      memset (raw, 0, x->codec.ringbuffer_input->chunk_size * sizeof (short));  //zero insertion
    }
    x->codec.drop_next_frame = false;
  } else {
    short uncompressed[x->codec.ringbuffer_input->chunk_size];
    alaw_expand (x->codec.ringbuffer_input->chunk_size, compressed, uncompressed);

    g711plc_addtohistory (&x->lc, uncompressed);

    memcpy (raw, uncompressed, x->codec.ringbuffer_input->chunk_size * sizeof (short));
  }

  //Copy to outbuffer
  for (int i = 0; i < x->codec.ringbuffer_input->chunk_size; i++) {
    frame[i] = (float) raw[i] / SHRT_MAX;
  }
  generic_codec_resample_to_external (&x->codec, x->codec.ringbuffer_input->chunk_size, frame);

  if (free_required) {
    free (frame);
  }
}

void g711_packet_loss (t_g711_tilde * x) {
  x->codec.drop_next_frame = true;
}

void g711_tilde_dsp (t_g711_tilde * x, t_signal ** sp) {
  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, g711_tilde_perform, sp);
}

void g711_tilde_free (t_g711_tilde * x) {
  generic_codec_free (&x->codec);
}

void *g711_tilde_new (t_floatarg frame_size, t_floatarg packet_loss_concealment_mode) {
  t_g711_tilde *x = (t_g711_tilde *) pd_new (g711_tilde_class);

  //Parameters
  if ((int) frame_size != 80 && frame_size != 160 && frame_size != 240) {
    error ("g711~: invalid frame size specified (%d). Using 80.", (int) frame_size);
    frame_size = 80;
  }

  if (packet_loss_concealment_mode < 0 && packet_loss_concealment_mode > 1) {
    error ("g711~: invalid packet loss concealment mode specified (%d). Using mode 0.", (int) packet_loss_concealment_mode);
    packet_loss_concealment_mode = 0;
  }
  x->packet_loss_concealment_mode = packet_loss_concealment_mode;

  //Initialize
  generic_codec_init (&x->codec, &x->x_obj, 8000, frame_size);
  g711plc_construct (&x->lc);

  post ("g711~: Created with frame size (%d) and packet loss concealment mode (%d).", x->codec.frame_size, x->packet_loss_concealment_mode);

  return (void *) x;
}

void g711_tilde_setup (void) {
  g711_tilde_class = class_new (gensym ("g711~"), (t_newmethod) g711_tilde_new, (t_method) g711_tilde_free, sizeof (t_g711_tilde), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod (g711_tilde_class, (t_method) g711_tilde_dsp, gensym ("dsp"), 0);
  class_addbang (g711_tilde_class, g711_packet_loss);
  CLASS_MAINSIGNALIN (g711_tilde_class, t_g711_tilde, float_inlet_unused);
  class_sethelpsymbol (g711_tilde_class, gensym ("g711~"));
}
