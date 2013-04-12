/*****************************************************************************
*  Copyright 2011 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/
#define PMU_DEVICE_I2C_ADDR	0x08
#define PMU_DEVICE_I2C_ADDR1	0x0C
#define PMU_INT_IRQ_NUM		BCM_INT_ID_PMU_INT
#define PMU_DEVICE_I2C_BUSNO	2

static struct bcmpmu_rw_data register_init_data[] = {
	{.map = 0, .addr = 0x14, .val = 0x75, .mask = 0xFF},
	/* set PC3_EN */
	{.map = 0, .addr = 0x15, .val = 0x0C, .mask = 0x02},
	{.map = 0, .addr = 0xC9, .val = 0x30, .mask = 0xFF},
	{.map = 0, .addr = 0x30, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x31, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x32, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x33, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x34, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x35, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x36, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x37, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x38, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x39, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3A, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3B, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3C, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3D, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3E, .val = 0xFF, .mask = 0xFF},
	{.map = 0, .addr = 0x3F, .val = 0xFF, .mask = 0xFF},

	{.map = 0, .addr = 0x40, .val = 0x38, .mask = 0x38},
	{.map = 0, .addr = 0x41, .val = 0x00, .mask = 0x04},
	{.map = 0, .addr = 0x42, .val = 0x04, .mask = 0x04},
	{.map = 0, .addr = 0x45, .val = 0x0E, .mask = 0xFF},
	{.map = 0, .addr = 0x49, .val = 0x02, .mask = 0xFF},
	{.map = 0, .addr = 0x47, .val = 0x05, .mask = 0xFF},
	{.map = 1, .addr = 0xCC, .val = 0x43, .mask = 0xFF},
	{.map = 1, .addr = 0xCD, .val = 0x7F, .mask = 0xFF},
	{.map = 1, .addr = 0xCE, .val = 0x3B, .mask = 0xFF},
	{.map = 1, .addr = 0xCF, .val = 0xF8, .mask = 0xFF},
	{.map = 1, .addr = 0xD0, .val = 0x09, .mask = 0xFF},

	{.map = 1, .addr = 0x47, .val = 0xD2, .mask = 0xFF},
	{.map = 1, .addr = 0x48, .val = 0x98, .mask = 0xFF},
	{.map = 1, .addr = 0x49, .val = 0x04, .mask = 0xFF},
	{.map = 1, .addr = 0x4A, .val = 0x58, .mask = 0xFF},
	{.map = 1, .addr = 0x4B, .val = 0xC3, .mask = 0xFF},

/* Regulator configuration */
	{.map = 0, .addr = 0x15, .val = 0x0e, .mask = 0xff},//enable PC3, SIMLDO1EN and SIMLDO2EN
	{.map = 0, .addr = 0x82, .val = 0x01, .mask = 0xff},//sdsr1 opmode
	{.map = 0, .addr = 0x83, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x84, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0x85, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0xCC, .val = 0x24, .mask = 0xff},//sdsr1 nm1 1.2v
	{.map = 0, .addr = 0xCD, .val = 0x24, .mask = 0xff},//sdsr1 lpm 0.9v
	{.map = 0, .addr = 0x86, .val = 0x00, .mask = 0xff},//sdsr2 opmode
	{.map = 0, .addr = 0x87, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x88, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x89, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0xCF, .val = 0x24, .mask = 0xff},//sdsr2 nm1 1.2v
	{.map = 0, .addr = 0xD0, .val = 0x24, .mask = 0xff},//sdsr2 lpm 0.9v
	{.map = 0, .addr = 0x7A, .val = 0x01, .mask = 0xff},//iosr1 opmode
	{.map = 0, .addr = 0x7B, .val = 0x11, .mask = 0xff},
	{.map = 0, .addr = 0xC3, .val = 0x35, .mask = 0xff},//iosr1 nm1 1.8v
	{.map = 0, .addr = 0xC4, .val = 0x35, .mask = 0xff},//iosr1 lpm 1.8v
	{.map = 0, .addr = 0x7C, .val = 0x01, .mask = 0xff},//iosr2 opmode
	{.map = 0, .addr = 0x7D, .val = 0x11, .mask = 0xff},
	{.map = 0, .addr = 0xC6, .val = 0x35, .mask = 0xff},//iosr2 nm1 1.8v
	{.map = 0, .addr = 0xC7, .val = 0x35, .mask = 0xff},//iosr2 lpm 1.8v
	{.map = 0, .addr = 0x7E, .val = 0x00, .mask = 0xff},//csr opmode
	{.map = 0, .addr = 0x7F, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x80, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x81, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0xC1, .val = 0x00, .mask = 0xff},//csr lpm 0.7v
	{.map = 0, .addr = 0x8E, .val = 0x02, .mask = 0xff},//vsr opmode
	{.map = 0, .addr = 0x8F, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x90, .val = 0x02, .mask = 0xff},
	{.map = 0, .addr = 0x91, .val = 0x02, .mask = 0xff},
	{.map = 0, .addr = 0xD2, .val = 0x24, .mask = 0xff},//vsr nm1 1.2v
	{.map = 0, .addr = 0xD3, .val = 0x00, .mask = 0xff},//vsr lpm 0.7v(off)
	{.map = 0, .addr = 0x8A, .val = 0x09, .mask = 0xff},//msr opmode
	{.map = 0, .addr = 0x8B, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0x8C, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0x8D, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0xC9, .val = 0x1A, .mask = 0xff},//msr nm1 1.1v
	{.map = 0, .addr = 0xCA, .val = 0x01, .mask = 0xff},//msr lpm 0.9v

	{.map = 0, .addr = 0x60, .val = 0x22, .mask = 0xff},//rf opmode
	{.map = 0, .addr = 0x61, .val = 0x22, .mask = 0xff},
	{.map = 0, .addr = 0x96, .val = 0x9E, .mask = 0xff},//rfldo 2.7v
	{.map = 0, .addr = 0x72, .val = 0x02, .mask = 0xff},//audldo
	{.map = 0, .addr = 0x73, .val = 0x22, .mask = 0xff},
	{.map = 0, .addr = 0x9F, .val = 0xAF, .mask = 0xff},//audldo 2.9v
	{.map = 0, .addr = 0x62, .val = 0x01, .mask = 0xff},//camldo1
	{.map = 0, .addr = 0x63, .val = 0x01, .mask = 0xff},
	{.map = 0, .addr = 0x97, .val = 0xBE, .mask = 0xff},//camldo1 3.3v
	{.map = 0, .addr = 0x64, .val = 0x02, .mask = 0xff},//camldo2
	{.map = 0, .addr = 0x65, .val = 0x02, .mask = 0xff},
#if defined (CONFIG_MACH_CAPRI_SS_BAFFIN)
	{.map = 0, .addr = 0x98, .val = 0xBC, .mask = 0xff},//camldo2 3.3v

#else
	{.map = 0, .addr = 0x98, .val = 0x9C, .mask = 0xff},//camldo2 2.7v
#endif
	{.map = 0, .addr = 0x76, .val = 0x01, .mask = 0xff},//usbldo
	{.map = 0, .addr = 0x77, .val = 0x11, .mask = 0xff},
	{.map = 0, .addr = 0xA1, .val = 0xB9, .mask = 0xff},//usbldo 3.3v
	{.map = 0, .addr = 0x99, .val = 0x88, .mask = 0xff},
	{.map = 0, .addr = 0x68, .val = 0xAA, .mask = 0xff},//simldo2 off
	{.map = 0, .addr = 0x69, .val = 0xAA, .mask = 0xff},
	{.map = 0, .addr = 0x9A, .val = 0xB0, .mask = 0xff},
	{.map = 0, .addr = 0x78, .val = 0xAA, .mask = 0xff},//vibldo, off
	{.map = 0, .addr = 0x79, .val = 0xAA, .mask = 0xff},
	{.map = 0, .addr = 0xA2, .val = 0xAF, .mask = 0xff},
	{.map = 0, .addr = 0x9B, .val = 0xB4, .mask = 0xff},//sdldo 3v
	{.map = 0, .addr = 0x6E, .val = 0x00, .mask = 0xff},//mmcldo1
	{.map = 0, .addr = 0x6F, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x9D, .val = 0xA0, .mask = 0xff},//mmcldo1 2.8v
	{.map = 0, .addr = 0x70, .val = 0x00, .mask = 0xff},//mmcldo2
	{.map = 0, .addr = 0x71, .val = 0x00, .mask = 0xff},
	{.map = 0, .addr = 0x9E, .val = 0xA0, .mask = 0xff},//mmcldo2 2.8v
	{.map = 0, .addr = 0x9C, .val = 0x8D, .mask = 0xff},//sdxldo 1.8v
	{.map = 0, .addr = 0x74, .val = 0xAA, .mask = 0xff},//micldo off
	{.map = 0, .addr = 0x75, .val = 0xAA, .mask = 0xff},
#if defined (CONFIG_CAPRI_SS_REV03) || (CONFIG_MACH_CAPRI_SS_BAFFIN) || (CONFIG_MACH_CAPRI_SS_ARUBA)
	{.map = 1, .addr = 0x57, .val = 0xAA, .mask = 0xff},
	{.map = 1, .addr = 0x58, .val = 0x22, .mask = 0xff},
	{.map = 1, .addr = 0x59, .val = 0xAA, .mask = 0xff},
	{.map = 1, .addr = 0x5A, .val = 0x22, .mask = 0xff},
	{.map = 1, .addr = 0x5B, .val = 0xAA, .mask = 0xff},
	{.map = 1, .addr = 0x5C, .val = 0x22, .mask = 0xff},
	{.map = 1, .addr = 0x5D, .val = 0xAA, .mask = 0xff},
	{.map = 1, .addr = 0x5E, .val = 0x22, .mask = 0xff},
	{.map = 1, .addr = 0x16, .val = 0x02, .mask = 0xff},
	{.map = 1, .addr = 0x17, .val = 0x02, .mask = 0xff},
	{.map = 1, .addr = 0x18, .val = 0x02, .mask = 0xff},
	{.map = 1, .addr = 0x19, .val = 0x02, .mask = 0xff},
#endif	

/**/
	{.map = 1, .addr = 0xBF, .val = 0x64, .mask = 0xFF},
     // FG CIC
	{.map = 1, .addr = 0xA3, .val = 0x02, .mask = 0xFF},
	{.map = 1, .addr = 0xEF, .val = 0x11, .mask = 0xFF},	

	/* PWMLED blovk powerdown */
	{.map = 1, .addr = 0x00, .val = 0x23, .mask = 0xFF},

	/* NTCBIAS  Sync Mode + set part of OpMode*/
	{.map = 1, .addr = 0xD5, .val = 0x13, .mask = 0xFF},


	/* NTCBIAS  part of OPmode*/
	{.map = 1, .addr = 0xD6, .val = 0x01, .mask = 0xFF},

	/* RIDBIAS (BSI) Sync Mode + set part of OpMode */
	{.map = 1, .addr = 0xD7, .val = 0x13, .mask = 0xFF},

	/* Disable MB working voltage comparator */
	/* RIDBIAS  part of OPmode*/
	{.map = 1, .addr = 0xD8, .val = 0x01, .mask = 0xFF},

};

static struct bcmpmu_temp_map batt_temp_map[] = {
/* This table is hardware dependent and need to get from platform team */
/*	adc		temp*/
	{932, -400},			/* -40 C */
	{900, -350},			/* -35 C */
	{869, -300},            /* -30 */
	{754, -200},			/* -20 */
	{709, -150},			/* -15 */
	{644, -100},                    /* -10 */
	{577, -50},						/* -5 */
	{510,   0},                    /* 0   */
	{445,   50},                    /* 0   */
	{383,  100},                    /* 10  */
	{329,  150},                    /* 15  */
	{279,  200},                    /* 20  */
	{234,  250},                    /* 25  */
	{197,  300},                    /* 30  */
	{164,  350},                    /* 30  */
	{137,  400},                    /* 40  */
	{115,  450},                    /* 40  */
	{96 ,  500},                    /* 50  */
	{81 ,  550},                    /* 50  */
	{68 ,  600},                    /* 60  */
	{57 ,  650},                    /* 65  */
	{48 ,  700},            /* 70  */
	{34 ,  800},            /* 80  */
	{28, 850},			/* 85 C */
	{24, 900},			/* 90 C */
	{20, 950},			/* 95 C */
	{16, 1000},			/* 100 C */
};

struct regulator_consumer_supply usb_supply[] = {
	{ .supply = "usbldo_uc"},
};
static struct regulator_init_data bcm59056_usbldo_data =  {
	.constraints = {
		.name = "usbldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_MODE |
			REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 0, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(usb_supply),
	.consumer_supplies = usb_supply,
};

struct regulator_consumer_supply rf_supply[] = {
	{ .supply = "rfldo_uc"},
};
static struct regulator_init_data bcm59056_rfldo_data =  {
	.constraints = {
		.name = "rfldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_MODE |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(rf_supply),
	.consumer_supplies = rf_supply,
};

struct regulator_consumer_supply cam1_supply[] = {
	{.supply = "camldo1_uc"},
	REGULATOR_SUPPLY("vddmmc", "sdhci.1"), /* 0x3f1b0000.sdhci */
	
};
static struct regulator_init_data bcm59056_camldo1_data = {
	.constraints = {
		.name = "camldo1",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 0, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(cam1_supply),
	.consumer_supplies = cam1_supply,
};

struct regulator_consumer_supply cam2_supply[] = {
	{.supply = "camldo2_uc"},
	REGULATOR_SUPPLY("lcd_ldo2", NULL),
};
static struct regulator_init_data bcm59056_camldo2_data = {
	.constraints = {
		.name = "camldo2",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(cam2_supply),
	.consumer_supplies = cam2_supply,
};


struct regulator_consumer_supply sd_supply[] = {
	{.supply = "sdldo_uc"},
	REGULATOR_SUPPLY("vddmmc", "sdhci.3"), /* 0x3f1b0000.sdhci */
};
static struct regulator_init_data bcm59056_sdldo_data = {
	.constraints = {
		.name = "sdldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE |  REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(sd_supply),
	.consumer_supplies = sd_supply,
};

struct regulator_consumer_supply sdx_supply[] = {
	{.supply = "sdxldo_uc"},
	REGULATOR_SUPPLY("vddo", "sdhci.3"), /* 0x3f1b0000.sdhci */
};
static struct regulator_init_data bcm59056_sdxldo_data = {
	.constraints = {
		.name = "sdxldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(sdx_supply),
	.consumer_supplies = sdx_supply,
};

struct regulator_consumer_supply vib_supply[] = {
	{.supply = "vibldo_uc"},
};
static struct regulator_init_data bcm59056_vibldo_data = {
	.constraints = {
		.name = "vibldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(vib_supply),
	.consumer_supplies = vib_supply,
};

struct regulator_consumer_supply aud_supply[] = {
	{.supply = "audldo_uc"},
};
static struct regulator_init_data bcm59056_audldo_data = {
	.constraints = {
		.name = "audldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(aud_supply),
	.consumer_supplies = aud_supply,
};

struct regulator_consumer_supply mic_supply[] = {
	{.supply = "micldo_uc"},
};
static struct regulator_init_data bcm59056_micldo_data = {
	.constraints = {
		.name = "micldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(mic_supply),
	.consumer_supplies = mic_supply,
};

struct regulator_consumer_supply mmc1_supply[] = {
	{.supply = "mmcldo1_uc"},
};
static struct regulator_init_data bcm59056_mmcldo1_data = {
	.constraints = {
		.name = "mmcldo1",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(mmc1_supply),
	.consumer_supplies = mmc1_supply,
};

struct regulator_consumer_supply mmc2_supply[] = {
	{.supply = "mmcldo2_uc"},
};
static struct regulator_init_data bcm59056_mmcldo2_data = {
	.constraints = {
		.name = "mmcldo2",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(mmc2_supply),
	.consumer_supplies = mmc2_supply,
};

struct regulator_consumer_supply sim_supply[] = {
	{.supply = "sim_vcc"},
};
static struct regulator_init_data bcm59056_simldo_data = {
	.constraints = {
		.name = "simldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(sim_supply),
	.consumer_supplies = sim_supply,
};


struct regulator_consumer_supply sim2_supply[] = {
	{.supply = "sim2_vcc"},
};
static struct regulator_init_data bcm59056_sim2ldo_data = {
	.constraints = {
		.name = "sim2ldo",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(sim2_supply),
	.consumer_supplies = sim2_supply,
};

struct regulator_consumer_supply hdmi_supply[] = {
	{ .supply = "hdmi_uc"},
};
static struct regulator_init_data bcm59056_hdmildo_data =  {
	.constraints = {
		.name = "hdmildo",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(hdmi_supply),
	.consumer_supplies = hdmi_supply,
};

struct regulator_consumer_supply csr_supply[] = {
	{.supply = "csr_uc"},
};
static struct regulator_init_data bcm59056_csr_data = {
	.constraints = {
		.name = "csr",
		.min_uV = 700000,
		.max_uV = 1360000,
		.valid_ops_mask = REGULATOR_CHANGE_MODE |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(csr_supply),
	.consumer_supplies = csr_supply,
};

struct regulator_consumer_supply mmsr_supply[] = {
	{.supply = "mmsr_uc"},
};
static struct regulator_init_data bcm59056_mmsr_data = {
	.constraints = {
		.name = "mmsr",
		.min_uV = 700000,
		.max_uV = 1800000,
		.valid_ops_mask = REGULATOR_CHANGE_MODE |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(mmsr_supply),
	.consumer_supplies = mmsr_supply,
};

struct regulator_consumer_supply vsr_supply[] = {
	{.supply = "vsr_uc"},
};
static struct regulator_init_data bcm59056_vsr_data = {
	.constraints = {
		.name = "vsr",
		.min_uV = 700000,
		.max_uV = 1800000,
		.valid_ops_mask = REGULATOR_CHANGE_MODE |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(vsr_supply),
	.consumer_supplies = vsr_supply,
};

struct regulator_consumer_supply gp1_supply[] = {
	{.supply = "gpldo1_uc"},
};
static struct regulator_init_data bcm59056_gpldo1_data = {
	.constraints = {
		.name = "gpldo1",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp1_supply),
	.consumer_supplies = gp1_supply,
};

struct regulator_consumer_supply gp2_supply[] = {
	{.supply = "gpldo2_uc"},
};
static struct regulator_init_data bcm59056_gpldo2_data = {
	.constraints = {
		.name = "gpldo2",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp2_supply),
	.consumer_supplies = gp2_supply,
};

struct regulator_consumer_supply gp3_supply[] = {
	{.supply = "gpldo3_uc"},
};
static struct regulator_init_data bcm59056_gpldo3_data = {
	.constraints = {
		.name = "gpldo3",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp3_supply),
	.consumer_supplies = gp3_supply,
};

struct regulator_consumer_supply gp4_supply[] = {
	{.supply = "gpldo4_uc"},
};
static struct regulator_init_data bcm59056_gpldo4_data = {
	.constraints = {
		.name = "gpldo4",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp4_supply),
	.consumer_supplies = gp4_supply,
};

struct regulator_consumer_supply gp5_supply[] = {
	{.supply = "gpldo5_uc"},
	REGULATOR_SUPPLY("tsp_vdd", NULL), /* TSP AVDD */
};
static struct regulator_init_data bcm59056_gpldo5_data = {
	.constraints = {
		.name = "gpldo5",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp5_supply),
	.consumer_supplies = gp5_supply,
};

struct regulator_consumer_supply gp6_supply[] = {
	{.supply = "gpldo6_uc"},
	REGULATOR_SUPPLY("tsp_avdd", NULL), /* TSP AVDD */
};
static struct regulator_init_data bcm59056_gpldo6_data = {
	.constraints = {
		.name = "gpldo6",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
		REGULATOR_CHANGE_MODE | REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.state_standby = {
			.disabled = 1, /* 0 for LPM, 1 for OFF */
		},
	},
	.num_consumer_supplies = ARRAY_SIZE(gp6_supply),
	.consumer_supplies = gp6_supply,
};

struct bcmpmu_regulator_init_data bcm59056_regulators[BCMPMU_REGULATOR_MAX] = {
	[BCMPMU_REGULATOR_RFLDO] = {
		BCMPMU_REGULATOR_RFLDO, &bcm59056_rfldo_data, 0x2A, 0
	},
	[BCMPMU_REGULATOR_CAMLDO] = {
		BCMPMU_REGULATOR_CAMLDO, &bcm59056_camldo1_data, 0xff, 0
	},
	[BCMPMU_REGULATOR_CAMLDO2] = {
		BCMPMU_REGULATOR_CAMLDO2, &bcm59056_camldo2_data, 0x02, 1
	},
	[BCMPMU_REGULATOR_SDLDO] = {
		BCMPMU_REGULATOR_SDLDO, &bcm59056_sdldo_data, 0x02, BCMPMU_REGL_OFF_IN_DSM
	},
	[BCMPMU_REGULATOR_SDXLDO] = {
		BCMPMU_REGULATOR_SDXLDO, &bcm59056_sdxldo_data, 0x02, BCMPMU_REGL_OFF_IN_DSM
	},
	[BCMPMU_REGULATOR_VIBLDO] = {
		BCMPMU_REGULATOR_VIBLDO, &bcm59056_vibldo_data, 0xAA, 0
	},
	[BCMPMU_REGULATOR_AUDLDO] = {
		BCMPMU_REGULATOR_AUDLDO, &bcm59056_audldo_data, 0xff, 0
	},
	[BCMPMU_REGULATOR_MICLDO] = {
		BCMPMU_REGULATOR_MICLDO, &bcm59056_micldo_data, 0xAA, 0
	},
	[BCMPMU_REGULATOR_MMCLDO1] = {
		BCMPMU_REGULATOR_MMCLDO1, &bcm59056_mmcldo1_data, 0x02, BCMPMU_REGL_OFF_IN_DSM
	},
	[BCMPMU_REGULATOR_MMCLDO2] = {
		BCMPMU_REGULATOR_MMCLDO2, &bcm59056_mmcldo2_data, 0xff, BCMPMU_REGL_LPM_IN_DSM
	},
	[BCMPMU_REGULATOR_SIMLDO] = {
		BCMPMU_REGULATOR_SIMLDO, &bcm59056_simldo_data, 0xAA, BCMPMU_REGL_LPM_IN_DSM
	},
	[BCMPMU_REGULATOR_SIM2LDO] = {
		BCMPMU_REGULATOR_SIM2LDO, &bcm59056_sim2ldo_data, 0x11, BCMPMU_REGL_LPM_IN_DSM
	},
	[BCMPMU_REGULATOR_HDMILDO] = {
		BCMPMU_REGULATOR_HDMILDO, &bcm59056_hdmildo_data, 0x00, 0
	},
	[BCMPMU_REGULATOR_USBLDO] = {
		BCMPMU_REGULATOR_USBLDO, &bcm59056_usbldo_data, 0x01, 0
	},
	[BCMPMU_REGULATOR_CSR_NM] = {
		BCMPMU_REGULATOR_CSR_NM, &bcm59056_csr_data, 0x12, 0
	},
	[BCMPMU_REGULATOR_MMSR_NM] = {
		BCMPMU_REGULATOR_MMSR_NM, &bcm59056_mmsr_data, 0x09, 0
	},
	[BCMPMU_REGULATOR_VSR_NM] = {
		BCMPMU_REGULATOR_VSR_NM, &bcm59056_vsr_data, 0x02, 0
	},
	[BCMPMU_REGULATOR_GPLDO1] = {
		BCMPMU_REGULATOR_GPLDO1, &bcm59056_gpldo1_data, 0xAA, 1
	},
	[BCMPMU_REGULATOR_GPLDO2] = {
		BCMPMU_REGULATOR_GPLDO2, &bcm59056_gpldo2_data, 0xAA, 1
	},
	[BCMPMU_REGULATOR_GPLDO3] = {
		BCMPMU_REGULATOR_GPLDO3, &bcm59056_gpldo3_data, 0xAA, 1
	},
	[BCMPMU_REGULATOR_GPLDO4] = {
		BCMPMU_REGULATOR_GPLDO4, &bcm59056_gpldo4_data, 0xAA, 1
	},
	[BCMPMU_REGULATOR_GPLDO5] = {
		BCMPMU_REGULATOR_GPLDO5, &bcm59056_gpldo5_data, 0xAA, 1
	},
	[BCMPMU_REGULATOR_GPLDO6] = {
		BCMPMU_REGULATOR_GPLDO6, &bcm59056_gpldo6_data, 0xAA, 1
	},
};
