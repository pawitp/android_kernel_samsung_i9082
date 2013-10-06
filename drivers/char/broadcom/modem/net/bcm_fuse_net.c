/****************************************************************************
*
*     Copyright (c) 2009 Broadcom Corporation
*
*   Unless you and Broadcom execute a separate written software license
*   agreement governing use of this software, this software is licensed to you
*   under the terms of the GNU General Public License version 2, available
*    at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL").
*
*   Notwithstanding the above, under no circumstances may you combine this
*   software in any way with any other Broadcom software provided under
*   a license  other than the GPL, without Broadcom's express prior written
*   consent.
*
****************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/if_arp.h>

#define CSL_TYPES_H
#include <linux/broadcom/bcm_fuse_net_if.h>

#include "mobcom_types.h"
#include "resultcode.h"
#include "taskmsgs.h"

#include <linux/broadcom/ipcproperties.h>
#include "rpc_ipc.h"
#include "rpc_ipc_config.h"

#include "xdr_porting_layer.h"
#include "xdr.h"
#include "rpc_api.h"

#include "bcm_net_debug.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

static int __init bcm_fuse_net_init_module(void);
static void __exit bcm_fuse_net_exit_module(void);

/* used to generate valid RPC client ID for fuse_net driver */
extern unsigned char SYS_GenClientID(void);

/**
   Packet Data EndPoint buffer pool info
 */
#define BCM_NET_MAX_DATA_LEN       1500	/* bytes */
#define BCM_NET_MAX_NUM_PKTS       250	/* packets */

#define BCM_DUALSIM_SIMID_NETIOCTL (SIOCDEVPRIVATE + 1)
#define BCM_MAX_SIM_ID 2

/* ip protocol header */
#define IPV6_PROTOCOL_HEADER 0x60
#define PROTOCOL_HEADER_OFFSET 0xf0

typedef enum {
	EFree = 0,
	EInUse = 1
} net_dev_entry_stat_t;

#define FUSE_NET_NAPI
#ifdef FUSE_NET_NAPI
/* ring buffer for packets "PTR", this is steal, not copy, packets get
 * returned to IPC pool by calling RPC_PACKET_FreeBuffer()
 */
typedef struct {
	uint16_t readIndex;
	uint16_t writeIndex;
	PACKET_BufHandle_t bufferHandle[CFG_RPC_PKTDATA_MAX_NW2TE_PACKETS];
	struct napi_struct napiInfo;
} net_rx_Napi_T;
#endif

typedef struct {
	net_dev_entry_stat_t entry_stat;
	struct net_device *dev_ptr;
	uint8_t pdp_context_id;
	uint8_t sim_id;
	unsigned long ip_addr;
	struct net_device_stats stats;
#ifdef FUSE_NET_NAPI
	net_rx_Napi_T rx_napi;
#endif
} net_drvr_info_t;

spinlock_t g_dev_lock;
static net_drvr_info_t g_net_dev_tbl[BCM_NET_MAX_PDP_CNTXS];
static unsigned char g_NetClientId;

static void bcm_fuse_net_fc_cb(RPC_FlowCtrlEvent_t event, unsigned char cid);
static RPC_Result_t bcm_fuse_net_bd_cb(PACKET_InterfaceType_t interfaceType,
				       unsigned char cid,
				       PACKET_BufHandle_t dataBufHandle);
static void bcm_fuse_net_init(struct net_device *dev_ptr);
static net_drvr_info_t *bcm_fuse_net_device_pdp_lookup(uint8_t pdp_context_id);
static uint8_t bcm_fuse_net_pdp_id(net_drvr_info_t *drvr_info_ptr);
static uint8_t bcm_fuse_net_find_entry(net_drvr_info_t *ndrvr_info_ptr);
static void bcm_fuse_net_free_entry(uint8_t pdp_cid);
#ifdef FUSE_NET_NAPI
static int bcm_fuse_net_poll(struct napi_struct *napi, int budget);
#endif

/**
	Definitions for bcm_fuse_net proc entry
*/
#define BCM_FUSE_NET_PROC_MAX_STR_LEN       15
static struct proc_dir_entry *bcm_fuse_net_config_proc_entry;

/**
	Write function for bcm_fuse_net proc entry
*/
static ssize_t bcm_fuse_net_proc_write(struct file *procFp,
				       const char __user *ubuff,
				       unsigned long len, void *data)
{
	char uStr[BCM_FUSE_NET_PROC_MAX_STR_LEN];
	bool error = TRUE;
	int length = len;
	int proc_idx;
	int proc_c_id;
	int proc_sim_id;

	BNET_DEBUG(DBG_INFO, "%s: New user settings %s\n", __func__, ubuff);

	if (len > BCM_FUSE_NET_PROC_MAX_STR_LEN) {
		BNET_DEBUG(DBG_INFO,
			   "%s: New settings string is too long!\n", __func__);
	} else if (copy_from_user(uStr, ubuff, len)) {
		BNET_DEBUG(DBG_INFO, "%s: Failed to copy new settings!\n",
			   __func__);
		length = 0;
	} else if (sscanf(uStr, "%d %d %d", &proc_idx, &proc_c_id, &proc_sim_id)
		   != 3) {
		BNET_DEBUG(DBG_INFO, "%s: Failed to get new settings!\n",
			   __func__);
	} else if ((proc_idx < 0) || (proc_idx >= BCM_NET_MAX_PDP_CNTXS)
		   || (proc_c_id < 1) || (proc_c_id > BCM_NET_MAX_PDP_CNTXS)
		   || (proc_sim_id < 1) || (proc_sim_id > 2)) {
		BNET_DEBUG(DBG_INFO,
			   "%s: Invalid new settings! idx %d c_id %d   sim_id %d\n",
			   __func__, proc_idx, proc_c_id, proc_sim_id);
	} else {
		BNET_DEBUG(DBG_INFO,
			   "%s: New settings:  idx %d c_id %d   sim_id %d\n",
			   __func__, proc_idx, proc_c_id, proc_sim_id);
		error = FALSE;
	}

	if (!error) {
		g_net_dev_tbl[proc_idx].pdp_context_id = proc_c_id;
		g_net_dev_tbl[proc_idx].sim_id = proc_sim_id;
	}

	return (ssize_t)length;
}

/**
	Read function for bcm_fuse_net proc entry
*/
static int bcm_fuse_net_proc_read(char *ubuff, char **start, off_t off,
				  int count, int *eof, void *data)
{
	int len = 0;
	int i;

	len += sprintf(ubuff + len, "IFC     CID     SIM ID     Active\n");
	len += sprintf(ubuff + len, "---     ---     ------     ------\n");
	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].dev_ptr != NULL) {
			len +=
			    sprintf(ubuff + len, "%s  %d      %d         %d\n",
				    g_net_dev_tbl[i].dev_ptr->name,
				    g_net_dev_tbl[i].pdp_context_id,
				    g_net_dev_tbl[i].sim_id,
				    g_net_dev_tbl[i].entry_stat);
		}
	}

	return len;
}

/**
   @fn void bcm_fuse_net_fc_cb(RPC_FlowCtrlEvent_t event, unsigned char8 cid);
 */
static void bcm_fuse_net_fc_cb(RPC_FlowCtrlEvent_t event, unsigned char cid)
{
	struct net_device *dev_ptr = NULL;
	/* net_drvr_info_t *ndrvr_info_ptr = NULL; */
	int i;

	/* Note that, cid here from rpc_ipc.c implmentation is the pool index,
	 * where current implementation in ipc is that "All channels use the
	 * same buffer pool" so when buffer pool is close to the limit, all
	 * channels have to stop flow. cid" here means nothing then.
	 */
#if 0
	/* leave the code there incase later on ipc supports multi pool for
	 * multi data channel ndrvr_info_ptr = &g_net_dev_tbl[0];
	 */
	/* bcm_fuse_net_device_pool_lookup(pool); */
	ndrvr_info_ptr = bcm_fuse_net_device_cid_lookup(cid);
	if (ndrvr_info_ptr == NULL) {
		if (printk_ratelimit())
			BNET_DEBUG(
				DBG_ERROR,
				"%s: bcm_fuse_net_device_cid_lookup() failed\n",
				__func__);
		return;
	}

	dev_ptr = ndrvr_info_ptr->dev_ptr;
#endif

	if ((event != RPC_FLOW_START) && (event != RPC_FLOW_STOP)) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: RECVD Unknown Flow Control Message !!\n",
			   __func__);
		return;
	}

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].entry_stat == EInUse) {
			dev_ptr = g_net_dev_tbl[i].dev_ptr;
			if (event == RPC_FLOW_START) {
				if (printk_ratelimit())
					BNET_DEBUG(DBG_INFO,
						   "%s: RECVD RPC_FLOW_START!! cid=%d\n",
						   __func__,
						   g_net_dev_tbl[i].
						   pdp_context_id);
				if (netif_queue_stopped(dev_ptr))
					netif_wake_queue(dev_ptr);
			} else {
				if (printk_ratelimit())
					BNET_DEBUG(DBG_INFO,
						   "%s: RECVD RPC_FLOW_STOP!! cid=%d\n",
						   __func__,
						   g_net_dev_tbl[i].
						   pdp_context_id);
				netif_stop_queue(dev_ptr);
			}
		}
	}
	return;
}

/**
   @fn RPC_Result_t bcm_fuse_net_bd_cb(PACKET_InterfaceType_t interfaceType, unsigned char cid, PACKET_BufHandle_t dataBufHandle);
*/
static RPC_Result_t bcm_fuse_net_bd_cb(PACKET_InterfaceType_t interfaceType,
				       unsigned char cid,
				       PACKET_BufHandle_t dataBufHandle)
{
#ifndef FUSE_NET_NAPI
	unsigned long data_len = 0;
	struct sk_buff *skb = NULL;
	unsigned char *data_ptr = NULL;
#else
	net_rx_Napi_T *curNapi = NULL;
	unsigned long flags;
#endif
	net_drvr_info_t *ndrvr_info_ptr = NULL;

	BNET_DEBUG(DBG_TRACE2, "%s: receive packet\n", __func__);

	/* ndrvr_info_ptr = &g_net_dev_tbl[0]; */
	ndrvr_info_ptr = bcm_fuse_net_device_pdp_lookup(cid);
	if (ndrvr_info_ptr == NULL) {
		if (printk_ratelimit())
			BNET_DEBUG(DBG_ERROR,
				   "%s: bcm_fuse_net_device_pdp_lookup failed\n",
				   __func__);

		return RPC_RESULT_ERROR;
	}
#ifdef FUSE_NET_NAPI
	if (napi_schedule_prep(&(ndrvr_info_ptr->rx_napi.napiInfo))) {
		BNET_DEBUG(DBG_TRACE2, "done napi_schedule_prep()\n");
		/* can't disable IPC interrupt for now, TODO */

		/* Raise a NET_RX software interrupt and add the device to
		   the polling state */
		__napi_schedule(&(ndrvr_info_ptr->rx_napi.napiInfo));
	} else {
		/* already in poll state, do nothing for napi */
		BNET_DEBUG(DBG_TRACE2, "rx in poll state already\n");
	}

	spin_lock_irqsave(&g_dev_lock, flags);
	curNapi = &(ndrvr_info_ptr->rx_napi);
	curNapi->bufferHandle[curNapi->writeIndex] = dataBufHandle;
	curNapi->writeIndex++;
	if (curNapi->writeIndex == CFG_RPC_PKTDATA_MAX_NW2TE_PACKETS)
		curNapi->writeIndex = 0;
	spin_unlock_irqrestore(&g_dev_lock, flags);

	BNET_DEBUG(DBG_TRACE, "napi queue: readIndex=%d, writeIndex=%d\n",
		   curNapi->readIndex, curNapi->writeIndex);

	/* return RPC_RESULT_PENDING, so that RPC will not free IPC buffer
	   buffers will be freed during poll */
	return RPC_RESULT_PENDING;

#else /* if not FUSE_NET_NAPI */
	BNET_DEBUG(DBG_TRACE2,
		   "%s: RECVD Buffer Delivery on AP Packet channel, cid[%d] size[%d]!!\n",
		   __func__, cid, data_len);

	data_len = RPC_PACKET_GetBufferLength(dataBufHandle);

	skb = dev_alloc_skb(data_len);
	if (skb == NULL) {
		if (printk_ratelimit())
			BNET_DEBUG(DBG_ERROR,
				   "%s: dev_alloc_skb() failed - packet dropped\n",
				   __func__);

		ndrvr_info_ptr->stats.rx_dropped++;
		return RPC_RESULT_ERROR;
	}

	data_ptr = (unsigned char *)RPC_PACKET_GetBufferData(dataBufHandle);
	memcpy(skb_put(skb, data_len), data_ptr, data_len);

	skb->dev = ndrvr_info_ptr->dev_ptr;
	/* skb->ip_summed = CHECKSUM_UNNECESSARY; *//* don't check it */
	skb->pkt_type = PACKET_HOST;
	ndrvr_info_ptr->dev_ptr->last_rx = jiffies;

	ndrvr_info_ptr->stats.rx_packets++;
	ndrvr_info_ptr->stats.rx_bytes += data_len;

	/* check if ipv4 or ipv6 */
	if ((data_ptr[0] & PROTOCOL_HEADER_OFFSET) == IPV6_PROTOCOL_HEADER) {
		BNET_DEBUG(DBG_TRACE2, "%s: packet received as of ipv6\n",
			   __func__);
		skb->protocol = htons(ETH_P_IPV6);
	} else {
		BNET_DEBUG(DBG_TRACE2, "%s: packet received as of ipv4\n",
			   __func__);
		skb->protocol = htons(ETH_P_IP);
	}

	BNET_DEBUG(DBG_TRACE2, "%s: rx_bytes:%ld\n", __func__,
		   ndrvr_info_ptr->stats.rx_bytes);

	netif_rx(skb);

	return RPC_RESULT_OK;
#endif /* endof FUSE_NET_NAPI */
}

/* callback for CP silent reset events */
void bcm_fuse_net_cp_reset_cb(RPC_CPResetEvent_t event,
			      PACKET_InterfaceType_t interface)
{
	int i;
	struct net_device *dev_ptr = NULL;

	BNET_DEBUG(DBG_INFO, "event %s interface %d\n",
		   RPC_CPRESET_START == event ?
		   "RPC_CPRESET_START" : "RPC_CPRESET_COMPLETE", interface);

	/* should just need to stop outgoing packet flow here
	   until we get RPC_CPRESET_COMPLETE
	 */
	if (event == RPC_CPRESET_START) {
		for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++)
			if (g_net_dev_tbl[i].entry_stat == EInUse) {
				dev_ptr = g_net_dev_tbl[i].dev_ptr;
				netif_stop_queue(dev_ptr);
				BNET_DEBUG(DBG_INFO,
					   "stopping interface %d\n", i);
			}

		/* for now, just ack... */
		RPC_PACKET_AckReadyForCPReset(0, INTERFACE_PACKET);
	} else if (event == RPC_CPRESET_COMPLETE) {
		for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++)
			if (g_net_dev_tbl[i].entry_stat == EInUse) {
				dev_ptr = g_net_dev_tbl[i].dev_ptr;
				if (netif_queue_stopped(dev_ptr)) {
					netif_wake_queue(dev_ptr);
					BNET_DEBUG(DBG_INFO,
						   "waking interface %d\n", i);
				}
			}
	} else
		BNET_DEBUG(DBG_INFO, "unexpected event %d\n", (int)event);

	/* **FIXME** MAG - net interfaces should be brought down as
	   part of CP reset (for Android, RIL or DUN will do this). Are
	   there other situations where somebody else will need to bring
	   down the interface?
	 */
	return;
}

static int bcm_fuse_net_open(struct net_device *dev)
{
	int i;
	net_drvr_info_t ndrvr_info_ptr;
	unsigned long flags;
	uint8_t idx = BCM_NET_MAX_PDP_CNTXS;	/* 0; */
	int ret = 0;
	static int IsFirstCall;

	ndrvr_info_ptr.dev_ptr = dev;

    /**
       Register callbacks with the RPC Proxy server
    */
	if (0 == IsFirstCall) {
		g_NetClientId = SYS_GenClientID();
		ret = RPC_PACKET_RegisterDataInd(g_NetClientId,
						 INTERFACE_PACKET,
						 bcm_fuse_net_bd_cb,
						 bcm_fuse_net_fc_cb,
						 bcm_fuse_net_cp_reset_cb);
		if (RPC_RESULT_OK != ret) {
			BNET_DEBUG(DBG_ERROR,
				   "%s: first call client ID[%d] FAIL\n",
				   __func__, g_NetClientId);
			return -1;
		}
		IsFirstCall++;
		BNET_DEBUG(DBG_INFO,
			   "%s: first call client ID[%d]\n",
			   __func__, g_NetClientId);
	}

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		BNET_DEBUG(DBG_INFO,
			   "%s: g_net_dev_tbl[%d]=0x%x,dev_ptr 0x%x, dev 0x%x, 0x%x\n",
			   __func__, i,
			   (unsigned int)(&g_net_dev_tbl[i]),
			   (unsigned int)(g_net_dev_tbl[i].dev_ptr),
			   (unsigned int)dev, (unsigned int)(&ndrvr_info_ptr));
	}

	idx = bcm_fuse_net_find_entry(&ndrvr_info_ptr);
	if (idx == BCM_NET_MAX_PDP_CNTXS) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: No free device interface to assign for pdp_cid[%d]\n",
			   __func__, idx);
		return -EISCONN;
	}

	spin_lock_irqsave(&g_dev_lock, flags);
	/* g_net_dev_tbl[idx].pdp_context_id = idx+1; */
	g_net_dev_tbl[idx].pdp_context_id = RMNET_TO_CID(idx);

	spin_unlock_irqrestore(&g_dev_lock, flags);
	BNET_DEBUG(DBG_INFO,
		   "%s: BCM_FUSE_NET_ACTIVATE_PDP: rmnet[%d] pdp_info.cid=%d\n",
		   __func__, idx, g_net_dev_tbl[idx].pdp_context_id);

#ifdef FUSE_NET_NAPI
	/* for napi */
	BNET_DEBUG(DBG_INFO, "bcm_fuse_net_open(), napi enable\n");
	spin_lock_irqsave(&g_dev_lock, flags);
	napi_enable(&(g_net_dev_tbl[idx].rx_napi.napiInfo));
	spin_unlock_irqrestore(&g_dev_lock, flags);
#endif

	netif_start_queue(dev);
	BNET_DEBUG(DBG_INFO, "start queue\n");

	return 0;
}

static int bcm_fuse_net_stop(struct net_device *dev)
{
	int i;
#ifdef FUSE_NET_NAPI
	unsigned long flags;
#endif
	BNET_DEBUG(DBG_INFO, "%s: <<\n", __func__);
	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].dev_ptr == dev) {
			bcm_fuse_net_free_entry(g_net_dev_tbl[i].
						pdp_context_id);
			BNET_DEBUG(DBG_INFO,
				   "%s: free g_net_dev_tbl[%d].cid:%d\n",
				   __func__, i,
				   g_net_dev_tbl[i].pdp_context_id);

#ifdef FUSE_NET_NAPI
			spin_lock_irqsave(&g_dev_lock, flags);
			napi_disable(&(g_net_dev_tbl[i].rx_napi.napiInfo));
			spin_unlock_irqrestore(&g_dev_lock, flags);
#endif
			break;
		}
	}
	netif_stop_queue(dev);
	return 0;
}

static int bcm_fuse_net_tx(struct sk_buff *skb, struct net_device *dev)
{
	void *buff_data_ptr;
	uint8_t pdp_cid = BCM_NET_MAX_PDP_CNTXS;
	PACKET_BufHandle_t buffer;

	static int sim_id;
	net_drvr_info_t *t_ndrvr_info_ptr = NULL;
	int i;

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].dev_ptr == dev) {
			sim_id = g_net_dev_tbl[i].sim_id;
			t_ndrvr_info_ptr = &g_net_dev_tbl[i];
			BNET_DEBUG(DBG_TRACE2,
				   "%s: g_net_dev_tbl[%d]=0x%x, a_sim_id %d, sim_id %d\n",
				   __func__, i,
				   (unsigned int)(&g_net_dev_tbl[i]),
				   g_net_dev_tbl[i].sim_id, sim_id);
			break;
		}
	}

	if (NULL == t_ndrvr_info_ptr) {
		BNET_DEBUG(DBG_ERROR, "bcm_fuse_net_tx(), no device found\n");
		return -EINVAL;
	}

	if (BCM_NET_MAX_DATA_LEN < skb->len) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: len[%d] exceeds supported len[%d] failed\n",
			   __func__, skb->len, BCM_NET_MAX_DATA_LEN);
		t_ndrvr_info_ptr->stats.tx_errors++;
		return -1;
	}

	if (0 == skb->len) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: len[%d] is zero size failed\n",
			   __func__, skb->len);
		return -1;
	}

	pdp_cid = bcm_fuse_net_pdp_id(t_ndrvr_info_ptr);
	if (BCM_NET_INVALID_PDP_CNTX == pdp_cid) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: net device to pdp context id mapping failed\n",
			   __func__);
		t_ndrvr_info_ptr->stats.tx_errors++;
		return -1;
	}

	/* Allocate a buffer */
	buffer = RPC_PACKET_AllocateBuffer(INTERFACE_PACKET, skb->len, pdp_cid);
	if (!buffer) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: Error buffer Handle cid %d\n",
			   __func__, pdp_cid);
		t_ndrvr_info_ptr->stats.tx_errors++;
		return -ENOBUFS;
	}
	/* transfer data from skb to ipc_buffer */
	buff_data_ptr = RPC_PACKET_GetBufferData(buffer);
	if (buff_data_ptr == NULL) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: RPC_PACKET_GetBufferData() failed\n", __func__);
		t_ndrvr_info_ptr->stats.tx_errors++;
		return -ENOBUFS;
	}

	memset(buff_data_ptr, 0, BCM_NET_MAX_DATA_LEN);
	memcpy(buff_data_ptr, skb->data, skb->len);

	RPC_PACKET_SetBufferLength(buffer, skb->len);

	dev->trans_start = jiffies;	/* save the timestamp */
	RPC_PACKET_SetContext(INTERFACE_PACKET, buffer, sim_id);
	RPC_PACKET_SendData(g_NetClientId, INTERFACE_PACKET, pdp_cid, buffer);

    /**
      The IPC buffer is freed by the receiving end point.
    */
	t_ndrvr_info_ptr->stats.tx_packets++;
	t_ndrvr_info_ptr->stats.tx_bytes += skb->len;
	BNET_DEBUG(DBG_TRACE2,
		   "%s: tx_bytes:%ld simid:%d cid:%d\n",
		   __func__, t_ndrvr_info_ptr->stats.tx_bytes, sim_id, pdp_cid);

	dev_kfree_skb(skb);

	return 0;
}

static struct net_device_stats *bcm_fuse_net_stats(struct net_device *dev)
{
	net_drvr_info_t *ndrvr_info_ptr = NULL;
	int i;

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].dev_ptr == dev) {
			ndrvr_info_ptr = &g_net_dev_tbl[i];
			break;
		}
	}

	return &ndrvr_info_ptr->stats;
}

/**
   @fn int bcm_fuse_net_config(struct net_device *dev_ptr, struct ifmap *map);
*/
int bcm_fuse_net_config(struct net_device *dev_ptr, struct ifmap *map)
{
	BNET_DEBUG(DBG_INFO, "%s: <<\n", __func__);
	if (dev_ptr->flags & IFF_UP)	/* can't act on a running interface */
		return -EBUSY;

	return 0;
}

/**
   @fn int bcm_fuse_net_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
*/
int bcm_fuse_net_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int i;
	int sim_id = 3;

	BNET_DEBUG(DBG_INFO, "bcm_fuse_net_ioctl cmd=%d\n", cmd);

	if (BCM_DUALSIM_SIMID_NETIOCTL != cmd) {
		BNET_DEBUG(DBG_INFO, "%s: Incorrect IOCTL ID 0x%x\n",
			   __func__, cmd);
		return -ENOTTY;
	}

	/* get sim_id */
	if (copy_from_user(&sim_id, ifr->ifr_data, sizeof(int))) {
		BNET_DEBUG(DBG_ERROR, "error reading ifr_data\n");
		return -EFAULT;
	}

	if ((sim_id < 0) || (sim_id > BCM_MAX_SIM_ID)) {
		BNET_DEBUG(DBG_ERROR, "invalid sim id=%d", sim_id);
		return -EINVAL;
	}

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].dev_ptr == dev) {
			g_net_dev_tbl[i].sim_id = sim_id;
			BNET_DEBUG(DBG_ERROR,
				   "%s: g_net_dev_tbl[%d]=0x%x, a_sim_id:%d, sim_id:%d\n",
				   __func__, i,
				   (unsigned int)(&g_net_dev_tbl[i]),
				   g_net_dev_tbl[i].sim_id, sim_id);
			break;
		}
	}
	if (i < BCM_NET_MAX_PDP_CNTXS)	/* To avoid Out-of-bounds read */
		BNET_DEBUG(DBG_ERROR, "%s: Use A SIM ID:%d, SIM ID:%d\n",
			   __func__, g_net_dev_tbl[i].sim_id, sim_id);
	return 0;
}

#ifdef FUSE_NET_NAPI
static int bcm_fuse_net_poll(struct napi_struct *napi, int budget)
{
	int pkt_processed = 0;
	int dev_index = 0;
	net_rx_Napi_T *curNapi;
	PACKET_BufHandle_t curBuf;
	unsigned long data_len = 0;
	struct sk_buff *skb = NULL;
	unsigned char *data_ptr = NULL;
	net_drvr_info_t *ndrvr_info_ptr = NULL;

	BNET_DEBUG(DBG_TRACE2, "%s enter\n", __func__);

	/* find the device */
	for (dev_index = 0; dev_index < BCM_NET_MAX_PDP_CNTXS; dev_index++) {
		if (g_net_dev_tbl[dev_index].dev_ptr == napi->dev) {
			ndrvr_info_ptr = &(g_net_dev_tbl[dev_index]);
			break;
		}
	}

	if (ndrvr_info_ptr == NULL) {
		BNET_DEBUG(DBG_ERROR, "%s: Can't find device\n", __func__);
		return 0;
	}
	curNapi = &(ndrvr_info_ptr->rx_napi);

	spin_lock(&g_dev_lock);
	while ((pkt_processed < budget)
	       && (curNapi->readIndex != curNapi->writeIndex)) {

		curBuf = curNapi->bufferHandle[curNapi->readIndex];

		data_len = RPC_PACKET_GetBufferLength(curBuf);
		skb = dev_alloc_skb(data_len);
		if (skb == NULL) {
			if (printk_ratelimit()) {
				BNET_DEBUG(DBG_ERROR,
				   "%s: dev_alloc_skb() failed, just return\n",
				__func__);
			}
			spin_unlock(&g_dev_lock);

			/*as there is no more memory left, we just return*/
			/*The packet is not copied out, so still stays in IPC*/
			return pkt_processed;
		}

		/*now mark as read and packet can be returned to the IPC pool*/
		curNapi->readIndex++;
		if (curNapi->readIndex == CFG_RPC_PKTDATA_MAX_NW2TE_PACKETS)
			curNapi->readIndex = 0;
		spin_unlock(&g_dev_lock);

		data_ptr = (unsigned char *)RPC_PACKET_GetBufferData(curBuf);
		memcpy(skb_put(skb, data_len), data_ptr, data_len);
		RPC_PACKET_FreeBuffer(curBuf);

		skb->dev = ndrvr_info_ptr->dev_ptr;
		skb->pkt_type = PACKET_HOST;
		ndrvr_info_ptr->dev_ptr->last_rx = jiffies;

		ndrvr_info_ptr->stats.rx_packets++;
		ndrvr_info_ptr->stats.rx_bytes += data_len;

		/* check if ipv4 or ipv6 */
		if ((data_ptr[0] & PROTOCOL_HEADER_OFFSET)
		    == IPV6_PROTOCOL_HEADER) {
			BNET_DEBUG(DBG_TRACE2,
				   "%s: packet received as of ipv6\n",
				   __func__);
			skb->protocol = htons(ETH_P_IPV6);
		} else {
			BNET_DEBUG(DBG_TRACE2,
				   "%s: packet received as of ipv4\n",
				   __func__);
			skb->protocol = htons(ETH_P_IP);
		}

		BNET_DEBUG(DBG_TRACE2, "%s: rx_bytes:%ld\n", __func__,
			   ndrvr_info_ptr->stats.rx_bytes);
		if (netif_receive_skb(skb) == NET_RX_DROP) {
			BNET_DEBUG(DBG_ERROR,
				   "%s: packet dropped by upper stack\n",
				   __func__);
			/* TODO DO we continue or return? */
		}

		pkt_processed++;
		spin_lock(&g_dev_lock);
	}
	spin_unlock(&g_dev_lock);

	/* due to the implementation in napi stack, do not
	   call napi_complete in the case pkt processed are the same
	   as the weight */
	if (pkt_processed == budget) {
		BNET_DEBUG(DBG_INFO,
			   "%s processed %d packets == budget passed in\n",
			   __func__, pkt_processed);
		return pkt_processed;
	}

	spin_lock(&g_dev_lock);
	if (curNapi->readIndex == curNapi->writeIndex) {
		BNET_DEBUG(DBG_TRACE,
			   "%s no more packets, napi complete\n", __func__);
		/* all done, no more packets, Remove the device from
		   the polling state */
		napi_complete(&(curNapi->napiInfo));

		/* TODO, enable IPC interrupt */
	}
	spin_unlock(&g_dev_lock);

	BNET_DEBUG(DBG_TRACE,
		   "%s processed %d packets, current napi queue-%d readIndex=%d, writeIndex=%d\n",
		   __func__, pkt_processed, dev_index,
		   curNapi->readIndex, curNapi->writeIndex);
	return pkt_processed;
}
#endif

/**
   Initialize a net device. (Called from kernel in alloc_netdev())

   @fn static void bcm_fuse_net_init(struct net_device *dev_ptr);
*/

static const struct net_device_ops bcm_netdev_ops = {
	.ndo_open = bcm_fuse_net_open,
	.ndo_stop = bcm_fuse_net_stop,
	.ndo_start_xmit = bcm_fuse_net_tx,
	.ndo_get_stats = bcm_fuse_net_stats,
	.ndo_set_config = bcm_fuse_net_config,
	.ndo_do_ioctl = bcm_fuse_net_ioctl,
};

static void bcm_fuse_net_init(struct net_device *dev)
{
	dev->netdev_ops = &bcm_netdev_ops;

	dev->mtu = BCM_NET_MAX_DATA_LEN;
	dev->tx_queue_len = BCM_NET_MAX_NUM_PKTS;
	dev->flags = IFF_NOARP | IFF_DYNAMIC;
	dev->type = ARPHRD_NONE;	/* per MobC00183378 request */
}

/**
   @fn static int bcm_fuse_net_attach(unsigned int dev_index);
*/
static int bcm_fuse_net_attach(unsigned int dev_index)
{
	int ret = 0;
	unsigned long flags;
	struct net_device *dev_ptr = NULL;

	if (dev_index >= BCM_NET_MAX_PDP_CNTXS) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: Invalid dev_index[%d] passed as parameter\n",
			   __func__, dev_index);
		return -1;
	}

	/* Allocate etherdev, including space for private structure */
	dev_ptr = alloc_netdev(sizeof(net_drvr_info_t *),
			       BCM_NET_DEV_STR, bcm_fuse_net_init);
	if (dev_ptr == NULL) {
		BNET_DEBUG(DBG_ERROR, "%s: Out of memory\n", __func__);
		return -1;
	}

	/* Allocate Buffer pool for this channel */

	spin_lock_irqsave(&g_dev_lock, flags);

	/* dev_ptr->priv = &g_net_dev_tbl[dev_index]; *//* from kernel 2.6.29 */
	g_net_dev_tbl[dev_index].dev_ptr = dev_ptr;
	g_net_dev_tbl[dev_index].entry_stat = EFree;
	g_net_dev_tbl[dev_index].pdp_context_id = BCM_NET_MAX_PDP_CNTXS;
	g_net_dev_tbl[dev_index].sim_id = 1;	/* default set to SIM ID 1 */
#ifdef FUSE_NET_NAPI
	/* for napi */
	/* weight is 16 - 64, TODO tune */
	netif_napi_add(dev_ptr, &(g_net_dev_tbl[dev_index].rx_napi.napiInfo),
		       bcm_fuse_net_poll, 64);
#endif
	BNET_DEBUG(DBG_INFO,
		   "%s: g_net_dev_tbl[%d] = 0x%x, dev_ptr 0x%x, napi weight=%d\n",
		   __func__, dev_index,
		   (unsigned int)(&g_net_dev_tbl[dev_index]),
		   (unsigned int)dev_ptr,
		   g_net_dev_tbl[dev_index].rx_napi.napiInfo.weight);
	BNET_DEBUG(DBG_INFO, "%s: entry_stat 0x%x\n",
		   __func__, g_net_dev_tbl[dev_index].entry_stat);

	spin_unlock_irqrestore(&g_dev_lock, flags);

	ret = register_netdev(dev_ptr);
	if (ret != 0) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: Error [%d] registering device \"%s\"\n",
			   __func__, ret, BCM_NET_DEV_STR);

		/* error recovery, do clean up */
		spin_lock_irqsave(&g_dev_lock, flags);
#ifdef FUSE_NET_NAPI
		netif_napi_del(&(g_net_dev_tbl[dev_index].rx_napi.napiInfo));
#endif
		memset(&g_net_dev_tbl[dev_index], 0, sizeof(net_drvr_info_t));
		spin_unlock_irqrestore(&g_dev_lock, flags);
		return -1;
	}

	return 0;
}

/**
   @fn static int bcm_fuse_net_deattach(unsigned int  dev_index);
*/
static int bcm_fuse_net_deattach(unsigned int dev_index)
{
	unsigned long flags;

	if (dev_index >= BCM_NET_MAX_PDP_CNTXS) {
		BNET_DEBUG(DBG_ERROR,
			   "%s: Invalid dev_index[%d] passed as parameter\n",
			   __func__, dev_index);
		return -1;
	}

	spin_lock_irqsave(&g_dev_lock, flags);

#ifdef FUSE_NET_NAPI
	netif_napi_del(&(g_net_dev_tbl[dev_index].rx_napi.napiInfo));
#endif

	unregister_netdev(g_net_dev_tbl[dev_index].dev_ptr);
	free_netdev(g_net_dev_tbl[dev_index].dev_ptr);

	/* Reset most entries except for the buffer pool */
	g_net_dev_tbl[dev_index].dev_ptr = NULL;
	g_net_dev_tbl[dev_index].entry_stat = EFree;
	g_net_dev_tbl[dev_index].ip_addr = 0;
	g_net_dev_tbl[dev_index].pdp_context_id = BCM_NET_MAX_PDP_CNTXS;
	g_net_dev_tbl[dev_index].sim_id = 0;
	memset(&g_net_dev_tbl[dev_index].stats, 0,
	       sizeof(struct net_device_stats));

	spin_unlock_irqrestore(&g_dev_lock, flags);

	return 0;
}

/**
   @fn static net_drvr_info_t *bcm_fuse_net_device_pdp_lookup(uint8_t pdp_context_id);
*/
static net_drvr_info_t *bcm_fuse_net_device_pdp_lookup(uint8_t pdp_context_id)
{
	int i = 0;
	unsigned long flags;
	net_drvr_info_t *drvr_info_ptr = NULL;

	spin_lock_irqsave(&g_dev_lock, flags);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].entry_stat == EFree)
			continue;

		if (g_net_dev_tbl[i].pdp_context_id == pdp_context_id) {
			drvr_info_ptr = &g_net_dev_tbl[i];
			goto FOUND_ENTRY;
		}
	}

	BNET_DEBUG(DBG_ERROR,
		   "%s: No network device mapping for PDP context id[%d]\n",
		   __func__, pdp_context_id);

FOUND_ENTRY:
	spin_unlock_irqrestore(&g_dev_lock, flags);

	return drvr_info_ptr;
}

/**
   @fn static uint8_t bcm_fuse_net_find_entry(net_drvr_info_t *ndrvr_info_ptr);
*/
static uint8_t bcm_fuse_net_find_entry(net_drvr_info_t *ndrvr_info_ptr)
{
	int i = 0;
	unsigned long flags;
	uint8_t dev_idx = BCM_NET_MAX_PDP_CNTXS;

	if (ndrvr_info_ptr == NULL)
		return dev_idx;

	spin_lock_irqsave(&g_dev_lock, flags);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if ((g_net_dev_tbl[i].dev_ptr == ndrvr_info_ptr->dev_ptr) &&
		    (g_net_dev_tbl[i].entry_stat == EFree)) {
			dev_idx = i;
			g_net_dev_tbl[i].entry_stat = EInUse;
			goto FOUND_ENTRY;
		}
	}

	BNET_DEBUG(DBG_ERROR,
		   "%s: Entry for device interface in use\n", __func__);

FOUND_ENTRY:
	spin_unlock_irqrestore(&g_dev_lock, flags);

	return dev_idx;
}

/**
   @fn static void bcm_fuse_net_free_entry(uint8_t pdp_cid);
*/
static void bcm_fuse_net_free_entry(uint8_t pdp_cid)
{
	int i = 0;
	unsigned long flags;

	if (pdp_cid == BCM_NET_INVALID_PDP_CNTX)
		return;

	spin_lock_irqsave(&g_dev_lock, flags);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].pdp_context_id == pdp_cid) {
			g_net_dev_tbl[i].entry_stat = EFree;
			g_net_dev_tbl[i].pdp_context_id =
			    BCM_NET_INVALID_PDP_CNTX;
			g_net_dev_tbl[i].ip_addr = 0;
#ifdef FUSE_NET_NAPI
			BNET_DEBUG(DBG_INFO, "%s: NAPI free IPC buffers\n",
				   __func__);
			{
				net_rx_Napi_T *curNapi;

				/* free unused packets from IPC buffer */
				curNapi = &(g_net_dev_tbl[i].rx_napi);
				while (curNapi->readIndex !=
				       curNapi->writeIndex) {
					RPC_PACKET_FreeBuffer(curNapi->
							      bufferHandle
							      [curNapi->
							       readIndex]);
					curNapi->readIndex++;
					if (curNapi->readIndex ==
					    CFG_RPC_PKTDATA_MAX_NW2TE_PACKETS) {
						curNapi->readIndex = 0;
					}
				}
				BNET_DEBUG(DBG_INFO,
					   "current napi queue state: readIndex=%d, writeIndex=%d\n",
					   curNapi->readIndex,
					   curNapi->writeIndex);
				curNapi->readIndex = curNapi->writeIndex = 0;
				BNET_DEBUG(DBG_INFO,
					   "set readIndex=writeIndex=0\n");
			}
#endif

			goto FREE_ENTRY_EXIT;
		}
	}

FREE_ENTRY_EXIT:
	spin_unlock_irqrestore(&g_dev_lock, flags);

	return;
}

/**
   @fn static uint8_t bcm_fuse_net_pdp_id(net_drvr_info_t *drvr_info_ptr);
*/
static uint8_t bcm_fuse_net_pdp_id(net_drvr_info_t *drvr_info_ptr)
{
	int i = 0;
	unsigned long flags;
	uint8_t pdp_cid = BCM_NET_INVALID_PDP_CNTX;

	if (drvr_info_ptr == NULL)
		return pdp_cid;

	spin_lock_irqsave(&g_dev_lock, flags);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].entry_stat == EFree)
			continue;

		if (g_net_dev_tbl[i].dev_ptr == drvr_info_ptr->dev_ptr) {
			pdp_cid = g_net_dev_tbl[i].pdp_context_id;
			goto FOUND_ENTRY;
		}
	}

	BNET_DEBUG(DBG_ERROR,
		   "%s: No PDP context id for net devie [%p]\n",
		   __func__, drvr_info_ptr->dev_ptr);

FOUND_ENTRY:
	spin_unlock_irqrestore(&g_dev_lock, flags);

	return pdp_cid;
}

/**
   @fn static net_drvr_info_t *bcm_fuse_net_device_cid_lookup(unsigned char cid);
*/
#ifdef INCLUDE_UNUSED_CODE
static net_drvr_info_t *bcm_fuse_net_device_cid_lookup(unsigned char cid)
{
	int i = 0;
	unsigned long flags;
	net_drvr_info_t *drvr_info_ptr = NULL;

	spin_lock_irqsave(&g_dev_lock, flags);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (g_net_dev_tbl[i].entry_stat == EFree)
			continue;

		if (g_net_dev_tbl[i].pdp_context_id == cid) {
			drvr_info_ptr = &g_net_dev_tbl[i];
			goto FOUND_ENTRY;
		}
	}

	BNET_DEBUG(DBG_ERROR,
		   "%s: No network device mapping for cid[%d]\n",
		   __func__, cid);

FOUND_ENTRY:
	spin_unlock_irqrestore(&g_dev_lock, flags);

	return drvr_info_ptr;
}
#endif /* #ifdef INCLUDE_UNUSED_CODE */

static int __init bcm_fuse_net_init_module(void)
{
	unsigned int i = 0;

	BNET_DEBUG(DBG_INFO, "%s: <<\n", __func__);
	spin_lock_init(&g_dev_lock);

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++)
		memset(&g_net_dev_tbl[i], 0, sizeof(net_drvr_info_t));

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++) {
		if (bcm_fuse_net_attach(i) == -1) {
			/* no need to carry on, something is wrong already,
			 * hopefully the already attached drivers can be enough
			 * to use
			 */
			break;
		}
	}

	/* proc entry for net config settings */
	bcm_fuse_net_config_proc_entry =
	    create_proc_entry("bcm_fuse_net_config", 0666, NULL);
	if (bcm_fuse_net_config_proc_entry == NULL) {
		BNET_DEBUG(DBG_INFO,
			   "%s: Couldn't create bcm_fuse_net_config_proc_entry!\n",
			   __func__);
	} else {
		BNET_DEBUG(DBG_INFO,
			   "%s: bcm_fuse_net_config_proc_entry created\n",
			   __func__);
		bcm_fuse_net_config_proc_entry->write_proc =
		    bcm_fuse_net_proc_write;
		bcm_fuse_net_config_proc_entry->read_proc =
		    bcm_fuse_net_proc_read;
	}

	return 0;
}

/**
   @fn static void __exit bcm_fuse_net_exit_module(void);
*/
static void __exit bcm_fuse_net_exit_module(void)
{
	unsigned int i = 0;

	for (i = 0; i < BCM_NET_MAX_PDP_CNTXS; i++)
		bcm_fuse_net_deattach(i);

	remove_proc_entry("bcm_fuse_net_sim", bcm_fuse_net_config_proc_entry);

	return;
}

module_init(bcm_fuse_net_init_module);
module_exit(bcm_fuse_net_exit_module);
