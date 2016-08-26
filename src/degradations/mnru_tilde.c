/**
@file mnru_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-25
@license GPLv3 or later

mnru~ applies noise simulated by the ITU-T's [Modulated Noise Reference Unit](https://en.wikipedia.org/wiki/Modulated_Noise_Reference_Unit) aka Schroedinger Noise (8 kHz).

Parameters:
  mnru~

Inlets:
  1x Audio inlet

Outlets:
  1x Audio outlet

*/

#include <m_pd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "ringbuffer.h"
#include "generic_codec.h"

#include "mnru.h"

//Suggestion
#define DEFAULT_BLK_SIZE 64

static t_class *mnru_tilde_class;

typedef struct _mnru_tilde {
  t_object x_obj;

  t_generic_codec codec;

  t_float float_inlet_unused;

  //MNRU parameters.
  MNRU_state mnru_state;
  double mnru_qdb;
  char mnru_mode;
  char mnru_operation;

} t_mnru_tilde;

void mnru_add_to_outbuffer (t_mnru_tilde * x);

t_int *mnru_tilde_perform (t_int * w) {
  t_mnru_tilde *x = (t_mnru_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  generic_codec_resample_to_internal (&x->codec, n, in);
  if (float_buffer_has_chunk (x->codec.ringbuffer_input)) {
    mnru_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->codec.ringbuffer_output)) {
    generic_codec_to_outbuffer (&x->codec, out);
  }

  return (w + 5);
}

void mnru_add_to_outbuffer (t_mnru_tilde * x) {
  bool free_required = false;
  float *frame;
  float_buffer_pop_chunk (x->codec.ringbuffer_input, &frame, x->codec.ringbuffer_input->chunk_size, &free_required);

  float mnru_output[x->codec.frame_size];

  double *mnru_ok = MNRU_process (x->mnru_operation, &x->mnru_state, frame, mnru_output, (long) x->codec.frame_size, 314159265L, x->mnru_mode, x->mnru_qdb);
  if (x->mnru_operation == MNRU_START) {
    x->mnru_operation = MNRU_CONTINUE;
  }

  if (mnru_ok == NULL) {
    error ("mnru~: MǸRU process reported an error; applying zero insertion.");
    for (int i = 0; i < x->codec.frame_size; i++) {
      mnru_output[i] = 0;
    }
  }

  generic_codec_resample_to_external (&x->codec, x->codec.frame_size, mnru_output);

  if (free_required) {
    free (frame);
  }
}

void mnru_packet_loss (t_mnru_tilde * x) {
  x->codec.drop_next_frame = true;
  error ("mnru~: Packet-loss is not implemented.");
}

void mnru_tilde_dsp (t_mnru_tilde * x, t_signal ** sp) {
  MNRU_state mnru_state;
  x->mnru_state = mnru_state;

  x->mnru_operation = MNRU_START;

  generic_codec_dsp_add (&x->codec, sp[0]->s_n, x, mnru_tilde_perform, sp);
}

void mnru_tilde_free (t_mnru_tilde * x) {
  generic_codec_free (&x->codec);
}

void *mnru_tilde_new (t_floatarg frame_size, t_floatarg mnru_qdb) {
  t_mnru_tilde *x = (t_mnru_tilde *) pd_new (mnru_tilde_class);

  if ((int) frame_size != 80 && frame_size != 160) {
    error ("mnru~: invalid frame size specified (%d). Using 80.", (int) frame_size);
    frame_size = 80;
  }

  x->mnru_mode = MOD_NOISE;
  x->mnru_qdb = mnru_qdb;

  generic_codec_init (&x->codec, &x->x_obj, 8000, frame_size);

  post ("mnru~: Created with Q in db (%f) and block size (%d).", x->mnru_qdb, x->codec.frame_size);
  return (void *) x;
}
void mnru_tilde_setup (void) {
  mnru_tilde_class = class_new (gensym ("mnru~"), (t_newmethod) mnru_tilde_new, (t_method) mnru_tilde_free, sizeof (t_mnru_tilde), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod (mnru_tilde_class, (t_method) mnru_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (mnru_tilde_class, t_mnru_tilde, float_inlet_unused);
  class_sethelpsymbol (mnru_tilde_class, gensym ("mnru~"));
}
