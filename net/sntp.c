/*
 * SNTP support driver
 *
 * Masami Komiya <mkomiya@sonare.it> 2005
 *
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <rtc.h>

#include "sntp.h"

#if ((CONFIG_COMMANDS & CFG_CMD_NET) && (CONFIG_COMMANDS & CFG_CMD_SNTP))

#define SNTP_TIMEOUT 10

static int SntpOurPort;

static void
SntpSend (void)
{
	struct sntp_pkt_t pkt;
	int pktlen = SNTP_PACKET_LEN;
	int sport;

	debug ("%s\n", __FUNCTION__);

	memset (&pkt, 0, sizeof(pkt));

	pkt.li = NTP_LI_NOLEAP;
	pkt.vn = NTP_VERSION;
	pkt.mode = NTP_MODE_CLIENT;

	memcpy ((char *)NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE, (char *)&pkt, pktlen);

	SntpOurPort = 10000 + (get_timer(0) % 4096);
	sport = NTP_SERVICE_PORT;

	NetSendUDPPacket (NetServerEther, NetNtpServerIP, sport, SntpOurPort, pktlen);
}

static void
SntpTimeout (void)
{
	puts ("Timeout\n");
	NetState = NETLOOP_FAIL;
	return;
}

static void
SntpHandler (uchar *pkt, unsigned dest, unsigned src, unsigned len)
{
	struct sntp_pkt_t rpkt;
	struct rtc_time tm;

	debug ("%s\n", __FUNCTION__);

	if (dest != SntpOurPort) return;

	memcpy ((unsigned char *)&rpkt, pkt, len);

#if (CONFIG_COMMANDS & CFG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
to_tm(ntohl(rpkt.transmit_timestamp), &tm);
printf ("Date: %4d-%02d-%02d Time: %2d:%02d:%02d\n",
tm.tm_year, tm.tm_mon, tm.tm_mday,
tm.tm_hour, tm.tm_min, tm.tm_sec);
	to_tm(ntohl(rpkt.transmit_timestamp) - 2208988800u + NetTimeOffset, &tm);
#if (CONFIG_COMMANDS & CFG_CMD_DATE)
	rtc_set (&tm);
#endif
	printf ("Date: %4d-%02d-%02d Time: %2d:%02d:%02d\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif

	NetState = NETLOOP_SUCCESS;
}

void
SntpStart (void)
{
	debug ("%s\n", __FUNCTION__);

	NetSetTimeout (SNTP_TIMEOUT * CFG_HZ, SntpTimeout);
	NetSetHandler(SntpHandler);
	memset (NetServerEther, 0, 6);

	SntpSend ();
}

#endif /* CONFIG_COMMANDS & CFG_CMD_SNTP */