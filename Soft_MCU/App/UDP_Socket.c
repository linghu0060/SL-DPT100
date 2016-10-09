/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    UDP_Socket.c
 * Purpose: UDP Socket Code Template
 * Rev.:    V6.00
 *----------------------------------------------------------------------------*/
//! [code_UDP_Socket]
#include "rl_net.h"
 
int32_t udp_sock;                       // UDP socket handle number
 
// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const uint8_t *ip_addr, uint16_t port, const uint8_t *buf, uint32_t len) {

  // Data received
  /* Example
  if ((buf[0] == 0x01) && (len == 2)) {
    // Switch LEDs on and off
    // LED_out (buf[1]);
  }
  */
  return (0);
}
 
// Send UDP data to destination client.
void send_udp_data (void) {

  if (udp_sock > 0) {
    /* Example
    uint8_t dest_ip [IP4_ADDR_LEN];
    uint8_t *sendbuf;

    dest_ip[0] = 192;
    dest_ip[1] = 168;
    dest_ip[2] = 0;
    dest_ip[3] = 100;

    sendbuf = udp_get_buf (2);
    sendbuf[0] = 0x01;
    sendbuf[1] = 0xAA;

    udp_send (udp_sock, dest_ip, 2000, sendbuf, 2);
    */
  }
}
 
// Allocate and initialize the socket.
/* Example
int main (void) {

  net_initialize ();

  // Initialize UDP socket and open port 2000
  udp_sock = udp_get_socket (0, UDP_OPT_SEND_CHECKSUM | UDP_OPT_VERIFY_CHECKSUM, udp_cb_func);
  if (udp_sock > 0) {
    udp_open (udp_sock, 2000);
  }

  while(1) {
    net_main ();
    osThreadYield ();
  }
}
*/
//! [code_UDP_Socket]
