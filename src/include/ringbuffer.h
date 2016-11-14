/**
@file ringbuffer.h
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

Implementation of ringbuffers.

*/

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdlib.h>
#include <stdbool.h>
#include <m_pd.h>

//PureData's t_sample
typedef struct _t_sample_buffer {
  t_sample *data;
  unsigned int size;
  unsigned int number_elements;
  unsigned int chunk_size;
  unsigned int start;
  unsigned int end;
} t_sample_buffer;

static t_sample_buffer *t_sample_buffer_alloc (unsigned int size, unsigned int chunk_size) {
  t_sample_buffer *buffer = malloc (sizeof (t_sample_buffer));
  if (buffer == NULL) {
    return NULL;
  }

  buffer->data = malloc (sizeof (t_sample) * size);
  if (buffer->data == NULL) {
    free (buffer);
    return NULL;
  }

  buffer->size = size;
  buffer->number_elements = 0;
  buffer->start = 0;
  buffer->end = 0;
  buffer->chunk_size = chunk_size;
  return buffer;
}

static void t_sample_buffer_free (t_sample_buffer * buffer) {
  free (buffer->data);
}

static void t_sample_buffer_add (t_sample_buffer * buffer, t_sample element) {
  buffer->data[buffer->end] = element;
  buffer->end = (buffer->end + 1) % buffer->size;
  if (buffer->end == buffer->start) {
    buffer->start = (buffer->start + 1) % buffer->size;
  }
  buffer->number_elements++;
}

static void t_sample_buffer_add_chunk (t_sample_buffer * buffer, t_sample * chunk, unsigned int size) {
  for (int i = 0; i < size; i++) {
    t_sample_buffer_add (buffer, chunk[i]);
  }
}

static bool t_sample_buffer_has_chunk (t_sample_buffer * buffer) {
  return buffer->number_elements >= buffer->chunk_size;
}

static void t_sample_buffer_pop_chunk (t_sample_buffer * buffer, t_sample ** chunk, unsigned int size, bool * manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = 1;
    t_sample *cpx = (t_sample *) malloc (sizeof (t_sample) * size);
    for (int i = 0; i < size; i++) {
      cpx[i] = buffer->data[buffer->start];
      buffer->start = (buffer->start + 1) % buffer->size;
      *chunk = cpx;
    }
  } else {
    *chunk = &buffer->data[buffer->start];
    buffer->start = (buffer->start + size) % buffer->size;
  }
  buffer->number_elements -= size;
}

//Float
typedef struct _float_buffer {
  float *data;
  unsigned int size;
  unsigned int number_elements;
  unsigned int chunk_size;
  unsigned int start;
  unsigned int end;
} float_buffer;

static float_buffer *float_buffer_alloc (unsigned int size, unsigned int chunk_size) {
  float_buffer *buffer = malloc (sizeof (float_buffer));
  if (buffer == NULL) {
    return NULL;
  }

  buffer->data = malloc (sizeof (float) * size);
  if (buffer->data == NULL) {
    free (buffer);
    return NULL;
  }

  buffer->size = size;
  buffer->number_elements = 0;
  buffer->start = 0;
  buffer->end = 0;
  buffer->chunk_size = chunk_size;
  return buffer;
}

static void float_buffer_free (float_buffer * buffer) {
  free (buffer->data);
}

static void float_buffer_add (float_buffer * buffer, float element) {
  buffer->data[buffer->end] = element;
  buffer->end = (buffer->end + 1) % buffer->size;
  if (buffer->end == buffer->start) {
    buffer->start = (buffer->start + 1) % buffer->size;
  }
  buffer->number_elements++;
}

static void float_buffer_add_chunk (float_buffer * buffer, float *chunk, unsigned int size) {
  for (int i = 0; i < size; i++) {
    float_buffer_add (buffer, chunk[i]);
  }
}

static bool float_buffer_has_chunk (float_buffer * buffer) {
  return buffer->number_elements >= buffer->chunk_size;
}

static bool float_buffer_has_chunk_n (float_buffer * buffer, unsigned int n) {
  return buffer->number_elements >= buffer->chunk_size * n;
}

static void float_buffer_read_chunk_n (float_buffer * buffer, float **chunk, unsigned int size, unsigned int n, bool * manual_delete) {

  unsigned int buffer_index = (buffer->start + size * n) % buffer->size;

  if (buffer->start + size * n > buffer_index) {
    *manual_delete = true;
    float *cpx = (float *) malloc (sizeof (float) * size);
    for (int i = 0; i < size; i++) {
      cpx[i] = buffer->data[buffer_index];
      buffer_index = (buffer_index + 1) % buffer->size;
      *chunk = cpx;
    }
  } else {
    *manual_delete = false;
    *chunk = &buffer->data[buffer_index];
  }
}

static void float_buffer_pop_chunk (float_buffer * buffer, float **chunk, unsigned int size, bool * manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = true;
    float *cpx = (float *) malloc (sizeof (float) * size);
    for (int i = 0; i < size; i++) {
      cpx[i] = buffer->data[buffer->start];
      buffer->start = (buffer->start + 1) % buffer->size;
      *chunk = cpx;
    }
  } else {
    *manual_delete = false;
    *chunk = &buffer->data[buffer->start];
    buffer->start = (buffer->start + size) % buffer->size;
  }
  buffer->number_elements -= size;
}

#endif /* RINGBUFFER_H_ */
