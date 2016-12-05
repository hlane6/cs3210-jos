#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
  binaryname = "ns_input";

  // LAB 6: Your code here:
  //  - read a packet from the device driver
  //	- send it to the network server
  // Hint: When you IPC a page to the network server, it will be
  // reading from it for a while, so don't immediately receive
  // another packet in to the same physical page.
  //
  
  uint8_t buf[1028];

  while (0) {
    while (sys_receive(buf) < 0) {
      cprintf("error receiving\n");
      sys_yield();
    }

    while (sys_page_alloc(0, &nsipcbuf, PTE_U | PTE_W | PTE_P) < 0)
      ;

    nsipcbuf.pkt.jp_len = 1028;
    memmove(nsipcbuf.pkt.jp_data, buf, 1028);

    while (sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_U | PTE_W) < 0)
      ;
  }
  
}
