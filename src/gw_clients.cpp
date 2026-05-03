/**
 * @file gw_clients.cpp
 * @brief Shows TCP clients connected to the MySensors gateway port.
 *
 * Isolated in its own translation unit to avoid the enum name clash between
 * lwip's tcp_state (ESTABLISHED, TIME_WAIT …) and the identically-named
 * members in ESP8266WiFi's wl_definitions.h.  This file must NOT include
 * <ESP8266WiFi.h> or any header that pulls it in.
 */

extern "C" {
#include <lwip/priv/tcp_priv.h>   // tcp_active_pcbs, struct tcp_pcb
#include <lwip/ip_addr.h>          // ip_2_ip4, ip4_addr_get_u32, ipaddr_ntoa_r
}

#include <Arduino.h>
#include "gw_clients.h"

// ── configuration ────────────────────────────────────────────────────────────
static const uint16_t GW_PORT        = 5003;  // MY_PORT in main.cpp
static const int      GW_MAX_CLIENTS = 4;     // MY_GATEWAY_MAX_CLIENTS

// ── connection tracking ───────────────────────────────────────────────────────
struct GwClientEntry {
    uint32_t      remoteIp;
    uint16_t      remotePort;
    unsigned long connectedAtMs;
    bool          active;
};
static GwClientEntry gwClients[GW_MAX_CLIENTS];

static const char* tcpStateStr(int s) {
    switch (s) {
        case 4:  return "ESTABLISHED";
        case 7:  return "CLOSE_WAIT";
        case 10: return "TIME_WAIT";
        case 5:  return "FIN_WAIT_1";
        case 6:  return "FIN_WAIT_2";
        case 3:  return "SYN_RCVD";
        case 2:  return "SYN_SENT";
        default: return "OTHER";
    }
}

static unsigned long gwClientTrack(uint32_t rip, uint16_t rport) {
    for (int i = 0; i < GW_MAX_CLIENTS; i++) {
        if (gwClients[i].active &&
            gwClients[i].remoteIp == rip &&
            gwClients[i].remotePort == rport) {
            return gwClients[i].connectedAtMs;
        }
    }
    for (int i = 0; i < GW_MAX_CLIENTS; i++) {
        if (!gwClients[i].active) {
            gwClients[i].remoteIp      = rip;
            gwClients[i].remotePort    = rport;
            gwClients[i].connectedAtMs = millis();
            gwClients[i].active        = true;
            return gwClients[i].connectedAtMs;
        }
    }
    return millis();
}

static void gwClientsPrune() {
    for (int i = 0; i < GW_MAX_CLIENTS; i++) {
        if (!gwClients[i].active) continue;
        bool found = false;
        for (struct tcp_pcb *pcb = tcp_active_pcbs; pcb; pcb = pcb->next) {
            if (pcb->local_port != GW_PORT) continue;
            if (ip4_addr_get_u32(ip_2_ip4(&pcb->remote_ip)) == gwClients[i].remoteIp &&
                pcb->remote_port == gwClients[i].remotePort) {
                found = true;
                break;
            }
        }
        if (!found) gwClients[i].active = false;
    }
}

// ── public API ────────────────────────────────────────────────────────────────
void buildGwClientsHtml(char *buf, size_t buflen) {
    if (!buf || buflen == 0) return;
    gwClientsPrune();

    char *p   = buf;
    size_t rem = buflen;
    int n;
#define GW_APPEND(...) do { n = snprintf(p, rem, __VA_ARGS__); if (n > 0 && (size_t)n < rem) { p += n; rem -= n; } } while(0)

    GW_APPEND("<b>Connected Clients</b> (Port %u)<br />"
              "<table><thead><tr>"
              "<th>Remote IP</th><th>Port</th><th>State</th><th>Duration</th>"
              "</tr></thead><tbody>", GW_PORT);

    int count = 0;
    for (struct tcp_pcb *pcb = tcp_active_pcbs; pcb; pcb = pcb->next) {
        if (pcb->local_port != GW_PORT) continue;

        uint32_t      rip    = ip4_addr_get_u32(ip_2_ip4(&pcb->remote_ip));
        unsigned long connAt = gwClientTrack(rip, pcb->remote_port);
        unsigned long durSec = (millis() - connAt) / 1000UL;

        char ipbuf[16];
        ipaddr_ntoa_r(&pcb->remote_ip, ipbuf, sizeof(ipbuf));

        GW_APPEND("<tr><td>%s</td><td>%u</td><td>%s</td>"
                  "<td>%luh%02lum%02lus</td></tr>",
                  ipbuf, pcb->remote_port, tcpStateStr((int)pcb->state),
                  durSec / 3600UL, (durSec % 3600UL) / 60UL, durSec % 60UL);
        count++;
    }

    if (count == 0)
        GW_APPEND("<tr><td colspan=\"4\"><em>keine Clients verbunden</em></td></tr>");

    GW_APPEND("</tbody></table>");
#undef GW_APPEND
}
