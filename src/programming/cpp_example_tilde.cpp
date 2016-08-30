/**
@file cpp_example_tilde.cpp
@author Dennis Guse, Frank Haase
@date 2016-08-19
@license GPLv3 or later

A short example how to implement an external using C++.
Prints a message to STDOUT on each DSP cycle.

Usage:
  cpp_example~

Inlets:
  1x Float inlet
 
 */
 
extern "C" {
#include <m_pd.h>
}

#include <iostream>
#include <string>


static t_class *cpp_example_tilde_class;

typedef struct _cpp_example_tilde {
  t_object x_obj;
  t_float inlet_float;
} t_cpp_example_tilde;

extern "C" t_int * cpp_example_tilde_perform (t_int * w) {
  t_cpp_example_tilde *x = (t_cpp_example_tilde *) (w[1]);
  int n = (int) (w[3]);

  std::string message ("cpp_demo~: hello world.\n");
  std::cout << message;

  return (w + 4);
}

extern "C" void cpp_example_tilde_dsp (t_cpp_example_tilde * x, t_signal ** sp) {
  dsp_add (cpp_example_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

extern "C" void cpp_example_tilde_free (t_cpp_example_tilde * x) {
}

extern "C" void *cpp_example_tilde_new () {
  t_cpp_example_tilde *x = (t_cpp_example_tilde *) pd_new (cpp_example_tilde_class);
  return (void *) x;
}

extern "C" void cpp_example_tilde_setup (void) {
  cpp_example_tilde_class = class_new (gensym ("cpp_example~"), (t_newmethod) cpp_example_tilde_new, (t_method) cpp_example_tilde_free, sizeof (t_cpp_example_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addmethod (cpp_example_tilde_class, (t_method) cpp_example_tilde_dsp, gensym ("dsp"), (t_atomtype) 0);
  CLASS_MAINSIGNALIN (cpp_example_tilde_class, t_cpp_example_tilde, inlet_float);
}
