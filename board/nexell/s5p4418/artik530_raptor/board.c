/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>
#include <memalign.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#endif

#ifdef CONFIG_SENSORID_ARTIK
#include <sensorid.h>
#include <sensorid_artik.h>
#endif

#ifdef CONFIG_ARTIK_OTA
#include <artik_ota.h>
#endif

#ifdef CONFIG_ARTIK_MAC
#include <artik_mac.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define TYPE_A530	0
#define TYPE_A533	1
#define BOARD_COMPY	0
#define BOARD_RAPTOR	1

#ifdef CONFIG_CHECK_BONDING_ID
int check_bonding_id(void)
{
	int bonding_id = 0;

	bonding_id = readl(PHY_BASEADDR_ECID + ID_REG_EC0);

	return bonding_id & WIRE0_MASK;
}
#endif

#ifdef CONFIG_CHECK_BOARD_TYPE
int check_board_type(void)
{	int val = 0;

	val |= (nx_gpio_get_input_value(1, 25) << 1);
	val |= nx_gpio_get_input_value(1, 24);

	return val;
}
#endif

#ifdef CONFIG_SUPPORT_COMPY_BOARD
int check_sub_board(void)
{
	return nx_gpio_get_input_value(1, 30);
}
#endif

#ifdef CONFIG_REVISION_TAG
u32 board_rev;

u32 get_board_rev(void)
{
	return board_rev;
}

static void check_hw_revision(void)
{
	u32 val = 0;

#if 0 /* HBAHN */
	val |= nx_gpio_get_input_value(4, 6);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 5);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 4);
#endif /* HBAHN */

	board_rev = val;
}

static void set_board_rev(u32 revision)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", revision);
	setenv("board_rev", info);
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	char board_type[20] = "ARTIK530 Raptor";
#ifdef CONFIG_CHECK_BONDING_ID
	int is_dual = check_bonding_id();

	if (is_dual) {
		memset(board_type, 0, 20);
		strncpy(board_type, "ARTIK532 Raptor", 20);
	}
#endif
#ifdef CONFIG_CHECK_BOARD_TYPE
	if (check_board_type() == TYPE_A533) {
		memset(board_type, 0, 20);
		strncpy(board_type, "ARTIK533 Raptor", 20);
	}
#endif
	printf("\nBoard: %s\n", board_type);

	return 0;
}
#endif

#ifdef CONFIG_SENSORID_ARTIK
static void get_sensorid(u32 revision)
{
	static struct udevice *dev;
	uint16_t buf[5] = {0, };
	char panel_env[64], *panel_str;
	bool found_panel = false;
	int i, ret;

	ret = uclass_get_device_by_name(UCLASS_SENSOR_ID, "sensor_id@36", &dev);
	if (ret < 0) {
		printf("Cannot find sensor_id device\n");
		return;
	}

	ret = sensorid_get_type(dev, &buf[0], 4);
	if (ret < 0) {
		printf("Cannot read sensor type - %d\n", ret);
		return;
	}

	ret = sensorid_get_addon(dev, &buf[4]);
	if (ret < 0) {
		printf("Cannot read add-on board type - %d\n", ret);
		return;
	}

	printf("LCD#1:0x%X, LCD#2:0x%X, CAM#1:0x%X, CAM#2:0x%X\n",
			buf[0], buf[1], buf[2], buf[3]);
	printf("ADD-ON-BOARD : 0x%X\n", buf[4]);

	for (i = 0; i < SENSORID_LCD_MAX; i++) {
		if (buf[i] != SENSORID_LCD_NONE) {
			snprintf(panel_env, sizeof(panel_env), "lcd%d_%d",
				 i + 1, buf[i]);
			panel_str = getenv(panel_env);
			if (panel_str) {
				setenv("lcd_panel", panel_str);
				found_panel = true;
			}
			break;
		}
	}

	if (!found_panel)
		setenv("lcd_panel", "NONE");
}
#endif

#ifdef CONFIG_SET_DFU_ALT_INFO
void set_dfu_alt_info(char *interface, char *devstr)
{
	size_t buf_size = CONFIG_SET_DFU_ALT_BUF_LEN;
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, buf_size);

	snprintf(buf, buf_size, "setenv dfu_alt_info \"%s\"", CONFIG_DFU_ALT);
	run_command(buf, 0);
}
#endif

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void nx_phy_init(void)
{
#ifdef CONFIG_SENSORID_ARTIK
	/* I2C-GPIO for AVR */
	nx_gpio_set_pad_function(1, 11, 2);
	nx_gpio_set_pad_function(1, 18, 2);
#endif
}

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
}

/* Set the environment value about booting from which device */
static void set_booting_device(void)
{
	int port_num;
	int boot_mode = readl(PHY_BASEADDR_CLKPWR + SYSRSTCONFIG);

	if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_SDMMC) {
		port_num = readl(SCR_ARM_SECOND_BOOT_REG1);
		/*
		 * Kernel will be detected as the below block devices.
		 * mmcblk0 - eMMC card
		 * mmcblk1 - SD card
		 */
		if (port_num == EMMC_PORT_NUM)
			setenv("rootdev", "0");
		else if (port_num == SD_PORT_NUM)
			setenv("rootdev", "1");
	}
}

int mmc_get_env_dev(void)
{
	int port_num;
	int boot_mode = readl(PHY_BASEADDR_CLKPWR + SYSRSTCONFIG);

	if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_SDMMC) {
		//port_num = readl(PHY_BASEADDR_SRAM + DEVICEBOOTINFO);
		port_num = readl(SCR_ARM_SECOND_BOOT_REG1);
		if (port_num == EMMC_PORT_NUM)
			return 0;
		else if (port_num == SD_PORT_NUM)
			return 1;
	} else if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_USB) {
		return 0;
	}

	return -1;
}

int board_init(void)
{
#ifdef CONFIG_REVISION_TAG
	check_hw_revision();
	printf("HW Revision:\t%d\n", board_rev);
#endif

	nx_phy_init();

	return 0;
}

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

static void check_reboot_mode(void)
{
	u32 val;

	val = readl(PHY_BASEADDR_ALIVE + SCRATCHREADREG7);

	/* Clear the reboot mode */
	writel(0xFFFFFFFF, PHY_BASEADDR_ALIVE + SCRATCHRSTREG7);
	writel(0x0, PHY_BASEADDR_ALIVE + SCRATCHSETREG7);

	if ((val & REBOOT_PREFIX_MASK) == REBOOT_PREFIX) {
		val &= ~REBOOT_PREFIX;

		if (val == REBOOT_DOWNLOAD)
			run_command("thordown", 0);
		else if (val == REBOOT_RECOVERY)
			setenv("bootmode", "recovery");
		else if (val == REBOOT_FOTA)
			setenv("bootmode", "fota");
		else
			setenv("bootmode", "ramdisk");

		saveenv();
	}
}

static int board_set_regulator(const char *name, uint uv)
{
        struct udevice *dev;
        int ret;

        ret = regulator_get_by_platname(name, &dev);
        if (ret) {
                debug("%s: Cannot find regulator %s\n", __func__, name);
                return ret;
        }
        ret = regulator_set_value(dev, uv);
        if (ret) {
                debug("%s: Cannot set regulator %s\n", __func__, name);
                return ret;
        }

        return 0;
}

#ifdef CONFIG_DM_PMIC_NXE2000
void pmic_init(void)
{
	struct udevice *pmic;
	static struct udevice *dev;
	int ret = -ENODEV;
	uint8_t bit_mask = 0;
#ifdef CONFIG_DM_REGULATOR
	struct dm_regulator_uclass_platdata *reg_uc_pdata;
	struct udevice *regulator;
#endif

	ret = pmic_get("nxe2000_gpio@32", &pmic);
	if (ret)
		printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");

	if (device_has_children(pmic)) {
#ifdef CONFIG_DM_REGULATOR
		for (ret = uclass_find_first_device(UCLASS_REGULATOR, &dev);
			dev;
			ret = uclass_find_next_device(&dev)) {
			if (ret)
			continue;

			reg_uc_pdata = dev_get_uclass_platdata(dev);
			if (!reg_uc_pdata)
				continue;

			uclass_get_device_tail(dev, 0, &regulator);
		}
#endif
	}

	ret = regulators_enable_boot_on(false);
	if (ret)
		printf("Can't enable PMIC: %s!\n", "nxe2000_gpio@32");

	/* set CPU voltage to 1.1v */
	ret = board_set_regulator("dcdc1", 1100000);
	if (ret)
		printf("Can't set regulator : %s!\n", "dcdc1");


	/* set core voltage to 1.05v, avaliable range 1V ~ 1.1V */
	ret = board_set_regulator("dcdc2", 1050000);
	if (ret)
		printf("Can't set regulator : %s!\n", "dcdc2");

	/* set DCDC3 voltage to 3.3v */
	ret = board_set_regulator("dcdc3", 3300000);
	if (ret)
		printf("Can't set regulator : %s!\n", "dcdc3");

	/* set DCDC4 voltage to 1.5v */
	ret = board_set_regulator("dcdc4", 1500000);
	if (ret)
		printf("Can't set regulator : %s!\n", "dcdc4");

	/* set DCDC5 voltage to 1.5v */
	ret = board_set_regulator("dcdc5", 1500000);
	if (ret)
		printf("Can't set regulator : %s!\n", "dcdc5");

}
#endif

int board_late_init(void)
{
#ifdef CONFIG_CHECK_BONDING_ID
	int is_dual = check_bonding_id();
	int nr_cpus = 4;

	if (is_dual) {
		nr_cpus = 2;
		setenv_ulong("model_id", 532);
	}

	setenv_ulong("nr_cpus", nr_cpus);
#endif

#ifdef CONFIG_CHECK_BOARD_TYPE
	if (check_board_type() == TYPE_A533)
		setenv_ulong("model_id", 533);
#endif
#ifdef CONFIG_SUPPORT_COMPY_BOARD
	if (check_sub_board() == BOARD_COMPY) {
		setenv("board_type" ,"compy");
		/* Clear Indicator LED */
		nx_gpio_set_pad_function(1, 11, 2);
		nx_gpio_set_pad_function(4, 31, 1);
		nx_gpio_set_output_enable(1, 11, 1);
		nx_gpio_set_output_enable(4, 31, 1);
		nx_gpio_set_output_value(1, 11, 0);
		nx_gpio_set_output_value(4, 31, 0);
	}
#endif
#ifdef CONFIG_DM_PMIC_NXE2000
	pmic_init();
#endif

#ifdef CONFIG_REVISION_TAG
	set_board_rev(board_rev);
#endif
#ifdef CONFIG_CMD_FACTORY_INFO
	run_command("run factory_load", 0);
#endif
#ifdef CONFIG_ARTIK_MAC
	generate_mac();
#endif
#ifdef CONFIG_SENSORID_ARTIK
#ifdef CONFIG_SUPPORT_COMPY_BOARD
	if (check_sub_board() == BOARD_RAPTOR)
		get_sensorid(board_rev);
#else
	get_sensorid(board_rev);
#endif
#endif
#ifdef CONFIG_ARTIK_OTA
	check_ota_update();
#endif
	check_reboot_mode();

	set_booting_device();

	return 0;
}

#ifdef CONFIG_USB_GADGET
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_thor")) {
		put_unaligned(CONFIG_G_DNL_THOR_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_THOR_PRODUCT_NUM, &dev->idProduct);
	} else if (!strcmp(name, "usb_dnl_ums")) {
		put_unaligned(CONFIG_G_DNL_UMS_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_UMS_PRODUCT_NUM, &dev->idProduct);
	} else {
		put_unaligned(CONFIG_G_DNL_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_PRODUCT_NUM, &dev->idProduct);
	}
	return 0;
}
#endif
