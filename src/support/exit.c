/**
@file exit.c
@author Frank Haase, Dennis Guse
@license GPLv3 or later

exit waits until a bang is received and then immediately exits PureData via sys_exit().

ATTENTION: Errors are not reported - `pd && echo "Success"` will always print  `Success`.

Inlet:
  Bang: exits on bang
*/

#include <m_pd.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *exit_class;

typedef struct _exit {
  t_object x_obj;
} t_exit;

void exit_bang (t_exit * x) {
  puts ("exit: exiting... now.");
  sys_exit ();
}

void *exit_new (t_symbol * s, int argc, t_atom * argv) {
  t_exit *x = (t_exit *) pd_new (exit_class);
  return (void *) x;
}

void exit_setup (void) {
  exit_class = class_new (gensym ("exit"), (t_newmethod) exit_new, 0, sizeof (t_exit), CLASS_DEFAULT, 0);
  class_addbang (exit_class, exit_bang);
  class_sethelpsymbol (exit_class, gensym ("exit"));
}
