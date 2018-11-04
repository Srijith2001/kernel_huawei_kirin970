#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <huawei_platform/power/wireless_charger.h>
#include <huawei_platform/power/wired_channel_switch.h>
#include "idtp9221.h"

#define HWLOG_TAG wireless_idtp9221
HWLOG_REGIST();

static struct idtp9221_device_info *g_idtp9221_di;
static struct wake_lock g_idtp9221_wakelock;
static int stop_charging_flag;
static int irq_abnormal_flag = false;
static const u8 idtp9221_send_msg_len[IDT9221_RX_TO_TX_DATA_LEN+2] = {0,0x18,0x28,0x38,0x48,0x58};
extern unsigned int runmode_is_factory(void);

/**********************************************************
*  Function:       idtp9221_read_block
*  Discription:    register read block interface
*  Parameters:   reg:register addr
*                      data:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_read_block(struct idtp9221_device_info *di, u16 reg, u8 *data, u8 len)
{
	struct i2c_msg msg[2];
	int ret = 0;
	int i = 0;

	u8 reg_be[IDT9221_ADDR_LEN];
	reg_be[0] = reg >> BITS_PER_BYTE;
	reg_be[1] = reg & BYTE_MASK;

	if (!di->client->adapter)
		return -ENODEV;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = reg_be;
	msg[0].len = IDT9221_ADDR_LEN;

	msg[1].addr = di->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = len;

	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(di->client->adapter, msg, 2);
		if(ret >= 0)
			break;
		msleep(I2C_OPS_SLEEP_TIME);
	}

	if (ret < 0) {
		hwlog_err("idt9221 read block fail, start_reg = 0x%x\n", reg);
		return -1;
	}

	return 0;
}
/**********************************************************
*  Function:       idtp9221_wite_block
*  Discription:    register wite block interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_write_block(struct idtp9221_device_info *di, u16 reg, u8 *buff, u8 len)
{
	struct i2c_msg msg;
	int ret;
	int i = 0;

	if (!di->client->adapter)
		return -ENODEV;

	buff[0] = reg >> BITS_PER_BYTE;
	buff[1] = reg & BYTE_MASK;

	msg.buf = buff;
	msg.addr = di->client->addr;
	msg.flags = 0;
	msg.len = len + IDT9221_ADDR_LEN;
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(di->client->adapter, &msg, 1);
		if (ret >= 0)
			break;
		msleep(I2C_OPS_SLEEP_TIME);
	}

	if (1 != ret) {
		hwlog_err("idt9221 write block fail, start_reg = 0x%x\n", reg);
		return -1;
	}

	return 0;
}
/**********************************************************
*  Function:       idtp9221_read_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register addr
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_read_byte(u16 reg, u8* data)
{
	struct idtp9221_device_info *di = g_idtp9221_di;

	return idtp9221_read_block(di, reg, data, BYTE_LEN);
}

/**********************************************************
*  Function:       idtp9221_read_word
*  Discription:    register write byte interface
*  Parameters:   reg:register addr
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_read_word(u16 reg, u16* data)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	u8 buff[WORD_LEN] = { 0 };
	int ret = 0;

	ret = idtp9221_read_block(di, reg, buff, WORD_LEN);
	if (ret)
		return -1;

	*data = buff[0] | buff[1] << BITS_PER_BYTE;
	return 0;
}
/**********************************************************
*  Function:       idtp9221_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_write_byte(u16 reg, u8 data)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	u8 buff[IDT9221_ADDR_LEN+BYTE_LEN];
	buff[IDT9221_ADDR_LEN] = data;

	return idtp9221_write_block(di, reg, buff, BYTE_LEN);
}
/**********************************************************
*  Function:       idtp9221_write_word
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_write_word(u16 reg, u16 data)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	u8 buff[IDT9221_ADDR_LEN+WORD_LEN];
	buff[IDT9221_ADDR_LEN] = data  & BYTE_MASK;
	buff[IDT9221_ADDR_LEN+1] = data >> BITS_PER_BYTE;

	return idtp9221_write_block(di, reg, buff, 2);
}
static void idtp9221_wake_lock(void)
{
	if (!wake_lock_active(&g_idtp9221_wakelock)) {
		wake_lock(&g_idtp9221_wakelock);
		hwlog_info("idtp9221 wake lock\n");
	}
}
static void idtp9221_wake_unlock(void)
{
	if (wake_lock_active(&g_idtp9221_wakelock)) {
		wake_unlock(&g_idtp9221_wakelock);
		hwlog_info("idtp9221 wake unlock\n");
	}
}

static int idtp9221_clear_interrupt(u16 itr)
{
	int ret;
	u8 itrs[IDT9221_ADDR_LEN + IDT9221_RX_INT_CLEAR_LEN];
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}

	itrs[IDT9221_ADDR_LEN] = itrs[IDT9221_ADDR_LEN]  | (itr & BYTE_MASK);	//clear interrupt set 1
	itrs[IDT9221_ADDR_LEN+1] = itrs[IDT9221_ADDR_LEN+1]  | (itr >> BITS_PER_BYTE);

	ret = idtp9221_write_block(di, IDT9221_RX_INT_CLEAR_ADDR,
						itrs, IDT9221_RX_INT_CLEAR_LEN);
	if(ret) {
		hwlog_err("%s:write interrupt  clear register failed!\n",__func__);
		return -1;
	}
	ret = idtp9221_write_byte(IDT9221_CMD_ADDR, IDT9221_CMD_CLEAR_INTERRUPT);
	if(ret) {
		hwlog_err("%s:write interrupt  clear cmd failed!\n",__func__);
		return -1;
	}
	return 0;

}
static int idtp9221_set_interrupt(u16 itr)
{
	int ret;
	u8 itrs[IDT9221_ADDR_LEN + IDT9221_RX_INT_CLEAR_LEN];
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}

	ret = idtp9221_read_block(di, IDT9221_RX_INT_ENABLE_ADDR,
			itrs+IDT9221_RX_INT_ENABLE_LEN, IDT9221_RX_INT_ENABLE_LEN);
	if(ret) {
		hwlog_err("%s:read interrupt enable register failed!\n", __func__);
		return -1;
	}

	itrs[IDT9221_ADDR_LEN] = itrs[IDT9221_ADDR_LEN] |(itr & BYTE_MASK);
	itrs[IDT9221_ADDR_LEN+1] = itrs[IDT9221_ADDR_LEN+1] |(itr >> BITS_PER_BYTE);


	ret = idtp9221_write_block(di, IDT9221_RX_INT_ENABLE_ADDR, itrs,
			IDT9221_RX_INT_ENABLE_LEN);
	if(ret) {
		hwlog_err("%s:write interrupt  enable register failed!\n", __func__);
		return -1;
	}
	hwlog_info("%s:set interrupt success!\n", __func__);
	return 0;
}
static int idtp9221_send_msg(u8 cmd, u8 *data, int data_len)
{
	int ret;
	u8 msg_len;
	u8 write_data[IDT9221_RX_TO_TX_DATA_LEN + IDT9221_ADDR_LEN] = {0};//transfer into I2C client,must offset 2 for i2c address
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}

	if(data_len > IDT9221_RX_TO_TX_DATA_LEN || data_len < 0) {
		hwlog_err("%s:send data out of range!\n", __func__);
		return -1;
	}

	di->irq_val &= ~IDT9221_RX_STATUS_TX2RX_ACK;
	msg_len = idtp9221_send_msg_len[data_len + 1];//msg_len=cmd_len+data_len  cmd_len=1

	ret =idtp9221_write_byte(IDT9221_RX_TO_TX_HEADER_ADDR, msg_len);
	if(ret) {
		hwlog_err("%s:write header failed!\n", __func__);
		return -1;
	}
	ret = idtp9221_write_byte(IDT9221_RX_TO_TX_CMD_ADDR, cmd);
	if(ret) {
		hwlog_err("%s:write cmd failed!\n", __func__);
		return -1;
	}

	if(NULL != data && data_len > 0) {
		memcpy(write_data+IDT9221_ADDR_LEN, data, data_len);
		ret = idtp9221_write_block(di, IDT9221_RX_TO_TX_DATA_ADDR, write_data, data_len);
		if(ret) {
			hwlog_err("%s:write data into RX to TX register failed!\n", __func__);
			return -1;
		}
	}

	ret = idtp9221_write_byte(IDT9221_CMD_ADDR, IDT9221_CMD_SEND_RX_DATA);
	if(ret) {
		hwlog_err("%s:send msg to tx failed!\n", __func__);
		return -1;
	}
	hwlog_info("%s:send msg success!\n", __func__);
	return 0;
}

static int idtp9221_send_msg_ack(u8 cmd, u8 *data, int data_len)
{
	int count = 0;
	int ack_cnt = 0;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}

	do {
		idtp9221_send_msg(cmd, data, data_len);
		for(ack_cnt = 0; ack_cnt < IDT9221_WAIT_FOR_ACK_RETRY_CNT; ack_cnt++) {
			msleep(IDT9221_WAIT_FOR_ACK_SLEEP_TIME);
			if(IDT9221_RX_STATUS_TX2RX_ACK & di->irq_val) {
				di->irq_val &= ~IDT9221_RX_STATUS_TX2RX_ACK;
				hwlog_info("[%s] succ, retry times = %d!\n", __func__,count);
				return 0;
			}
			if (stop_charging_flag) {
				hwlog_err("%s: already stop charging, return\n", __func__);
				return -1;
			}
		}
		count++;
		hwlog_info("[%s] retry\n", __func__);
	}while(count < IDT9221_SNED_MSG_RETRY_CNT);

	if(IDT9221_SNED_MSG_RETRY_CNT == count) {
		hwlog_err("[%s] fail, retry times = %d\n", __func__, count);
		return -1;
	}
	return 0;
}
static int idtp9221_receive_msg(u8 *data, int data_len)
{
	int ret;
	int count = 0;
	u8 buff[IDT9221_TX_TO_RX_DATA_LEN] = {0};
	struct idtp9221_device_info *di = g_idtp9221_di;

	if (NULL == di ||NULL == data) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}

	idtp9221_write_block(di, IDT9221_TX_TO_RX_DATA_ADDR, buff, IDT9221_TX_TO_RX_DATA_LEN);

	do {
		idtp9221_read_block(di, IDT9221_TX_TO_RX_DATA_ADDR, data, data_len);
		if(di->irq_val & IDT9221_RX_STATUS_TXDATA_RECEIVED) {
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			goto FuncEnd;
		}
		if (stop_charging_flag) {
			hwlog_err("%s already stop charging, return\n", __func__);
			return -1;
		}
		msleep(IDT9221_RCV_MSG_SLEEP_TIME);
		count++;
	} while (count < IDT9221_RCV_MSG_SLEEP_CNT);

FuncEnd:
	ret = idtp9221_read_block(di, IDT9221_TX_TO_RX_DATA_ADDR, data, data_len);
	if(ret) {
		hwlog_err("%s:get tx to rx data failed!\n", __func__);
		return -1;
	}
	if(!data[0]) {
		hwlog_err("%s: no msg received from tx\n", __func__);
		return -1;
	}
	hwlog_info("[%s] get tx to rx data, succ\n", __func__);
	return 0;
}
static void idtp9221_chip_enable(int enable)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	hwlog_info("%s %d ++\n", __func__, enable);
	if (di) {
		gpio_set_value(di->gpio_en, enable);
		hwlog_info("%s --\n", __func__);
	}
}
static void idtp9221_sleep_enable(int enable)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	hwlog_info("%s %d ++\n", __func__, enable);
	if (di && !irq_abnormal_flag) {
		gpio_set_value(di->gpio_sleep_en, enable);
		hwlog_info("%s --\n", __func__);
	}
}
static int idtp9221_send_ept(enum wireless_etp_type ept_type)
{
	int ret;
	u8 rx_ept_type = IDT9221_RX_EPT_UNKOWN;
	switch(ept_type) {
	case WIRELESS_EPT_ERR_VRECT:
		rx_ept_type = IDT9221_RX_ERR_VRECT;
		break;
	default:
		hwlog_err("%s: err ept_type(0x%x)", __func__, ept_type);
		return -1;
	}
	ret = idtp9221_write_byte(IDT9221_RX_EPT_ADDR, rx_ept_type);
	ret |= idtp9221_write_byte(IDT9221_CMD_ADDR, IDT9221_CMD_SEND_EPT);
	if (ret) {
		hwlog_err("%s: failed!\n", __func__);
		return -1;
	}
	return 0;
}
static int idtp9221_get_rx_vrect(void)
{
	int ret;
	u8 volt[IDT9221_RX_VRECT_LEN];
	u32 vol;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}
	ret = idtp9221_read_block(di,IDT9221_RX_GET_VRECT_ADDR,
								volt, IDT9221_RX_VRECT_LEN);
	if(ret) {
		hwlog_err("get idt9221 rx vrect failed!\n");
		return -1;
	}
	vol = volt[1];
	vol = (vol << BITS_PER_BYTE) + volt[0];
	vol = vol * IDT9221_RX_VRECT_VALUE_MAX /IDT9221_RX_VRECT_REG_MAX;

	return vol;
}

static int idtp9221_get_rx_vout(void)
{
	int ret;
	u8 volt[IDT9221_RX_VOUT_LEN];
	u32 vol;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}

	ret = idtp9221_read_block(di,IDT9221_RX_GET_VOUT_ADDR,
								volt, IDT9221_RX_VOUT_LEN);
	if(ret) {
		hwlog_err("get idt9221 rx vout failed!\n");
		return -1;
	}

	vol = volt[1];
	vol = (vol << BITS_PER_BYTE) + volt[0];
	vol = vol * IDT9221_RX_VOUT_VALUE_MAX /IDT9221_RX_VOUT_REG_MAX;

	return vol;
}
static int idtp9221_get_rx_iout(void)
{
	int ret;
	u8 curr[IDT9221_RX_IOUT_LEN];
	u32 cur;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}
	ret = idtp9221_read_block(di,IDT9221_RX_GET_IOUT_ADDR, curr, IDT9221_RX_IOUT_LEN);
	if(ret) {
		hwlog_err("get idt9221 rx iout  value failed!\n");
		return -1;
	}

	cur = curr[1];
	cur = (cur<<BITS_PER_BYTE) + curr[0];
	return cur;
}
static int idtp9221_set_rx_vout(int vol)
{
	int ret;

	if(vol < IDT9221_RX_VOUT_MIN || vol > IDT9221_RX_VOUT_MAX) {
		hwlog_err("%s set ldo out voltage value out of range!\n", __func__);
		return -1;
	}

	vol = vol /IDT9221_RX_VOUT_STEP -IDT9221_RX_VOUT_OFFSET/IDT9221_RX_VOUT_STEP;
	ret = idtp9221_write_byte(IDT9221_RX_SET_VOUT_ADDR, (u8)vol);
	if(ret) {
		hwlog_err("set idtp9221 rx vout failed!\n");
		return -1;
	}

	return 0;
}
static int idtp9221_set_tx_vout(int vset)
{
	int ret, i, vout;
	int cnt = IDT9221_SET_TX_VOUT_TIMEOUT/IDT9221_SET_TX_VOUT_SLEEP_TIME;

	u8 buff[IDT9221_ADDR_LEN + IDT9221_SET_TX_VOUT_LEN];

	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}

	buff[IDT9221_ADDR_LEN] = vset & BYTE_MASK;
	buff[IDT9221_ADDR_LEN+1] = vset >> BITS_PER_BYTE;

	ret = idtp9221_write_word(IDT9221_SET_TX_VOUT_ADDR, vset);
	if(ret) {
		hwlog_err("%s: set tx vout reg failed!\n", __func__);
		return -1;
	}
	ret = idtp9221_write_byte(IDT9221_CMD_ADDR, IDT9221_CMD_SEND_FAST_CHRG);
	if(ret) {
		hwlog_err("set tx vout cmd reg failed!\n");
		return -1;
	}
	for (i = 0; i < cnt; i++) {
		msleep(IDT9221_SET_TX_VOUT_SLEEP_TIME);
		vout = idtp9221_get_rx_vout();
		if (vout < 0) {
			hwlog_err("get tx vout invalid!\n");
			return -1;
		}
		if (vout >= vset - IDT9221_TX_VOUT_ERR_LTH &&
			vout <= vset + IDT9221_TX_VOUT_ERR_UTH) {
			return 0;
		}
		if (stop_charging_flag) {
			hwlog_err("%s already stop charging, return\n", __func__);
			return -1;
		}
	}
	return -1;
}
static int idtp9221_get_rx_fop(void)
{
	int ret;
	u8 val[IDT9221_RX_FOP_LEN] = {0};
	u32 fop;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}
	ret = idtp9221_read_block(di,IDT9221_RX_GET_FOP_ADDR, val, IDT9221_RX_FOP_LEN);
	if(ret) {
		hwlog_err("get idt9221 rx fop failed!\n");
		return -1;
	}
	fop = val[1];
	fop = (fop << BITS_PER_BYTE) + val[0];
	fop = IDT9221_RX_FOP_COEF /fop;

	return (int)fop;
}
static int idtp9221_get_tx_para(u8 cmd, u8 * receive_data, int receive_len)
{
	int ret;
	int count = 0;
	do{
		ret = idtp9221_send_msg(cmd, NULL, 0);
		ret |= idtp9221_receive_msg(receive_data, receive_len);
		if(!ret) {
			hwlog_info("[%s] get tx para, succ!\n", __func__);
			return 0;
		}
		count++;
		hwlog_info("%s: get tx para, fail! try next time!\n", __func__);
	}while(count < IDT9221_GET_TX_PARA_RETRY_CNT);

	hwlog_info("%s: get tx para, fail!\n", __func__);
	return -1;
}
 /**********************************************************
*  Function:       idtp9221 program_bootloader
*  Discription:    loading bootloade into p9221 MCU SRAM
*  Parameters:    NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_program_bootloader(void)
{
  	int i;
  	int ret;
  	int len;
	u8 data;
  	len = ARRAY_SIZE(idt_bootloader_data);

  	for (i = 0; i < len; i++) {
		ret = idtp9221_write_byte(IDT9221_BOOTLOADER_ADDR + i, idt_bootloader_data[i]);
		if (ret) {
			hwlog_err("loading bootloader failed!\n");
			return -1;
		}
		ret = idtp9221_read_byte(IDT9221_BOOTLOADER_ADDR + i, &data);
		if (ret ||data != idt_bootloader_data[i]) {
			hwlog_err("write and read is not same!\n");
			return -1;
		}
  	}

	hwlog_info("write bootloader ok, len = %d!\n", len);
	return 0;
}
static int idtp9221_program_otp_first_step(void)
{
  	/*  === Step-1 ===
     		- Transfer 9220 boot loader code "OTPBootloader" to 9220 SRAM
     		- Setup 9220 registers before transferring the boot loader code
     		- Transfer the boot loader code to 9221 SRAM
     		- Reset 9220 => 9220 M0 runs the boot loader
  	*/
	int ret;
  	// configure the system
	ret = idtp9221_write_byte(IDT9221_KEY_ADDR, IDT9221_KEY_VALUE); // write key
  	if(ret) {
		hwlog_info("write %d error!\n", IDT9221_KEY_ADDR);
		return -1;
  	}
	ret = idtp9221_write_byte(IDT9221_M0_ADDR, IDT9221_M0_EXE); // halt M0 execution
  	if(ret) {
		hwlog_info("write %d error!\n", IDT9221_M0_ADDR);
		return -1;
  	}

	ret = idtp9221_program_bootloader();  	//loading bootloade into p9221 MCU SRAM
	if(ret) {
		hwlog_info("write bootloader error!\n");
		return -1;
	}

	ret = idtp9221_write_byte(IDT9221_MAP_ADDR, IDT9221_MAP_RAM2OTP); // map RAM to OTP
  	if(ret) {
		hwlog_info("write %d error!\n", IDT9221_MAP_ADDR);
		return -1;
  	}
  	/* ignoreNAK */
	idtp9221_write_byte(IDT9221_M0_ADDR, IDT9221_M0_RESET);  // reset chip and run the bootloader.if noACK writing error

  	hwlog_info("program OTP step 1 OK!\n");
	return 0;
}
static int idtp9221_program_otp_second_step(char *srcData, int srcOffs, int size, u16 addr)
{
	/*	=== Step-2 ===
		-- Program OTP image data to 9220 OTP memory
	*/
	int i;
	u16 write_size = 0;
	u16 current_addr = addr;
	u16 offset = 0;
	u8 bufs[IDT9221_PAGE_SIZE + IDT9221_OTP_DATA_OFFSET];
	u16 nozero_len = 0;
	u16 check_sum = 0;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return -1;
	}

	while(size>0)
	{
		if(size >= IDT9221_PAGE_SIZE)
			write_size = IDT9221_PAGE_SIZE;
		else
			write_size = size;

		//(1) Copy the 128 bytes of the OTP image data to the packet data buffer
		memset(bufs, 0, IDT9221_PAGE_SIZE + IDT9221_OTP_DATA_OFFSET);		//138=2+8+128  2-byte register address for special I2C interface write consecutive bytes
		memcpy(bufs+IDT9221_OTP_DATA_OFFSET, srcData+offset+ srcOffs, write_size);	//copy the data from raw source

		check_sum = current_addr;	//check sum = sum of 128 bytes + StartAddr + expect last zero length

		//(2) Calculate the packet checksum of the 128-byte data,and nozero_len; update vars
		nozero_len = IDT9221_PAGE_SIZE;
		for (i = IDT9221_PAGE_SIZE -1; i >= 0; i --) {
			if (bufs[i + IDT9221_OTP_DATA_OFFSET] != 0)
        				break;
      			else
        				nozero_len --;
    		}
    		if (nozero_len == 0) {
    			offset+= write_size;
			size -= write_size;
			current_addr += write_size;
      			continue;
    		}
		for (; i >= 0; i--)
			check_sum += bufs[i + IDT9221_OTP_DATA_OFFSET];

		check_sum += nozero_len;

		//(3) Fill up StartAddr, CodeLength, CheckSum of the current packet.
		memcpy(bufs+IDT9221_OTP_STARTADDR_OFFSET, &current_addr, IDT9221_OTP_STARTADDR_SIZE); //add startaddr
    		memcpy(bufs+IDT9221_OTP_DATALEN_OFFSET, &nozero_len, IDT9221_OTP_DATALEN_SIZE);		//add CodeLength
    		memcpy(bufs+IDT9221_OTP_CHECKSUM_OFFSET, &check_sum, IDT9221_OTP_CHECKSUM_SIZE);	//add CheckSum

		//(4) write data into MCU SRAM for writing into OTP
		idtp9221_write_block(di, IDT9221_OTP_SRAM_ADDR,
							bufs,  nozero_len + IDT9221_OTP_DATA_OFFSET - IDT9221_ADDR_LEN); //package length is not always 8+128

		//(5) Write 1 to the Status in the SRAM. This informs the 9220 to start programming the new packet
		// from SRAM to OTP memory
    		if (idtp9221_write_byte(IDT9221_OTP_SRAM_ADDR, IDT9221_OTP_START_WRITE))
    		{
    			hwlog_info("write  1  into 0x400 register error !\n");
			return -1;
    		}
		do
		{
			mdelay(IDT9221_OTP_SLEEP_TIME);
		  	idtp9221_read_byte(IDT9221_OTP_SRAM_ADDR, bufs);
		} while (bufs[0] == IDT9221_OTP_BUFFER_VALID);

		if (bufs[0] != IDT9221_OTP_FINISH_OK)
		{
			hwlog_info("write  data from SRAM 0x400+ to OTP error !\n");
			hwlog_info("status=%d\n",bufs[0]);
		  	return -1;
		}

		//(7) update offset, size and current_addr
		offset+= write_size;
		size -= write_size;
		current_addr += write_size;
	}
	hwlog_info("program OTP step 2 OK!\n");
	return 0;
}
static int idtp9221_program_otp_third_step(void)
{
	/*	=== Step-3 ===
		Restore system (Need to reset or power cycle 9220 to run the OTP code)
	*/
	if (idtp9221_write_byte(IDT9221_KEY_ADDR, IDT9221_KEY_VALUE)) {
		hwlog_info("%s write key fail\n", __func__);
		return -1;
	}
	if (idtp9221_write_byte(IDT9221_MAP_ADDR, IDT9221_MAP_UNMAPING)) {
		hwlog_info("%s remove code remapping fail\n", __func__);
		return -1;
	}
	hwlog_info("program OTP step 3 OK!\n");
	return 0;
}

/**********************************************************
*  Function:       read_otp
*  Discription:    loading bootloader into idtp9221 MCU SRAM
*  Parameters:     size  read data length, MAX=128
*                  addr OTP physical address
			 data read buffer, notice:data must offset 2
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_read_otp(u16 addr ,u8 *data,int size)
{
	int ret;
	int offset = 0;
	int read_size = 0;
	int current_addr = addr;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s para is null\n", __func__);
		return -1;
	}
	 /*  === Step-1 ===
		init OTP
	 */
	//disable PWM
	if(idtp9221_write_byte(IDT9221_PWM_CTRL_ADDR, IDT9221_PWM_CTRL_DISABLE))
		return -1;
	//core key
	if(idtp9221_write_byte(IDT9221_KEY_ADDR, IDT9221_KEY_VALUE))
		return -1;
	//hold M0
	if(idtp9221_write_byte(IDT9221_M0_ADDR, IDT9221_M0_HOLD))
		return -1;
	//OTP_VRR (VRR=3.0V)
	if(idtp9221_write_byte(IDT9221_OTP_VRR_ADDR, IDT9221_OTP_VRR_3V))
		return -1;
	//OTP_CTRL (VRR_EN=1, EN=1)
	if(idtp9221_write_byte(IDT9221_OTP_CTRL_ADDR, IDT9221_OTP_VRR_EN))
		return -1;

	hwlog_info("read OTP step 1 OK!\n");

	 /*  === Step-2 ===
		read data form OTP, in fact the OTP data is in M0 SRAM(0x8000)
	 */
	while(size > 0){
		if(size >= IDT9221_PAGE_SIZE)
			read_size = IDT9221_PAGE_SIZE;
		else
			read_size = size;

		ret = idtp9221_read_block(di, IDT9221_OTP_START_ADDR + current_addr, data + offset, read_size);
		if(ret){
			hwlog_info("read data from OTP failed!\n");
			return -1;
		}

		current_addr += read_size;
		offset += read_size;
		size -= read_size;
	}
	hwlog_info("read OTP step 2 OK!\n");
	return 0;
}

static int idtp9221_check_is_otp_exist(void)
{
	int is_otp_exist = IDT9221_OTP_NON_PROGRAMED;
	int ret;
	int i;
	u8 data[IDT9221_OTP_SIZE_CHECK]={0};

	idtp9221_chip_enable(0);// enable RX
	ret = idtp9221_read_otp(0, data, IDT9221_OTP_SIZE_CHECK);
	if (ret) {
		hwlog_info("read OTP FAIL!\n");
		return -1;
	}
	for(i = 0;i < IDT9221_OTP_SIZE_CHECK;i++){
		if(data[i] != 0x00){
			is_otp_exist = IDT9221_OTP_PROGRAMED;
			break;
		}
	}
	return is_otp_exist;
}
static int idtp9221_program_otp(void)
{
	int ret = 0;
	ret = idtp9221_check_is_otp_exist();
	if(ret < 0){
		hwlog_info("read OTP FAIL!\n");
		return -1;
	}
	if(IDT9221_OTP_PROGRAMED == ret){
		hwlog_info("RX OTP is already programed !\n");
		return 0;
	}

	ret = idtp9221_program_otp_first_step();
	mdelay(IDT9221_OTP_SLEEP_TIME);  //delay for bootloader startup
	if (ret) {
		hwlog_info("program OTP step 1 FAIL!\n");
		return -1;
	}

	ret = idtp9221_program_otp_second_step(idt_firmware_otp, 0, ARRAY_SIZE(idt_firmware_otp), 0x00);
	if (ret) {
		hwlog_info("program OTP step 2 FAIL!\n");
		return -1;
	}
	ret = idtp9221_program_otp_third_step();
	if (ret) {
		hwlog_info("program OTP step 3 FAIL!\n");
		return -1;
	}

	hwlog_info("program OTP successfully!\n");
	return ret;
}
static int idtp9221_get_mode(u8 *mode)
{
	int ret;
	ret = idtp9221_read_byte(IDT9221_SYS_MODE_ADDR, mode);
	if(ret){
		hwlog_err("%s:read failed!\n", __func__);
		return -1;
	}
	return 0;
}
static bool idtp9221_check_tx_exist(void)
{
	u8 mode;
	int ret = idtp9221_get_mode(&mode);
	if (ret) {
		hwlog_err("%s: get rx mode failed!\n", __func__);
		return false;
	}
	if ((IDT9221_WPC_MODE & mode) || (IDT9221_PMA_MODE & mode)) {
		return true;
	}

	return false;
}
static int idtp9221_send_charge_state(u8 chrg_state)
{
	int ret = idtp9221_send_msg_ack(IDT9221_CMD_SEND_CHRG_STATE, &chrg_state, IDT9221_CHRG_STATE_LEN);
	if(ret) {
		hwlog_err("%s: send charge_state to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] charge_state = %d!\n", __func__, chrg_state);
	return 0;
}
static int idtp9221_kick_watchdog(void)
{
	int ret;
	ret = idtp9221_write_word(IDT9221_RX_FC_TIMER_ADDR, 0);
	if(ret){
		hwlog_err("%s: write register[0x%x] failed!\n", __func__, IDT9221_RX_FC_TIMER_ADDR);
		return -1;
	}
	return 0;
}
static int idtp9221_program_sramupdate(char *srcdata, unsigned int size)
{
	u8 mode;
	u8 bufs[IDT9221_PAGE_SIZE + IDT9221_ADDR_LEN];	//write datas must offset 2 bytes
	u8 write_size;
	u16 offset;
	int i,j;
	int ret;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di || NULL == srcdata) {
		hwlog_err("%s:para is null\n", __func__);
		return -1;
	}

	/* === Step -1 : check idtp9221 mode === */
	ret = idtp9221_get_mode(&mode);
	if(ret) {
		hwlog_err("%s:get mode failed!\n", __func__);
		return -1;
	}
	if(!(IDT9221_OTPONLY_MODE & mode)) {
		hwlog_err("%s:not runing in OTP!\n", __func__);
		return -1;
	}
	if(!((IDT9221_WPC_MODE & mode) || (IDT9221_PMA_MODE & mode))) {
		hwlog_err("%s:not in WPC/PMA mode!\n", __func__);
		return -1;
	}

	/* === Step -2 : write FW into SRAM and check === */
	i = size;
	offset = 0;
	while(i > 0) {
		if(i >= IDT9221_PAGE_SIZE) {
			write_size = IDT9221_PAGE_SIZE;
		}else {
			write_size = i;
		}
		memcpy(bufs + IDT9221_ADDR_LEN, srcdata + offset, write_size);
		ret = idtp9221_write_block(di, IDT9221_SRAMUPDATE_ADDR + offset, bufs, write_size);
		if(ret) {
			hwlog_err("%s:write SRAM firmware failed!\n", __func__);
			return -1;
		}
		offset += write_size;
		i -= write_size;
	}

	i = size;
	offset = 0;
	while(i > 0) {
		if(i >= IDT9221_PAGE_SIZE) {
			write_size = IDT9221_PAGE_SIZE;
		}else {
			write_size = i;
		}
		ret = idtp9221_read_block(di, IDT9221_SRAMUPDATE_ADDR + offset, bufs, write_size);
		if(ret) {
			hwlog_err("%s: read SRAM failed!\n", __func__);
			return -1;
		}
		for(j = 0; j < write_size; j++) {
			if(bufs[j] != srcdata[offset + j]) {
				hwlog_err("%s:not equal: read = 0x%x, write = 0x%x!\n", __func__, bufs[j], srcdata[offset + j]);
				return -1;
			}
		}
		i -= write_size;
		offset += write_size;
	}

	/*  === Step -3 : switch to SRAM running === */
	ret = idtp9221_write_byte(IDT9221_CMD1_ADDR, IDT9221_CMD1_UNLOCK_SWITCH);//unlock the switch.
	ret |= idtp9221_write_byte(IDT9221_CMD_ADDR, IDT9221_CMD_SWITCH_TO_SRAM);//switch to SRAM
	if(ret) {
		hwlog_err("%s: write 0x004e/0x004f failed!\n", __func__);
		return -1;
	}
	msleep(50);//wait for swtiching to SRAM running
	ret = idtp9221_get_mode(&mode);
	if(!(IDT9221_RAMPROGRAM_MODE & mode)) {
		hwlog_err("%s: switch to SRAM running failed!\n", __func__);
		return -1;
	}
	return 0;
}

static char* idtp9221_get_rx_fod_coef(void)
{
	int ret, i;
	static char fod_coef_str[IDT9221_RX_FOD_COEF_STRING_LEN] = {0};
	u16 fod_coef[IDT9221_RX_FOD_COEF_LEN] = {0};
	char tmp[IDT9221_RX_TMP_BUFF_LEN] = {0};

	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return "error";
	}
	memset(fod_coef_str, 0, IDT9221_RX_FOD_COEF_STRING_LEN);
	for (i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		ret = idtp9221_read_word(IDT9221_RX_FOD_COEF_STSRT_ADDR + i*WORD_LEN, &fod_coef[i]);
		if (ret) {
			hwlog_err("%s read fod_coed[%d] fail\n", __func__, i);
			return "error";
		}
	}
	for (i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		snprintf(tmp, IDT9221_RX_TMP_BUFF_LEN, "%d ", fod_coef[i]);
		strncat(fod_coef_str, tmp, strlen(tmp));
	}
	return fod_coef_str;
}
static int idtp9221_set_rx_fod_coef(char* fod_coef)
{
	char* cur;
	char* token;
	u16 fod_arr[IDT9221_RX_FOD_COEF_LEN] = {0};
	int i = 0;
	long val;
	int ret;
	if (!fod_coef) {
		hwlog_err("%s input para null \n", __func__);
		return -1;
	}
	if (strlen(fod_coef) >= IDT9221_RX_FOD_COEF_STRING_LEN) {
		hwlog_err("%s length(%d) exceeds max size(%d)\n",
			__func__, strlen(fod_coef), IDT9221_RX_FOD_COEF_STRING_LEN);
		return -1;
	}
	cur = fod_coef;
	token = strsep(&cur, " ,");
	while (token) {
		if ((strict_strtol(token, 10, &val) < 0)) {  //10: decimalism
			hwlog_err("%s input para type err\n", __func__);
			return -1;
		}
		token = strsep(&cur, " ,");
		fod_arr[i++] = (u16)val;
		if (i == IDT9221_RX_FOD_COEF_LEN)
			break;
	}
	if (i < IDT9221_RX_FOD_COEF_LEN) {
		hwlog_err("%s input para number err\n", __func__);
		return -1;
	}
	for(i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		hwlog_info("%s fod_coef[%d] = %u\n", __func__, i, fod_arr[i]);
		ret = idtp9221_write_word(IDT9221_RX_FOD_COEF_STSRT_ADDR + i*WORD_LEN, fod_arr[i]);
		if(ret) {
			hwlog_err("%s set fod register[%d] failed!\n", __func__, i);
			return -1;
		}
	}

	return 0;
}

static int idtp9221_fix_tx_fop(int fop)
{
	if(fop < IDT9221_FIXED_FOP_MIN || fop > IDT9221_FIXED_FOP_MAX ) {
		hwlog_err("%s fixed fop(%d) exceeds range[%d, %d]\n",
			__func__, fop, IDT9221_FIXED_FOP_MIN, IDT9221_FIXED_FOP_MAX);
		return -1;
	}

	return idtp9221_send_msg_ack(IDT9221_CMD_FIX_TX_FOP,
		(u8 *)(&fop), IDT9221_TX_FOP_LEN);
}
static int idtp9221_unfix_tx_fop(void)
{
	return idtp9221_send_msg_ack(IDT9221_CMD_UNFIX_TX_FOP, NULL, 0);
}
static void idtp9221_rx_fod_coef_5v(void)
{
	int ret, i;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s: para is null\n", __func__);
		return;
	}
	/*set rx 5v fod coef*/
	for(i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		ret = idtp9221_write_word(IDT9221_RX_FOD_COEF_STSRT_ADDR + i*WORD_LEN, di->rx_fod_5v[i]);
		if(ret) {
			hwlog_err("%s: set rx_fod_5v reg[%d] failed!\n", __func__, i);
		}
	}
	hwlog_info("[%s] update ok", __func__);
}
static void idtp9221_rx_fod_coef_9v(void)
{
	int ret, i;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s: para is null\n", __func__);
		return;
	}
	/*set rx 9v fod coef*/
	for(i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		ret = idtp9221_write_word(IDT9221_RX_FOD_COEF_STSRT_ADDR + i*WORD_LEN, di->rx_fod_9v[i]);
		if(ret) {
			hwlog_err("%s: set rx_fod_9v reg[%d] failed!\n", __func__, i);
		}
	}
	hwlog_info("[%s] update ok", __func__);
}

static int idtp9221_chip_init(int tx_vset)
{
	int ret = 0;
	switch(tx_vset) {
		case WIRELESS_CHIP_INIT:
			/*set wdt*/
			ret |= idtp9221_write_word(IDT9221_RX_FC_TIMEOUT_ADDR, IDT9221_RX_FC_TIMEOUT);
			/*set vrect coef*/
			ret |= idtp9221_write_byte(IDT9221_RX_PWR_KNEE_ADDR, IDT9221_RX_PWR_KNEE_VAL);
			ret |= idtp9221_write_byte(IDT9221_RX_VRCORR_FACTOR_ADDR, IDT9221_RX_VRCORR_FACTOR_VAL);
			ret |= idtp9221_write_word(IDT9221_RX_VRMAX_COR_ADDR, IDT9221_RX_VRMAX_COR_VAL);
			ret |= idtp9221_write_word(IDT9221_RX_VRMIN_COR_ADDR, IDT9221_RX_VRMIN_COR_VAL);
			/*CAP Enable*/
			ret |= idtp9221_write_byte(IDT9221_RX_USER_FLAGS_ADDR, IDT9221_RX_USER_FLAGS);
		case ADAPTER_5V*MVOLT_PER_VOLT:
			idtp9221_rx_fod_coef_5v();
			break;
		case ADAPTER_9V*MVOLT_PER_VOLT:
			idtp9221_rx_fod_coef_9v();
			break;
		default:
			hwlog_info("%s: input para is invalid!\n", __func__);
			break;
	}
	return ret;
}
static int idtp9221_stop_charging(void)
{
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s: para is null\n", __func__);
		return -1;
	}
	if(irq_abnormal_flag &&
		WIRED_CHANNEL_ON == wireless_charge_get_wired_channel_state()) {
		di->irq_cnt = 0;
		irq_abnormal_flag = false;
		enable_irq(di->irq_int);
		hwlog_info("[%s] wired channel on, enable irq_int!\n", __func__);
	}
	stop_charging_flag = 1;
	return 0;
}
static enum tx_adaptor_type idtp9221_get_tx_type(void)
{
	int ret;
	u8 adapter[IDT9221_TX_ADAPTER_TYPE_LEN];

	ret = idtp9221_get_tx_para(IDT9221_CMD_GET_TX_ADAPTER_TYPE,
			adapter, IDT9221_TX_ADAPTER_TYPE_LEN);
	if(ret) {
		hwlog_err("%s: get tx adapter failed !\n", __func__);
		return WIRELESS_TYPE_ERR;
	}
	if(IDT9221_CMD_GET_TX_ADAPTER_TYPE != adapter[0]) {
		hwlog_err("%s: cmd 0x%x err!\n", __func__, adapter[0]);
		return WIRELESS_TYPE_ERR;
	}
	hwlog_info("%s:get tx adapter success, adapter = %d!\n", __func__, adapter[1]);
	return (enum tx_adaptor_type)adapter[1];
}
static struct tx_capability * idtp9221_get_tx_capability(void)
{
	static struct tx_capability tx_cap;
	int ret;
	u8 tx_cap_data[TX_CAP_TOTAL];
	ret = idtp9221_get_tx_para(IDT9221_CMD_GET_TX_CAP, tx_cap_data, TX_CAP_TOTAL);
	if(ret) {
		hwlog_err("%s: get tx capability failed !\n", __func__);
		tx_cap.type = WIRELESS_TYPE_ERR;
		return &tx_cap;
	}
	if(IDT9221_CMD_GET_TX_CAP != tx_cap_data[0]) {
		hwlog_err("%s: cmd 0x%x err!\n", __func__, tx_cap_data[0]);
		tx_cap.type = WIRELESS_TYPE_ERR;
		return &tx_cap;
	}
	tx_cap.type = tx_cap_data[TX_CAP_TYPE];
	tx_cap.vout_max = tx_cap_data[TX_CAP_VOUT_MAX] * IDT9221_TX_CAP_VOUT_STEP;
	tx_cap.iout_max = tx_cap_data[TX_CAP_IOUT_MAX] * IDT9221_TX_CAP_IOUT_STEP;
	tx_cap.boost = tx_cap_data[TX_CAP_ATTR] & IDT9221_TX_CAP_BOOST_MASK;
	tx_cap.cable = tx_cap_data[TX_CAP_ATTR] & IDT9221_TX_CAP_CABLE_MASK;
	tx_cap.fan = tx_cap_data[TX_CAP_ATTR] & IDT9221_TX_CAP_FAN_MASK;
	tx_cap.tec = tx_cap_data[TX_CAP_ATTR] & IDT9221_TX_CAP_TEC_MASK;

	hwlog_info("[%s] type = %d, vout_max = %d, iout_max = %d,"
		"boost = %d, cable = %d, fan = %d, nec = %d\n", __func__,
		tx_cap.type, tx_cap.vout_max, tx_cap.iout_max,
		tx_cap.boost, tx_cap.cable, tx_cap.fan, tx_cap.tec);

	return &tx_cap;
}
static u8* idtp9221_get_tx_fw_version(void)
{
	int ret, i;
	static u8 tx_fw_version[IDT9221_TX_FW_VERSION_STRING_LEN] = {0};
	u8 data[IDT9221_TX_FW_VERSION_LEN + 1] = {0}; //bit[0]:command,  bit[1:2:3:4]: tx fw version
	u8 buff[IDT9221_RX_TMP_BUFF_LEN] = {0};

	memset(tx_fw_version, 0, IDT9221_TX_FW_VERSION_STRING_LEN);
	ret = idtp9221_get_tx_para(IDT9221_CMD_GET_TX_VERSION, data, IDT9221_TX_FW_VERSION_LEN + 1);
	if(ret) {
		hwlog_err("%s: get tx info failed !\n", __func__);
		return "error";
	}
	if(IDT9221_CMD_GET_TX_VERSION != data[0]) {
		hwlog_err("%s: cmd 0x%x err!\n", __func__, data[0]);
		return "error";
	}

	for(i = IDT9221_TX_FW_VERSION_LEN; i >= 1; i--) {
		if(1 != i) {
			snprintf(buff, IDT9221_RX_TMP_BUFF_LEN, "0x%02x ", data[i]);
		}else{
			snprintf(buff, IDT9221_RX_TMP_BUFF_LEN, "0x%02x", data[i]);
		}
		strncat(tx_fw_version, buff, strlen(buff));
	}
	return tx_fw_version;
}
static u8* idtp9221_get_chip_id(void)
{
	int ret, i;
	static u8 chip_id[IDT9221_RX_CHIP_ID_STRING_LEN] = {0};
	u8 data[IDT9221_RX_CHIP_ID_LEN] = {0};
	u8 buff[IDT9221_RX_TMP_BUFF_LEN] = {0};
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return "error";
	}
	memset(chip_id, 0, IDT9221_RX_CHIP_ID_STRING_LEN);
	ret = idtp9221_read_block(di, IDT9221_RX_CHIP_ID_ADDR, data, IDT9221_RX_CHIP_ID_LEN);
	if (!ret) {
		for (i = 0; i < IDT9221_RX_CHIP_ID_LEN; i++) {
			snprintf(buff, IDT9221_RX_TMP_BUFF_LEN, "0x%x ", data[i]);
			strncat(chip_id, buff, strlen(buff));
		}
		return chip_id;
	} else {
		return "error";
	}
}
static u8* idtp9221_get_otp_fw_version(void)
{
	int ret, i;
	static u8 fw_version[IDT9221_RX_OTP_FW_VERSION_STRING_LEN] = {0};
	u8 data[IDT9221_RX_OTP_FW_VERSION_LEN] = {0};
	u8 buff[IDT9221_RX_TMP_BUFF_LEN] = {0};

	struct idtp9221_device_info *di = g_idtp9221_di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return "error";
	}
	memset(fw_version, 0, IDT9221_RX_OTP_FW_VERSION_STRING_LEN);
	ret = idtp9221_read_block(di, IDT9221_RX_OTP_FW_VERSION_ADDR,
				data, IDT9221_RX_OTP_FW_VERSION_LEN);
	if (!ret) {
		for (i = IDT9221_RX_OTP_FW_VERSION_LEN - 1; i >= 0; i--) {
			if(0 != i) {
				snprintf(buff, IDT9221_RX_TMP_BUFF_LEN, "0x%02x ", data[i]);
			}else {
				snprintf(buff, IDT9221_RX_TMP_BUFF_LEN, "0x%02x", data[i]);
			}
			strncat(fw_version, buff, strlen(buff));
		}
		return fw_version;
	} else {
		return "error";
	}
}
static void idtp9221_check_fwupdate(void)
{
	int ret;
	int i = 0;
	u8 *otp_fw_version;
	unsigned int fw_update_size;

	fw_update_size = ARRAY_SIZE(fw_update);
	otp_fw_version = idtp9221_get_otp_fw_version();
	if(strcmp(otp_fw_version, "error") == 0) {
		hwlog_err("%s:get firmware version fail!\n", __func__);
		return;
	}

	for(i = 0; i < fw_update_size; i++) {
		if(strcmp(otp_fw_version, fw_update[i].name_fw_update_from) >= 0
			&& strcmp(otp_fw_version, fw_update[i].name_fw_update_to) <= 0) {
			hwlog_info("[%s] SRAM update start! otp_fw_version = %s!\n", __func__, otp_fw_version);
			ret = idtp9221_program_sramupdate(fw_update[i].fw_sram, fw_update[i].fw_sram_size);
			if(ret) {
				hwlog_err("%s:SRAM update fail!\n", __func__);
				return;
			}
			otp_fw_version = idtp9221_get_otp_fw_version();
			hwlog_info("[%s] SRAM update succ! otp_fw_version = %s\n", __func__, otp_fw_version);
			idtp9221_chip_init(WIRELESS_CHIP_INIT);
			return;
		}
	}
	hwlog_info("%s:SRAM update, no need!\n", __func__);
}

static int idtp9221_ldo_ready(void)
{
	int ret = -1;
	u8 data = -1;
	ret = idtp9221_send_msg_ack(IDT9221_CMD_SEND_READY, NULL, 0);
	if(ret) {
		hwlog_info("%s:send ldo ready fail\n",__func__);
		return -1;
	}
	hwlog_info("[%s]send ldo ready success\n",__func__);

	/*0x3404 clear*/
	ret = idtp9221_read_byte(IDT9221_LDO_ADDR, &data);
	if(ret){
		hwlog_info("%s:read 0x3404 fail\n",__func__);
		return -1;
	}
	hwlog_info("[%s]before read 0x3404 value =0x%x\n",__func__,data);
	ret = idtp9221_write_byte(IDT9221_LDO_ADDR, 0);
	if(ret){
		hwlog_info("%s:write 0x3404 fail\n",__func__);
		return -1;
	}
	ret = idtp9221_read_byte(IDT9221_LDO_ADDR, &data);
	if(ret){
		hwlog_info("%s:read 0x3404 fail\n",__func__);
		return -1;
	}
	hwlog_info("[%s]after read 0x3404 value =0x%x\n",__func__,data);
	return 0;
}

static int idtp9221_get_tx_id(void)
{
	int ret;
	u8 id_para[IDT9221_TX_ID_LEN - 1] = {0x88,0x66}; //bit[0:1]: tx id parameters, not include command
	u8 id_received[IDT9221_TX_ID_LEN] = {0}; //bit[0]:command,  bit[1:2]: tx id parameters
	int tx_id;
	int count = 0;

	do {
		ret = idtp9221_send_msg(IDT9221_CMD_GET_TX_ID, id_para, IDT9221_TX_ID_LEN - 1);
		ret |= idtp9221_receive_msg(id_received, IDT9221_TX_ID_LEN);
		if(!ret) {
			hwlog_info("[%s] get tx id, succ!\n", __func__);
			break;
		}
		count++;
		hwlog_info("%s: get tx id, fail! try next time!\n", __func__);
	} while (count < IDT9221_GET_TX_PARA_RETRY_CNT);

	if(IDT9221_CMD_GET_TX_ID != id_received[0]) {
		hwlog_err("%s:cmd 0x%x err!\n", __func__, id_received[0]);
		return -1;
	}

	tx_id = (id_received[1] << BITS_PER_BYTE) | id_received[2];

	/*send ldo ready message*/
	if(runmode_is_factory()) {
		idtp9221_ldo_ready();
	}
	return tx_id;
}

static int idtp9221_send_msg_rx_vout(int rx_vout)
{
	int ret;
	ret = idtp9221_send_msg_ack(IDT9221_CMD_START_SAMPLE, (u8*)(&rx_vout), IDT9221_RX_VOUT_LEN);
	if(ret){
		hwlog_err("%s:idtp9221 send RX_VOUT to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] rx_vout = %d \n", __func__,rx_vout);
	return 0;
}
static int idtp9221_send_msg_rx_iout(int rx_iout)
{
	int ret;
	ret = idtp9221_send_msg_ack(IDT9221_CMD_STOP_SAMPLE, (u8*)(&rx_iout), IDT9221_RX_IOUT_LEN);
	if(ret){
		hwlog_err("%s:idtp9221 send RX_IOUT to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] rx_iout = %d \n", __func__,rx_iout);
	return 0;
}
static int idtp9221_send_msg_certification_confirm(bool result)
{
	int ret;
	if(!result){
		ret = idtp9221_send_msg_ack(IDT9221_CMD_CERT_SUCC, NULL, 0);
	}else{
		ret = idtp9221_send_msg_ack(IDT9221_CMD_CERT_FAIL, NULL, 0);
	}
	if(ret){
		hwlog_err("%s:send CERT result to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] cert_result = %d \n", __func__,result);
	return 0;
}
static int idtp9221_send_msg_rx_boost_succ(void)
{
	int ret;
	ret = idtp9221_send_msg_ack(IDT9221_CMD_RX_BOOST_SUCC, NULL, 0);
	if(ret) {
		hwlog_err("%s:send rx boost succ to TX failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int idtp9221_get_tx_certification(u8 *random, unsigned int random_len, u8 *cipherkey ,unsigned int key_len)
{
	int ret;
	int i;
	int count = 0;
	u8 data[IDT9221_RX_TO_TX_DATA_LEN + 1] = {0};
	if(NULL == random || random_len < WIRELESS_RANDOM_LEN || NULL == cipherkey || key_len < WIRELESS_TX_KEY_LEN) {
		hwlog_info("%s: NULL pointer!\n", __func__);
		return -1;
	}

	/*send message  to tx: random number */
	for(i = 0; i < WIRELESS_RANDOM_LEN/IDT9221_RX_TO_TX_DATA_LEN; i++ ) {
		ret = idtp9221_send_msg_ack(IDT9221_CMD_START_CERT + i,
					random + i*IDT9221_RX_TO_TX_DATA_LEN, IDT9221_RX_TO_TX_DATA_LEN);
		if(ret) {
			hwlog_err("%s: send message to TX fail, after retry!\n", __func__);
			return -1;
		}
		hwlog_info("[%s] send random succ, the %dth data\n", __func__, i + 1);
	}

	/*get cipherkey from TX */
	for(i = 0; i < WIRELESS_TX_KEY_LEN/IDT9221_RX_TO_TX_DATA_LEN; i++ ) {
		ret = idtp9221_get_tx_para(IDT9221_CMD_GET_HASH + i, data, IDT9221_RX_TO_TX_DATA_LEN + 1);
		if(ret) {
			hwlog_err("%s: get tx cipherkey, failed!\n", __func__);
			return -1;
		}
		if(IDT9221_CMD_GET_HASH + i == data[0]) {
			memcpy(cipherkey + i * IDT9221_RX_TO_TX_DATA_LEN, data + 1, IDT9221_RX_TO_TX_DATA_LEN);
			hwlog_info("[%s] get tx cipherkey succ, the %dth data\n", __func__, i + 1);
		}
	}
	return 0;
}

static int idtp9221_send_msg_serialno(char *serial_no, unsigned int len)
{
	int ret;
	u8 data_no[SERIALNO_LEN] = {0};
	int i;
	if(NULL == serial_no || len < SERIALNO_LEN) {
		hwlog_info("%s: NULL pointer!\n", __func__);
		return -1;
	}

	for(i = 0;i < SERIALNO_LEN ; i++) {
		if(*(serial_no + i) != '\0') {
			data_no[i] = *(serial_no + i);
		}
	}

	for(i = 0; i < SERIALNO_LEN/IDT9221_RX_TO_TX_DATA_LEN; i++ ) {
		ret = idtp9221_send_msg_ack(IDT9221_CMD_SEND_SN + i, &(data_no[i*IDT9221_RX_TO_TX_DATA_LEN]), IDT9221_RX_TO_TX_DATA_LEN);
		if(ret) {
			hwlog_err("[%s] fail, serialno = %s!\n", __func__, serial_no);
			return -1;
		}
		hwlog_info("[%s] send serialno, the %dth data \n", __func__, i + 1);
	}
	hwlog_info("[%s] succ, serialno = %s\n", __func__, serial_no);
	return 0;
}
static int idtp9221_send_msg_batt_temp(int batt_temp)
{
	int ret;
	u8 data_temp;
	if(batt_temp > 0 || batt_temp <= IDT9221_BATT_TEMP_MAX ) {
		data_temp = (u8)(batt_temp);
	}else{
		data_temp = 0;
	}

	ret = idtp9221_send_msg_ack(IDT9221_CMD_SEND_BATT_TEMP, &data_temp, IDT9221_BATT_TEMP_LEN);
	if(ret) {
		hwlog_err("%s:send batt_temp to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] batt_temp = %d!\n", __func__, batt_temp);
	return 0;
}
static int idtp9221_send_msg_batt_capacity(int batt_capacity)
{
	int ret;
	ret = idtp9221_send_msg_ack(IDT9221_CMD_SEND_BATT_CAPACITY, (u8 *)(&batt_capacity), IDT9221_BATT_CAPACITY_LEN);
	if(ret) {
		hwlog_err("%s: send batt_volt to TX failed!\n", __func__);
		return -1;
	}
	hwlog_info("[%s] batt_capacity = %d \n", __func__, batt_capacity);
	return 0;
}

/**********************************************************
*  Function:       idtp9221_data_received_handler
*  Discription:    handler data received form TX
*  Parameters:     di:idtp9221 device info
*  return value:   0-sucess or others-fail
**********************************************************/
static int idtp9221_data_received_handler(struct idtp9221_device_info *di)
{
	int ret = 0;
	int i;
	u8 buff[IDT9221_TX_TO_RX_DATA_LEN] = { 0 };
	u8 command = 0;
	u8 data_received[IDT9221_TX_TO_RX_DATA_LEN - 1] = {0};

	/* TX send message to RX on it's own initiative , to get something it needs. handler the interruption */
	ret = idtp9221_read_block(di, IDT9221_TX_TO_RX_DATA_ADDR, &(buff[0]), IDT9221_TX_TO_RX_DATA_LEN);
	if(ret){
		hwlog_err("%s:read data received from TX failed!\n", __func__);
		return -1;
	}
	command = buff[0];
	hwlog_info("[%s] data received from TX, command:0x%x\n", __func__, command);
	for(i = 0;i < IDT9221_TX_TO_RX_DATA_LEN - 1; i++){
		data_received[i] = buff[i + 1];
		hwlog_info("%s:data received from TX, data:0x%x\n", __func__, data_received[i]);
	}

	/* analysis data received*/
	switch(command){
		case IDT9221_CMD_SEND_SN:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: serialno\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_GET_SERIAL_NO, NULL);
			break;
		case IDT9221_CMD_SEND_BATT_TEMP:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: batt temp\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_GET_BATTERY_TEMP, NULL);
			break;
		case IDT9221_CMD_SEND_BATT_CAPACITY:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: batt capacity\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_GET_BATTERY_CAPACITY, NULL);
			break;
		case IDT9221_CMD_SET_CURRENT_LIMIT:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: set current limit\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_SET_CURRENT_LIMIT, &(data_received[0]));
			msleep(IDT9221_LIMIT_CURRENT_TIME);//wait for chargeIC to limit input current!
			ret = idtp9221_send_msg(IDT9221_CMD_SET_CURRENT_LIMIT, &(data_received[0]), 1);
			if(ret){
				hwlog_err("%s:send current limit is alreay set to TX failed!\n", __func__);
				return -1;
			}
			hwlog_err("%s:send current limit is alreay set to TX !\n", __func__);
			break;
		case IDT9221_CMD_START_SAMPLE:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: start sample\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_START_SAMPLE, NULL);
			break;
		case IDT9221_CMD_STOP_SAMPLE:
			di->irq_val &= ~IDT9221_RX_STATUS_TXDATA_RECEIVED;
			hwlog_info("[%s] receive command: stop sample\n",__func__);
			blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_STOP_SAMPLE, NULL);
			break;
		default:
			hwlog_info("[%s] receive command: data received from TX, not handler it!\n", __func__);
			break;
	}
	return 0;
}

/**********************************************************
*  Function:       idtp9221_rx_ready_handler
*  Discription:    handler rx ready
*  Parameters:     di:idtp9221 device info
*  return value:   null
**********************************************************/
static void idtp9221_rx_ready_handler(struct idtp9221_device_info *di)
{
	if (WIRED_CHANNEL_ON != wireless_charge_get_wired_channel_state()) {
		hwlog_info("%s rx ready, goto wireless charging\n", __func__);
		stop_charging_flag = 0;
		di->irq_cnt = 0;
		wired_chsw_set_wired_channel(WIRED_CHANNEL_CUTOFF);
		msleep(CHANNEL_SW_TIME);
		gpio_set_value(di->gpio_sleep_en, RX_SLEEP_EN_DISABLE);
		blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_RX_READY, NULL);
	}
}
static void idtp9221_handle_abnormal_irq(struct idtp9221_device_info *di)
{
	static struct timespec64 ts64_timeout;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;
	ts64_now = current_kernel_time64();
	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = WIRELESS_INT_TIMEOUT_TH * NSEC_PER_MSEC;

	hwlog_info("[%s] irq_cnt = %d\n", __func__, ++di->irq_cnt);
	/*power on interrupt happen first time, so start monitor it!*/
	if (di->irq_cnt == 1) {
		ts64_timeout = timespec64_add_safe(ts64_now, ts64_interval);
		if (ts64_timeout.tv_sec == TIME_T_MAX) {
			di->irq_cnt = 0;
			hwlog_err("%s: time overflow happend\n", __func__);
			return;
		}
	}

	if (timespec64_compare(&ts64_now, &ts64_timeout) >= 0) {
		if (di->irq_cnt >= WIRELESS_INT_CNT_TH) {
			irq_abnormal_flag = true;
			disable_irq_nosync(di->irq_int);
			wired_chsw_set_wired_channel(WIRED_CHANNEL_CUTOFF);
			msleep(CHANNEL_SW_TIME);
			gpio_set_value(di->gpio_sleep_en, RX_SLEEP_EN_DISABLE);
			hwlog_err("%s: more than %d interrupts happened in %ds, disable irq\n",
				__func__, WIRELESS_INT_CNT_TH, WIRELESS_INT_TIMEOUT_TH/MSEC_PER_SEC);
		} else {
			di->irq_cnt = 0;
			hwlog_info("%s: less than %d interrupts happened in %ds, clear irq count!\n",
				__func__, WIRELESS_INT_CNT_TH, WIRELESS_INT_TIMEOUT_TH/MSEC_PER_SEC);
		}
	}
}
static void idtp9221_rx_power_on_handler(struct idtp9221_device_info *di)
{
	u8 rx_ss = 0;//ss: Signal Strength
	bool power_on_good_flag = false;

	idtp9221_handle_abnormal_irq(di);
	idtp9221_read_byte(IDT9221_RX_SS_ADDR, &rx_ss);
	if(rx_ss > di->rx_ss_good_lth) {
		power_on_good_flag = true;
	}
	hwlog_info("[%s] Signal_Strength = %u\n", __func__, rx_ss);
	blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_RX_POWER_ON, &power_on_good_flag);
}
/**********************************************************
*  Function:       idtp9221_irq_work
*  Description:   handler for wireless receiver irq in charging process
*  Parameters:   work:wireless receiver fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void idtp9221_irq_work(struct work_struct *work)
{
	u16 irq_value = 0;
	struct idtp9221_device_info *di =
		container_of(work, struct idtp9221_device_info, irq_work);
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		idtp9221_wake_unlock();
		return;
	}

	int ret = idtp9221_read_word(IDT9221_RX_INT_STATUS_ADDR, &di->irq_val);
	if (ret) {
		hwlog_err("%s read interrupt fail, clear interrupt\n", __func__);
		idtp9221_clear_interrupt(WORD_MASK);
		idtp9221_handle_abnormal_irq(di);
		goto FuncEnd;
	}

	hwlog_info("%s interrupt 0x%x\n", __func__, di->irq_val);
	idtp9221_clear_interrupt(di->irq_val);

	if (di->irq_val & IDT9221_RX_STATUS_READY) {
		di->irq_val &= ~IDT9221_RX_STATUS_READY;
		idtp9221_rx_ready_handler(di);
	}
	if (di->irq_val & IDT9221_RX_STATUS_POWER_ON) {
		di->irq_val &= ~IDT9221_RX_STATUS_POWER_ON;
		idtp9221_rx_power_on_handler(di);
	}
	if (di->irq_val & IDT9221_RX_STATUS_OCP) {
		di->irq_val &= ~IDT9221_RX_STATUS_OCP;
		blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_RX_OCP, NULL);
	}
	if (di->irq_val & IDT9221_RX_STATUS_OVP) {
		di->irq_val &= ~IDT9221_RX_STATUS_OVP;
		blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_RX_OVP, NULL);
	}
	if (di->irq_val & IDT9221_RX_STATUS_OTP) {
		di->irq_val &= ~IDT9221_RX_STATUS_OTP;
		blocking_notifier_call_chain(&rx_event_nh, WIRELESS_CHARGE_RX_OTP, NULL);
	}
	/* receice data from TX, please handler the interrupt! */
	if (di->irq_val & IDT9221_RX_STATUS_TXDATA_RECEIVED) {
		idtp9221_data_received_handler(di);
	}

FuncEnd:
	if (!gpio_get_value(di->gpio_int)) {
		idtp9221_read_word(IDT9221_RX_INT_STATUS_ADDR, &irq_value);
		hwlog_info("[%s] gpio_int is low, clear interrupt again! irq_value = 0x%x\n", __func__,irq_value);
		idtp9221_clear_interrupt(WORD_MASK);
	}
	if (di->irq_active == 0) {
		di->irq_active = 1;
		enable_irq(di->irq_int);
	}
	idtp9221_wake_unlock();
}
/**********************************************************
*  Function:       idtp9221_interrupt
*  Description:    callback function for wireless reveiver irq in charging process
*  Parameters:   irq:wireless reveiver interrupt
*                      _di:idt9221_device_info
*  return value:  IRQ_HANDLED-success or others
**********************************************************/
static irqreturn_t idtp9221_interrupt(int irq, void *_di)
{
	struct idtp9221_device_info *di = _di;
	if (!di) {
		hwlog_err("%s di null\n", __func__);
		return IRQ_HANDLED;
	}

	idtp9221_wake_lock();
	hwlog_info("%s ++\n", __func__);
	if (di->irq_active == 1) {
		di->irq_active = 0;
		disable_irq_nosync(di->irq_int);
		schedule_work(&di->irq_work);
	} else {
		hwlog_info("irq is not enable,do nothing!\n");
		idtp9221_wake_unlock();
	}
	hwlog_info("%s --\n", __func__);

	return IRQ_HANDLED;
}
static void idtp9221_pmic_vbus_handler(bool vbus_state)
{
	int irq_val = 0;
	int ret = 0;
	struct idtp9221_device_info *di = g_idtp9221_di;
	if (NULL == di) {
		hwlog_err("%s: di is null\n", __func__);
		return;
	}
	if (vbus_state && irq_abnormal_flag
		&& WIRED_CHANNEL_ON != wireless_charge_get_wired_channel_state()
		&& idtp9221_check_tx_exist()) {
		ret = idtp9221_read_word(IDT9221_RX_INT_STATUS_ADDR, &irq_val);
		if (ret) {
			hwlog_err("%s: read interrupt fail, clear interrupt\n", __func__);
			return;
		}
		hwlog_info("[%s] irq_val = 0x%x\n", __func__, irq_val);
		if (irq_val & IDT9221_RX_STATUS_READY) {
			idtp9221_clear_interrupt(WORD_MASK);
			idtp9221_rx_ready_handler(di);
			di->irq_cnt = 0;
			irq_abnormal_flag = false;
			enable_irq(di->irq_int);
		}
	}
}

/**********************************************************
*  Function:       idtp9221_parse_dts
*  Description:   idtp9221_parse_dts
*  Parameters:    device_node:wireless receiver device_node
*                      di:idtp9221_device_info
*  return value:  NULL
**********************************************************/

static int idtp9221_parse_dts(struct device_node *np, struct idtp9221_device_info *di)
{
	int ret, i;
	ret = of_property_read_u32_array(np, "rx_fod_5v", di->rx_fod_5v, IDT9221_RX_FOD_COEF_LEN);
	if (ret) {
		hwlog_err("%s: get para fail!\n", __func__);
		return -EINVAL;
	}
	for(i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		hwlog_info("[%s] rx_fod_5v[%d] = %d\n", __func__, i, di->rx_fod_5v[i]);
	}
	ret = of_property_read_u32_array(np, "rx_fod_9v", di->rx_fod_9v, IDT9221_RX_FOD_COEF_LEN);
	if (ret) {
		hwlog_err("%s: get para fail!\n", __func__);
		return -EINVAL;
	}
	for(i = 0; i < IDT9221_RX_FOD_COEF_LEN; i++) {
		hwlog_info("[%s] rx_fod_9v[%d] = %d\n", __func__, i, di->rx_fod_9v[i]);
	}
	ret = of_property_read_u32(np, "rx_ss_good_lth", &di->rx_ss_good_lth);
	if (ret) {
		hwlog_err("%s: get rx_ss_good_lth failed\n", __func__);
		di->rx_ss_good_lth = IDT9221_RX_SS_MAX;
	}
	hwlog_info("[%s] rx_ss_good_lth = %d\n", __func__, di->rx_ss_good_lth);
	return 0;
}
/**********************************************************
*  Function:       idtp9221_gpio_init
*  Discription:    wireless receiver gpio init, for en and sleep_en
*  Parameters:   di:idtp9221_device_info
*                      np:device_node
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_gpio_init(struct idtp9221_device_info *di, struct device_node *np)
{
	int ret = 0;

	di->gpio_en = of_get_named_gpio(np, "gpio_en", 0);
	if (!gpio_is_valid(di->gpio_en)) {
		hwlog_err("gpio_en is not valid\n");
		ret =  -EINVAL;
		goto gpio_init_fail_0;
	}
	hwlog_info("%s gpio_en = %d\n", __func__, di->gpio_en);
	ret = gpio_request(di->gpio_en, "idt9221_en");
	if (ret) {
		hwlog_err("could not request idt9221_en\n");
		ret =  -ENOMEM;
		goto gpio_init_fail_0;
	}
	gpio_direction_output(di->gpio_en, RX_EN_ENABLE);

	di->gpio_sleep_en = of_get_named_gpio(np, "gpio_sleep_en", 0);
	if (!gpio_is_valid(di->gpio_sleep_en)) {
		hwlog_err("gpio_sleep_en is not valid\n");
		ret = -EINVAL;
		goto gpio_init_fail_1;
	}
	hwlog_info("%s gpio_sleep_en = %d\n", __func__, di->gpio_sleep_en);
	ret = gpio_request(di->gpio_sleep_en, "idt9221_sleep_en");
	if (ret) {
		hwlog_err("could not request idt9221_sleep_en\n");
		ret = -ENOMEM;
		goto gpio_init_fail_1;
	}
	gpio_direction_output(di->gpio_sleep_en, RX_SLEEP_EN_DISABLE);
	return 0;

gpio_init_fail_1:
	gpio_free(di->gpio_en);
gpio_init_fail_0:
	return ret;
}
/**********************************************************
*  Function:       idtp9221_irq_init
*  Discription:    wireless receiver interrupt init
*  Parameters:   di:idt9221_device_info
*                      np:device_node
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_irq_init(struct idtp9221_device_info *di, struct device_node *np)
{
	int ret = 0;
	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("gpio_int is not valid\n");
		ret = -EINVAL;
		goto irq_init_fail_0;
	}
	hwlog_info("%s gpio_int = %d\n", __func__, di->gpio_int);
	ret = gpio_request(di->gpio_int, "idt9221_int");
	if (ret) {
		hwlog_err("could not request idt9221_int\n");
		ret = -ENOMEM;
		goto irq_init_fail_0;
	}
	gpio_direction_input(di->gpio_int);
	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("could not map idt9221 gpio_int to irq\n");
		ret = -1;
		goto irq_init_fail_1;
	}
	ret = request_irq(di->irq_int, idtp9221_interrupt,
				IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "idt9221_irq", di);
	if (ret) {
		hwlog_err("could not request idt9221_irq\n");
		di->irq_int = -1;
		goto irq_init_fail_1;
	}
	enable_irq_wake(di->irq_int);
	di->irq_active = 1;
	INIT_WORK(&di->irq_work, idtp9221_irq_work);

	return 0;

irq_init_fail_1:
	gpio_free(di->gpio_int);
irq_init_fail_0:
	return ret;
}

static struct wireless_charge_device_ops idtp9221_ops = {
	.chip_init              = idtp9221_chip_init,
	.get_tx_id              = idtp9221_get_tx_id,
	.get_rx_vrect           = idtp9221_get_rx_vrect,
	.get_rx_vout            = idtp9221_get_rx_vout,
	.get_rx_iout            = idtp9221_get_rx_iout,
	.set_tx_vout            = idtp9221_set_tx_vout,
	.set_rx_vout            = idtp9221_set_rx_vout,
	.get_rx_fop             = idtp9221_get_rx_fop,
	.get_tx_adaptor_type    = idtp9221_get_tx_type,
	.get_rx_chip_id         = idtp9221_get_chip_id,
	.get_rx_fw_version      = idtp9221_get_otp_fw_version,
	.get_rx_fod_coef        = idtp9221_get_rx_fod_coef,
	.set_rx_fod_coef        = idtp9221_set_rx_fod_coef,
	.fix_tx_fop             = idtp9221_fix_tx_fop,
	.unfix_tx_fop           = idtp9221_unfix_tx_fop,
	.rx_enable              = idtp9221_chip_enable,
	.rx_sleep_enable        = idtp9221_sleep_enable,
	.check_tx_exist         = idtp9221_check_tx_exist,
	.send_chrg_state        = idtp9221_send_charge_state,
	.kick_watchdog          = idtp9221_kick_watchdog,
	.rx_program_otp         = idtp9221_program_otp,
	.check_fwupdate         = idtp9221_check_fwupdate,
	.get_tx_capability      = idtp9221_get_tx_capability,
	.get_tx_fw_version      = idtp9221_get_tx_fw_version,
	.send_ept               = idtp9221_send_ept,
	.stop_charging          = idtp9221_stop_charging,
	.check_is_otp_exist     = idtp9221_check_is_otp_exist,
	.get_tx_cert            = idtp9221_get_tx_certification,
	.send_msg_rx_vout       = idtp9221_send_msg_rx_vout,
	.send_msg_rx_iout       = idtp9221_send_msg_rx_iout,
	.send_msg_serialno      = idtp9221_send_msg_serialno,
	.send_msg_batt_temp     = idtp9221_send_msg_batt_temp,
	.send_msg_batt_capacity = idtp9221_send_msg_batt_capacity,
	.send_msg_cert_confirm  = idtp9221_send_msg_certification_confirm,
	.send_msg_rx_boost_succ = idtp9221_send_msg_rx_boost_succ,
	.pmic_vbus_handler      = idtp9221_pmic_vbus_handler,
};

static void idtp9221_shutdown(struct i2c_client *client)
{
	hwlog_info("%s ++\n", __func__);
	if (WIRELESS_CHANNEL_ON ==
		wireless_charge_get_wireless_channel_state()) {
		idtp9221_set_tx_vout(ADAPTER_5V * MVOLT_PER_VOLT);
		idtp9221_set_rx_vout(ADAPTER_5V * MVOLT_PER_VOLT);
		msleep(IDT9221_SHUTDOWN_SLEEP_TIME);
	}
	hwlog_info("%s --\n", __func__);
}
/**********************************************************
*  Function:       idtp9221_probe
*  Discription:    wireless receiver module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int idtp9221_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct idtp9221_device_info *di = NULL;
	struct device_node *np = NULL;
	struct wireless_charge_device_ops *ops = NULL;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("idt9221_di is NULL!\n");
		return -ENOMEM;
	}
	g_idtp9221_di = di;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	ret = idtp9221_parse_dts(np, di);
	if (ret)
		goto idt9221_fail_0;

	ret = idtp9221_gpio_init(di, np);
	if (ret)
		goto idt9221_fail_0;
	ret = idtp9221_irq_init(di, np);
	if (ret)
		goto idt9221_fail_1;

	ops = &idtp9221_ops;
	wake_lock_init(&g_idtp9221_wakelock, WAKE_LOCK_SUSPEND, "idtp9221_wakelock");
	ret = wireless_charge_ops_register(ops);
	if (ret) {
		hwlog_err("%s: register wireless charge ops failed!\n", __func__);
		goto idt9221_fail_2;
	}
	if (idtp9221_check_tx_exist()) {
		idtp9221_clear_interrupt(WORD_MASK);
		hwlog_info("[%s] rx exsit, execute rx_ready_handler\n", __func__);
		idtp9221_rx_ready_handler(di);
	} else {
		gpio_set_value(di->gpio_sleep_en, RX_SLEEP_EN_ENABLE);
	}

	hwlog_info("wireless_idtp9221 probe ok!\n");
	return 0;

idt9221_fail_2:
	free_irq(di->irq_int, di);
idt9221_fail_1:
	gpio_free(di->gpio_en);
	gpio_free(di->gpio_sleep_en);
idt9221_fail_0:
	devm_kfree(&client->dev, di);
	di = NULL;
	np = NULL;
	return ret;
}
MODULE_DEVICE_TABLE(i2c, wireless_idtp9221);
static struct of_device_id idtp9221_of_match[] = {
	{
	 .compatible = "huawei, wireless_idtp9221",
	 .data = NULL,
	 },
	 {
	 },
};
static const struct i2c_device_id idtp9221_i2c_id[] = {
	{"wireless_idtp9221", 0}, {}
};

static struct i2c_driver idtp9221_driver = {
	.probe = idtp9221_probe,
	.shutdown = idtp9221_shutdown,
	.id_table = idtp9221_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "wireless_idtp9221",
		   .of_match_table = of_match_ptr(idtp9221_of_match),
		   },
};
/**********************************************************
*  Function:       idt9221_init
*  Discription:    wireless receiver module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init idtp9221_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&idtp9221_driver);
	if (ret)
		hwlog_err("%s: i2c_add_driver error!!!\n", __func__);

	return ret;
}
/**********************************************************
*  Function:       idtp9221_exit
*  Description:    wpc receiver module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit idtp9221_exit(void)
{
	i2c_del_driver(&idtp9221_driver);
}

fs_initcall_sync(idtp9221_init);
module_exit(idtp9221_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wpc  receiver module driver");
MODULE_AUTHOR("HUAWEI Inc");
