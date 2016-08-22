/**
@file test_undefined_symbols.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

Tests PureData externals if all symbols (except PureData) are defined.
If a externals fails this test, please check the linker flags (-l) for missing libraries.

Usage:
  test_undefined_symbols PATH1 PATH2 ... PATHn

*/

#include <dlfcn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Symbols provided by Puredata's API
void atom_getintarg(void) {
}
void atom_getfloat(void) {
}
void atom_string (void) {
}
void binbuf_add (void) {
}
void binbuf_free (void) {
}
void binbuf_gettext (void) {
}
void binbuf_new (void) {
}
void class_addanything(void) {
}
void class_addbang (void) {
}
void class_addmethod (void) {
}
void class_doaddfloat (void) {
}
void class_domainsignalin (void) {
}
void class_new (void) {
}
void class_sethelpsymbol (void) {
}
void dsp_add (void) {
}
void dsp_addv (void) {
}
void error (void) {
}
void freebytes (void) {
}
void gensym (void) {
}
void inlet_free (void) {
}
void inlet_new (void) {
}
void outlet_anything(void) {
}
void outlet_bang (void) {
}
void outlet_float (void) {
}
void outlet_free (void) {
}
void outlet_new (void) {
}
void outlet_symbol (void) {
}
void pd_new (void) {
}
void post (void) {
}
void resizebytes (void) {
}
void sys_getsr (void) {
}
void s_bang (void) {
}
void s_float (void) {
}
void s_signal (void) {
}
void s_symbol (void) {
}

//Used by external exit
void sys_exit(void) {
}

//Code
int main (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Usage:\n  %s PATH1 PATH2 ... PATHn\n", argv[0]);
    return -1;
  }
  //Test all shared-objects
  void *dlobj;
  int failed_to_load = 0;

  for (int i = 1; i < argc; i++) {
    dlobj = dlopen (argv[i], RTLD_NOW);
    if (dlobj == NULL) {
      fprintf (stderr, "Testing %s: failed for reason '%s'\n", argv[i], dlerror ());
      failed_to_load++;
    } else {
      printf ("Testing %s: ok\n", argv[i]);
      dlclose (dlobj);
    }
  }

  if (failed_to_load > 0) {
    fprintf (stderr, "%d of %d could not be loaded.\n", failed_to_load, argc - 1);
    return failed_to_load;
  }

  return EXIT_SUCCESS;
}
