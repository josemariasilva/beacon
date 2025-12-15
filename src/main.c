#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#define COMPANY_ID_CODE 0x0000

#define I2C_NODE DT_NODELABEL(lis2dh12)
#define LED0_NODE1 DT_ALIAS(led0)
#define LED0_NODE2 DT_ALIAS(led1)
#define LED0_NODE3 DT_ALIAS(led2)

#define SLEEP_TIME_MS 200

LOG_MODULE_REGISTER(Test, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define SPIOP      SPI_WORD_SET(8) | SPI_TRANSFER_MSB

struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_NODELABEL(lis2dh12), SPIOP, 0);

// Lisdh12 register

#define WHO_AM_I 0x0F
#define CHIP_ID 0x33
#define CTRL_REG1 0x20 // enable axis, set rate
#define CTRL_REG3 0x22 // wake-up + moviment
#define CTRL_REG4 0x23 // set scale 2g, 4g, 8g, 16g and op mode
#define CTRL_REG5 0x24 // enable fifo mode
#define CTRL_REG6 0x25 // FIFO watermark
#define FIFO_CTRL_REG 0x2E // watermarks -> 16 samples
#define INT1_CFG 0x30 // wake-up when moviment all axis
#define INT1_THS 0x32 // Threshold limiar =! 250mg(LP)
#define INT1_DURATION 0x33 // duration 
#define ACT_THS 0x3E // enalble low power mode and set threshold
#define ACT_DUR 0x3F // set duration (xHz * seconds)


struct lis2dh_data {
    double ax;
	double ay;
	double az;
}lis2dh_data;

typedef struct adv_mfg_data {
	uint8_t company_code; 
	uint8_t status; 
} adv_mfg_data_type;



static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE,800,801,NULL);

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED0_NODE1, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED0_NODE2, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE3, gpios);

static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, 1 };

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	/* STEP 3 - Include the Manufacturer Specific Data in the advertising packet. */

	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),

};

static const struct bt_data sd[] = {
	//BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};

void lis2dh12_configuration(void){



};

static int lis2dh12_write_reg(uint8_t reg, uint8_t value)
{
	int err;

	/* STEP 5.1 - declare a tx buffer having register address and data */
	uint8_t tx_buf[] = {(reg & 0x7F), value};	
	struct spi_buf 		tx_spi_buf 		= {.buf = tx_buf, .len = sizeof(tx_buf)};
	struct spi_buf_set 	tx_spi_buf_set	= {.buffers = &tx_spi_buf, .count = 1};

	/* STEP 5.2 - call the spi_write_dt function with SPISPEC to write buffers */
	err = spi_write_dt(&spispec, &tx_spi_buf_set);
	if (err < 0) {
		LOG_ERR("spi_write_dt() failed, err %d", err);
		return err;
	}

	return 0;
}

static int lis2dh12_read_reg(uint8_t reg, uint8_t *data, uint8_t size)
{
	int err;

	/* STEP 4.1 - Set the transmit and receive buffers */
	uint8_t tx_buffer = reg;
	struct spi_buf tx_spi_buf			= {.buf = (void *)&tx_buffer, .len = 1};
	struct spi_buf_set tx_spi_buf_set 	= {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf rx_spi_bufs 			= {.buf = data, .len = size};
	struct spi_buf_set rx_spi_buf_set	= {.buffers = &rx_spi_bufs, .count = 1};

	/* STEP 4.2 - Call the transceive function */
	err = spi_transceive_dt(&spispec, &tx_spi_buf_set, &rx_spi_buf_set);
	if (err < 0) {
		LOG_ERR("spi_transceive_dt() failed, err: %d", err);
		return err;
	}

	return 0;
}

int lis2dh12_read_fifo_samples(void)
{

	int err;

	int32_t datap = 0, datat = 0, datah = 0;
	float pressure = 0.0f, temperature = 0.0f, humidity = 0.0f;

	/* STEP 9.1 - Store register addresses to do burst read */
	uint8_t regs[] = {PRESSMSB, PRESSLSB, PRESSXLSB, \
					  TEMPMSB, TEMPLSB, TEMPXLSB, \
					  HUMMSB, HUMLSB, DUMMY};	//0xFF is dummy reg
	uint8_t readbuf[sizeof(regs)];

	/* STEP 9.2 - Set the transmit and receive buffers */
	struct spi_buf 		tx_spi_buf 			= {.buf = (void *)&regs, .len = sizeof(regs)};
	struct spi_buf_set 	tx_spi_buf_set		= {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf 		rx_spi_bufs			= {.buf = readbuf, .len = sizeof(regs)};
	struct spi_buf_set 	rx_spi_buf_set	= {.buffers = &rx_spi_bufs, .count = 1};

	/* STEP 9.3 - Use spi_transceive() to transmit and receive at the same time */
	err = spi_transceive_dt(&spispec, &tx_spi_buf_set, &rx_spi_buf_set);
	if (err < 0) {
		LOG_ERR("spi_transceive_dt() failed, err: %d", err);
		return err;
	}

	// continue...

	
	return 0;
}

int main(void)
{

    int err;


	err = spi_is_ready_dt(&spispec);
	if (!err) {
		LOG_ERR("Error: SPI device is not ready, err: %d", err);
		return 0;
	}
    
	
    err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
	 	LOG_ERR("Advertising failed to start (err %d)\n", err);
	 	return -1;
	}
	
	LOG_INF("Advertising successfully started\n");
	
	for (;;)
		{
			k_msleep(SLEEP_TIME_MS);
		}
	return 0;
}
