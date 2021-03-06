/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
* foss@huawei.com
*
* If distributed as part of the Linux kernel, the following license terms
* apply:
*
* * This program is free software; you can redistribute it and/or modify
* * it under the terms of the GNU General Public License version 2 and
* * only version 2 as published by the Free Software Foundation.
* *
* * This program is distributed in the hope that it will be useful,
* * but WITHOUT ANY WARRANTY; without even the implied warranty of
* * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* * GNU General Public License for more details.
* *
* * You should have received a copy of the GNU General Public License
* * along with this program; if not, write to the Free Software
* * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Otherwise, the following license terms apply:
*
* * Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions
* * are met:
* * 1) Redistributions of source code must retain the above copyright
* *    notice, this list of conditions and the following disclaimer.
* * 2) Redistributions in binary form must reproduce the above copyright
* *    notice, this list of conditions and the following disclaimer in the
* *    documentation and/or other materials provided with the distribution.
* * 3) Neither the name of Huawei nor the names of its contributors may
* *    be used to endorse or promote products derived from this software
* *    without specific prior written permission.
*
* * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/
#ifndef _RNIC_CTX_H_
#define _RNIC_CTX_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "PsLogdef.h"
#include "PsTypeDef.h"
#include "ImmInterface.h"
#include "ImsaRnicInterface.h"
#include "AtRnicInterface.h"
#include "RnicLinuxInterface.h"
#include "RnicTimerMgmt.h"
#include "RnicProcMsg.h"
#include "NVIM_Interface.h"
#include "acore_nv_stru_gucnas.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
  1 全局变量定义
*****************************************************************************/

/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define RNIC_MAC_ADDR_LEN               (6)                                     /* RNIC MAC地址长度，6字节 */
#define RNIC_MAC_HDR_LEN                (14)

#define RNIC_MAX_PACKET                 (1536)                                  /* RNIC允许传输的最大包长为1500字节 */
#define RNIC_R_IMS_MAX_PACKET           (2048)                                  /* R_IMS RNIC允许传输的最大包长为2000字节 */

#define RNIC_IPV4_VERSION               (4)                                     /* IP头部中IP V4版本号 */
#define RNIC_IPV6_VERSION               (6)                                     /* IP头部中IP V6版本号 */

#define RNIC_IP_HEAD_VER_OFFSET_MASK    (0x0F)                                  /* IP头部协议版本偏移量掩码 */
#define RNIC_IP_HEAD_DEST_ADDR_OFFSET   (4 * 4)                                 /* IP头部目的IP地址偏移量 */

#define RNIC_IPV4_HDR_LEN               (20)                                    /* IPV4固定头长度*/
#define RNIC_IPV4_BROADCAST_ADDR        (0xFFFFFFFF)                            /* IPV4广播包地址 */

#define RNIC_DEFAULT_MTU                (1500)                                  /* RNIC默认的MTU值 */

#define RNIC_ETH_TYPE_ARP               (0x0806)                                /* 主机序形式，表示ARP包类型  */
#define RNIC_ETH_TYPE_IP                (0x0800)                                /* 主机序形式，表示IPv4包类型  */
#define RNIC_ETH_TYPE_IPV6              (0x86DD)                                /* 主机序形式，表示IPv6包类型  */
#define RNIC_ETH_TYPE                   (0x0001)                                /* 网络序形式，ARP报文的HardwareFormat字段用，表示以太网类型  */
#define RNIC_ARP_REPLY_TYPE             (0x0002)                                /* 网络序形式，ARP报文的OpCode字段用，表示ARP应答类型         */

#define RNIC_NET_ID_MAX_NUM             (RNIC_RMNET_ID_BUTT)
#define RNIC_MODEM_ID_MAX_NUM           (MODEM_ID_BUTT)

#define RNIC_MAX_IPV6_ADDR_LEN          (16)                                    /* IPV6地址长度 */

#define RNIC_MAX_DSFLOW_BYTE            (0xFFFFFFFF)                            /* RNIC最大流量值 */

#define RNIC_DIAL_DEMA_IDLE_TIME        (600)

#define RNIC_IPV4_ADDR_LEN              (4)                                     /* IPV4地址长度 */
#define RNIC_IPV6_ADDR_LEN              (16)                                    /* IPV6地址长度 */

#define RNIC_INVALID_SPE_PORT           (-1)

/* RabId的高两位表示ModemId, 00表示Modem0, 01表示Modem1, 10表示Modem1 */
#define RNIC_RABID_TAKE_MODEM_1_MASK    (0x40)                                  /* Rabid携带Modem1的掩码 */
#define RNIC_RABID_TAKE_MODEM_2_MASK    (0x80)                                  /* Rabid携带Modem2的掩码 */
#define RNIC_RABID_UNTAKE_MODEM_MASK    (0x3F)                                  /* Rabid去除Modem的掩码 */

/* 最大RABID值 */
#define RNIC_RAB_ID_MAX_NUM             (11)
#define RNIC_RAB_ID_OFFSET              (5)

/* Rab Id的最小值 */
#define RNIC_RAB_ID_MIN                 (5)

/* Rab Id的最大值 */
#define RNIC_RAB_ID_MAX                 (15)

#define RNIC_RAB_ID_INVALID             (0xFF)


#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
#define RNIC_NAPI_POLL_DEFAULT_WEIGHT   (16)                                    /* NAPI 一次Poll轮询的默认最大包个数 */
#define RNIC_NAPI_POLL_MAX_WEIGHT       (64)                                    /* NAPI 一次Poll轮询的协议栈支持的最大包个数 */
#define RNIC_POLL_QUEUE_DEFAULT_MAX_LEN (15000)                                 /* NAPI Poll队列的最大长度 */
#endif

#define RNIC_RAB_ID_IS_VALID(ucRabId) \
            (((ucRabId) >= RNIC_RAB_ID_MIN) && ((ucRabId) <= RNIC_RAB_ID_MAX))

#define RNIC_RMNET_IS_VALID(RmNetId) \
            ((RmNetId) < RNIC_RMNET_ID_BUTT)

#if (MULTI_MODEM_NUMBER >= 2)
#define RNIC_RMNET_R_IS_VALID(RmNetId) \
            ((RNIC_RMNET_ID_R_IMS00 == (RmNetId)) \
          || (RNIC_RMNET_ID_R_IMS10 == (RmNetId)) \
          || (RNIC_RMNET_ID_R_IMS01 == (RmNetId)) \
          || (RNIC_RMNET_ID_R_IMS11 == (RmNetId)))
#else
#define RNIC_RMNET_R_IS_VALID(RmNetId) \
            ((RNIC_RMNET_ID_R_IMS00 == (RmNetId)) \
          || (RNIC_RMNET_ID_R_IMS01 == (RmNetId)))
#endif

#if (FEATURE_ON == FEATURE_MULTI_MODEM)
#define RNIC_RMNET_R_IS_EMC_BEAR(RmNetId) \
            ((RNIC_RMNET_ID_R_IMS01 == (RmNetId)) \
          || (RNIC_RMNET_ID_R_IMS11 == (RmNetId)))
#else
#define RNIC_RMNET_R_IS_EMC_BEAR(RmNetId) \
            (RNIC_RMNET_ID_R_IMS01 == (RmNetId))
#endif


#define RNIC_GET_IP_VERSION(ucFirstData) \
            (((ucFirstData) >> 4) & (RNIC_IP_HEAD_VER_OFFSET_MASK))


#define RNIC_SET_FLOW_CTRL_STATUS(status, index)    (g_stRnicCtx.astSpecCtx[index].enFlowCtrlStatus = (status))
#define RNIC_GET_FLOW_CTRL_STATUS(index)            (g_stRnicCtx.astSpecCtx[index].enFlowCtrlStatus)

/* 获取指定网卡的上下文地址 */
#define RNIC_GET_SPEC_NET_CTX(index)                (&(g_stRnicCtx.astSpecCtx[index]))

/* 获取指定网卡的PDP上下文地址 */
#define RNIC_GET_SPEC_NET_PDP_CTX(index)            (&(g_stRnicCtx.astSpecCtx[index].stPdpCtx))

/* 获取IPV4类型PDP激活的RABID */
#define RNIC_GET_SPEC_NET_IPV4_RABID(index)         (RNIC_GET_SPEC_NET_PDP_CTX(index)->stIpv4PdpInfo.ucRabId)

/* 获取IPV6类型PDP激活的RABID */
#define RNIC_GET_SPEC_NET_IPV6_RABID(index)         (RNIC_GET_SPEC_NET_PDP_CTX(index)->stIpv6PdpInfo.ucRabId)

/* 获取IPV4类型PDP激活的RABID */
#define RNIC_GET_SPEC_NET_IPV4_REG_STATE(index)     (RNIC_GET_SPEC_NET_PDP_CTX(index)->stIpv4PdpInfo.enRegStatus)

/* 获取IPV6类型网卡注册状态 */
#define RNIC_GET_SPEC_NET_IPV6_REG_STATE(index)     (RNIC_GET_SPEC_NET_PDP_CTX(index)->stIpv6PdpInfo.enRegStatus)

/* 获取当前的拨号模式 */
#define RNIC_GET_DIAL_MODE()                        (g_stRnicCtx.stDialMode.enDialMode)

/* 获取上行上下文地址 */
#define RNIC_GET_UL_CTX_ADDR(index)                 (&g_stRnicCtx.astSpecCtx[index].stUlCtx)

/* 获取下行上下文地址 */
#define RNIC_GET_DL_CTX_ADDR(index)                 (&g_stRnicCtx.astSpecCtx[index].stDlCtx)



/* 获取指定Modem的RABID信息 */
#define RNIC_GET_SPEC_MODEM_RABID_INFO(index)       (&g_stRnicCtx.astRabIdInfo[index])

/* 获取网卡私有数据地址 */
#define RNIC_GET_SPEC_NET_PRIV_PTR(index)           (g_stRnicCtx.astSpecCtx[index].pstPriv)

/* 获取网卡私有数据地址 */
#define RNIC_GET_SPEC_NET_DEV_NAME(index)           (g_stRnicCtx.astSpecCtx[index].pstPriv->pstDev->name)

/* 获取网卡ID对应的ModemId */
#define RNIC_GET_MODEM_ID_BY_NET_ID(index)          (g_astRnicManageTbl[index].enModemId)

/* 获取网卡ID对应的源MAC地址 */
#define RNIC_GET_DST_MAC_ADDR(index)                (g_astRnicManageTbl[index].stIpv4Ethhead.aucEtherDhost)

/* 获取网卡ID对应的MAC头 */
#define RNIC_GET_ETH_HDR_ADDR(index)                (&(g_astRnicManageTbl[index].stIpv4Ethhead))

/* 获取当前的IPF模式 */
#define RNIC_GET_IPF_MODE()                         (g_stRnicCtx.ucIpfMode)

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
/* 获取RNIC网卡下行数据到Linux网络协议栈的接口模式 */
#define RNIC_GET_Net_API()                                 (g_stRnicCtx.stRnicNetIfCfg.ucNetInterfaceMode)
/* 获取网卡NAPI weight值调整模式 */
#define RNIC_GET_NAPI_WEIGHT_ADJ_MODE()                    (g_stRnicCtx.stRnicNetIfCfg.enNapiWeightAdjMode)
/* 获取网卡NAPI weight值 */
#define RNIC_GET_NAPI_WEIGHT()                             (g_stRnicCtx.stRnicNetIfCfg.ucNapiPollWeight)
/* 获取网卡NAPI Poll队列长度最大值 */
#define RNIC_GET_NAPI_POLL_QUE_MAX_LEN()                   (g_stRnicCtx.stRnicNetIfCfg.usNapiPollQueMaxLen)
/* 获取NAPI poll buffer队列地址 */
#define RNIC_GET_PollBuff_QUE(index)                       (&(g_stRnicCtx.astSpecCtx[index].stPollBuffQue))
/* 获取网卡NAPI下行每秒收包个数档位值1 */
#define RNIC_GET_DL_PKT_NUM_PER_SEC_LEVEL1()               (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stDlPktNumPerSecLevel.ulDlPktNumPerSecLevel1)
/* 获取网卡NAPI下行每秒收包个数档位值2 */
#define RNIC_GET_DL_PKT_NUM_PER_SEC_LEVEL2()               (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stDlPktNumPerSecLevel.ulDlPktNumPerSecLevel2)
/* 获取网卡NAPI下行每秒收包个数档位值3 */
#define RNIC_GET_DL_PKT_NUM_PER_SEC_LEVEL3()               (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stDlPktNumPerSecLevel.ulDlPktNumPerSecLevel3)
/* 获取网卡NAPI下行每秒收包个数档位值4 */
#define RNIC_GET_DL_PKT_NUM_PER_SEC_LEVEL4()               (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stDlPktNumPerSecLevel.ulDlPktNumPerSecLevel4)

/* 获取网卡NAPI weight档位值1 */
#define RNIC_GET_NAPI_WEIGHT_LEVEL1()                      (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stNapiWeightLevel.ucNapiWeightLevel1)
/* 获取网卡NAPI weight档位值2 */
#define RNIC_GET_NAPI_WEIGHT_LEVEL2()                      (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stNapiWeightLevel.ucNapiWeightLevel2)
/* 获取网卡NAPI weight档位值3 */
#define RNIC_GET_NAPI_WEIGHT_LEVEL3()                      (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stNapiWeightLevel.ucNapiWeightLevel3)
/* 获取网卡NAPI weight档位值4 */
#define RNIC_GET_NAPI_WEIGHT_LEVEL4()                      (g_stRnicCtx.stRnicNetIfCfg.stNapiWeightDynamicAdjCfg.stNapiWeightLevel.ucNapiWeightLevel4)

/* 获取下行NAPI接口收到的报文数 */
#define RNIC_GET_NAPI_RECV_PKT_NUM(index)                  (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiRecvPktNum)
/* 获取下行NAPI netif_receive_skb接口收到的报文数 */
#define RNIC_GET_NAPI_NETIF_RCV_PKT_NUM(index)             (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiNetifRcvPktNum)
/* 获取下行NAPI napi_gro_receive接口收到的报文数 */
#define RNIC_GET_NAPI_GRO_RCV_PKT_NUM(index)               (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiGroRcvPktNum)
/* 获取网卡NAPI Poll队列主动丢弃的报文数 */
#define RNIC_GET_NAPI_POLL_QUE_DISCARD_PKT_NUM(index)      (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiPollQueDiscardPktNum)
/* 累加下行NAPI接口收到的报文数 */
#define RNIC_ADD_NAPI_RECV_PKT_NUM(pktNum, index)          (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiRecvPktNum += (pktNum))
/* 累加下行NAPI netif_receive_skb接口收到的报文数 */
#define RNIC_ADD_NAPI_NETIF_RCV_PKT_NUM(pktNum, index)     (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiNetifRcvPktNum += (pktNum))
/* 累加下行NAPI napi_gro_receive接口收到的报文数 */
#define RNIC_ADD_NAPI_GRO_RCV_PKT_NUM(pktNum, index)       (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiGroRcvPktNum += (pktNum))
/* 累加下行NAPI Poll队列主动丢弃的报文数 */
#define RNIC_ADD_NAPI_POLL_QUE_DISCARD_PKT_NUM(pktNum, index)     (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiPollQueDiscardPktNum += (pktNum))
/* 下行NAPI接口收到的报文数清零 */
#define RNIC_RESET_NAPI_RECV_PKT_NUM(index)                       (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiRecvPktNum = 0)
/* 下行NAPI Poll队列主动丢弃的报文数清零 */
#define RNIC_RESET_NAPI_POLL_QUE_DISCARD_PKT_NUM(index)           (g_stRnicCtx.astSpecCtx[index].stNapiStats.ulDlNapiPollQueDiscardPktNum = 0)
/* 获取NAPI收包接口 */
#define RNIC_GET_NAPI_RCV_IF(index)                               (g_stRnicCtx.astSpecCtx[index].enNapiRcvIf)
#endif


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


enum RNIC_NETCARD_STATUS_TYPE_ENUM
{
    RNIC_NETCARD_STATUS_CLOSED,                                                 /* RNIC为关闭状态 */
    RNIC_NETCARD_STATUS_OPENED,                                                 /* RNIC为打开状态 */
    RNIC_NETCARD_STATUS_BUTT
};
typedef VOS_UINT8 RNIC_NETCARD_STATUS_ENUM_UINT8;


enum RNIC_PDP_REG_STATUS_ENUM
{
    RNIC_PDP_REG_STATUS_DEACTIVE,                                               /* 未注册上 */
    RNIC_PDP_REG_STATUS_ACTIVE,                                                 /* 已注册上 */
    RNIC_PDP_REG_STATUS_BUTT
};
typedef VOS_UINT32 RNIC_PDP_REG_STATUS_ENUM_UINT32;


enum RNIC_FLOW_CTRL_STATUS_ENUM
{
    RNIC_FLOW_CTRL_STATUS_STOP          = 0x00,                                 /* 流控停止 */
    RNIC_FLOW_CTRL_STATUS_START         = 0x01,                                 /* 流控启动 */
    RNIC_FLOW_CTRL_STATUS_BUTT          = 0xFF
};
typedef VOS_UINT32 RNIC_FLOW_CTRL_STATUS_ENUM_UINT32;


enum RNIC_IPF_MODE_ENUM
{
    RNIC_IPF_MODE_INT  = 0x00,                                                   /* 中断上下文 */
    RNIC_IPF_MODE_THRD = 0x01,                                                   /* 线程上下文 */
    RNIC_IPF_MODE_BUTT
};
typedef VOS_UINT8 RNIC_IPF_MODE_ENUM_UINT8;

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)

enum RNIC_NET_IF_ENUM
{
    RNIC_NET_IF_NETRX  = 0x00,                                                   /* Net_rx */
    RNIC_NET_IF_NAPI   = 0x01,                                                   /* Napi */
    RNIC_NET_IF_BUTT
};
typedef VOS_UINT8 RNIC_NET_IF_ENUM_UINT8;


enum RNIC_NAPI_RCV_IF_ENUM
{
    RNIC_NAPI_GRO_RCV_IF                = 0x00,                                 /* USB tethering未连接 */
    RNIC_NAPI_NETIF_RCV_IF              = 0x01,                                 /* USB tethering已连接 */
    RNIC_NAPI_RCV_BUTT
};
typedef VOS_UINT8 RNIC_NAPI_RCV_IF_ENUM_UINT8;
#endif


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


typedef struct
{
    VOS_UINT8                           aucEtherDhost[RNIC_MAC_ADDR_LEN];       /* destination ethernet address */
    VOS_UINT8                           aucEtherShost[RNIC_MAC_ADDR_LEN];       /* source ethernet address */
    VOS_UINT16                          usEtherType;                            /* packet type ID */
    VOS_UINT8                           aucReserved[2];
}RNIC_ETH_HEADER_STRU;


typedef struct
{
    const VOS_CHAR                     *pucRnicNetCardName;                     /* 网卡名称 */
    RNIC_ETH_HEADER_STRU                stIpv4Ethhead;                          /* IPV4以太网头 */
    RNIC_ETH_HEADER_STRU                stIpv6Ethhead;                          /* IPV6以太网头 */
    MODEM_ID_ENUM_UINT16                enModemId;                              /* Modem Id */
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;                              /* 网卡ID */
    VOS_UINT8                           aucReserved[5];
}RNIC_NETCARD_ELEMENT_TAB_STRU;


typedef struct
{
    struct net_device                  *pstDev;                                 /* 用于记录Linux内核分配的网卡虚地址 */
    struct net_device_stats             stStats;                                /* Linxu内核标准的网卡统计信息结构 */
    RNIC_NETCARD_STATUS_ENUM_UINT8      enStatus;                               /* 网卡是否打开标志 */
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;                              /* 设备对应的网卡ID */
    VOS_UINT8                           aucRsv[6];                              /* 保留 */
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    struct napi_struct                  stNapi;                                 /* Linxu内核标准的NAPI结构 */
#endif
}RNIC_NETCARD_DEV_INFO_STRU;


typedef struct
{
    VOS_UINT32                          OP_EnableDestAddr      : 1;             /* 使能目的IP地址作为过滤条件 */
    VOS_UINT32                          OP_EnableDestMask      : 1;             /* 使能目的网络掩码作为过滤条件 */
    VOS_UINT32                          OP_EnableDestPortLow   : 1;             /* 使能目的端口号下限作为过滤条件 */
    VOS_UINT32                          OP_EnableDestPortHigh  : 1;             /* 使能目的端口号上限作为过滤条件 */
    VOS_UINT32                          OP_EnableSrcPortLow    : 1;             /* 使能源端口号下限作为过滤条件 */
    VOS_UINT32                          OP_EnableSrcPortHigh   : 1;             /* 使能源端口号上限作为过滤条件 */
    VOS_UINT32                          OP_EnableReserved      : 26;            /* 保留 */
    VOS_UINT32                          ulDestAddr;                             /* 目的IP地址 */
    VOS_UINT32                          ulDestMask;                             /* 目的网络掩码 */
    VOS_UINT16                          usDestPortLow;                          /* 目的端口号下限 */
    VOS_UINT16                          usDestPortHigh;                         /* 目的端口号上限 */
    VOS_UINT16                          usSrcPortLow;                           /* 源端口号下限 */
    VOS_UINT16                          usSrcPortHigh;                          /* 源端口号上限 */
}RNIC_FILTER_INFO_STRU;


typedef struct RNIC_FTI
{
    VOS_INT32                           ulFilterId;                             /* 发送过滤器的ID */
    RNIC_FILTER_INFO_STRU               stFilter;                               /* 发送过滤器的信息 */
    struct RNIC_FTI                    *pNextItem;                              /* 指向下一个列表项 */
}RNIC_FILTER_LST_STRU;


typedef struct
{
    RNIC_PDP_REG_STATUS_ENUM_UINT32     enRegStatus;                            /* 标识该PDP上下文是否注册 */

    VOS_UINT8                           ucRabId;                                /* 承载标识 */
    VOS_UINT8                           aucReserved[3];
    VOS_UINT32                          ulIpv4Addr;                             /* IP地址 */
}RNIC_IPV4_PDP_INFO_STRU;


typedef struct
{
    RNIC_PDP_REG_STATUS_ENUM_UINT32     enRegStatus;                            /* 标识该PDP上下文是否注册 */

    /* Modified by l60609 for L-C互操作项目, 2014-01-06, Begin */
    VOS_UINT8                           ucRabId;                                /* 承载标识 */
    VOS_UINT8                           aucReserved[3];
    /* Modified by l60609 for L-C互操作项目, 2014-01-06, End */

    VOS_UINT8                           aucIpv6Addr[RNIC_MAX_IPV6_ADDR_LEN];    /* 从AT带来的IPV6地址长度，不包括":" */
}RNIC_IPV6_PDP_INFO_STRU;


typedef struct
{
    RNIC_PDP_REG_STATUS_ENUM_UINT32     enRegStatus;                            /* 标识该PDP上下文是否注册 */

    /* Modified by l60609 for L-C互操作项目, 2014-01-06, Begin */
    VOS_UINT8                           ucRabId;                                /* 承载标识 */
    VOS_UINT8                           aucReserved[3];
    /* Modified by l60609 for L-C互操作项目, 2014-01-06, End */

    VOS_UINT32                          ulIpv4Addr;                             /* IP地址 */
    VOS_UINT8                           aucIpv6Addr[RNIC_MAX_IPV6_ADDR_LEN];    /* 从AT带来的IPV6地址长度，不包括":" */
}RNIC_IPV4V6_PDP_INFO_STRU;


typedef struct
{
    RNIC_IPV4_PDP_INFO_STRU             stIpv4PdpInfo;                          /* IPV4的PDP信息 */
    RNIC_IPV6_PDP_INFO_STRU             stIpv6PdpInfo;                          /* IPV6的PDP信息 */
    RNIC_IPV4V6_PDP_INFO_STRU           stIpv4v6PdpInfo;                        /* IPV4V6的PDP信息 */
    IMM_ZC_HEAD_STRU                    stImsQue;
    /* 为后续扩展，以下2个公共域保留 */
    RNIC_FILTER_LST_STRU               *pstFilterList;                          /* 发送过滤器列表 */
    VOS_UINT8                          *pucFragmBuf;                            /* 用于IP分片的缓存 */
} RNIC_PDP_CTX_STRU;


/*****************************************************************************
 结构名称  : RNIC_DSFLOW_STATS_STRU
 结构说明  : 流量统计结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulCurrentRecvRate;                      /* 当前下行速率，保存PDP激活后2秒的速率，去激活后清空 */
    VOS_UINT32                          ulPeriodRecvPktNum;                     /* 下行收到数据包个数,统计一个拨号断开定时器周期内收到的个数，超时后清空 */
    VOS_UINT32                          ulTotalRecvFluxLow;                     /* 累计接收流量低四字节 */
    VOS_UINT32                          ulTotalRecvFluxHigh;                    /* 累计接收流量高四字节 */

    VOS_UINT32                          ulCurrentSendRate;                      /* 当前上行速率，保存PDP激活后2秒的速率，去激活后清空 */
    VOS_UINT32                          ulPeriodSendPktNum;                     /* 上行发送有效个数,统计一个拨号断开定时器周期内收到的个数，超时后清空 */
    VOS_UINT32                          ulTotalSendFluxLow;                     /* 累计发送流量低四字节 */
    VOS_UINT32                          ulTotalSendFluxHigh;                    /* 累计发送流量高四字节 */

} RNIC_DSFLOW_STATS_STRU;

/*****************************************************************************
 结构名称  : RNIC_NAPI_STATS_STRU
 结构说明  : NAPI GRO报文统计结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulDlNapiRecvPktNum;                     /* 统计周期内NAPI接口收到的报文数 */
    VOS_UINT32                          ulDlNapiNetifRcvPktNum;                 /* NAPI netif_receive_skb接口收到的报文数 */
    VOS_UINT32                          ulDlNapiGroRcvPktNum;                   /* NAPI napi_gro_receive接口收到的报文数 */
    VOS_UINT32                          ulDlNapiPollQueDiscardPktNum;           /* NAPI Poll Queue主动丢弃的报文数 */
} RNIC_NAPI_STATS_STRU;


typedef struct
{
    VOS_UINT8                                     ucNetInterfaceMode;           /* RNIC网卡下行数据到Linux网络协议栈的接口模式, 0: Net_rx(默认)，1：NAPI接口 */
    NAPI_WEIGHT_ADJ_MODE_ENUM_UINT8               enNapiWeightAdjMode;          /* NAPI Weight调整模式, 0: 静态模式，1：动态调整模式 */
    VOS_UINT8                                     ucNapiPollWeight;             /* RNIC网卡NAPI方式一次poll的最大报文数 */
    VOS_UINT8                                     aucReserved0;                 /* 保留位 */
    VOS_UINT16                                    usNapiPollQueMaxLen;          /* Napi poll队列最大支持长度 */
    VOS_UINT8                                     aucReserved[2];               /* 保留位 */
    RNIC_NAPI_WEIGHT_DYNAMIC_ADJ_CFG_STRU         stNapiWeightDynamicAdjCfg;    /* Napi Weight动态调整配置 */

} RNIC_NET_IF_CFG_STRU;


typedef struct
{
    VOS_UINT8                           aucRmnetName[RNIC_RMNET_NAME_MAX_LEN];  /* Rmnet网卡名 */
    AT_RNIC_USB_TETHER_CONN_ENUM_UINT8  enTetherConnStat;                       /* USB Tethering连接状态 */
    VOS_UINT8                           ucMatchRmnetNameFlg;                    /* 是否匹配上rmnet网卡名的标识 */
    VOS_UINT8                           aucRsv[2];
} RNIC_USB_TETHER_INFO_STRU;


typedef struct
{
    RNIC_PDP_CTX_STRU                   stPdpCtx;                               /* RNIC的PDP上下文 */
    RNIC_DSFLOW_STATS_STRU              stDsFlowStats;                          /* 流量信息 */
    RNIC_FLOW_CTRL_STATUS_ENUM_UINT32   enFlowCtrlStatus;                       /* 流控状态 */
    MODEM_ID_ENUM_UINT16                enModemId;                              /* 网卡归属哪个modem */
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;                              /* 设备对应的网卡ID */
    IMSA_RNIC_IMS_RAT_TYPE_ENUM_UINT8   enRatType;                              /* 注册域 */
    VOS_INT32                           lSpePort;                               /* SPE端口句柄 */
    VOS_UINT32                          ulIpfPortFlg;                           /* SPE端口IPF标识 */

    /* 保存系统分配的Netcard私有数据虚存地址 */
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv;                                /* 网卡设备信息 */

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    IMM_ZC_HEAD_STRU                    stPollBuffQue;                          /* Poll缓存队列 */
    RNIC_NAPI_STATS_STRU                stNapiStats;                            /* NAPI统计信息 */
    RNIC_NAPI_RCV_IF_ENUM_UINT8         enNapiRcvIf;
    VOS_UINT8                           aucRsv[7];
#endif

}RNIC_SPEC_CTX_STRU;


typedef struct
{
    /****** RNIC每个网卡专有的上下文 ******/
    RNIC_SPEC_CTX_STRU                  astSpecCtx[RNIC_NET_ID_MAX_NUM];        /* 特定网卡上下文信息 */

    /****** RNIC定时器上下文 ******/
    RNIC_TIMER_CTX_STRU                 astTimerCtx[RNIC_MAX_TIMER_NUM];

    /****** RNIC公共上下文 ******/
    RNIC_DIAL_MODE_STRU                 stDialMode;                             /* 拨号模式 */
    VOS_UINT32                          ulTiDialDownExpCount;                   /* 拨号断开定时器超时次数参数统计 */

    VOS_UINT32                          ulSetTimer4WakeFlg;                     /* 是否设置Timer4唤醒标志 */

    VOS_SEM                             hResetSem;                              /* 二进制信号量，用于复位处理  */

    VOS_UINT8                           ucIpfMode;                              /* IPF处理ADS下行数据的模式, 0: 中断上下文(默认)，1：线程上下文 */
    VOS_UINT8                           aucRsv[7];
    RNIC_USB_TETHER_INFO_STRU           stUsbTetherInfo;
#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
    RNIC_NET_IF_CFG_STRU                stRnicNetIfCfg;
#endif

}RNIC_CTX_STRU;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

extern RNIC_CTX_STRU                                g_stRnicCtx;
extern const RNIC_NETCARD_ELEMENT_TAB_STRU          g_astRnicManageTbl[];


/*****************************************************************************
  10 函数声明
*****************************************************************************/
VOS_VOID RNIC_InitCtx(
    RNIC_CTX_STRU                      *pstRnicCtx
);
VOS_VOID RNIC_InitPdpCtx(
    RNIC_PDP_CTX_STRU                  *pstPdpCtx,
    VOS_UINT8                           ucRmNetId
);
VOS_VOID RNIC_InitDialMode(
    RNIC_DIAL_MODE_STRU                *pstDialMode
);
VOS_VOID RNIC_InitIpv4PdpCtx(
    RNIC_IPV4_PDP_INFO_STRU            *pstIpv4PdpCtx
);
VOS_VOID RNIC_InitIpv6PdpCtx(
    RNIC_IPV6_PDP_INFO_STRU            *pstIpv6PdpCtx
);
VOS_VOID RNIC_InitIpv4v6PdpCtx(
    RNIC_IPV4V6_PDP_INFO_STRU          *pstIpv4v6PdpCtx,
    VOS_UINT8                           ucRmNetId
);
RNIC_CTX_STRU* RNIC_GetRnicCtxAddr(VOS_VOID);
VOS_UINT32 RNIC_GetTiDialDownExpCount( VOS_VOID);
VOS_VOID RNIC_IncTiDialDownExpCount( VOS_VOID);
VOS_VOID RNIC_ClearTiDialDownExpCount( VOS_VOID);

VOS_UINT32 RNIC_GetCurrentUlRate(VOS_UINT8 ucIndex);
VOS_VOID RNIC_SetCurrentUlRate(
    VOS_UINT32                          ulULDataRate,
    VOS_UINT8                           ucRmNetId
);
VOS_UINT32 RNIC_GetCurrentDlRate(VOS_UINT8 ucIndex);
VOS_VOID RNIC_SetCurrentDlRate(
    VOS_UINT32                          ulDLDataRate,
    VOS_UINT8                           ucIndex
);
RNIC_DIAL_MODE_STRU* RNIC_GetDialModeAddr(VOS_VOID);
RNIC_PDP_CTX_STRU* RNIC_GetPdpCtxAddr(VOS_UINT8 ucIndex);

RNIC_TIMER_CTX_STRU*  RNIC_GetTimerAddr( VOS_VOID );
VOS_VOID RNIC_SetTimer4WakeFlg(VOS_UINT32 ulFlg);
VOS_UINT32 RNIC_GetTimer4WakeFlg(VOS_VOID);


RNIC_SPEC_CTX_STRU *RNIC_GetSpecNetCardCtxAddr(VOS_UINT8 ucIndex);


VOS_VOID RNIC_ClearNetDsFlowStats(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId);
RNIC_SPEC_CTX_STRU* RNIC_GetNetCntxtByRmNetId(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId);
#if defined(CONFIG_BALONG_SPE)
RNIC_SPEC_CTX_STRU* RNIC_GetNetCntxtBySpePort(VOS_INT32 lPort);
#endif

VOS_VOID RNIC_ResetDialMode(
    RNIC_DIAL_MODE_STRU                *pstDialMode
);
VOS_VOID RNIC_InitResetSem(VOS_VOID);
VOS_SEM RNIC_GetResetSem(VOS_VOID);

VOS_VOID RNIC_InitIpfMode(
    RNIC_CTX_STRU                      *pstRnicCtx
);

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
VOS_VOID RNIC_InitRnicNetInterfaceCfg(
    RNIC_CTX_STRU                      *pstRnicCtx
);
VOS_VOID RNIC_CheckNetIfCfgValid(
    RNIC_CTX_STRU                      *pstRnicCtx
);
VOS_UINT32 RNIC_UpdateRmnetNapiRcvIfByName(VOS_INT32 ulRmNetId);
VOS_VOID RNIC_UpdateIpv6RmnetNapiRcvIfDefault(VOS_INT32 ulRmNetId);
#endif

VOS_VOID RNIC_InitUsbTetherInfo(VOS_VOID);

#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of RnicCtx.h */

