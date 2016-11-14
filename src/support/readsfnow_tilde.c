/**
@file readsfnow_tilde.c
@author Frank Haase, Dennis Guse
@license GPLv3 or later

readsfnow~ reads a wave file completely before enabling DSP.
Multi-channel files are supported while for each channel an outlet ist provided.
Resampling is _not_ applied.
Playback starts automatically enabling DSP (always from first frame).

ATTENTION: 
  The file is read completely while "new" and resampled while added to dsp processing (might block UI).

Parameters:
  readsfnow~ FILENAME

Inlets:
  float: skip N frames

Outlets:
  Nx: one per channel in FILENAME
  LAST: last outlet emits bangs after reaching EOF 

Methods:
  rewind: rewind and start playing again.
*/

#include <math.h>
#include <m_pd.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_BUFFER 8172000

static t_class *readsfnow_tilde_class;

typedef struct _readsfnow_tilde {
  t_object x_obj;

  char filename[500];
  float *wave_data;
  unsigned int wave_length;
  unsigned int channel_count;
  unsigned int frame_count;
  int current_frame_index;      //negative if reached EOF

  t_outlet *outlet_bang;
  t_outlet **outlet_channel;

  t_float goto_frame;           //Winding
  t_float goto_frame_previous;  //Default -1
} t_readsfnow_tilde;

t_int *readsfnow_tilde_perform (t_int * w) {
  t_readsfnow_tilde *x = (t_readsfnow_tilde *) (w[1]);
  int n = (int) (w[x->channel_count + 3]);

  if ((int) x->goto_frame > 0 && x->goto_frame != x->goto_frame_previous) {     //Skip frames
    post ("readsfnow~: goto %d frame.", (int) x->goto_frame);
    x->current_frame_index = (int) x->goto_frame;
    x->goto_frame_previous = x->goto_frame;
  }

  if (x->current_frame_index >= 0) {
    int reached_eof = 0;

    for (int i = 0; i < x->channel_count; i++) {
      t_sample *out = (t_sample *) (w[3 + i]);
      // Deinterleave
      for (int k = 0; k < n; k++) {
        if (x->current_frame_index + k >= x->frame_count) {
          reached_eof = 1;
          out[k] = 0;
        } else {
          out[k] = x->wave_data[x->current_frame_index + i + k * x->channel_count];
        }
      }
    }

    x->current_frame_index += n;

    if (reached_eof) {
      post ("readsfnow~ (%s): Reached EOF.", x->filename);
      outlet_bang (x->outlet_bang);
      x->current_frame_index = -1;
    }
  } else {
    //EOF
    for (int i = 0; i < x->channel_count; i++) {
      t_sample *out = (t_sample *) (w[3 + i]);
      for (int k = 0; k < n; k++) {
        out[k] = 0;
      }
    }
  }

  return (w + 2 + x->channel_count + 1 + 1);
}

void readsfnow_toggle_rewind (t_readsfnow_tilde * x) {
  post ("readsfnow~: rewinding.");
  x->current_frame_index = 0;
}

void readsfnow_tilde_dsp (t_readsfnow_tilde * x, t_signal ** sp) {
  t_int signal_ref[x->channel_count + 3];
  signal_ref[0] = (t_int) x;
  signal_ref[1] = (t_int) sp[0]->s_vec;

  for (int i = 0; i < x->channel_count; i++) {
    signal_ref[2 + i] = (t_int) sp[i + 1]->s_vec;
  }
  signal_ref[x->channel_count + 2] = sp[0]->s_n;

  readsfnow_toggle_rewind (x);
  x->goto_frame_previous = -1;
  dsp_addv (readsfnow_tilde_perform, 3 + x->channel_count, signal_ref);
}

void readsfnow_tilde_free (t_readsfnow_tilde * x) {
  for (int i = 0; i < x->channel_count; i++) {
    outlet_free (x->outlet_channel[i]);
  }
  free (x->outlet_channel);

  outlet_free (x->outlet_bang);
  free (x->wave_data);
}

/**
@param s
@param argc
@param argv[0] filename
 */
void *readsfnow_tilde_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc < 1) {
    error ("readsfnow~: No input filename provided.");
    return NULL;
  }

  t_readsfnow_tilde *x = (t_readsfnow_tilde *) pd_new (readsfnow_tilde_class);
  atom_string (argv, x->filename, 500);

  //Read file completely.
  SNDFILE *infile = NULL;
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));

  if ((infile = sf_open (x->filename, SFM_READ, &sfinfo)) == NULL) {
    char pwd[512];
    getcwd (pwd, 512);

    error ("readsfnow~ (%s): Not able to open input file %s/%s: %s.", x->filename, pwd, x->filename, sf_strerror (NULL));
    return NULL;
  }

  if (fabsf (sys_getsr () - sfinfo.samplerate) > 0.0001) {
    error ("readsfnow~ (%s): Sampling rate of input file (%d Hz) does not match Puredatas (%f Hz).", x->filename, sfinfo.samplerate, sys_getsr ());
    return NULL;
  }

  x->channel_count = sfinfo.channels;
  x->wave_data = (float *) malloc (MAX_BUFFER * sizeof (float));
  x->wave_length = sf_read_float (infile, x->wave_data, MAX_BUFFER);
  x->frame_count = sfinfo.frames;

  sf_close (infile);

  post ("readsfnow~ (%s): Opened file with channels: %d, samplerate: %d, frames %d and size %d.", x->filename, sfinfo.channels, sfinfo.samplerate, sfinfo.frames, x->wave_length);

  //Allocate signal outlet for each channel.
  x->outlet_channel = (t_outlet **) malloc ((x->channel_count) * sizeof (t_outlet *));
  for (int i = 0; i < x->channel_count; i++) {
    x->outlet_channel[i] = outlet_new (&x->x_obj, &s_signal);
  }

  x->outlet_bang = outlet_new (&x->x_obj, &s_bang);

  return (void *) x;
}

void readsfnow_tilde_setup (void) {
  readsfnow_tilde_class = class_new (gensym ("readsfnow~"), (t_newmethod) readsfnow_tilde_new, (t_method) readsfnow_tilde_free, sizeof (t_readsfnow_tilde), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod (readsfnow_tilde_class, (t_method) readsfnow_toggle_rewind, gensym ("rewind"), 0);
  class_addmethod (readsfnow_tilde_class, (t_method) readsfnow_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (readsfnow_tilde_class, t_readsfnow_tilde, goto_frame);
  class_sethelpsymbol (readsfnow_tilde_class, gensym ("readsfnow~"));
}
