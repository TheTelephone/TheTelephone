/**
@file benchmark_delay_tilde.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

benchmark_delay~ measures the time between the processing of subsequent blocks of audio.
This component counts the number of blocks while calculating the average execution time (incl. standard deviation, minima, and maxima) online.
Results are printed on _every_ block computation.
Counters are resetted on every DSPadd.

ATTENTION: It is recommended to only use one benchmark_delay~ per execution.

Usage:
  benchmark~

Inlets:
  1x Audio inlet

Outlets:
  None

*/

#include <m_pd.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>

static t_class *benchmark_delay_tilde_class;

typedef struct _benchmark_delay_tilde {
  t_object x_obj;

  double time_previous_ms;

  bool reset;

  unsigned int s0;              //Number of blocks
  double s1;                    // \Sum(execution_time) [ms]
  double s2;                    // \Sum(execution_time * execution_time) [ms^2]

  double execution_time_min;
  double execution_time_max;

  t_float float_inlet;          //unused

} t_benchmark_delay_tilde;

t_int *benchmark_delay_tilde_perform (t_int * w) {
  t_benchmark_delay_tilde *x = (t_benchmark_delay_tilde *) (w[1]);

  struct timespec time_current;
  clock_gettime (CLOCK_MONOTONIC, &time_current);

  if (x->reset) {
    x->reset = false;
    x->s0 = 1;
    x->s1 = 0;
    x->s2 = 0;

    x->execution_time_min = FLT_MAX_10_EXP;
    x->execution_time_max = FLT_MIN_10_EXP;

    x->time_previous_ms = (1.0e+3 * (double) time_current.tv_sec + 1.0e-6 * time_current.tv_nsec);      //in ms
  } else {
    double time_current_ms = (1.0e+3 * (double) time_current.tv_sec + 1.0e-6 * time_current.tv_nsec);   //in ms
    double execution_time = time_current_ms - x->time_previous_ms;
    x->time_previous_ms = time_current_ms;

    x->s0 = x->s0 + 1;
    x->s1 = x->s1 + execution_time;
    x->s2 = x->s2 + execution_time * execution_time;

    x->execution_time_min = fmin (execution_time, x->execution_time_min);
    x->execution_time_max = fmax (execution_time, x->execution_time_max);

    if (x->s0 >= 2) {
      double stdev = sqrt ((x->s0 * x->s2 - x->s1 * x->s1) / (x->s0 * (x->s0 - 1)));
      post ("benchmark_delay~: current=%f;n=%d;avg=%f;stdev=%f;min=%f;max=%f)", execution_time, x->s0, x->s1 / x->s0, stdev, x->execution_time_min, x->execution_time_max);
    }
  }

  return (w + 4);
}

void benchmark_delay_tilde_dsp (t_benchmark_delay_tilde * x, t_signal ** sp) {
  //Reset counter on every dsp-add
  x->reset = true;
  dsp_add (benchmark_delay_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void benchmark_delay_tilde_free (t_benchmark_delay_tilde * x) {
}

void *benchmark_delay_tilde_new () {
  t_benchmark_delay_tilde *x = (t_benchmark_delay_tilde *) pd_new (benchmark_delay_tilde_class);

  post ("benchmark_delay~: created.");
  return (void *) x;
}

void benchmark_delay_tilde_setup (void) {
  benchmark_delay_tilde_class = class_new (gensym ("benchmark_delay~"), (t_newmethod) benchmark_delay_tilde_new, (t_method) benchmark_delay_tilde_free, sizeof (t_benchmark_delay_tilde), CLASS_DEFAULT, 0);
  class_addmethod (benchmark_delay_tilde_class, (t_method) benchmark_delay_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (benchmark_delay_tilde_class, t_benchmark_delay_tilde, float_inlet);
  class_sethelpsymbol (benchmark_delay_tilde_class, gensym ("benchmark_delay~"));
}
