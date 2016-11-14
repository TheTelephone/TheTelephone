/**
@file websocket_recv_client.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

websocket_recv_client connects to a websocket server and waits for incoming JSON messages.
Messages should like this: { key: VALUE } 
On message, the content of IDENTIFER is send as signal to outlet.
VALUE is interpreted as float (if it is a number) or as otherwise as string (aka PD symbol).

On disconnect automatic retry is done started (set RECONNECT_DELAY).

Parameters:
  websocket_recv_client IP/FQDN PORT PATH KEY

Outlets:
  1x Symbol/Float outlet

 */

#include <m_pd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <syslog.h>
#include <signal.h>
#include <libwebsockets.h>
#include <json-c/json.h>

#define RECONNECT_DELAY 0.5     //in seconds

static t_class *websocket_recv_client_class;

typedef struct _websocket_recv_client {
  t_object x_obj;

  t_outlet *outlet_string;

  char server_fqdn[100];
  int server_port;
  char server_path[100];
  char json_key[100];

  pthread_t websocket_thread_handle;
  bool websocket_should_exit;   //Set true to stop websocket thread.
  bool websocket_connection_failure;    //Websocket thread reports connection failures

  struct lws_context *websocket_context;
} t_websocket_recv_client;

static int websocket_recv_client_callback (struct lws *wsi, enum lws_callback_reasons reason, void *user, void *message, size_t message_length) {
  struct lws_context *context = lws_get_context (wsi);
  t_websocket_recv_client *x = (t_websocket_recv_client *) lws_context_user (context);

  char buf[message_length + 1];
  double value_number;
  const char *value_string;
  switch (reason) {

  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    post ("websocket_recv_client: Connected succesfully to (%s:%d); waiting for messages.", x->server_fqdn, x->server_port);
    x->websocket_connection_failure = false;
    lws_callback_on_writable (wsi);
    break;

  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    x->websocket_connection_failure = true;
    error ("websocket_recv_client: lost connection (%s:%d).", x->server_fqdn, x->server_port);
    break;

  case LWS_CALLBACK_CLOSED:
    x->websocket_connection_failure = true;
    error ("websocket_recv_client: Server closed connection (%s:%d).", x->server_fqdn, x->server_port);
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
    memcpy (&buf, message, message_length);
    buf[message_length] = '\0';

    json_object *json_message = json_tokener_parse (buf);
    json_object *json_key = json_object_new_object ();

    if (!json_object_object_get_ex (json_message, x->json_key, &json_key)) {
      error ("websocket_recv_client: Got mesage from (%s:%d) without fitting key (%s): %s.", x->server_fqdn, x->server_port, x->json_key, buf);
    } else {
      switch (json_object_get_type (json_key)) {
      case json_type_string:
        value_string = json_object_get_string (json_key);
        post ("websocket_recv_client: Got message from (%s:%d) with value %s; sending as symbol.", x->server_fqdn, x->server_port, value_string);
        outlet_anything (x->outlet_string, gensym (value_string), 0, NULL);
        break;
      case json_type_int:
        value_number = json_object_get_double (json_key);
        post ("websocket_recv_client: Got message from (%s:%d)  with value %f; sending as float.", x->server_fqdn, x->server_port, value_number);
        outlet_float (x->outlet_string, value_number);
        break;
      case json_type_double:
        value_number = json_object_get_int (json_key);
        post ("websocket_recv_client: Got message from (%s:%d)  with value %f; sending as float.", x->server_fqdn, x->server_port, value_number);
        outlet_float (x->outlet_string, value_number);
        break;
      default:
        error ("websocket_recv_client: Got from (%s:%d) unknown data type: %d.", x->server_fqdn, x->server_port, json_object_get_type (json_key));
        break;
      }
    }

    break;
  default:
    //post("websocket_recv_client: got unknown status code from libwebsocket %d.", reason);
    break;
  }
  return 0;
}

void websocket_recv_client_client_create (void *x_void) {
  lws_set_log_level (0, NULL);
  t_websocket_recv_client *x = (t_websocket_recv_client *) x_void;

  struct lws_context_creation_info info;
  memset (&info, 0, sizeof (info));
  info.port = CONTEXT_PORT_NO_LISTEN;
  static struct lws_protocols protocols[] = {
    {"switch-protocol", websocket_recv_client_callback,},
    {NULL, NULL, 0, 0}
  };
  info.protocols = protocols;
  info.user = x;                //pass t_websocket_recv_client struct to callback

  x->websocket_context = lws_create_context (&info);
  if (x->websocket_context == NULL) {
    error ("websocket_recv_client: Creating libwebsocket context failed - websockets are not available.");
    pthread_exit (NULL);
    return;
  }

  while (!x->websocket_should_exit) {
    //Connect
    struct lws *wsi_dumb = lws_client_connect (x->websocket_context, x->server_fqdn, x->server_port, 0, x->server_path, "PDHOST", "PDSOCK", NULL, -1);

    if (wsi_dumb == NULL) {
      error ("websocket_recv_client: libweboscket (lws_client_connect) returned null?");
    } else {
      //Processing loop
      int n = 0;
      do {
        n = lws_service (x->websocket_context, 100);    //Timeout in ms
      } while (n >= 0 && !x->websocket_should_exit && !x->websocket_connection_failure);

      if (x->websocket_connection_failure) {
        error ("websocket_recv_client: Waiting %.2fs to reconnect to (%s:%d).", RECONNECT_DELAY, x->server_fqdn, x->server_port);
        usleep (1000 * 1000 * RECONNECT_DELAY); //Reconnect
      }
    }
  }

  lws_context_destroy (x->websocket_context);
  pthread_exit (NULL);
}

void websocket_recv_client_free (t_websocket_recv_client * x) {
  x->websocket_should_exit = 1;
  pthread_join (x->websocket_thread_handle, NULL);      //We still hope that the thread is running....

  outlet_free (x->outlet_string);
}

void *websocket_recv_client_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc != 4) {
    error ("websocket_recv_client: Server address, server port, path and key are required.");
    return NULL;
  }

  t_websocket_recv_client *x = (t_websocket_recv_client *) pd_new (websocket_recv_client_class);
  x->websocket_connection_failure = false;
  x->websocket_should_exit = false;

  x->outlet_string = outlet_new (&x->x_obj, 0);

  atom_string (argv, x->server_fqdn, 100);
  x->server_port = (int) atom_getfloat (argv + 1);
  atom_string (argv + 2, x->server_path, 100);
  atom_string (argv + 3, x->json_key, 100);

  pthread_create (&x->websocket_thread_handle, NULL, (void *) &websocket_recv_client_client_create, x);

  post ("websocket_recv_client: Connecting to %s:%d/%s and waiting for JSON messages with key \"%s\".", x->server_fqdn, x->server_port, x->server_path, x->json_key);
  return (void *) x;
}

void websocket_recv_client_setup (void) {
  websocket_recv_client_class = class_new (gensym ("websocket_recv_client"), (t_newmethod) websocket_recv_client_new, (t_method) websocket_recv_client_free, sizeof (t_websocket_recv_client), CLASS_NOINLET, A_GIMME, 0);
  class_sethelpsymbol (websocket_recv_client_class, gensym ("websocket_recv_client"));
}
