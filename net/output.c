#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
  binaryname = "ns_output";

  // LAB 6: Your code here:
  //  - read a packet from the network server
  //	- send the packet to the device driver
  while (1) {
    cprintf("ipc recving\n");
    sys_ipc_recv(&nsipcbuf);

    if ( (thisenv->env_ipc_from != ns_envid) ||
        (thisenv->env_ipc_value != NSREQ_OUTPUT) ) {
      cprintf("what\n");
      continue;
    }

    cprintf("attempting to transmit\n");
    while (sys_net_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0) {
      cprintf("bad transmit\n");
    }
  }

}
