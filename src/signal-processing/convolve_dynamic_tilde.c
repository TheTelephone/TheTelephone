/**
@file convolve_dynamic_tilde.c
@author Dennis Guse, Frank Haase
@date 2016-08-16
@license GPLv3 or later

convolve_dynamic~ applies [overlap-add convolution](https://en.wikipedia.org/wiki/Overlap%E2%80%93add_method) to the input signal with the current impulse response.
It provides dynamic convolution, i.e., the impulse response can be changed and crossfading is applied (cos^2 for length of the impulse response).
ATTENTION: Sampling rate of the set of impulse responses must be identical to PureData's.

Parameters:
  convolve_dynamic~ fileIR
  fileIR is a MULTI-channel Wave-file containing the impulse responses.

inlets:
  1x Float inlet: index of the impulse response to use (default: 0)
  1x Audio inlet

Outlets:
  1x Audio (convolved)

Internal Signal flow:
  inlet -> ringbuffer in -> fft -> multiplication with ffted impulse response -> ifft -> volume correction -> ringbuffer out -> outlet

Implementation details:

1. (DSP-Add) Prepare impulse response: load and FFT
2. (perform) Wait for a block (length of the resampled impulse response)
3. (perform) FFT block
4. (perform) multiple fft(block) * fft(impulse response)
5. (perform) ifft(multiplied)
6. (perform) volume correction (divide by impulse response length)
7. (perform) write out buffer
8. (perform) write overlap add buffer

*/

#include <m_pd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include <stdbool.h>
#include <fftw3.h>
#include <math.h>
#include "ringbuffer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static t_class *convolve_dynamic_tilde_class;

typedef struct _convolve_dynamic_tilde {
  t_object x_obj;

  float impulse_response_next;
  unsigned int impulse_response_current;

  t_outlet *outlet;

  float *impulse_response;      //ATTENTION: may contain interleaved (multi-channel) data!
  unsigned int impulse_response_size;   //Length of the interleaved audio
  unsigned int impulse_response_length; //Length of ONE impulse response!
  unsigned int impulse_response_sample_rate;
  unsigned int impulse_response_channels;       //Number of impulse responses

  double *irtf;                 //Impulse response transfer function (FFT of impulse_response); FFTW3: half-complex
  unsigned int irtf_length;     //Length of ONE irtf
  unsigned int irtf_size;       //Length of ALL irtfs contained in this buffer!

  fftw_plan fftw_plan;
  fftw_plan fftw_plan_inverse;
  double *fftw_in;              //FFTW3: half-complex
  double *fftw_out;             //FFTW3: half-complex

  float *overlap_add;
  float *crossfading_filter;

  float_buffer *input_buffer;      //Ringbuffer for incoming signals
  float_buffer *output_buffer;     //Ringbuffer for outgoing signals
} t_convolve_dynamic_tilde;

void convolve_dynamic_add_to_output (t_convolve_dynamic_tilde * x, t_sample * out);
void convolve_dynamic_add_to_outbuffer (t_convolve_dynamic_tilde * x);
void convolve_dynamic_free_internal (t_convolve_dynamic_tilde * x);

t_int *convolve_dynamic_tilde_perform (t_int * w) {
  t_convolve_dynamic_tilde *x = (t_convolve_dynamic_tilde *) (w[1]);
  t_sample *in = (t_sample *) (w[2]);
  t_sample *out = (t_sample *) (w[3]);
  int n = (int) (w[4]);

  float_buffer_add_chunk (x->input_buffer, in, n);

  if (float_buffer_has_chunk (x->input_buffer)) {
    convolve_dynamic_add_to_outbuffer (x);
  }

  if (float_buffer_has_chunk (x->output_buffer)) {
    convolve_dynamic_add_to_output (x, out);
  }

  return (w + 5);
}

//Complex multiplication and IFFT used for convolving (helper function).
//ATTENTION: content of x->fftw_in AND x->fftw_out is destroyed!
void convolve_dynamic_mul (t_convolve_dynamic_tilde * x, unsigned int impulse_response_id) {
  //imaginary multiplication (in frequency space)
  unsigned int channel = impulse_response_id;
  unsigned int n = x->irtf_length;
  x->fftw_in[0] = x->fftw_out[0] * x->irtf[0];  //The first item has no imaginary part.

  for (int i = 1; i < n / 2; i++) {
    x->fftw_in[i] = x->fftw_out[i] * x->irtf[channel + i * x->impulse_response_channels] - x->fftw_out[n - i] * x->irtf[channel + (n - i) * x->impulse_response_channels];      //REAL
    x->fftw_in[n - i] = x->fftw_out[i] * x->irtf[channel + (n - i) * x->impulse_response_channels] + x->fftw_out[n - i] * x->irtf[channel + i * x->impulse_response_channels];  //IMAGINARY
  }

  if (n % 2 == 0) {
    //The last (if even) itemhas no imaginary part.
    x->fftw_in[n / 2] = x->fftw_out[n / 2] * x->irtf[channel + (n / 2) * x->impulse_response_channels];
  }
  //IFFT
  fftw_execute (x->fftw_plan_inverse);

  //Remove signal increase due to IFFT(FFT)
  for (int i = 0; i < x->irtf_length; i++) {
    x->fftw_out[i] = x->fftw_out[i] / (n / 2);
  }
}

void convolve_dynamic_add_to_outbuffer (t_convolve_dynamic_tilde * x) {
  bool free_required = false;
  float *signal_block;
  float_buffer_read_chunk (x->input_buffer, &signal_block, x->input_buffer->chunk_size, &free_required);

  //FFT - prepare incoming signal (to be convolved)
  for (int i = 0; i < x->input_buffer->chunk_size; i++) {
    x->fftw_in[i] = signal_block[i];
  }
  for (int i = x->input_buffer->chunk_size; i < x->irtf_length; i++) {
    x->fftw_in[i] = 0;
  }
  fftw_execute (x->fftw_plan);


  //Check if IR needs to be changed
  int next_response = (unsigned int) (x->impulse_response_next);
  if (!(0 <= next_response && next_response <= x->impulse_response_channels)) {
    error ("convolve_dynamic~: requested impulse response (%d) is not available; 0..%d are available.", next_response, x->impulse_response_channels);
    next_response = x->impulse_response_current;
  }

  if (next_response == x->impulse_response_current) {
    //IR not changed
    convolve_dynamic_mul (x, x->impulse_response_current);
    for (int i = 0; i < x->input_buffer->chunk_size; i++) {
      //Copy real part to output, ignore complex part
      x->overlap_add[i] += x->fftw_out[i];
    }
  } else {
    post ("convolve_dynamic~: going to change impulse response from %d to %d.", x->impulse_response_current, next_response);

    //backup x->fftw_out containing fft(signal_block)
    double signal_block_fft[x->irtf_length];
    for (int i = 0; i < x->irtf_length; i++) {
      signal_block_fft[i] = x->fftw_out[i];
    }

    //convolve with current IR and crossfade
    convolve_dynamic_mul (x, x->impulse_response_current);
    for (int i = 0; i < x->input_buffer->chunk_size; i++) {
      //Copy real part to output, ignore complex part
      x->overlap_add[i] += x->fftw_out[i] * x->crossfading_filter[i];
    }

    //restore x->fftw_out containing fft(signal_block)
    for (int i = 0; i < x->irtf_length; i++) {
      x->fftw_out[i] = signal_block_fft[i];
    }

    //convolve with next IR and crossfade
    convolve_dynamic_mul (x, next_response);
    for (int i = 0; i < x->input_buffer->chunk_size; i++) {
      //Copy real part to output, ignore complex part
      x->overlap_add[i] += x->fftw_out[i] * x->crossfading_filter[x->impulse_response_length - i - 1];
    }

    x->impulse_response_current = next_response;
  }

  float_buffer_add_chunk (x->output_buffer, x->overlap_add, x->input_buffer->chunk_size);

  //overlap-add: save for next block
  for (int i = 0; i < x->input_buffer->chunk_size; i++) {
    x->overlap_add[i] = x->fftw_out[x->input_buffer->chunk_size + i];
  }

  if (free_required) {
    free (signal_block);
  }
}

void convolve_dynamic_add_to_output (t_convolve_dynamic_tilde * x, t_sample * out) {
  bool free_required = false;
  float *signal_block;
  float_buffer_read_chunk (x->output_buffer, &signal_block, x->output_buffer->chunk_size, &free_required);

  for (int i = 0; i < x->output_buffer->chunk_size; i++) {
    out[i] = signal_block[i];
  }

  if (free_required) {
    free (signal_block);
  }
}

void convolve_dynamic_tilde_dsp (t_convolve_dynamic_tilde * x, t_signal ** sp) {
  convolve_dynamic_free_internal (x);

  //Prepare FFT and IFFT
  x->irtf_length = (x->impulse_response_length + x->impulse_response_length - 1);
  x->irtf_size = x->irtf_length * x->impulse_response_channels;
  x->irtf = (double *) fftw_alloc_real (x->irtf_size);

  x->fftw_in = (double *) fftw_alloc_real (x->irtf_length);
  x->fftw_out = (double *) fftw_alloc_real (x->irtf_length);

  x->fftw_plan = fftw_plan_r2r_1d (x->irtf_length, x->fftw_in, x->fftw_out, FFTW_R2HC, FFTW_PATIENT);
  x->fftw_plan_inverse = fftw_plan_r2r_1d (x->irtf_length, x->fftw_in, x->fftw_out, FFTW_HC2R, FFTW_PATIENT);   //content x->fftw_in will be destroyed as out-of-place

  //FFT for all (IRIR to IRTF)
  //ATTENTION impulse_response contains interleaved data!
  for (int channel = 0; channel < x->impulse_response_channels; channel++) {
    for (int i = 0; i < x->impulse_response_length; i++) {
      x->fftw_in[i] = x->impulse_response[channel + i * x->impulse_response_channels];
    }
    //Zero-padding
    for (int i = x->impulse_response_length; i < x->irtf_length; i++) {
      x->fftw_in[i] = 0;
    }

    fftw_execute (x->fftw_plan);

    for (int i = 0; i < x->irtf_length; i++) {
      x->irtf[channel + i * x->impulse_response_channels] = x->fftw_out[i];
    }
  }

  //Allocate overlap-add buffer
  x->overlap_add = (float *) malloc (sizeof (float) * (x->impulse_response_length));

  //Initialize crossfading filter: cos^2 from 0deg to 90deg
  x->crossfading_filter = (float *) malloc (sizeof (float) * (x->impulse_response_length));
  for (int i = 0; i < x->impulse_response_length; i++) {
    float rad = (float) i / x->impulse_response_length * 90 * M_PI / 180;       //90deg to rad
    x->crossfading_filter[i] = cos (rad) * cos (rad);
  }

  //Buffers are allocated with a maximum of three times the INPUT block size
  x->input_buffer = float_buffer_alloc (x->impulse_response_length * 3, x->impulse_response_length);
  x->output_buffer = float_buffer_alloc (x->impulse_response_length * 3, sp[0]->s_n);

  dsp_add (convolve_dynamic_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
  post ("convolve_dynamic~: number of impulse responses %d, impulse response length %d; sampling rate %d.", x->impulse_response_channels, x->impulse_response_length, (int) x->impulse_response_sample_rate);


}

void convolve_dynamic_tilde_free (t_convolve_dynamic_tilde * x) {
  outlet_free (x->outlet);
  free (x->impulse_response);
  fftw_free (x->irtf);

  fftw_free (x->fftw_in);
  fftw_free (x->fftw_out);
  free (x->overlap_add);
  free (x->crossfading_filter);

  fftw_destroy_plan (x->fftw_plan);
  fftw_destroy_plan (x->fftw_plan_inverse);

  convolve_dynamic_free_internal (x);
}

void convolve_dynamic_free_internal (t_convolve_dynamic_tilde * x) {
  if (x->input_buffer != NULL) {
    float_buffer_free (x->input_buffer);
    free (x->input_buffer);
  }
  if (x->output_buffer != NULL) {
    float_buffer_free (x->output_buffer);
    free (x->output_buffer);
  }
}

void *convolve_dynamic_tilde_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc < 1) {
    error ("convolve_dynamic~: Please provide a path to the IRs-file (multi-channel WAVE).");
    return NULL;
  }

  char infilename[500];
  atom_string (argv, infilename, 500);

  //Prepare reading IRs-file
  SNDFILE *infile = NULL;
  SF_INFO sfinfo;
  memset (&sfinfo, 0, sizeof (sfinfo));

  if ((infile = sf_open (infilename, SFM_READ, &sfinfo)) == NULL) {
    error ("convolve_dynamic~: Not able to open input file %s. libsndfile reported: %s.\n", infilename, sf_strerror (NULL));
    return NULL;
  }
  t_convolve_dynamic_tilde *x = (t_convolve_dynamic_tilde *) pd_new (convolve_dynamic_tilde_class);
  x->impulse_response_sample_rate = sfinfo.samplerate;
  x->impulse_response_channels = sfinfo.channels;
  x->impulse_response_length = sfinfo.frames;
  
  if ((int) x->impulse_response_sample_rate != (int) sys_getsr ()) {
    error ("convolve_dynamic~: PureData's sampling rate (%d) and the sampling rate of IRs (%d).", x->impulse_response_sample_rate, (int) sys_getsr ());
    return NULL;
  }
  
  //Read IRs-file
  x->impulse_response = (float *) malloc (x->impulse_response_length * x->impulse_response_channels * sizeof (float));
  x->impulse_response_size = sf_readf_float (infile, x->impulse_response, x->impulse_response_length);
  sf_close (infile);

  x->outlet = outlet_new (&x->x_obj, &s_signal);

  x->input_buffer = NULL;
  x->output_buffer = NULL;

  post ("convolve_dynamic~: Opened %s with channels: %d, samplerate: %d, frames %d.", infilename, x->impulse_response_channels, x->impulse_response_sample_rate, x->impulse_response_length);
  return (void *) x;
}

void convolve_dynamic_tilde_setup (void) {
  convolve_dynamic_tilde_class = class_new (gensym ("convolve_dynamic~"), (t_newmethod) convolve_dynamic_tilde_new, (t_method) convolve_dynamic_tilde_free, sizeof (t_convolve_dynamic_tilde), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod (convolve_dynamic_tilde_class, (t_method) convolve_dynamic_tilde_dsp, gensym ("dsp"), 0);
  CLASS_MAINSIGNALIN (convolve_dynamic_tilde_class, t_convolve_dynamic_tilde, impulse_response_next);
  class_sethelpsymbol (convolve_dynamic_tilde_class, gensym ("convolve_dynamic~"));
}
