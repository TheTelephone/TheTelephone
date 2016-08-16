/**
@file audiorouting_tilde.c
@author Dennis Guse, Frank Haase
@license GPLv3 or later

audiorouting~ is an input switcher for signal inlets.
Provides a variable number of inlets where only one inlet (e.g., the active inlet) is routed to the outlet.
Inlets can be changed by sending to the first inlet:
a) a message containing the name of inlet to be activated, or
b) a float containing the id of the inlet.

On inlet change, a crossfilter is applied.
By default the inlet1 is active.

Parameters:
  audiorouting~ INLET1 INLET2 ... INLETn

Inlets: <br>
  INLETx: Audio inlet
  float: change active inlet by index (counting starts with 1).
 
Incoming Message (first inlet): <br>
  Send message with name of the new active inlet.
  Send a float with id of the new active inlet.

Outlets:
  1x Audio outlet

Technical documentation:
  One inlet is provided by default, so only INLET-1 are created and need to be freed.
  The protocol is the following:
    w[1] _tilde-struct <br>
    w[2] signal block size <br>
    w[3]... w[2+INLET] flexible inlets (default is w[3]) <br>
    w[3+INLET] outlet
*/

#include <m_pd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static t_class *audiorouting_tilde_class;

typedef struct _audiorouting_tilde {
  t_object x_obj;

  t_int inlet_active_id;        //The id of the currently active inlet (1:inlet1...n:inletN).

  t_int inlet_next_id;          //Used to apply crossfading filter for transition.

  float *crossfading_filter;

  t_int inlet_count;            //The total number of inlets

  t_inlet **inlet_additional;   //The inlets without the default inlet (inlet_num - 1).

  char **inlet_names;           //Contains the name of each inlet (size == inlet_num)

  t_inlet *inlet_control;

  t_outlet *outlet;

  t_float f;
} t_audiorouting_tilde;

t_int *audiorouting_tilde_perform (t_int * w) {
  t_audiorouting_tilde *x = (t_audiorouting_tilde *) (w[1]);
  int n = (int) (w[2]);

  if (1 <= (t_int) x->f && (t_int) x->f <= x->inlet_count && (t_int) x->f != x->inlet_active_id) {
    x->inlet_next_id = (t_int) x->f;

    post ("audiorouting~: Changing to inlet %s.", x->inlet_names[x->inlet_next_id - 1]);
  }

  t_sample *in = (t_sample *) (w[2 + x->inlet_active_id]);      //Active inlet
  t_sample *out = (t_sample *) (w[3 + x->inlet_count]);

  if (x->inlet_next_id == x->inlet_active_id) {
    //Copy from active inlet to outlet.
    for (int i = 0; i < n; i++) {
      out[i] = in[i];
    }
  } else {
    //Change active inlet by crossfading
    for (int i = 0; i < n; i++) {
      out[i] += in[i] * x->crossfading_filter[i];
    }

    x->inlet_active_id = x->inlet_next_id;
    in = (t_sample *) (w[2 + x->inlet_active_id]);

    for (int i = 0; i < n; i++) {
      out[i] += in[i] * x->crossfading_filter[n - i - 1];
    }
  }

  return (w + 2 + x->inlet_count + 1 + 1);
}

void audiorouting_tilde_change_inlet (t_audiorouting_tilde * x, t_symbol * s, int argc, t_atom * argv) {
  for (int i = 0; i < x->inlet_count; i++) {
    if (strcmp (x->inlet_names[i], s->s_name) == 0) {
      post ("audiorouting~: Changing to inlet %s.", s->s_name);
      x->inlet_next_id = i + 1; //Index is 0 zero-based, but inlet 1.
      return;
    }
  }
  error ("audiorouting~: No inlet with name %s available; message ignored.", s->s_name);
}

void audiorouting_tilde_dsp (t_audiorouting_tilde * x, t_signal ** sp) {
  t_int signal_ref[2 + x->inlet_count + 1];

  signal_ref[0] = (t_int) x;
  signal_ref[1] = sp[0]->s_n;

  //Add inlets to dsp_addv-parameter
  for (int i = 0; i < x->inlet_count; i++) {
    signal_ref[2 + i] = (t_int) sp[i]->s_vec;
  }

  //Outlet
  signal_ref[2 + x->inlet_count] = (t_int) sp[x->inlet_count]->s_vec;

  //Initialize crossfading filter: cos^2 from 0deg to 90deg
  x->crossfading_filter = (float *) malloc (sizeof (float) * sp[0]->s_n);
  for (int i = 0; i < sp[0]->s_n; i++) {
    float rad = (float) i / sp[0]->s_n * 90 * M_PI / 180;       //90deg to rad
    x->crossfading_filter[i] = cos (rad) * cos (rad);
  }

  dsp_addv (audiorouting_tilde_perform, 2 + x->inlet_count + 1, signal_ref);
}

void audiorouting_tilde_free (t_audiorouting_tilde * x) {
  for (int i = 0; i < x->inlet_count - 1; i++) {
    inlet_free (x->inlet_additional[i]);
  }
  free (x->inlet_additional);

  for (int i = 0; i < x->inlet_count; i++) {
    free (x->inlet_names[i]);
  }
  free (x->inlet_names);

  free (x->crossfading_filter);

  outlet_free (x->outlet);
}

/**
 * @param s
 * @param argc
 * @param argv[n] contains the name for the n-th inlet.
 */
void *audiorouting_tilde_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc < 1) {
    error ("audiorouting~: At least one inlet required.");
    return NULL;
  }

  t_audiorouting_tilde *x = (t_audiorouting_tilde *) pd_new (audiorouting_tilde_class);
  x->inlet_count = argc;

  //Preparing inlet names
  x->inlet_names = (char **) malloc (argc * sizeof (char *));
  char current_name[500];

  for (int i = 0; i < x->inlet_count; i++) {
    atom_string (argv++, current_name, 500);
    x->inlet_names[i] = (char *) malloc ((strlen (current_name) + 1) * sizeof (char));
    strcpy (x->inlet_names[i], current_name);
    x->inlet_names[i][strlen (current_name)] = '\0';

    //Check for duplicate names
    int duplicate = 0;
    for (int j = 0; j < i; j++) {
      if (strcmp (x->inlet_names[j], current_name) == 0) {
        duplicate = 1;
        break;
      }
    }
    if (duplicate) {
      error ("audiorouting~: Names for inlets must be unique, but <%s> is used more than once.", x->inlet_names[i]);
      return NULL;
    }
  }

  x->inlet_active_id = (t_int) 1;
  x->inlet_next_id = x->inlet_active_id;

  x->outlet = outlet_new (&x->x_obj, &s_signal);

  x->inlet_additional = malloc ((x->inlet_count - 1) * sizeof (t_inlet *));
  for (int i = 0; i < x->inlet_count - 1; i++) {
    //Create additional inlets (one is available by default)
    x->inlet_additional[i] = inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  }

  post ("audiorouting~: Setup with %d inlets. Active inlet: %d.", x->inlet_count, x->inlet_active_id);
  return (void *) x;
}

void audiorouting_tilde_setup (void) {
  audiorouting_tilde_class = class_new (gensym ("audiorouting~"), (t_newmethod) audiorouting_tilde_new, (t_method) audiorouting_tilde_free, sizeof (t_audiorouting_tilde), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod (audiorouting_tilde_class, (t_method) audiorouting_tilde_dsp, gensym ("dsp"), 0);
  class_addanything (audiorouting_tilde_class, (t_method) audiorouting_tilde_change_inlet);
  CLASS_MAINSIGNALIN (audiorouting_tilde_class, t_audiorouting_tilde, f);
  class_sethelpsymbol (audiorouting_tilde_class, gensym ("audiorouting~"));
}
