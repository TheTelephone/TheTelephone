/**
@file websocket_recv_server.c
@author Frank Haase, Dennis Guse
@date 2016-08-16
@license GPLv3 or later

websocket_recv_server starts a websocket server and waits for incoming JSON messages.
Messages should like this: { key: VALUE } 
On message, the content of IDENTIFER is send as signal to outlet.
VALUE is interpreted as float (if it is a number) or as otherwise as string (aka PD symbol).

On disconnect automatic retry is done started (set RECONNECT_DELAY).

Does not support SSL

Parameters:
  websocket_recv_server PORT KEY

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
#include <json/json.h>

static t_class *websocket_recv_server_class;

typedef struct _websocket_recv_server {
  t_object x_obj;

  t_outlet *outlet_string;

  int server_port;
  char json_key[100];

  pthread_t websocket_thread_handle;
  bool websocket_should_exit;   //Set true to stop websocket thread.

  struct lws_context *websocket_context;
} t_websocket_recv_server;

static int websocket_recv_server_callback (struct lws *wsi, enum lws_callback_reasons reason, void *user, void *message, size_t message_length) {
  struct lws_context *context = lws_get_context (wsi);
  t_websocket_recv_server *x = (t_websocket_recv_server *) lws_context_user (context);

  char buf[message_length + 1];
  double value_number;
  const char *value_string;
  switch (reason) {

  case LWS_CALLBACK_ESTABLISHED:
    post ("websocket_recv_server: Clientconnected; waiting for messages.");
    //TODO lws_get_peer_addresses
    break;

  case LWS_CALLBACK_CLOSED:
    error ("websocket_recv_server: Client disconnected.");
    break;

  case LWS_CALLBACK_RECEIVE:
    memcpy (&buf, message, message_length);
    buf[message_length] = '\0';

    json_object *json_message = json_tokener_parse (buf);
    json_object *json_key = json_object_new_object ();

    if (!json_object_object_get_ex (json_message, x->json_key, &json_key)) {
      error ("websocket_recv_server: Got mesage without fitting key (%s): %s.", x->json_key, buf);
    } else {
      switch (json_object_get_type (json_key)) {
      case json_type_string:
        value_string = json_object_get_string (json_key);
        post ("websocket_recv_server: Got message with value %s; sending as symbol.", value_string);
        outlet_anything (x->outlet_string, gensym (value_string), 0, NULL);
        break;
      case json_type_int:
        value_number = json_object_get_double (json_key);
        post ("websocket_recv_server: Got message with value %f; sending as float.", value_number);
        outlet_float (x->outlet_string, value_number);
        break;
      case json_type_double:
        value_number = json_object_get_int (json_key);
        post ("websocket_recv_server: Got message with value %f; sending as float.", value_number);
        outlet_float (x->outlet_string, value_number);
        break;
      default:
        error ("websocket_recv_server: Got message with unknown data type: %d.", json_object_get_type (json_key));
        break;
      }
    }

    break;
  default:
    //post("websocket_recv_server: got unknown status code from libwebsocket %d.", reason);
    break;
  }
  return 0;
}

void websocket_recv_server_create (void *x_void) {
  lws_set_log_level (0, NULL);
  t_websocket_recv_server *x = (t_websocket_recv_server *) x_void;

  static struct lws_protocols protocols[] = {
    {"switch-protocol", websocket_recv_server_callback,},
    {NULL, NULL, 0, 0}
  };
  struct lws_context_creation_info info;
  memset (&info, 0, sizeof (info));
  info.port = x->server_port;
  info.iface = NULL;
  info.protocols = protocols;
//  info.extensions = lws_get_internal_extensions();
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.gid = -1;
  info.uid = -1;
//  info.options = opts;

  info.user = x;                //pass t_websocket_recv_server struct to callback

  x->websocket_context = lws_create_context (&info);
  if (x->websocket_context == NULL) {
    error ("websocket_recv_server: Creating libwebsocket context failed - websockets are not available.");
    pthread_exit (NULL);
    return;
  }

  //Start listening
  while (!x->websocket_should_exit) {
    lws_service(x->websocket_context, 50);
  }

  lws_context_destroy (x->websocket_context);
  pthread_exit (NULL);
}

void websocket_recv_server_free (t_websocket_recv_server * x) {
  x->websocket_should_exit = 1;
  pthread_join (x->websocket_thread_handle, NULL);      //We still hope that the thread is running....

  outlet_free (x->outlet_string);
}

void *websocket_recv_server_new (t_symbol * s, int argc, t_atom * argv) {
  if (argc != 2) {
    error ("websocket_recv_server: Server port and key are required.");
    return NULL;
  }

  t_websocket_recv_server *x = (t_websocket_recv_server *) pd_new (websocket_recv_server_class);
  x->websocket_should_exit = false;

  x->outlet_string = outlet_new (&x->x_obj, 0);

  x->server_port = (int) atom_getfloat (argv);
  atom_string (argv + 1, x->json_key, 100);

  pthread_create (&x->websocket_thread_handle, NULL, (void *) &websocket_recv_server_create, x);

  post ("websocket_recv_server: Started server on port %d and waiting for JSON messages with key \"%s\".", x->server_port, x->json_key);
  return (void *) x;
}

void websocket_recv_server_setup (void) {
  websocket_recv_server_class = class_new (gensym ("websocket_recv_server"), (t_newmethod) websocket_recv_server_new, (t_method) websocket_recv_server_free, sizeof (t_websocket_recv_server), CLASS_NOINLET, A_GIMME, 0);
  class_sethelpsymbol (websocket_recv_server_class, gensym ("websocket_recv_server"));
}
