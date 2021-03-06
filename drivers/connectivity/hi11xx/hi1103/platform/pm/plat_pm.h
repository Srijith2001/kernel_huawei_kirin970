

#ifndef __PLAT_PM_H__
#define __PLAT_PM_H__

/*****************************************************************************
  1 Include other Head file
*****************************************************************************/
#include <linux/mutex.h>
#include <linux/kernel.h>
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
#include <linux/pm_wakeup.h>
#endif
#include "plat_pm_wlan.h"
#include "hw_bfg_ps.h"
#include "board.h"
/*****************************************************************************
  2 Define macro
*****************************************************************************/
/*#define ENABLE_BFG_LOWPOWER_FEATURE*/
#define BFG_LOCK_NAME                   "bfg_wake_lock"

#define FIRMWARE_CFG_INIT_OK            0x01

#define SUCCESS                         (0)
#define FAILURE                         (1)

#define GNSS_AGREE_SLEEP                (1)
#define GNSS_NOT_AGREE_SLEEP            (0)

#define BFGX_SLEEP                      (0)
#define BFGX_ACTIVE                     (1)

#define HOST_DISALLOW_TO_SLEEP          (0)
#define HOST_ALLOW_TO_SLEEP             (1)

#define BFGX_PM_ENABLE                  (1)
#define BFGX_PM_DISABLE                 (0)

#define WAIT_DEVACK_MSEC               (10)
#define WAIT_DEVACK_CNT               (10)
#define WAIT_DEVACK_TIMEOUT_MSEC       (200)

/*超时时间要大于wkup dev work中的最长执行时间，否则超时以后进入DFR和work中会同时操作tty，导致冲突*/
#define WAIT_WKUPDEV_MSEC              (10000)

enum UART_STATE_ENUM
{
    UART_NOT_READY              = 0,
    UART_READY                  = 1,
    UART_BPS_CHG_SEND_ACK       = 2,
    UART_BPS_CHG_IN_PROGRESS    = 3,
    UART_BPS_CHG_SEND_COMPLETE  = 4,
};


/* BFGX系统上电加载异常类型 */
enum BFGX_POWER_ON_EXCEPTION_ENUM
{
    BFGX_POWER_FAILED                               = -1,
    BFGX_POWER_SUCCESS                              = 0,

    BFGX_POWER_PULL_POWER_GPIO_FAIL                 = 1,
    BFGX_POWER_TTY_OPEN_FAIL                        = 2,
    BFGX_POWER_TTY_FLOW_ENABLE_FAIL                 = 3,

    BFGX_POWER_WIFI_DERESET_BCPU_FAIL               = 4,
    BFGX_POWER_WIFI_ON_BOOT_UP_FAIL                 = 5,

    BFGX_POWER_WIFI_OFF_BOOT_UP_FAIL                = 6,
    BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL               = 7,

    BFGX_POWER_WAKEUP_FAIL                          = 8,
    BFGX_POWER_OPEN_CMD_FAIL                        = 9,

    BFGX_POWER_ENUM_BUTT,
};

/* wifi系统上电加载异常类型 */
enum WIFI_POWER_ON_EXCEPTION_ENUM
{
    WIFI_POWER_FAIL                                 = -1,
    WIFI_POWER_SUCCESS                              = 0,
    WIFI_POWER_PULL_POWER_GPIO_FAIL                 = 1,

    WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL                = 2,
    WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL      = 3,

    WIFI_POWER_BFGX_ON_BOOT_UP_FAIL                 = 4,
    WIFI_POWER_BFGX_DERESET_WCPU_FAIL               = 5,
    WIFI_POWER_BFGX_ON_FIRMWARE_DOWNLOAD_FAIL       = 6,

    WIFI_POWER_ENUM_BUTT,
};

/*****************************************************************************
  3 STRUCT DEFINE
*****************************************************************************/

/*private data for pm driver*/
struct pm_drv_data
{
    /*3 in 1 interface pointer*/
    struct ps_pm_s                  *ps_pm_interface;

    /*wlan interface pointer*/
    struct wlan_pm_s                *pst_wlan_pm_info;

    /* board customize info */
    BOARD_INFO                      *board;
    /*wake lock for bfg,be used to prevent host form suspend*/
    oal_wakelock_stru                bfg_wake_lock;

    /*mutex for sync*/
    struct mutex                    host_mutex;

    /*wakelock protect spinlock while wkup isr VS allow sleep ack and devack_timer*/
    spinlock_t                      wakelock_protect_spinlock;

    /*bfgx VS. bfg timer spinlock */
    spinlock_t                      uart_state_spinlock;

    /* uart could be used or not */
    uint8   uart_state;

    /* mark receiving data after set dev as sleep state but before get ack of device */
    uint8 rcvdata_bef_devack_flag;

    uint8 uc_dev_ack_wait_cnt;

    /*bfgx sleep state*/
    uint8 bfgx_dev_state;

    /*flag for firmware cfg file init*/
    uint64 firmware_cfg_init_flag;

    spinlock_t bfg_irq_spinlock;

    /*bfg irq num*/
    uint32 bfg_irq;

    uint32 ul_irq_stat;

    /*bfg wakeup host count*/
    uint32 bfg_wakeup_host;

    /* gnss lowpower state */
    atomic_t gnss_sleep_flag;

    atomic_t bfg_needwait_devboot_flag;

    /* flag to mark whether enable lowpower or not */
    uint32 bfgx_pm_ctrl_enable;
    uint8  bfgx_lowpower_enable;
    uint8  bfgx_bt_lowpower_enable;
    uint8  bfgx_gnss_lowpower_enable;
    uint8  bfgx_nfc_lowpower_enable;

    /* workqueue for wkup device */
    struct workqueue_struct *wkup_dev_workqueue;
    struct work_struct wkup_dev_work;
    struct work_struct send_disallow_msg_work;
    struct work_struct send_allow_sleep_work;
    struct work_struct baud_change_work;
    uint32 uart_baud_switch_to;

    /* wait device ack timer */
    struct timer_list bfg_timer;
    struct timer_list dev_ack_timer;
    struct timer_list baud_change_timer;

    /* the completion for waiting for host wake up device ok */
    struct completion host_wkup_dev_comp;
    /* the completion for waiting for dev ack(host allow sleep) ok */
    struct completion dev_ack_comp;
    /* the completion for waiting for dev boot ok */
    struct completion dev_bootok_ack_comp;
};

/*****************************************************************************
  4 EXTERN VARIABLE
*****************************************************************************/

/*****************************************************************************
  5 EXTERN FUNCTION
*****************************************************************************/
extern struct pm_drv_data * pm_get_drvdata(void);
extern int32 host_wkup_dev(void);
extern struct pm_drv_data * pm_get_drvdata(void);
extern int32 bfgx_other_subsys_all_shutdown(uint8 subsys);
extern void bfgx_print_subsys_state(void);
extern int32 wlan_is_shutdown(void);
extern int32 bfgx_is_shutdown(void);
extern int32 wlan_power_on(void);
extern int32 wlan_power_off(void);
extern int32 bfgx_power_on(uint8 subsys);
extern int32 bfgx_power_off(uint8 subsys);
extern int32 bfgx_pm_feature_set(void);
extern int firmware_download_function(uint32 which_cfg);
extern oal_int32 hi110x_get_wifi_power_stat(oal_void);
extern int32 device_mem_check(unsigned long long *time);
extern int32 bfgx_uart_rcv_baud_change_req(uint8 uc_msg_type);
extern int32 bfgx_uart_rcv_baud_change_complete_ack(void);
extern int32 memcheck_is_working(void);
#endif

