#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int res;
	int feid;

	while(1) {
		if ((res = ipc_recv(&feid, &nsipcbuf, NULL)) < 0)
		{
			panic("in output, ipc_recv: %e", res);
		}
		if (res != NSREQ_OUTPUT || feid != ns_envid)
		{
			continue;
		}
		while ((res = sys_dl_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0)
			/* spinning */;
	}
}
