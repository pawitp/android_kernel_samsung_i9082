/****************************************************************************
*
* Copyright 2010 --2011 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/

#include <mach/rdb/brcm_rdb_bintc.h>
#include <mach/rdb/brcm_rdb_bmdm_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_dsp_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_bmdm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_dsp_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_bmdm_pwrmgr.h>
#include <mach/rdb/brcm_rdb_pwrmgr.h>
#include <mach/rdb/brcm_rdb_layer_2_async.h>
#include <mach/rdb/brcm_rdb_sleeptimer3g.h>
#include <mach/rdb/brcm_rdb_dsp_tl3r.h>
#include <mach/rdb/brcm_rdb_bmodem_syscfg.h>
#include <mach/io_map.h>
#include <asm/io.h>
#include <mach/pm.h>
#include <mach/comms/platform_mconfig_shared.h>

#include <mach/rdb/brcm_rdb_cr4dbg.h>

#include <chal/chal_util.h>
/* Define to use static WFI code for CP subsystem sleep
(comment out to use WFI code from bmodem_sleep.s) */
#define USE_STATIC_CP_WFI_CODE

/* DSP,BMODEM CCU/Reset access enable */
#define WR_ACCESS_ENABLE	0xA5A501

/* DSP access enable */
#define AHB_TL3R_TL3_A2D_ACCESS_EN_R_ACCE55 0xACCE5500
#define AHB_TL3R_TL3_A2D_ACCESS_ENA_ALL (AHB_TL3R_TL3_A2D_ACCESS_EN_R_ACCE55 |\
	DSP_TL3R_TL3_A2D_ACCESS_EN_R_ARM2DSP_PERIPHERAL_ACCESS_EN_MASK |\
	DSP_TL3R_TL3_A2D_ACCESS_EN_R_ARM2DSP_DMEM_ACCESS_EN_MASK |\
	DSP_TL3R_TL3_A2D_ACCESS_EN_R_ARM2DSP_PMEM_ACCESS_EN_MASK)

/* BMODEM PwrMgr related defines */
/* Power Manager policy definitions (same for all PMs)
 * BMODEM PwrMgr does not support sleep (use retention) */
#define PM_POLICY_SLEEP			0
#define PM_POLICY_RETENTION		1
#define PM_POLICY_NORM_MIN		4
#define PM_POLICY_NORM			5
#define PM_POLICY_OVERDRIVE		6
#define PM_POLICY_POWERUP		7

/* Macro to help set BMODEM Power manager policy */
#define BMDM_PM_POLICY(TYP, EVT, POL, ATL, AC)(\
	((POL << BMDM_PWRMGR_##EVT##_VI_BMODEM_POLICY_##EVT##_PI_##TYP##_PM_POLICY_SHIFT)\
	& BMDM_PWRMGR_##EVT##_VI_BMODEM_POLICY_##EVT##_PI_##TYP##_PM_POLICY_MASK)\
	| (ATL ? BMDM_PWRMGR_##EVT##_VI_BMODEM_POLICY_##EVT##_PI_##TYP##_PM_ATL_MASK : 0)\
	| (AC ? BMDM_PWRMGR_##EVT##_VI_BMODEM_POLICY_##EVT##_PI_##TYP##_PM_AC_MASK : 0))

/* CP TCM Base Address */

#define		BMDM_RST_BASE_ADR	BMDM_RST_VA
#define		DSP_RST_BASE_ADR		DSP_RST_VA
#define		R4DEBUG_BASE_ADR	R4DEBUG_VA
#define		BMDM_CCU_BASE_ADR	BMDM_CCU_VA
#define		DSP_CCU_BASE_ADR	DSP_CCU_VA
#define		BMDM_PWRMGR_BASE_ADR	BMDM_PWRMGR_VA
#define		PWRMGR_BASE_ADR			KONA_PWRMGR_VA
#define		AHB_DSP_TL3R_BASE_ADR	AHB_DSP_TL3R_VA
#define		WCDMAL2INT_ASYNC_BASE_ADR	WCDMAL2INT_ASYNC_VA
#define		SLEEPTIMER3G_BASE_ADR	SLEEPTIMER3G_VA
#define		BMODEM_SYSCFG_BASE_ADR	BMODEM_SYSCFG_VA
#define		BINTC_BASE_ADR	KONA_BINTC_BASE_ADDR

#ifdef USE_STATIC_CP_WFI_CODE

static UInt32 CP_WFI_code[128] = {
	0xE320F000, 0xE320F000, 0xE320F000, 0xE320F000,	/* 10 NOPs */
	0xE320F000, 0xE320F000, 0xE320F000, 0xE320F000,
	0xE320F000, 0xE320F000,
	0xF57FF05F, 0xE320F003,	/* DMB, WFI */
	0xE320F000, 0xE320F000, 0xE320F000, 0xE320F000,	/* 8 NOPs */
	0xE320F000, 0xE320F000, 0xE320F000, 0xE320F000,
	0xEAFFFFF4		/* branch back */
};
#else
/* CP WFI code start and end labels - to allow code copy */
static UINT8 CPWFI_begin, CPWFI_end;
#endif

/* CP TCM Base Address */
static void *cp_tcm_base;

/* CP WFI code start and end labels - to allow code copy */
extern UInt8 exit_wfi;

static void reset_BMODEM(void);
static void reset_DSP(void);

static void setup_R4DEBUG_pwrdn_ctrl(void);

static void setup_BMODEM_clocks_for_sleep(void);
static void setup_DSP_clocks_for_sleep(void);

static void setup_BMODEM_pwrmgr_for_sleep(void);
static void clear_pwrmgr_overrides(void);
static void wakeup_DSP(void);
static void force_WCDMA_modem_sleep(void);
static void BMODEM_pwrmgr_initiate_modem_sleep(void);
static void BMODEM_pwrmgr_enable_memory_standby(void);
static void put_CP_into_WFI(void);

/****************************************************************************
*
*   NAME:	 put_CPSubsystem_to_sleep
*
*   Description: Code to put CP Subsystem (CP, DSP, BMODEM/WCDMA) to sleep
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
int put_CPSubsystem_to_sleep(void *data, u64 clk_idle)
{
	if (cp_tcm_base == NULL) {
		cp_tcm_base = ioremap(MODEM_ITCM_ADDRESS, SZ_4K);
		if (cp_tcm_base == NULL) {
			printk(KERN_ERR "Unable to map CP ITCM registers\n");
			return -ENOMEM;
		}
	}

	/* Reset and clean up BMODEM and DSP subsystems */
	reset_BMODEM();
	reset_DSP();

	/* Set up R4DEBUG Power Down control */
	setup_R4DEBUG_pwrdn_ctrl();

	/* Setup BMODEM and DSP clocks to allow sleep */
	setup_BMODEM_clocks_for_sleep();
	setup_DSP_clocks_for_sleep();

	/* Set up BMDM Power Manager event policies for sleep */
	setup_BMODEM_pwrmgr_for_sleep();

	/* Clear all overrides in top level Power Manager */
	clear_pwrmgr_overrides();

	/* Bring DSP out of 'core wait' so that it can sleep */
	wakeup_DSP();

	/* Put WCDMA modem to sleep */
	force_WCDMA_modem_sleep();

	/* Drop to lower policy to put DSP and WCDMA to sleep */
	BMODEM_pwrmgr_initiate_modem_sleep();

	/* Clear memory standby disable bits for CP and DSP domains */
	BMODEM_pwrmgr_enable_memory_standby();

	/* Copy WFI code into CP TCM and bring CP out of halt
	   CP Subsystem should go into deep sleep! */
	put_CP_into_WFI();
	return 0;

}

/****************************************************************************
*
*   NAME:	 reset_BMODEM
*
*   Description: Toggles reset status of all BMODEM Subsystems
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void reset_BMODEM(void)
{
/* Revert to this once RDB is fixed
*	BRCM_WRITE_REG(BMDM_RST_BASE_ADR,
*			   BMDM_RST_MGR_REG_WR_ACCESS, WR_ACCESS_ENABLE);
*/
	writel(WR_ACCESS_ENABLE,
	       (BMDM_RST_BASE_ADR + BMDM_RST_MGR_REG_WR_ACCESS_OFFSET));

	/* Put all subsystems into reset */
	/* APB Core */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_APB_CORE_RSTN,
			     APB_CORE_SOFT_RSTN, 0);
	/* WCDMA */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_WCDMA_RSTN,
			     WCDMA_SOFT_RSTN, 0);
	/* WCDMA Peripherals */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     EDGE_MP_SOFT_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_DATAPACKER_SOFT_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_HUCM_SOFT_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_CIPHER_SOFT_RSTN, 0);
	/* Other peripherals */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     RFBB_SOFT_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     SCLKCAL_23G_SOFT_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     SCLKCAL_SOFT_RSTN, 0);
	/* CP and CP Debug */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_CP_RSTN,
			     CP_DEBUG_RSTN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_CP_RSTN,
			     CP_RSTN, 0);
	/* Bring all subsystems back out of reset */
	/* APB Core */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_APB_CORE_RSTN,
			     APB_CORE_SOFT_RSTN, 1);
	/* WCDMA */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_WCDMA_RSTN,
			     WCDMA_SOFT_RSTN, 1);
	/* WCDMA Peripherals */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     EDGE_MP_SOFT_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_DATAPACKER_SOFT_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_HUCM_SOFT_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN0,
			     WCDMA_CIPHER_SOFT_RSTN, 1);
	/* Other peripherals */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     RFBB_SOFT_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     SCLKCAL_23G_SOFT_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_SOFT_RSTN1,
			     SCLKCAL_SOFT_RSTN, 1);
	/* CP and CP Debug */
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_CP_RSTN,
			     CP_DEBUG_RSTN, 1);
	BRCM_WRITE_REG_FIELD(BMDM_RST_BASE_ADR, BMDM_RST_MGR_REG_CP_RSTN,
			     CP_RSTN, 1);
}

/****************************************************************************
*
*   NAME:	 reset_DSP
*
*   Description: Toggles reset status of all DSP Subsystems
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void reset_DSP(void)
{
	writel(WR_ACCESS_ENABLE,
	       (DSP_RST_BASE_ADR + DSP_RST_MGR_REG_WR_ACCESS_OFFSET));

	/* Put all subsystems into reset */
	/* DSP */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_DSP_RSTN,
			     DSP_SOFT_RSTN, 0);
	/* DSP Core and DSP JTAG */
	/* These start off in reset... we remove reset here */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_SOFT_RESET,
			     CORE_RESET, 1);
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_SOFT_RESET,
			     ORST_RESET, 1);
	/* Transceiver */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_TRANSCEIVER_RSTN,
			     TRANSCEIVER_SOFT_RSTN, 0);
	/* Take all subsystems out of reset */
	/* DSP */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_DSP_RSTN,
			     DSP_SOFT_RSTN, 1);
	/* DSP Core and DSP JTAG */
	/* These start off in reset... we put them back in reset here */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_SOFT_RESET,
			     CORE_RESET, 0);
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_SOFT_RESET,
			     ORST_RESET, 0);
	/* Transceiver */
	BRCM_WRITE_REG_FIELD(DSP_RST_BASE_ADR, DSP_RST_MGR_REG_TRANSCEIVER_RSTN,
			     TRANSCEIVER_SOFT_RSTN, 1);
}

/****************************************************************************
*
*   NAME:	 setup_R4DEBUG_pwrdn_ctrl
*
*   Description: Sets up R4DEBUG Powerdown and Reset Control register
*			 to allow power down
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 RDB only available for Capri... leave out others for now
****************************************************************************/
static void setup_R4DEBUG_pwrdn_ctrl(void)
{
	BRCM_WRITE_REG(R4DEBUG_BASE_ADR, CR4DBG_LOCKACCESS, 0xC5ACCE55);
	BRCM_WRITE_REG(R4DEBUG_BASE_ADR, CR4DBG_PRCR, 0x0);
}

/****************************************************************************
*
*   NAME:	 setup_BMODEM_clocks_for_sleep
*
*   Description: Sets up most BMODEM clocks
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void setup_BMODEM_clocks_for_sleep(void)
{
	u32 RegVal;
/* Revert to this once RDB is fixed
*	BRCM_WRITE_REG(DSP_CCU_BASE_ADR,
*			   DSP_CLK_MGR_REG_WR_ACCESS, WR_ACCESS_ENABLE);
*/

	writel(WR_ACCESS_ENABLE,
	       (BMDM_CCU_BASE_ADR + BMDM_CLK_MGR_REG_WR_ACCESS_OFFSET));
	/* HW Autogate all clocks that can be configured that way...
	   Disable others explicitly */
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_CP_CLKGATE,
			     CP_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_SWITCH_AXI_CLKGATE,
			     COMMS_SWITCH_AXI_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_ASYNC_AHB_CLKGATE,
			     ASYNC_AHB_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_APB_AON_CLKGATE,
			     APB_AON_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_APB_CORE_CLKGATE,
			     APB_CORE_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_APB_CORE_CLKGATE,
			     APB_CORE_CLK_EN, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_CIPHER_CLKGATE,
			     WCDMA_CIPHER_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_DATAPACKER_CLKGATE,
			     WCDMA_DATAPACKER_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_EDGE_MP_CLKGATE,
			     EDGE_MP_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_HUCM_CLKGATE,
			     WCDMA_HUCM_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_CLKGATE,
			     WCDMA_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_26M_CLKGATE,
			     WCDMA_26M_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(BMDM_CCU_BASE_ADR,
			     BMDM_CLK_MGR_REG_WCDMA_32K_CLKGATE,
			     WCDMA_32K_HW_SW_GATING_SEL, 0);
	/* SCLKCAL and SCLKCAL_23G cannot be HW autogated
	   Set up to be software controlled and disabled */
	RegVal = BRCM_READ_REG(BMDM_CCU_BASE_ADR,
			       BMDM_CLK_MGR_REG_SCLKCAL_CLKGATE);
	RegVal |=
	    BMDM_CLK_MGR_REG_SCLKCAL_CLKGATE_SCLKCAL_HW_SW_GATING_SEL_MASK;
	RegVal &= ~BMDM_CLK_MGR_REG_SCLKCAL_CLKGATE_SCLKCAL_CLK_EN_MASK;
	BRCM_WRITE_REG(BMDM_CCU_BASE_ADR,
		       BMDM_CLK_MGR_REG_SCLKCAL_CLKGATE, RegVal);
	RegVal = BRCM_READ_REG(BMDM_CCU_BASE_ADR,
			       BMDM_CLK_MGR_REG_SCLKCAL_23G_CLKGATE);
	RegVal |=
	    BMDM_CLK_MGR_REG_SCLKCAL_23G_CLKGATE_SCLKCAL_23G_HW_SW_GATING_SEL_MASK;

	RegVal &= ~BMDM_CLK_MGR_REG_SCLKCAL_23G_CLKGATE_SCLKCAL_23G_CLK_EN_MASK;
	BRCM_WRITE_REG(BMDM_CCU_BASE_ADR,
		       BMDM_CLK_MGR_REG_SCLKCAL_23G_CLKGATE, RegVal);
}

/****************************************************************************
*
*   NAME:	 setup_DSP_clocks_for_sleep
*
*   Description: Sets up all DSP clocks to be auto-gated
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void setup_DSP_clocks_for_sleep(void)
{
	/* Revert to this once RDB is fixed
	 * BRCM_WRITE_REG(DSP_CCU_BASE_ADR,
	 DSP_CLK_MGR_REG_WR_ACCESS, WR_ACCESS_ENABLE);
	 */

	writel(WR_ACCESS_ENABLE,
	       (DSP_CCU_BASE_ADR + DSP_CLK_MGR_REG_WR_ACCESS_OFFSET));
	/* All clocks HW auto-gated */
	BRCM_WRITE_REG_FIELD(DSP_CCU_BASE_ADR,
			     DSP_CLK_MGR_REG_DSP_CLKGATE,
			     DSP_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(DSP_CCU_BASE_ADR,
			     DSP_CLK_MGR_REG_TRANSCEIVER_26M_CLKGATE,
			     TRANSCEIVER_26M_HW_SW_GATING_SEL, 0);
	BRCM_WRITE_REG_FIELD(DSP_CCU_BASE_ADR,
			     DSP_CLK_MGR_REG_TRANSCEIVER_52M_CLKGATE,
			     TRANSCEIVER_52M_HW_SW_GATING_SEL, 0);
}

/****************************************************************************
*
*   NAME:	 setup_BMODEM_pwrmgr_for_sleep
*
*   Description: Sets up SW1, CP and SW0 events to take subsystem down to
*			 sleep state. Clears all other events.
*			 Also sets bit to ignore powerup request from DAP when
*			 JTAG debugger is connected
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void setup_BMODEM_pwrmgr_for_sleep(void)
{
	/* Set up SW1 event policy... */
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR,
		       BMDM_PWRMGR_SOFTWARE_1_VI_BMODEM_POLICY,
		       (BMDM_PM_POLICY
			(WCDMA, SOFTWARE_1, PM_POLICY_NORM, false, true)
			| BMDM_PM_POLICY(DSP, SOFTWARE_1, PM_POLICY_NORM, false,
					 true)
			| BMDM_PM_POLICY(CP, SOFTWARE_1, PM_POLICY_NORM, false,
					 true)));
	/* ...and make it active to keep us awake */
	BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_SOFTWARE_1_EVENT,
			     SOFTWARE_1_CONDITION_ACTIVE, 1);
	/* Set up CP event policy to fallback to when we clear SW1 event */
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_CP_VI_BMODEM_POLICY,
		       (BMDM_PM_POLICY(WCDMA, CP, PM_POLICY_SLEEP, false, false)
			| BMDM_PM_POLICY(DSP, CP, PM_POLICY_SLEEP, false,
					 false) | BMDM_PM_POLICY(CP, CP,
								 PM_POLICY_NORM_MIN,
								 false, true)));
	/* ...and make it active */
	BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_CP_EVENT,
			     CP_CONDITION_ACTIVE, 1);
	/* Set up SW0 policy to fallback to when CP goes WFI
	 * This is what should put the system to sleep */
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR,
		       BMDM_PWRMGR_SOFTWARE_0_VI_BMODEM_POLICY,
		       (BMDM_PM_POLICY
			(WCDMA, SOFTWARE_0, PM_POLICY_SLEEP, false, false)
			| BMDM_PM_POLICY(DSP, SOFTWARE_0, PM_POLICY_SLEEP,
					 false, false)
			| BMDM_PM_POLICY(CP, SOFTWARE_0, PM_POLICY_SLEEP, false,
					 false)));
	/* ...and make it active */
	BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_SOFTWARE_0_EVENT,
			     SOFTWARE_0_CONDITION_ACTIVE, 1);

	/* Clear all other events and ensure that they are disabled
	 * (ignore the ones set up and spares) */
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_TIMER_3G_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_TIMER_2G_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_WCDMA_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_DSP_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_PERIPH1_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_PERIPH2_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_SOFTWARE_2_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_R4_INT_WAKEUP_EVENT,
		       0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_DSP_INT_WAKEUP_EVENT,
		       0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_HUB_WAKEUP_EVENT, 0);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_EXT_WAKEUP_EVENT, 0);

	/* Set bit to ignore power
	 * up request from DAP when debugger is connected */
	BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR,
			     BMDM_PWRMGR_PI_DEFAULT_POWER_STATE,
			     IGNORE_DAP_POWERUPREQ, 1);
	 /*CAPRI*/
	    /* Workaround to correct problem with Capri - HWCAPRI-1340
	       This bit must be set for proper CP deep sleep operation */
	    BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR,
				 BMDM_PWRMGR_PI_DEFAULT_POWER_STATE,
				 PI_CP_RETENTION_CLAMP_DISABLE, 1);

}

/****************************************************************************
*
*   NAME:	 CPS_hw_clear_pwrmgr_overrides
*
*   Description: Clears all overrides in top level Power Manager
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void clear_pwrmgr_overrides(void)
{
	u32 RegVal;
	RegVal = BRCM_READ_REG(PWRMGR_BASE_ADR, PWRMGR_PI_DEFAULT_POWER_STATE);
	RegVal &=
	    ~(PWRMGR_PI_DEFAULT_POWER_STATE_PI_ARM_CORE_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_MM_SUB_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_MM_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_ARM_SUBSYSTEM_WAKEUP_OVERRIDE_MASK
	      |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_HUB_SWITCHABLE_WAKEUP_OVERRIDE_MASK
	      | PWRMGR_PI_DEFAULT_POWER_STATE_PI_HUB_AON_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_MODEM_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_ESUB_SUB_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_ESUB_WAKEUP_OVERRIDE_MASK |
	      PWRMGR_PI_DEFAULT_POWER_STATE_PI_MM_SUB2_WAKEUP_OVERRIDE_MASK);
	BRCM_WRITE_REG(PWRMGR_BASE_ADR, PWRMGR_PI_DEFAULT_POWER_STATE, RegVal);
}

/****************************************************************************
*
*   NAME:	 wakeup_DSP
*
*   Description: Brings DSP out of 'core wait' state
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void wakeup_DSP(void)
{
	BRCM_WRITE_REG(AHB_DSP_TL3R_BASE_ADR,
		       DSP_TL3R_TL3_A2D_ACCESS_EN_R,
		       AHB_TL3R_TL3_A2D_ACCESS_ENA_ALL);
	BRCM_WRITE_REG_FIELD(AHB_DSP_TL3R_BASE_ADR, DSP_TL3R_TL3_CTRL_REG,
			     TL3_CORE_WAIT, 0);
}

/****************************************************************************
*
*   NAME:	 force_WCDMA_modem_sleep
*
*   Description: Sets bit to force WCDMA modem sleep and deassert 624MHz/26MHz
*		 clock request. Also configures 3G Sleep timer for power down.
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void /*CPS_hw_ */ force_WCDMA_modem_sleep(void)
{
	/* Set bit to force WCDMA modem sleep and deassert 624MHz/26MHz
	 * clock request */
	BRCM_WRITE_REG_FIELD(WCDMAL2INT_ASYNC_BASE_ADR, LAYER_2_ASYNC_CLOCKON,
			     FORCE_MODEM_SLEEP, 1);
	/* Set pwd_override bit and
	 * pwd_override_val=1 (32k clock not requested) */
	BRCM_WRITE_REG_FIELD(SLEEPTIMER3G_BASE_ADR, SLEEPTIMER3G_STATUS,
			     PWD_OVERRIDE_VAL, 1);
	BRCM_WRITE_REG_FIELD(SLEEPTIMER3G_BASE_ADR, SLEEPTIMER3G_STATUS,
			     PWD_OVERRIDE, 1);
}

/****************************************************************************
*
*   NAME:	 BMODEM_pwrmgr_initiate_modem_sleep
*
*   Description: Clears SW 1 event to drop to CP policy which should put
*			 WCDMA and DSP to sleep
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void BMODEM_pwrmgr_initiate_modem_sleep(void)
{
	BRCM_WRITE_REG_FIELD(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_SOFTWARE_1_EVENT,
			     SOFTWARE_1_CONDITION_ACTIVE, 0);
}

/****************************************************************************
*
*   NAME:	 BMODEM_pwrmgr_enable_memory_standby
*
*Description: Allow CP, DSP and WCDMA memories to go into standby during
*		 deep sleep allow PM and PDA pins to be asserted when they
*		 do to shutdown supply to the memory array to save leakage
*
*   Parameters:  none
*
*   Returns:	 none
*
*   Notes:	 none
****************************************************************************/
static void BMODEM_pwrmgr_enable_memory_standby(void)
{
	u32 RegVal;
	RegVal = BRCM_READ_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_CP_MEM_CTRL);
	/* Allow CP memories to go into standby */
	RegVal &= ~(BMDM_PWRMGR_CP_MEM_CTRL_RFBB_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_PERIPH_ROM_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_PERIPH_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_BTCM_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_ATCM_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1I_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1D_STBY_DISABLE_MASK);
	/* Allow PM pin to be asserted */
	RegVal &= ~(BMDM_PWRMGR_CP_MEM_CTRL_AHB_PM_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_BTCM_PM_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_ATCM_PM_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1I_PM_DISABLE_MASK
		    | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1D_PM_DISABLE_MASK);
	/* Allow PDA pin to be asserted */
	RegVal |= (BMDM_PWRMGR_CP_MEM_CTRL_CP_PDA_SHUTDOWN_MODE_MASK
		   | BMDM_PWRMGR_CP_MEM_CTRL_CP_BTCM_PDA_MASK
		   | BMDM_PWRMGR_CP_MEM_CTRL_CP_ATCM_PDA_MASK
		   | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1I_PDA_MASK
		   | BMDM_PWRMGR_CP_MEM_CTRL_CP_L1D_PDA_MASK
		   | BMDM_PWRMGR_CP_MEM_CTRL_AHB_PDA_MASK);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_CP_MEM_CTRL, RegVal);

	RegVal = BRCM_READ_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_DSP_MEM_CTRL);
	/* Allow DSP memories to go into standby */
	RegVal &= ~(BMDM_PWRMGR_DSP_MEM_CTRL_DSP_STBY_RF_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_ROM_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE1_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE0_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_MEM_STBY_DISABLE_MASK);
	/* Allow PM pin to be asserted */
	RegVal &= ~(BMDM_PWRMGR_DSP_MEM_CTRL_DSP_ROM_PM_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE1_PM_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE0_PM_DISABLE_MASK
		    | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_MEM_PM_DISABLE_MASK);
	/* Allow PDA pin to be asserted */
	RegVal |= (BMDM_PWRMGR_DSP_MEM_CTRL_DSP_PDA_SHUTDOWN_MODE_MASK
		   | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_ROM_PDA_MASK
		   | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE1_PDA_MASK
		   | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_CACHE0_PDA_MASK
		   | BMDM_PWRMGR_DSP_MEM_CTRL_DSP_MEM_PDA_MASK);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_DSP_MEM_CTRL, RegVal);

	RegVal =
	    BRCM_READ_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_WCDMA_MEM_CTRL);
	/* Allow WCDMA memories to go into standby */
	RegVal &= ~(BMDM_PWRMGR_WCDMA_MEM_CTRL_WCDMA_RF_STBY_DISABLE_MASK
		    | BMDM_PWRMGR_WCDMA_MEM_CTRL_WCDMA_STBY_DISABLE_MASK);
	/* Allow PM pin to be asserted */
	RegVal &= ~(BMDM_PWRMGR_WCDMA_MEM_CTRL_WCDMA_PM_DISABLE_MASK);
	/* Allow PDA pin to be asserted */
	RegVal |= (BMDM_PWRMGR_WCDMA_MEM_CTRL_WCDMA_PDA_SHUTDOWN_MODE_MASK
		   | BMDM_PWRMGR_WCDMA_MEM_CTRL_WCDMA_PDA_MASK);
	BRCM_WRITE_REG(BMDM_PWRMGR_BASE_ADR, BMDM_PWRMGR_WCDMA_MEM_CTRL,
		       RegVal);
}

/****************************************************************************
*
*   NAME:	put_CP_into_WFI
*
*   Description: Copies WFI loop code into CP TCM
*			Brings CP out of halt so that it will execute code and
*			go into WFI state
*			This should cause the whole CP
*			system to drop to SW0 policy
*			which puts all domains into deep sleep
*
*   Parameters:  none
*
*   Returns:	 none
*
*Notes:	 WFI code in bmodem_sleep.s
*			 Fill in extra 512 bytes (per reference code)
****************************************************************************/
#ifdef USE_STATIC_CP_WFI_CODE
static void put_CP_into_WFI(void)
{
	/* Load WFI loop code into CP TCM */
	UInt8 *pTCM = (UInt8 *)cp_tcm_base;
	UInt8 *pSRC = (UInt8 *)&CP_WFI_code[0];
	UInt16 index;

	for (index = 0; index < 512; index++)
		*pTCM++ = *pSRC++;

	/* Bring CP out of halt state */
	BRCM_WRITE_REG_FIELD(BMODEM_SYSCFG_BASE_ADR, BMODEM_SYSCFG_R4_CFG0,
			     NCPUHALT, 1);
}
#else
static void put_CP_into_WFI(void)
{
	/* Load WFI loop code into CP TCM */
	UInt8 *pTCM = (UInt8 *)cp_tcm_base;
	UInt8 *pSRC = (UInt8 *)&enter_wfi;
	UInt8 i;

	while (pSRC <= &exit_wfi + 512)
		*pTCM++ = *pSRC++;
	/* Bring CP out of halt state */
	BRCM_WRITE_REG_FIELD(BMODEM_SYSCFG_BASE_ADR, BMODEM_SYSCFG_R4_CFG0,
			     NCPUHALT, 1);
}
#endif
