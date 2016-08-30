/**
@file writesfnow_tilde.c
@author Dennis Guse, Frank Haase
@license GPLv3 or later

writesfnow~ writes audio data immediately into a wave file (synchronous write).
Puredata's sampling rate is used.
ATTENTION: Should only be used for offline processing.

Parameters:
  writesfnow~ FILENAME NumberOfInlets...

Inlets:
  INLETx: 1 to 255

Technical documentation:
  One inlet is provided by default, so only INLET-1 are created and need to be freed.
  The protocol is the following:
    w[1] _tilde-struct
    w[2] signal block size
    w[2+1]... w[2+INLET] flexible inlets
*/

#include <m_pd.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

static t_class *writesfnow_tilde_class;

typedef struct _writesfnow_tilde {
  t_object x_obj;

  unsigned int inlet_count; //The number of inlets.

  char filename[500];
  SNDFILE *file;

  t_inlet **inlet_additional; //The signal inlets without the default inlet

  t_float f;                    //Unused
} t_writesfnow_tilde;

t_int *writesfnow_tilde_perform (t_int * w) {
  t_writesfnow_tilde *x = (t_writesfnow_tilde *) (w[1]);
  int n = (int) (w[2]);

  float *buffer = malloc (x->inlet_count * n * sizeof (float));

  //Prepare data (interleave)
  for (int channel = 0; channel < x->inlet_count; channel++) {
    float *in = (float *) (w[2 + 1 + channel]);
    for (int i = 0; i < n; i++) {
      buffer[channel + i * x->inlet_count] = in[i];
    }
  }

  //Write data
  int written = sf_write_float (x->file, buffer, x->inlet_count * n);
  if (written != x->inlet_count * n) {
    error ("writesfnow~: partial write (%s); %d/%d.", x->filename, written, x->inlet_count * n);
  }

  free (buffer);
  return (w + 2 + x->inlet_count + 1);
}

void writesfnow_tilde_dsp (t_writesfnow_tilde * x, t_signal ** sp) {
  //Open file
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));
  sfinfo.samplerate = sys_getsr ();
  sfinfo.channels = x->inlet_count;
  sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);
  if (!(x->file = sf_open (x->filename, SFM_WRITE, &sfinfo))) {
    error ("writesfnow~: Could not open file %s. Nothing will be written.", x->filename);
    return;
  }

  t_int signal_ref[2 + x->inlet_count + 1];

  signal_ref[0] = (t_int) x;
  signal_ref[1] = sp[0]->s_n;

  //Add inlets to dsp_addv-parameter
  for (int i = 0; i < x->inlet_count; i++) {
    signal_ref[2 + i] = (t_int) sp[i]->s_vec;
  }

  dsp_addv (writesfnow_tilde_perform, 2 + x->inlet_count, signal_ref);
}

void writesfnow_tilde_free (t_writesfnow_tilde * x) {
  for (int i = 0; i < x->inlet_count - 1; i++) {
    inlet_free (x->inlet_additional[i]);
  }
  free (x->inlet_additional);

  //Flush
  post ("writesfnow~: Flushing data to disc.");
  sf_write_sync (x->file);
  sf_close (x->file);
}

/**
@param s
@param argc
@param argv[0] filename
@param argv[1] number of channels
 */
void *writesfnow_tilde_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc < 1 && argc > 2) {
    error ("writesfnow~: needs the filename and the channel count (default: 1).");
    return NULL;
  }

  int requested_inlet_count = 1;
  if (argc == 2) {
    requested_inlet_count = atom_getintarg (1, argc, argv);
  }

  if (requested_inlet_count < 1) {
    error ("writesfnow~: Number of channels must be at least one.");
    return NULL;
  }

  t_writesfnow_tilde *x = (t_writesfnow_tilde *) pd_new (writesfnow_tilde_class);

  atom_string (argv, x->filename, 500);

  x->inlet_count = requested_inlet_count;

  x->inlet_additional = malloc ((x->inlet_count - 1) * sizeof (t_inlet *));     //one inlet is available by default
  for (int i = 0; i < x->inlet_count - 1; i++) {
    //Create additional inlets (one is available by default)
    x->inlet_additional[i] = inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  }

  post ("writesfnow~: Going to write to %s with %d channels.", x->filename, x->inlet_count);
  return (void *) x;
}

void writesfnow_tilde_setup (void) {
  writesfnow_tilde_class = class_new (gensym ("writesfnow~"), (t_newmethod) writesfnow_tilde_new, (t_method) writesfnow_tilde_free, sizeof (t_writesfnow_tilde), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod (writesfnow_tilde_class, (t_method) writesfnow_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (writesfnow_tilde_class, t_writesfnow_tilde, f);
  class_sethelpsymbol (writesfnow_tilde_class, gensym ("writesfnow~"));
}
