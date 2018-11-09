/**
@file opus_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-25
@license GPLv3 or later

opus~ encodes the signal with [OPUS](https://en.wikipedia.org/wiki/Opus_(audio_format)).

Parameters:
  opus~

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

#include <opus/opus.h>

static t_class *opus_tilde_class;

typedef struct _opus_tilde {
  t_object x_obj;

  t_generic_codec codec;

  OpusEncoder *encoder;

  OpusDecoder *decoder;

  int opus_error;

  int forward_error_correction;

  t_float float_inlet_unused;
} t_opus_tilde;

void opus_add_to_outbuffer (t_opus_tilde * x);

t_int *opus_tilde_perform (t_int * w) {
  t_opus_tilde *x = (t_opus_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);

  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    opus_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void opus_add_to_outbuffer (t_opus_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  int decompressed_length = x->codec.frame_size;
  unsigned char compressed[x->codec.frame_size];        //Assuming coding requires less space
  int compressed_length = opus_encode_float (x->encoder, frame, x->codec.frame_size, compressed, x->codec.ringbuffer_input->chunk_size);
  if (compressed_length < 0) {
    error ("opus~: Compressing current frame failed with error code %d.", compressed_length);
    return;
  }

  if (x->codec.drop_next_frame) {
    decompressed_length = opus_decode_float (x->decoder, NULL, 0, frame, x->codec.frame_size, x->forward_error_correction);
    x->codec.drop_next_frame = false;
  } else {
    decompressed_length = opus_decode_float (x->decoder, compressed, compressed_length, frame, x->codec.frame_size, x->forward_error_correction);
  }

  generic_codec_resample_to_external (&x->codec, x->codec.ringbuffer_input->chunk_size, frame);

  if (free_required) {
    free (frame);
  }
}

void opus_packet_loss (t_opus_tilde * x) {
  x->codec.drop_next_frame = true;
}

void opus_tilde_dsp (t_opus_tilde * x, t_signal ** sp) {
  int opus_error;
  x->encoder = opus_encoder_create (x->codec.sample_rate_internal, 1, OPUS_APPLICATION_VOIP, &opus_error);
  if (opus_error != OPUS_OK) {
    error ("opus~: Initializing OPUS encoder failed.");
    return;
  }

  x->decoder = opus_decoder_create (x->codec.sample_rate_internal, 1, &opus_error);
  if (opus_error != OPUS_OK) {
    error ("opus~: Initializing OPUS decoder failed.");
    return;
  }

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, opus_tilde_perform, sp);
}

void opus_tilde_free (t_opus_tilde * x) {
  generic_codec_free (&x->codec);

  if (x->encoder != NULL) {
    opus_encoder_destroy (x->encoder);
    x->encoder = NULL;
  }
  if (x->decoder != NULL) {
    opus_decoder_destroy (x->decoder);
    x->decoder = NULL;
  }
}

void *opus_tilde_new (t_floatarg frame_size, t_floatarg forward_error_correction, t_floatarg sample_rate) {
  t_opus_tilde *x = (t_opus_tilde *) pd_new (opus_tilde_class);

  if ((int) frame_size != 80 && (int) frame_size != 160 && (int) frame_size != 240) {
    error ("opus~: invalid frame size specified (%d). Using 80.", (int) frame_size);
    frame_size = 80;
  }

  if ((int) sample_rate != 8000 && (int) sample_rate != 12000 && (int) sample_rate != 16000 && (int) sample_rate != 24000 && (int) sample_rate != 48000) {
    error ("opus~: invalid sample rate specified (%d). Using 8000.", (int) sample_rate);
    sample_rate = 8000;
  }

  if ((int) forward_error_correction != 0 && (int) forward_error_correction != 1) {
    error ("opus~: invalid forward error correction specified (%d). Using 0 (none).", (int) forward_error_correction);
    forward_error_correction = 0;
  }
  x->forward_error_correction = forward_error_correction;

  generic_codec_init (&x->codec, &x->x_obj, sample_rate, frame_size);

  post ("opus~: Created with frame size (%d), forward_error_correction (%d), and sample rate (%f).", x->codec.frame_size, x->forward_error_correction, sample_rate);

  return (void *) x;
}

void opus_tilde_setup (void) {
  opus_tilde_class = class_new (gensym ("opus~"), (t_newmethod) opus_tilde_new, (t_method) opus_tilde_free, sizeof (t_opus_tilde), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod (opus_tilde_class, (t_method) opus_tilde_dsp, gensym ("dsp"), 0);
  class_addbang (opus_tilde_class, opus_packet_loss);
  CLASS_MAINSIGNALIN (opus_tilde_class, t_opus_tilde, float_inlet_unused);
  class_sethelpsymbol (opus_tilde_class, gensym ("opus~"));
}
