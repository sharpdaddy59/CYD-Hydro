// mdns_advertise.h — minimal mDNS service advertisement.
//
// Single-device firmware: we ADVERTISE ourselves so a hydro-dash unit
// elsewhere on the LAN can discover us, but we never BROWSE — there's
// no remote-device list to populate. Browse logic from hydro-dash's
// discovery.cpp is deliberately not ported.
//
// Hostname: from device_id.cpp (cyd-hydro-<last4mac>). HTTP service
// announced on port HTTP_PORT (config.h, defaults to 80). Filename
// disambiguated from ESPmDNS's MDNS.h to avoid header confusion.

#pragma once

void mdns_advertise_begin();
