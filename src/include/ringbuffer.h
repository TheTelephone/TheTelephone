/**
@file ringbuffer.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

Implementation of ringbuffers.

*/

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdlib.h>

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
    float_buffer_add (buffer, *(chunk + i));
  }
}

static float float_buffer_get (float_buffer * buffer) {
  float to_return = buffer->data[buffer->number_elements - 1];
  buffer->number_elements--;
  return to_return;
}

static int float_buffer_has_chunk (float_buffer * buffer) {
  if (buffer->number_elements >= buffer->chunk_size) {
    return 1;
  }
  return 0;
}

static void float_buffer_read_chunk (float_buffer * buffer, float **chunk, unsigned int size, int *manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = 1;
    float *cpx = (float *) malloc (sizeof (float) * size);
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

//Double
typedef struct _double_buffer {
  double *data;
  unsigned int size;
  unsigned int number_elements;
  unsigned int chunk_size;
  unsigned int start;
  unsigned int end;
} double_buffer;

static double_buffer *double_buffer_alloc (unsigned int size, unsigned int chunk_size) {
  double_buffer *buffer = malloc (sizeof (double_buffer));
  if (buffer == NULL) {
    return NULL;
  }
  buffer->data = malloc (sizeof (double) * size);
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

static void double_buffer_free (double_buffer * buffer) {
  free (buffer->data);
}

static void double_buffer_add (double_buffer * buffer, double element) {
  buffer->data[buffer->end] = element;
  buffer->end = (buffer->end + 1) % buffer->size;
  if (buffer->end == buffer->start) {
    buffer->start = (buffer->start + 1) % buffer->size;
  }
  buffer->number_elements++;
}

static void double_buffer_add_chunk (double_buffer * buffer, double *chunk, unsigned int size) {
  for (int i = 0; i < size; i++) {
    double_buffer_add (buffer, *(chunk + i));
  }
}

static double double_buffer_get (double_buffer * buffer) {
  double to_return = buffer->data[buffer->number_elements - 1];
  buffer->number_elements--;
  return to_return;
}

static int double_buffer_has_chunk (double_buffer * buffer) {
  if (buffer->number_elements >= buffer->chunk_size) {
    return 1;
  }
  return 0;
}

static void double_buffer_read_chunk (double_buffer * buffer, double **chunk, unsigned int size, int *manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = 1;
    double *cpx = (double *) malloc (sizeof (double) * size);
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

//Short
typedef struct _short_buffer {
  short *data;
  unsigned int size;
  unsigned int number_elements;
  unsigned int chunk_size;
  unsigned int start;
  unsigned int end;
} short_buffer;

static short_buffer *short_buffer_alloc (unsigned int size, unsigned int chunk_size) {
  short_buffer *buffer = malloc (sizeof (short_buffer));
  if (buffer == NULL) {
    return NULL;
  }

  buffer->data = malloc (sizeof (short) * size);
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

static void short_buffer_free (short_buffer * buffer) {
  free (buffer->data);
}

static void short_buffer_add (short_buffer * buffer, short element) {
  buffer->data[buffer->end] = element;
  buffer->end = (buffer->end + 1) % buffer->size;
  if (buffer->end == buffer->start) {
    buffer->start = (buffer->start + 1) % buffer->size;
  }
  buffer->number_elements++;
}
static void short_buffer_add_chunk (short_buffer * buffer, short *chunk, unsigned int size) {
  for (int i = 0; i < size; i++) {
    short_buffer_add (buffer, *(chunk + i));
  }
}

static short short_buffer_get (short_buffer * buffer) {
  short to_return = buffer->data[buffer->number_elements - 1];
  buffer->number_elements--;
  return to_return;
}

static int short_buffer_has_chunk (short_buffer * buffer) {
  if (buffer->number_elements >= buffer->chunk_size) {
    return 1;
  }
  return 0;
}

static void short_buffer_read_chunk (short_buffer * buffer, short **chunk, unsigned int size, int *manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = 1;
    short *cpx = (short *) malloc (sizeof (short) * size);
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

//int
typedef struct _int_buffer {
  int *data;
  unsigned int size;
  unsigned int number_elements;
  unsigned int chunk_size;
  unsigned int start;
  unsigned int end;
} int_buffer;

static int_buffer *int_buffer_alloc (unsigned int size, unsigned int chunk_size) {
  int_buffer *buffer = malloc (sizeof (int_buffer));
  if (buffer == NULL) {
    return NULL;
  }

  buffer->data = malloc (sizeof (int) * size);
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

static void int_buffer_free (int_buffer * buffer) {
  free (buffer->data);
}

static void int_buffer_add (int_buffer * buffer, int element) {
  buffer->data[buffer->end] = element;
  buffer->end = (buffer->end + 1) % buffer->size;
  if (buffer->end == buffer->start) {
    buffer->start = (buffer->start + 1) % buffer->size;
  }
  buffer->number_elements++;
}
static void int_buffer_add_chunk (int_buffer * buffer, int *chunk, unsigned int size) {
  for (int i = 0; i < size; i++) {
    int_buffer_add (buffer, *(chunk + i));
  }
}

static int int_buffer_get (int_buffer * buffer) {
  int to_return = buffer->data[buffer->number_elements - 1];
  buffer->number_elements--;
  return to_return;
}

static int int_buffer_has_chunk (int_buffer * buffer) {
  if (buffer->number_elements >= buffer->chunk_size) {
    return 1;
  }
  return 0;
}

static void int_buffer_read_chunk (int_buffer * buffer, int **chunk, unsigned int size, int *manual_delete) {
  if (buffer->start > (buffer->start + size) % buffer->size) {
    *manual_delete = 1;
    int *cpx = (int *) malloc (sizeof (int) * size);
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
#endif /* RINGBUFFER_H_ */
