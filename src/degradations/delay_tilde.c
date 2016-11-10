/**
@file delay_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

delays~ delays the input signal by a given time (in ms).
Initial delay can be modified by sending a float to the first inlet.
ATTENTION: This will _add_ delay, but not set the overall delay.

Usage: 
 delay~ DelayInMilliseconds (default: 0)

Inlets: 
 1x Audio inlet
 1x Float inlet: adjust delay (milliseconds; >= 0)

Outlets: 
  1x Audio outlet

Internal Signal flow:
  inlet -> input_buffer -> output_buffer -> outlet

*/

#include <m_pd.h>
#include <stdbool.h>
#include "ringbuffer.h"

#define MAX_BUFFER 8172000      //Maximum buffer size of the internal ringbuffers

static t_class *delay_tilde_class;

typedef struct _delay_tilde {
  t_object x_obj;
  t_outlet *outlet;

  t_sample_buffer *input_buffer;
  t_sample_buffer *output_buffer;

  unsigned int delay_ms_initial;
  float delay_ms_inlet;         //float inlet
  unsigned int delay_ms_current;

  float one_sample_ms;          //Duration of one sample

  int block_size;               //The block size used while run time.
} t_delay_tilde;

void delay_adjust_buffer (t_delay_tilde * x);
void delay_free_internal (t_delay_tilde * x);


t_int *delay_tilde_perform (t_int * w) {
  t_delay_tilde *x = (t_delay_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);

  //Delay was adjusted?
  delay_adjust_buffer (x);

  //Add new block to input buffer.
  int n = (int) (w[4]);
  t_sample_buffer_add_chunk (x->input_buffer, in, n);

  //If a new chunk is available in the input buffer, then copy it to the output buffer.
  if (t_sample_buffer_has_chunk (x->input_buffer)) {
    bool free_required = false;
    float *out_chunk;
    t_sample_buffer_pop_chunk (x->input_buffer, &out_chunk, x->input_buffer->chunk_size, &free_required);
    t_sample_buffer_add_chunk (x->output_buffer, out_chunk, x->input_buffer->chunk_size);

    if (free_required) {
      free (out_chunk);
    }
  }
  //If a new chunk is available in the output buffer, then copy it to the outlet.
  if (t_sample_buffer_has_chunk (x->output_buffer)) {
    bool free_required = false;
    float *out_chunk;
    t_sample_buffer_pop_chunk (x->output_buffer, &out_chunk, x->output_buffer->chunk_size, &free_required);

    for (int i = 0; i < x->output_buffer->chunk_size; i++) {
      out[i] = out_chunk[i];
    }

    if (free_required) {
      free (out_chunk);
    }
  } else {
    //No new chunk available: send silence
    for (int i = 0; i < n; i++) {
      out[i] = 0;
    }
  }
  return (w + 5);
}

void delay_adjust_buffer (t_delay_tilde * x) {
  unsigned int delay_ms_inlet = (unsigned int) x->delay_ms_inlet;
  if (delay_ms_inlet == x->delay_ms_current) {
    return;
  }

  if ((int) (x->delay_ms_inlet / x->one_sample_ms) + x->block_size >= MAX_BUFFER / 2) {
    error ("delay~: Cannot delay for %.0f ms - please recompile and increase MAX_BUFFER.", x->delay_ms_inlet);
    return;
  }

  x->delay_ms_current = x->delay_ms_inlet;
  post ("delay~: Set new delay to %d ms.", x->delay_ms_current);
  x->input_buffer->chunk_size = (int) (x->delay_ms_current / x->one_sample_ms) + x->block_size;
}

void delay_tilde_dsp (t_delay_tilde * x, t_signal ** sp) {
  //Re-allocate ringbuffers
  delay_free_internal (x);

  x->input_buffer = t_sample_buffer_alloc (MAX_BUFFER, sp[0]->s_n);
  x->output_buffer = t_sample_buffer_alloc (MAX_BUFFER, sp[0]->s_n);
  x->block_size = sp[0]->s_n;

  dsp_add (delay_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void delay_free_internal (t_delay_tilde * x) {
  if (x->input_buffer != NULL) {
    t_sample_buffer_free (x->input_buffer);
  }
  if (x->output_buffer != NULL) {
    t_sample_buffer_free (x->output_buffer);
  }
  free (x->input_buffer);
  free (x->output_buffer);
}

void delay_tilde_free (t_delay_tilde * x) {
  outlet_free (x->outlet);
  delay_free_internal (x);
}

void *delay_tilde_new (t_floatarg delay_ms_initial) {
  t_delay_tilde *x = (t_delay_tilde *) pd_new (delay_tilde_class);

  if (delay_ms_initial < 0) {
    error ("delay~: initial delay must be larger than 0ms - provided (%0.f ms).", delay_ms_initial);
    delay_ms_initial = 0;
  }

  x->delay_ms_initial = (unsigned int) delay_ms_initial;
  x->delay_ms_inlet = x->delay_ms_initial;
  x->delay_ms_current = -1;     //Adjust buffer on first signal

  x->one_sample_ms = (1. / sys_getsr ()) * 1000.;
  x->outlet = outlet_new (&x->x_obj, &s_signal);

  x->input_buffer = NULL;
  x->output_buffer = NULL;

  post ("delay~: created with initial delay of %d ms.", x->delay_ms_initial);
  return (void *) x;
}

void delay_tilde_setup (void) {
  delay_tilde_class = class_new (gensym ("delay~"), (t_newmethod) delay_tilde_new, (t_method) delay_tilde_free, sizeof (t_delay_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addmethod (delay_tilde_class, (t_method) delay_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (delay_tilde_class, t_delay_tilde, delay_ms_inlet);
  class_sethelpsymbol (delay_tilde_class, gensym ("delay~"));
}
