// http_server.h — synchronous WebServer for /sensors, /status, /sim, /wifi/reset.
//
// `WebServer.h` is single-threaded: handle_client() must be polled from
// the main loop via http_server_loop(). Same convention as
// cores3-hydro and hydro-dash.

#pragma once

void http_server_begin();
void http_server_loop();
