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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/delay.h>
#include <asm/cpu.h>

#include <plat/clock.h>
#include <mach/io_map.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
#include <mach/rdb/brcm_rdb_kpm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kps_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khubaon_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_root_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kproc_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_bmdm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_dsp_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_root_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kproc_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khub_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khubaon_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_esub_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kpm_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kps_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_esub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_pwrmgr.h>
#ifdef CONFIG_DEBUG_FS
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#include <mach/rdb/brcm_rdb_chipreg.h>
#endif
#include <linux/clk.h>
#include <asm/io.h>
#include <mach/pi_mgr.h>

#include <plat/pi_mgr.h>
#include "volt_tbl.h"

static u32 cur_freq_id;
static u32 cur_policy_id;
static u32 cur_opp_inx;
static unsigned short enable_special_autogating;

enum {
	/* A9/AXI/APB0 frequencies, in MHz */
	kproc_fid0 = 0,
	kproc_fid1,
	kproc_fid2,
	kproc_fid3,		/* 0.9V target */
	kproc_fid4,
	kproc_fid5,		/* 1.0V target */
	kproc_fid6,		/* 1.1V target */
	kproc_fid7		/* 1.2V target */
} ccu_kproc_policy_freq_e;

static const char *const ccu_clks[] = {
	KPROC_CCU_CLK_NAME_STR,
	ROOT_CCU_CLK_NAME_STR,
	KHUB_CCU_CLK_NAME_STR,
	KHUBAON_CCU_CLK_NAME_STR,
	KPM_CCU_CLK_NAME_STR,
	KPS_CCU_CLK_NAME_STR,
	BMDM_CCU_CLK_NAME_STR,
	DSP_CCU_CLK_NAME_STR,
};

unsigned long clock_get_xtal(void)
{
	return FREQ_MHZ(26);
}

/*
modem CCU clock
*/
static struct ccu_clk CLK_NAME(bmdm) = {
	.clk = {
.flags = BMDM_CCU_CLK_FLAGS,.name = BMDM_CCU_CLK_NAME_STR,.id =
		    CLK_BMDM_CCU_CLK_ID,.ops =
		    &gen_ccu_clk_ops,.clk_type =
		    CLK_TYPE_CCU,},.ccu_ops = &gen_ccu_ops,.pi_id =
	    -1,.ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(BMDM_CCU_BASE_ADDR),.wr_access_offset =
	    BMDM_CLK_MGR_REG_WR_ACCESS_OFFSET,.lvm_en_offset =
	    BMDM_CLK_MGR_REG_LVM_EN_OFFSET,.policy_ctl_offset =
	    BMDM_CLK_MGR_REG_POLICY_CTL_OFFSET,.vlt0_3_offset =
	    BMDM_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    BMDM_CLK_MGR_REG_VLT4_7_OFFSET,.freq_volt =
	    BMDM_CCU_FREQ_VOLT_TBL,.freq_count = BMDM_CCU_FREQ_VOLT_TBL_SZ,};

/*
DSP CCU clock
*/
static struct ccu_clk CLK_NAME(dsp) = {
	.clk = {
.flags = DSP_CCU_CLK_FLAGS,.name = DSP_CCU_CLK_NAME_STR,.id =
		    CLK_DSP_CCU_CLK_ID,.ops =
		    &gen_ccu_clk_ops,.clk_type =
		    CLK_TYPE_CCU,},.ccu_ops = &gen_ccu_ops,.pi_id =
	    -1,.ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(DSP_CCU_BASE_ADDR),.wr_access_offset =
	    DSP_CLK_MGR_REG_WR_ACCESS_OFFSET,.lvm_en_offset =
	    DSP_CLK_MGR_REG_LVM_EN_OFFSET,.policy_ctl_offset =
	    DSP_CLK_MGR_REG_POLICY_CTL_OFFSET,.vlt0_3_offset =
	    DSP_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    DSP_CLK_MGR_REG_VLT4_7_OFFSET,.freq_volt =
	    DSP_CCU_FREQ_VOLT_TBL,.freq_count = DSP_CCU_FREQ_VOLT_TBL_SZ,};

/*root ccu ops */
static int root_ccu_clk_init(struct clk *clk);

static struct gen_clk_ops root_ccu_clk_ops = {
	.init = root_ccu_clk_init,
};

/*
Root CCU clock
*/
static struct ccu_clk_ops root_ccu_ops;
static struct ccu_clk CLK_NAME(root) = {
	.clk = {
.name = ROOT_CCU_CLK_NAME_STR,.id = CLK_ROOT_CCU_CLK_ID,.ops =
		    &root_ccu_clk_ops,.clk_type =
		    CLK_TYPE_CCU,},.pi_id = -1,.ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(ROOT_CLK_BASE_ADDR),.wr_access_offset =
	    ROOT_CLK_MGR_REG_WR_ACCESS_OFFSET,.ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(ROOT_RST_BASE_ADDR),.
	    reset_wr_access_offset =
	    ROOT_RST_MGR_REG_WR_ACCESS_OFFSET,.ccu_ops = &root_ccu_ops,};

/*
Ref 32khz clkRef clock name crystal
*/
static struct ref_clk CLK_NAME(crystal) = {

	.clk = {
.name = CRYSTAL_REF_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_REF,.rate = FREQ_MHZ(26),.ops =
		    &gen_ref_clk_ops,},.ccu_clk = &CLK_NAME(root),};

/*
PLL0 clk : 624MHZ (const PLL)
*/
static struct ref_clk clk_pll0 = {
	.clk = {
		.name = PLL0_REF_CLK_NAME_STR,
		.clk_type = CLK_TYPE_REF,
		.rate = FREQ_MHZ(312),
		.ops = &gen_ref_clk_ops,
		},
	.ccu_clk = &CLK_NAME(root),
};

/*
PLL1 clk : 624MHZ (Root PLL - desensing)
*/
static struct ref_clk clk_pll1 = {
	.clk = {
		.name = PLL1_REF_CLK_NAME_STR,
		.clk_type = CLK_TYPE_REF,
		.rate = FREQ_MHZ(312),
		.ops = &gen_ref_clk_ops,
		},
	.ccu_clk = &CLK_NAME(root),
};

static int en_8ph_pll1_clk_init(struct clk *clk)
{
	return 0;
}

static int en_8ph_pll1_clk_enable(struct clk *clk, int enable)
{
	struct ref_clk *ref_clk;
	u32 reg_val = 0;
	int insurance = 1000;

	BUG_ON(clk->clk_type != CLK_TYPE_REF);
	ref_clk = to_ref_clk(clk);

	BUG_ON(ref_clk->ccu_clk == NULL);
	clk_dbg("%s, clock: %s enable: %d\n", __func__, clk->name, enable);

	/* enable write access */
	ccu_write_access_enable(ref_clk->ccu_clk, true);

	if (enable) {
		CCU_ACCESS_EN(ref_clk->ccu_clk, 1);
		reg_val = readl(KONA_ROOT_CLK_VA
				+ ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		reg_val |= ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK;
		/*Enable 8ph bit in pll 1 */
		do {
			udelay(1);
			writel(reg_val, KONA_ROOT_CLK_VA
			       + ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
			insurance--;
		} while (!(readl(KONA_ROOT_CLK_VA +
				 ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET) &
			   ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK) &&
			 insurance);
		CCU_ACCESS_EN(ref_clk->ccu_clk, 0);
	} else {
		CCU_ACCESS_EN(ref_clk->ccu_clk, 1);
		reg_val = readl(KONA_ROOT_CLK_VA +
				ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		reg_val &= ~ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK;
		/*Disable 8ph bit in pll 1 */
		writel(reg_val, KONA_ROOT_CLK_VA +
		       ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		CCU_ACCESS_EN(ref_clk->ccu_clk, 0);
	}

	/* Disable write access */
	ccu_write_access_enable(ref_clk->ccu_clk, false);
	return 0;

}

struct gen_clk_ops en_8ph_pll1_ref_clk_ops;
/*
Ref 8phase_en_pll1 clk
*/
static struct ref_clk clk_8phase_en_pll1 = {

	.clk = {
		.name = REF_8PHASE_EN_PLL1_CLK_NAME_STR,
		.clk_type = CLK_TYPE_REF,
		.id = CLK_8PHASE_EN_PLL1_REF_CLK_ID,
		.rate = FREQ_MHZ(624),
		.ops = &en_8ph_pll1_ref_clk_ops,
		},
	.ccu_clk = &CLK_NAME(root),
};

static int misc_clk_init(struct clk *clk)
{
	return 0;
}

static int misc_clk_enable(struct clk *clk, int enable)
{
	u32 reg_val = 0;
	u32 reg_offset;
	u32 reg_mask;
	void __iomem *reg_base;

	BUG_ON(clk->clk_type != CLK_TYPE_MISC);

	clk_dbg("%s, clock: %s enable: %d\n", __func__, clk->name, enable);

	switch (clk->id) {
	case CLK_TPIU_PERI_CLK_ID:
		reg_base = KONA_CHIPREG_VA;
		reg_offset = CHIPREG_ARM_PERI_CONTROL_OFFSET;
		reg_mask = CHIPREG_ARM_PERI_CONTROL_TPIU_CLK_IS_IDLE_MASK;
		break;
	case CLK_PTI_PERI_CLK_ID:
		reg_base = KONA_CHIPREG_VA;
		reg_offset = CHIPREG_ARM_PERI_CONTROL_OFFSET;
		reg_mask = CHIPREG_ARM_PERI_CONTROL_PTI_CLK_IS_IDLE_MASK;
		break;
	default:
		return -EINVAL;
	}
	if (enable) {
		reg_val = readl(reg_base + reg_offset);
		if (clk->flags & INVERT_ENABLE)
			reg_val &= ~reg_mask;
		else
			reg_val |= reg_mask;
		writel(reg_val, reg_base + reg_offset);
	} else {
		reg_val = readl(reg_base + reg_offset);
		if (clk->flags & INVERT_ENABLE)
			reg_val |= reg_mask;
		else
			reg_val &= ~reg_mask;
		writel(reg_val, reg_base + reg_offset);
	}

	return 0;

}

struct gen_clk_ops misc_clk_ops = {
	.init = misc_clk_init,
	.enable = misc_clk_enable,
};

/*
Misc clock name TPIU
*/
/*peri clk src list*/
static struct clk clk_tpiu = {
	.clk_type = CLK_TYPE_MISC,
	.id = CLK_TPIU_PERI_CLK_ID,
	.name = TPIU_PERI_CLK_NAME_STR,
	.flags = TPIU_MISC_CLK_FLAGS,
#ifdef CONFIG_BCM_HWCAPRI_1508
	.dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(8phase_en_pll1), NULL),
#endif
	.ops = &misc_clk_ops,
};

static struct clk clk_pti = {
	.clk_type = CLK_TYPE_MISC,
	.id = CLK_PTI_PERI_CLK_ID,
	.name = PTI_PERI_CLK_NAME_STR,
	.flags = PTI_MISC_CLK_FLAGS,
#ifdef CONFIG_BCM_HWCAPRI_1508
	.dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(8phase_en_pll1), NULL),
#endif
	.ops = &misc_clk_ops,
};

/*
Ref clock name FRAC_1M
*/
static struct ref_clk CLK_NAME(frac_1m) = {

	.clk = {
.flags = FRAC_1M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_FRAC_1M_REF_CLK_ID,.name =
		    FRAC_1M_REF_CLK_NAME_STR,.rate =
		    1000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_FRAC_1M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_FRAC_1M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_FRAC_1M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_FRAC_1M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_FRAC_1M_CLKGATE_FRAC_1M_STPRSTS_MASK,};

/*
Ref clock name REF_96M_VARVDD
*/
static struct ref_clk CLK_NAME(ref_96m_varvdd) = {

	.clk = {
.flags = REF_96M_VARVDD_REF_CLK_FLAGS,.clk_type =
		    CLK_TYPE_REF,.id =
		    CLK_REF_96M_VARVDD_REF_CLK_ID,.name =
		    REF_96M_VARVDD_REF_CLK_NAME_STR,.rate =
		    96000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_96M_VARVDD_CLKGATE_OFFSET,.
	    clk_en_mask = 0,.gating_sel_mask = 0,.hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_96M_VARVDD_CLKGATE_REF_96M_VARVDD_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_96M_VARVDD_CLKGATE_REF_96M_VARVDD_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_96M_VARVDD_CLKGATE_REF_96M_VARVDD_STPRSTS_MASK,};

/*
Ref clock name REF_96M
*/
static struct ref_clk CLK_NAME(ref_96m) = {

	.clk = {
.flags = REF_96M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_96M_REF_CLK_ID,.name =
		    REF_96M_REF_CLK_NAME_STR,.rate =
		    96000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_REF_96M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_REF_96M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_REF_96M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_REF_96M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_48M_CLKGATE_REF_96M_STPRSTS_MASK,};

/*
Ref clock name VAR_96M
*/
static struct ref_clk CLK_NAME(var_96m) = {

	.clk = {
.flags = VAR_96M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_96M_REF_CLK_ID,.name =
		    VAR_96M_REF_CLK_NAME_STR,.rate =
		    96000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_STPRSTS_MASK,};

/*
Ref clock name VAR_500M_VARVDD
*/
static struct ref_clk CLK_NAME(var_500m_varvdd) = {

	.clk = {
.flags = VAR_500M_VARVDD_REF_CLK_FLAGS,.clk_type =
		    CLK_TYPE_REF,.id =
		    CLK_VAR_500M_VARVDD_REF_CLK_ID,.name =
		    VAR_500M_VARVDD_REF_CLK_NAME_STR,.rate =
		    500000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_VAR_500M_VARVDD_CLKGATE_OFFSET,.
	    clk_en_mask = 0,.gating_sel_mask = 0,.hyst_val_mask =
	    ROOT_CLK_MGR_REG_VAR_500M_VARVDD_CLKGATE_VAR_500M_VARVDD_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_VAR_500M_VARVDD_CLKGATE_VAR_500M_VARVDD_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_VAR_500M_VARVDD_CLKGATE_VAR_500M_VARVDD_STPRSTS_MASK,};

/*
Ref clock name REF_312M
*/
static struct ref_clk CLK_NAME(ref_312m) = {

	.clk = {
.flags = REF_312M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_312M_REF_CLK_ID,.name =
		    REF_312M_REF_CLK_NAME_STR,.rate =
		    312000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_REF_312M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_REF_312M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_REF_312M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_REF_312M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_312M_CLKGATE_REF_312M_STPRSTS_MASK,};

/*
Ref clock name REF_208M
*/
static struct ref_clk CLK_NAME(ref_208m) = {

	.clk = {
.flags = REF_208M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_208M_REF_CLK_ID,.name =
		    REF_208M_REF_CLK_NAME_STR,.rate =
		    208000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_REF_208M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_REF_208M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_REF_208M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_REF_208M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_208M_CLKGATE_REF_208M_STPRSTS_MASK,};

/*
Ref clock name REF_156M
*/
static struct ref_clk CLK_NAME(ref_156m) = {

	.clk = {
.flags = REF_156M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_156M_REF_CLK_ID,.name =
		    REF_156M_REF_CLK_NAME_STR,.rate =
		    156000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_104M
*/
static struct ref_clk CLK_NAME(ref_104m) = {

	.clk = {
.flags = REF_104M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_104M_REF_CLK_ID,.name =
		    REF_104M_REF_CLK_NAME_STR,.rate =
		    104000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_52M
*/
static struct ref_clk CLK_NAME(ref_52m) = {

	.clk = {
.flags = REF_52M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_52M_REF_CLK_ID,.name =
		    REF_52M_REF_CLK_NAME_STR,.rate =
		    52000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_13M
*/
static struct ref_clk CLK_NAME(ref_13m) = {

	.clk = {
.flags = REF_13M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_13M_REF_CLK_ID,.name =
		    REF_13M_REF_CLK_NAME_STR,.rate =
		    13000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_26M
*/
static struct ref_clk CLK_NAME(ref_26m) = {

	.clk = {
.flags = REF_26M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_26M_REF_CLK_ID,.name =
		    REF_26M_REF_CLK_NAME_STR,.rate =
		    26000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_REF_26M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_REF_26M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_REF_26M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_REF_26M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_26M_CLKGATE_REF_26M_STPRSTS_MASK,};

/*
Ref clock name VAR_312M
*/
static struct ref_clk CLK_NAME(var_312m) = {

	.clk = {
.flags = VAR_312M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_312M_REF_CLK_ID,.name =
		    VAR_312M_REF_CLK_NAME_STR,.rate =
		    312000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_STPRSTS_MASK,};

/*
Ref clock name VAR_500M
*/
#if 0
static struct ref_clk CLK_NAME(var_500m) = {

	.clk = {
.clk_type = CLK_TYPE_REF,.rate = 500000000,.ops =
		    &gen_ref_clk_ops,.name =
		    VAR_500M_REF_CLK_NAME_STR,},.ccu_clk = &CLK_NAME(root),};
#endif

/*
Ref clock name VAR_208M
*/
static struct ref_clk CLK_NAME(var_208m) = {

	.clk = {
.flags = VAR_208M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_208M_REF_CLK_ID,.name =
		    VAR_208M_REF_CLK_NAME_STR,.rate =
		    208000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_STPRSTS_MASK,};

/*
Ref clock name VAR_156M
*/
static struct ref_clk CLK_NAME(var_156m) = {

	.clk = {
.flags = VAR_156M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_156M_REF_CLK_ID,.name =
		    VAR_156M_REF_CLK_NAME_STR,.rate =
		    156000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name VAR_104M
*/
static struct ref_clk CLK_NAME(var_104m) = {

	.clk = {
.flags = VAR_104M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_104M_REF_CLK_ID,.name =
		    VAR_104M_REF_CLK_NAME_STR,.rate =
		    104000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name VAR_52M
*/
static struct ref_clk CLK_NAME(var_52m) = {

	.clk = {
.flags = VAR_52M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_52M_REF_CLK_ID,.name =
		    VAR_52M_REF_CLK_NAME_STR,.rate =
		    52000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name VAR_13M
*/
static struct ref_clk CLK_NAME(var_13m) = {

	.clk = {
.flags = VAR_13M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_VAR_13M_REF_CLK_ID,.name =
		    VAR_13M_REF_CLK_NAME_STR,.rate =
		    13000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name DFT_19_5M
*/
static struct ref_clk CLK_NAME(dft_19_5m) = {

	.clk = {
.flags = DFT_19_5M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_DFT_19_5M_REF_CLK_ID,.name =
		    DFT_19_5M_REF_CLK_NAME_STR,.rate =
		    19500000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_CX40_VARVDD
*/
static struct ref_clk CLK_NAME(ref_cx40_varvdd) = {

	.clk = {
.flags = REF_CX40_VARVDD_REF_CLK_FLAGS,.clk_type =
		    CLK_TYPE_REF,.id =
		    CLK_REF_CX40_VARVDD_REF_CLK_ID,.name =
		    REF_CX40_VARVDD_REF_CLK_NAME_STR,.rate =
		    40000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_REF_CX40_VARVDD_CLKGATE_OFFSET,.
	    clk_en_mask = 0,.gating_sel_mask = 0,.hyst_val_mask =
	    ROOT_CLK_MGR_REG_REF_CX40_VARVDD_CLKGATE_REF_CX40_VARVDD_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ROOT_CLK_MGR_REG_REF_CX40_VARVDD_CLKGATE_REF_CX40_VARVDD_HYST_EN_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_REF_CX40_VARVDD_CLKGATE_REF_CX40_VARVDD_STPRSTS_MASK,};

/*
Ref clock name REF_CX40
*/
static struct ref_clk CLK_NAME(ref_cx40) = {

	.clk = {
.flags = REF_CX40_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_CX40_REF_CLK_ID,.name =
		    REF_CX40_REF_CLK_NAME_STR,.rate =
		    153600000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_1M
*/
static struct ref_clk CLK_NAME(ref_1m) = {
	.clk = {
.flags = REF_1M_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_1M_REF_CLK_ID,.name =
		    REF_1M_REF_CLK_NAME_STR,.rate =
		    1000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
Ref clock name REF_32K
*/
static struct ref_clk CLK_NAME(ref_32k) = {
	.clk = {
.flags = REF_32K_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_32K_REF_CLK_ID,.name =
		    REF_32K_REF_CLK_NAME_STR,.rate =
		    32000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),};

/*
CCU clock name PROC_CCU
*/

static struct ccu_clk_ops kproc_ccu_ops;
static struct ccu_clk CLK_NAME(kproc) = {

	.clk = {
	.flags = KPROC_CCU_CLK_FLAGS,.id = CLK_KPROC_CCU_CLK_ID,.name =
		    KPROC_CCU_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_CCU,.ops = &gen_ccu_clk_ops,},.ccu_ops =
	    &kproc_ccu_ops,
/*	.ccu_ops = &gen_ccu_ops,  */
	    .pi_id = PI_MGR_PI_ID_ARM_CORE,
//      .pi_id = -1,
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(PROC_CLK_BASE_ADDR),.wr_access_offset =
	    KPROC_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    KPROC_CLK_MGR_REG_POLICY0_MASK_OFFSET,.policy_mask2_offset =
	    0,.policy_freq_offset =
	    KPROC_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    KPROC_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    KPROC_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    KPROC_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    0,.lvm_en_offset = KPROC_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    KPROC_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    KPROC_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    KPROC_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    KPROC_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    KPROC_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    KPROC_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,
#endif
.freq_volt =
	    DEFINE_ARRAY_ARGS(PROC_VLT_ID_ECO, PROC_VLT_ID_ECO,
					  PROC_VLT_ID_ECO,
					  PROC_VLT_ID_ECO,
					  PROC_VLT_ID_ECO1,
					  PROC_VLT_ID_NORMAL,
					  PROC_VLT_ID_TURBO1,
					  PROC_VLT_ID_TURBO),.
	    freq_count = PROC_CCU_FREQ_VOLT_TBL_SZ,.freq_policy =
	    DEFINE_ARRAY_ARGS(PROC_CCU_FREQ_POLICY_TBL),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(PROC_RST_BASE_ADDR),.
	    reset_wr_access_offset = KPROC_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
PLL Clk name a9_pll
*/

u32 a9_vc0_thold[] = { FREQ_MHZ(1750), PLL_VCO_RATE_MAX };
u32 a9_cfg_val[] = { 0x8000000, 0x8102000 };

static struct pll_cfg_ctrl_info a9_cfg_ctrl = {
	.pll_cfg_ctrl_offset = KPROC_CLK_MGR_REG_PLLARMCTRL3_OFFSET,
	.pll_cfg_ctrl_mask =
	    KPROC_CLK_MGR_REG_PLLARMCTRL3_PLLARM_PLL_CONFIG_CTRL_MASK,
	.pll_cfg_ctrl_shift =
	    KPROC_CLK_MGR_REG_PLLARMCTRL3_PLLARM_PLL_CONFIG_CTRL_SHIFT,

	.vco_thold = a9_vc0_thold,
	.pll_config_value = a9_cfg_val,
	.thold_count = ARRAY_SIZE(a9_cfg_val),
};

static struct pll_clk CLK_NAME(a9_pll) = {

	.clk = {
.flags = A9_PLL_CLK_FLAGS,.id = CLK_A9_PLL_CLK_ID,.name =
		    A9_PLL_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_PLL,.ops = &gen_pll_clk_ops,},.ccu_clk =
	    &CLK_NAME(kproc),.pll_ctrl_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.soft_post_resetb_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.soft_post_resetb_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_SOFT_POST_RESETB_MASK,.
	    soft_resetb_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.soft_resetb_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_SOFT_RESETB_MASK,.
	    pwrdwn_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.pwrdwn_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_PWRDWN_MASK,.
	    idle_pwrdwn_sw_ovrride_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_IDLE_PWRDWN_SW_OVRRIDE_MASK,.
	    ndiv_pdiv_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.ndiv_int_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_NDIV_INT_MASK,.
	    ndiv_int_shift =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_NDIV_INT_SHIFT,.
	    ndiv_int_max = 512,.pdiv_mask =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_PDIV_MASK,.pdiv_shift =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_PDIV_SHIFT,.pdiv_max =
	    8,.pll_lock_offset =
	    KPROC_CLK_MGR_REG_PLLARMA_OFFSET,.pll_lock =
	    KPROC_CLK_MGR_REG_PLLARMA_PLLARM_LOCK_MASK,.
	    ndiv_frac_offset =
	    KPROC_CLK_MGR_REG_PLLARMB_OFFSET,.ndiv_frac_mask =
	    KPROC_CLK_MGR_REG_PLLARMB_PLLARM_NDIV_FRAC_MASK,.
	    ndiv_frac_shift =
	    KPROC_CLK_MGR_REG_PLLARMB_PLLARM_NDIV_FRAC_SHIFT,.
	    cfg_ctrl_info = &a9_cfg_ctrl,};

/*A9 pll - channel 0*/
static struct pll_chnl_clk CLK_NAME(a9_pll_chnl0) = {

	.clk = {
.flags = A9_PLL_CHNL0_CLK_FLAGS,.id =
		    CLK_A9_PLL_CHNL0_CLK_ID,.name =
		    A9_PLL_CHNL0_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_PLL_CHNL,.ops =
		    &gen_pll_chnl_clk_ops,},.ccu_clk =
	    &CLK_NAME(kproc),.pll_clk =
	    &CLK_NAME(a9_pll),.cfg_reg_offset =
	    KPROC_CLK_MGR_REG_PLLARMC_OFFSET,.mdiv_mask =
	    KPROC_CLK_MGR_REG_PLLARMC_PLLARM_MDIV_MASK,.mdiv_shift =
	    KPROC_CLK_MGR_REG_PLLARMC_PLLARM_MDIV_SHIFT,.mdiv_max =
	    256,.pll_enableb_offset =
	    KPROC_CLK_MGR_REG_PLLARMC_OFFSET,.out_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMC_PLLARM_ENB_CLKOUT_MASK,.
	    pll_load_ch_en_offset =
	    KPROC_CLK_MGR_REG_PLLARMC_OFFSET,.load_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMC_PLLARM_LOAD_EN_MASK,.
	    pll_hold_ch_offset =
	    KPROC_CLK_MGR_REG_PLLARMC_OFFSET,.hold_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMC_PLLARM_HOLD_MASK,};

/*A9 pll - channel 1*/
static struct pll_chnl_clk CLK_NAME(a9_pll_chnl1) = {

	.clk = {
.flags = A9_PLL_CHNL1_CLK_FLAGS,.id =
		    CLK_A9_PLL_CHNL1_CLK_ID,.name =
		    A9_PLL_CHNL1_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_PLL_CHNL,.ops =
		    &gen_pll_chnl_clk_ops,},.ccu_clk =
	    &CLK_NAME(kproc),.pll_clk =
	    &CLK_NAME(a9_pll),.cfg_reg_offset =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET,.mdiv_mask =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_PLLARM_H_MDIV_MASK,.
	    mdiv_shift =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_PLLARM_H_MDIV_SHIFT,.
	    mdiv_max = 256,.pll_enableb_offset =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET,.out_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_PLLARM_H_ENB_CLKOUT_MASK,.
	    pll_load_ch_en_offset =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET,.load_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_PLLARM_H_LOAD_EN_MASK,.
	    pll_hold_ch_offset =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET,.hold_en_mask =
	    KPROC_CLK_MGR_REG_PLLARMCTRL5_PLLARM_H_HOLD_MASK,};

/*
Core clock name ARM
*/
static struct pll_chnl_clk *arm_pll_chnl[] =
    { &CLK_NAME(a9_pll_chnl0), &CLK_NAME(a9_pll_chnl1) };
static u32 freq_tbl[] = {
	FREQ_MHZ(26),
	FREQ_MHZ(52),
	FREQ_MHZ(156),
	FREQ_MHZ(156),
	FREQ_MHZ(312),
	FREQ_MHZ(312)
};

static struct core_clk CLK_NAME(a9_core) = {
	.clk = {
	.flags = ARM_CORE_CLK_FLAGS,.clk_type = CLK_TYPE_CORE,.id = CLK_ARM_CORE_CLK_ID,.name = ARM_CORE_CLK_NAME_STR,.dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops = &gen_core_clk_ops,},.ccu_clk = &CLK_NAME(kproc),.pll_clk = &CLK_NAME(a9_pll),.pll_chnl_clk = arm_pll_chnl,.num_chnls = 2,.active_policy = 1,	/*PI policy 5 */
.pre_def_freq = freq_tbl,.num_pre_def_freq =
	    ARRAY_SIZE(freq_tbl),.policy_bit_mask =
	    KPROC_CLK_MGR_REG_POLICY0_MASK_ARM_POLICY0_MASK_MASK,.
	    policy_mask_init =
	    DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_OFFSET,.clk_en_mask =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_ARM_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_ARM_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_ARM_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_ARM_HYST_EN_MASK,.
	    stprsts_mask =
	    KPROC_CLK_MGR_REG_CORE0_CLKGATE_ARM_STPRSTS_MASK,.
	    soft_reset_offset =
	    KPROC_RST_MGR_REG_A9_CORE_SOFT_RSTN_OFFSET,.
	    clk_reset_mask =
	    KPROC_RST_MGR_REG_A9_CORE_SOFT_RSTN_A9_CORE_0_SOFT_RSTN_MASK,};

/*
Bus clock name ARM_SWITCH
*/
static struct bus_clk CLK_NAME(arm_switch) = {

	.clk = {
.flags = ARM_SWITCH_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_ARM_SWITCH_CLK_ID,.name =
		    ARM_SWITCH_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kproc),.clk_gate_offset =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_OFFSET,.clk_en_mask =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_ARM_SWITCH_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_ARM_SWITCH_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_ARM_SWITCH_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_ARM_SWITCH_HYST_EN_MASK,.
	    stprsts_mask =
	    KPROC_CLK_MGR_REG_ARM_SWITCH_CLKGATE_ARM_SWITCH_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name APB0
*/
static struct bus_clk CLK_NAME(apb0)
    = {
	.clk = {
.flags = APB0_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB0_CLK_ID,.name =
		    APB0_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kproc),.clk_gate_offset =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_OFFSET,.clk_en_mask =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_APB0_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_APB0_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_APB0_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_APB0_HYST_EN_MASK,.
	    stprsts_mask =
	    KPROC_CLK_MGR_REG_APB0_CLKGATE_APB0_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KPROC_RST_MGR_REG_SOFT_RSTN_OFFSET,.clk_reset_mask =
	    KPROC_RST_MGR_REG_SOFT_RSTN_APB_SOFT_RSTN_MASK,};

static int dig_clk_set_gating_ctrl(struct peri_clk *peri_clk,
				   int clk_id, int gating_ctrl)
{
	u32 reg_val;
	int dig_ch0_req_shift;

	if (gating_ctrl != CLK_GATING_AUTO && gating_ctrl != CLK_GATING_SW)
		return -EINVAL;
	if (!peri_clk->clk_gate_offset)
		return -EINVAL;

	reg_val = readl(CCU_REG_ADDR(peri_clk->ccu_clk,
				     peri_clk->clk_gate_offset));
	if (peri_clk->gating_sel_mask) {
		if (gating_ctrl == CLK_GATING_SW)
			reg_val = SET_BIT_USING_MASK(reg_val,
						     peri_clk->gating_sel_mask);
		else
			reg_val = RESET_BIT_USING_MASK(reg_val,
						       peri_clk->
						       gating_sel_mask);

		writel(reg_val, CCU_REG_ADDR(peri_clk->ccu_clk,
					     peri_clk->clk_gate_offset));
	}

	reg_val = readl(CCU_REG_ADDR(peri_clk->ccu_clk,
				     ROOT_CLK_MGR_REG_DIG_AUTOGATE_OFFSET));
	switch (clk_id) {
	case CLK_DIG_CH0_PERI_CLK_ID:
		dig_ch0_req_shift =
		    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH0_CLK_REQ_ENABLE_SHIFT;
		break;
	case CLK_DIG_CH1_PERI_CLK_ID:
		dig_ch0_req_shift =
		    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH1_CLK_REQ_ENABLE_SHIFT;
		break;
	case CLK_DIG_CH2_PERI_CLK_ID:
		dig_ch0_req_shift =
		    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH2_CLK_REQ_ENABLE_SHIFT;
		break;
	case CLK_DIG_CH3_PERI_CLK_ID:
		dig_ch0_req_shift =
		    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH3_CLK_REQ_ENABLE_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	reg_val = reg_val & ~(DIG_CHANNEL_AUTO_GATE_REQ_MASK <<
			      dig_ch0_req_shift);
	if (gating_ctrl == CLK_GATING_AUTO)
		reg_val = reg_val | (DIG_CHANNEL_AUTO_GATE_REQ_MASK <<
				     dig_ch0_req_shift);

	writel(reg_val, CCU_REG_ADDR(peri_clk->ccu_clk,
				     ROOT_CLK_MGR_REG_DIG_AUTOGATE_OFFSET));

	return 0;
}

static int dig_clk_init(struct clk *clk)
{
	struct peri_clk *peri_clk;
	struct src_clk *src_clks;
	int inx;

	if (clk->clk_type != CLK_TYPE_PERI)
		return -EPERM;

	peri_clk = to_peri_clk(clk);
	BUG_ON(peri_clk->ccu_clk == NULL);

	clk_dbg("%s, clock name: %s\n", __func__, clk->name);
	clk->use_cnt = 0;
	/*Init source clocks */
	/*enable/disable src clk */
	BUG_ON(!PERI_SRC_CLK_VALID(peri_clk) &&
	       peri_clk->clk_div.pll_select_offset);

	/* enable write access */
	ccu_write_access_enable(peri_clk->ccu_clk, true);

	if (PERI_SRC_CLK_VALID(peri_clk)) {
		src_clks = &peri_clk->src_clk;
		for (inx = 0; inx < src_clks->count; inx++) {
			if (src_clks->clk[inx]->ops &&
			    src_clks->clk[inx]->ops->init)
				src_clks->clk[inx]->ops->init(src_clks->
							      clk[inx]);
		}
		/*set the default src clock */
		BUG_ON(peri_clk->src_clk.src_inx >= peri_clk->src_clk.count);
		peri_clk_set_pll_select(peri_clk, peri_clk->src_clk.src_inx);
	}

	if (clk->flags & AUTO_GATE)
		dig_clk_set_gating_ctrl(peri_clk, clk->id, CLK_GATING_AUTO);
	else
		dig_clk_set_gating_ctrl(peri_clk, clk->id, CLK_GATING_SW);

	BUG_ON(CLK_FLG_ENABLED(clk, ENABLE_ON_INIT) &&
	       CLK_FLG_ENABLED(clk, DISABLE_ON_INIT));

	if (CLK_FLG_ENABLED(clk, ENABLE_ON_INIT)) {
		if (clk->ops && clk->ops->enable)
			clk->ops->enable(clk, 1);
	} else if (CLK_FLG_ENABLED(clk, DISABLE_ON_INIT)) {
		if (clk->ops->enable)
			clk->ops->enable(clk, 0);
	}
	/* Disable write access */
	ccu_write_access_enable(peri_clk->ccu_clk, false);
	clk->init = 1;
	clk_dbg("***%s: peri clock %s count after init %d *****\n",
		__func__, clk->name, clk->use_cnt);

	return 0;
}

struct gen_clk_ops dig_ch_peri_clk_ops;
/*
Peri clock name DIG_CH0
*/
/*Source list of digital channels. Common for CH0, CH1, CH2, CH3 */
static struct clk *dig_ch_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(pll0), CLK_PTR(pll1));
static struct peri_clk CLK_NAME(dig_ch0)
    = {
	.clk = {
	.flags = DIG_CH0_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_DIG_CH0_PERI_CLK_ID,.name =
		    DIG_CH0_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &dig_ch_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_DIG_AUTOGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH0_SYS_CLK_REQ_A_ENABLE_MASK
	    |
	    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH0_SYS_CLK_REQ_B_ENABLE_MASK
	    | ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH0_CLK_REQ_ENABLE_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_DIG_AUTOGATE_DIGITAL_CH0_SYS_CLK_REQ_A_ENABLE_MASK,.
	    clk_div = {
	.div_offset = ROOT_CLK_MGR_REG_DIG0_DIV_OFFSET,.div_mask =
		    ROOT_CLK_MGR_REG_DIG0_DIV_DIGITAL_CH0_DIV_MASK,.
		    div_shift =
		    ROOT_CLK_MGR_REG_DIG0_DIV_DIGITAL_CH0_DIV_SHIFT,.
		    pre_div_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.pre_div_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_MASK,.
		    pre_div_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_SHIFT,.
		    div_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.div_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_CH0_TRIGGER_MASK,.
		    prediv_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.prediv_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.
		    pll_select_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_SHIFT},.
	    src_clk = {
.count = ARRAY_SIZE(dig_ch_peri_clk_src_list),.src_inx =
		    0,.clk = dig_ch_peri_clk_src_list,},};

/*
Peri clock name DIG_CH1
*/
static struct peri_clk CLK_NAME(dig_ch1)
    = {
	.clk = {
	.flags = DIG_CH1_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_DIG_CH1_PERI_CLK_ID,.name =
		    DIG_CH1_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &dig_ch_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_STPRSTS_MASK,.clk_div = {
	.div_offset = ROOT_CLK_MGR_REG_DIG1_DIV_OFFSET,.div_mask =
		    ROOT_CLK_MGR_REG_DIG1_DIV_DIGITAL_CH1_DIV_MASK,.
		    div_shift =
		    ROOT_CLK_MGR_REG_DIG1_DIV_DIGITAL_CH1_DIV_SHIFT,.
		    pre_div_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.pre_div_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_MASK,.
		    pre_div_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_SHIFT,.
		    div_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.div_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_CH1_TRIGGER_MASK,.
		    prediv_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.prediv_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.
		    pll_select_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_SHIFT},.
	    src_clk = {
.count = ARRAY_SIZE(dig_ch_peri_clk_src_list),.src_inx =
		    0,.clk = dig_ch_peri_clk_src_list,},};

/*
Peri clock name DIG_CH2
*/
static struct peri_clk CLK_NAME(dig_ch2)
    = {
	.clk = {
	.flags = DIG_CH2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_DIG_CH2_PERI_CLK_ID,.name =
		    DIG_CH2_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &dig_ch_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET,.clk_en_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH2_CLK_EN_MASK,.
	    gating_sel_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH2_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH2_STPRSTS_MASK,.clk_div = {
	.div_offset = ROOT_CLK_MGR_REG_DIG2_DIV_OFFSET,.div_mask =
		    ROOT_CLK_MGR_REG_DIG2_DIV_DIGITAL_CH2_DIV_MASK,.
		    div_shift =
		    ROOT_CLK_MGR_REG_DIG2_DIV_DIGITAL_CH2_DIV_SHIFT,.
		    pre_div_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.pre_div_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_MASK,.
		    pre_div_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_SHIFT,.
		    div_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.div_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_CH2_TRIGGER_MASK,.
		    prediv_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.prediv_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.
		    pll_select_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_SHIFT},.
	    src_clk = {
.count = ARRAY_SIZE(dig_ch_peri_clk_src_list),.src_inx =
		    0,.clk = dig_ch_peri_clk_src_list,},};

/*
Peri clock name DIG_CH3
*/
static struct peri_clk CLK_NAME(dig_ch3)
    = {
	.clk = {
	.flags = DIG_CH3_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_DIG_CH3_PERI_CLK_ID,.name =
		    DIG_CH3_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &dig_ch_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(root),.clk_gate_offset =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET,.clk_en_mask = 0,.stprsts_mask =
	    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH3_STPRSTS_MASK,.clk_div = {
	.div_offset = ROOT_CLK_MGR_REG_DIG3_DIV_OFFSET,.div_mask =
		    ROOT_CLK_MGR_REG_DIG3_DIV_DIGITAL_CH3_DIV_MASK,.
		    div_shift =
		    ROOT_CLK_MGR_REG_DIG3_DIV_DIGITAL_CH3_DIV_SHIFT,.
		    pre_div_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.pre_div_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_MASK,.
		    pre_div_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_DIV_SHIFT,.
		    div_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.div_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_CH3_TRIGGER_MASK,.
		    prediv_trig_offset =
		    ROOT_CLK_MGR_REG_DIG_TRG_OFFSET,.prediv_trig_mask =
		    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET,.
		    pll_select_mask =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_SHIFT},.
	    src_clk = {
.count = ARRAY_SIZE(dig_ch_peri_clk_src_list),.src_inx =
		    0,.clk = dig_ch_peri_clk_src_list,},};

/*
CCU clock name KHUB
*/
/* CCU freq list */
static u32 khub_clk_freq_list0[] =
DEFINE_ARRAY_ARGS(26000000, 26000000, 26000000, 26000000);
static u32 khub_clk_freq_list1[] =
DEFINE_ARRAY_ARGS(52000000, 52000000, 52000000, 52000000);
static u32 khub_clk_freq_list2[] =
DEFINE_ARRAY_ARGS(104000000, 104000000, 52000000, 52000000);
static u32 khub_clk_freq_list3[] =
DEFINE_ARRAY_ARGS(156000000, 156000000, 78000000, 78000000);
static u32 khub_clk_freq_list4[] =
DEFINE_ARRAY_ARGS(156000000, 156000000, 78000000, 78000000);
static u32 khub_clk_freq_list5[] =
DEFINE_ARRAY_ARGS(208000000, 104000000, 104000000, 104000000);
static u32 khub_clk_freq_list6[] =
DEFINE_ARRAY_ARGS(208000000, 104000000, 104000000, 104000000);

static struct ccu_clk CLK_NAME(khub) = {

	.clk = {
	.flags = KHUB_CCU_CLK_FLAGS,.id = CLK_KHUB_CCU_CLK_ID,.name =
		    KHUB_CCU_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_CCU,.ops = &gen_ccu_clk_ops,},.ccu_ops =
	    &gen_ccu_ops,.pi_id = PI_MGR_PI_ID_HUB_SWITCHABLE,
//      .pi_id = -1,
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(HUB_CLK_BASE_ADDR),.wr_access_offset =
	    KHUB_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_OFFSET,.policy_mask2_offset =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_OFFSET,.policy_freq_offset =
	    KHUB_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    KHUB_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    KHUB_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    KHUB_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    KHUB_CLK_MGR_REG_VLT_PERI_OFFSET,.lvm_en_offset =
	    KHUB_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    KHUB_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    KHUB_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    KHUB_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    KHUB_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    KHUB_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    KHUB_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,
#endif
.freq_volt = HUB_CCU_FREQ_VOLT_TBL,.freq_count =
	    HUB_CCU_FREQ_VOLT_TBL_SZ,.volt_peri =
	    DEFINE_ARRAY_ARGS(VLT_NORMAL_PERI,
					  VLT_HIGH_PERI),.
	    freq_policy =
	    DEFINE_ARRAY_ARGS(HUB_CCU_FREQ_POLICY_TBL),.freq_tbl =
	    ARRAY_LIST(khub_clk_freq_list0, khub_clk_freq_list1,
				   khub_clk_freq_list2,
				   khub_clk_freq_list3,
				   khub_clk_freq_list4,
				   khub_clk_freq_list5,
				   khub_clk_freq_list6),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(HUB_RST_BASE_ADDR),.
	    reset_wr_access_offset = KHUB_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
Bus clock name NOR_APB
*/
static struct bus_clk CLK_NAME(nor_apb) = {

	.clk = {
.flags = NOR_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_NOR_APB_BUS_CLK_ID,.name =
		    NOR_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_APB_STPRSTS_MASK,.
	    freq_tbl_index = 3,.src_clk = NULL,};

/*
Bus clock name TMON_APB
*/
static struct bus_clk CLK_NAME(tmon_apb) = {

	.clk = {
.flags = TMON_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_TMON_APB_BUS_CLK_ID,.name =
		    TMON_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name APB5
*/
static struct bus_clk CLK_NAME(apb5) = {

	.clk = {
.flags = APB5_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB5_BUS_CLK_ID,.name =
		    APB5_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_APB5_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_APB5_CLKGATE_APB5_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_APB5_CLKGATE_APB5_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_APB5_CLKGATE_APB5_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_APB5_SOFT_RSTN_MASK,};

/*
Bus clock name CTI_APB
*/
static struct bus_clk CLK_NAME(cti_apb) = {

	.clk = {
.flags = CTI_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_CTI_APB_BUS_CLK_ID,.name =
		    CTI_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_CTI_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_CTI_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_CTI_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_CTI_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_CTI_CLKGATE_CTI_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_CTI_SOFT_RSTN_MASK,};

/*
Bus clock name FUNNEL_APB
*/
static struct bus_clk CLK_NAME(funnel_apb) = {

	.clk = {
.flags = FUNNEL_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_FUNNEL_APB_BUS_CLK_ID,.name =
		    FUNNEL_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_FUNNEL_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_FUNNEL_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_FUNNEL_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_FUNNEL_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_FUNNEL_CLKGATE_FUNNEL_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_FUNNEL_SOFT_RSTN_MASK,};

/*
Bus clock name IPC_APB
*/
static struct bus_clk CLK_NAME(ipc_apb)
    = {

	.clk = {
.flags = IPC_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_IPC_APB_BUS_CLK_ID,.name =
		    IPC_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_IPC_SEC_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_IPC_SEC_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_IPC_SEC_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_IPC_SEC_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_IPC_CLKGATE_IPC_SEC_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_IPC_SOFT_RSTN_MASK,};

/*
Bus clock name SRAM_MPU_APB
*/
static struct bus_clk CLK_NAME(sram_mpu_apb)
    = {

	.clk = {
.flags = SRAM_MPU_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SRAM_MPU_APB_BUS_CLK_ID,.name =
		    SRAM_MPU_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_SRAM_MPU_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_SRAM_MPU_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_SRAM_MPU_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_SRAM_MPU_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SRAM_MPU_CLKGATE_SRAM_MPU_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_SRAM_MPU_SOFT_RSTN_MASK,};

/*
Bus clock name TPIU_APB
*/
static struct bus_clk CLK_NAME(tpiu_apb) = {

	.clk = {
.flags = TPIU_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_TPIU_APB_BUS_CLK_ID,.name =
		    TPIU_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_TPIU_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_TPIU_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_TPIU_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_TPIU_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_TPIU_CLKGATE_TPIU_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_TPIU_SOFT_RSTN_MASK,};

/*
Bus clock name VC_ITM_APB
*/
static struct bus_clk CLK_NAME(vc_itm_apb) = {

	.clk = {
.flags = VC_ITM_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_VC_ITM_APB_BUS_CLK_ID,.name =
		    VC_ITM_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_VC_ITM_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_VC_ITM_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_VC_ITM_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_VC_ITM_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_VC_ITM_CLKGATE_VC_ITM_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_VC_ITM_SOFT_RSTN_MASK,};

/*
Bus clock name SEC_VIOL_TRAP7
*/
static struct bus_clk CLK_NAME(sec_viol_trap7_apb)
    = {

	.clk = {
.flags = SEC_VIOL_TRAP7_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SEC_VIOL_TRAP7_APB_BUS_CLK_ID,.name =
		    SEC_VIOL_TRAP7_APB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_SEC_VIOL_TRAP_7_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_SEC_VIOL_TRAP_7_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_SEC_VIOL_TRAP_7_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_SEC_VIOL_TRAP_7_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SECTRAP7_CLKGATE_SEC_VIOL_TRAP_7_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name SEC_VIOL_TRAP6
*/
static struct bus_clk CLK_NAME(sec_viol_trap6_apb)
    = {

	.clk = {
.flags = SEC_VIOL_TRAP6_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SEC_VIOL_TRAP6_APB_BUS_CLK_ID,.name =
		    SEC_VIOL_TRAP6_APB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_SEC_VIOL_TRAP_6_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_SEC_VIOL_TRAP_6_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_SEC_VIOL_TRAP_6_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_SEC_VIOL_TRAP_6_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SECTRAP6_CLKGATE_SEC_VIOL_TRAP_6_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};
/*
Bus clock name SEC_VIOL_TRAP5
*/
static struct bus_clk CLK_NAME(sec_viol_trap5_apb)
    = {

	.clk = {
.flags = SEC_VIOL_TRAP5_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SEC_VIOL_TRAP5_APB_BUS_CLK_ID,.name =
		    SEC_VIOL_TRAP5_APB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_SEC_VIOL_TRAP_5_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_SEC_VIOL_TRAP_5_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_SEC_VIOL_TRAP_5_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_SEC_VIOL_TRAP_5_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SECTRAP5_CLKGATE_SEC_VIOL_TRAP_5_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name SEC_VIOL_TRAP4
*/
static struct bus_clk CLK_NAME(sec_viol_trap4_apb)
    = {

	.clk = {
.flags = SEC_VIOL_TRAP4_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SEC_VIOL_TRAP4_APB_BUS_CLK_ID,.name =
		    SEC_VIOL_TRAP4_APB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_SEC_VIOL_TRAP_4_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_SEC_VIOL_TRAP_4_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_SEC_VIOL_TRAP_4_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_SEC_VIOL_TRAP_4_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SECTRAP4_CLKGATE_SEC_VIOL_TRAP_4_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name AXI_TRACE19_APB
*/
static struct bus_clk CLK_NAME(axi_trace19_apb)
    = {

	.clk = {
.flags = AXI_TRACE19_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_AXI_TRACE19_APB_BUS_CLK_ID,.name =
		    AXI_TRACE19_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_AXI_TRACE_19_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_AXI_TRACE_19_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_AXI_TRACE_19_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_AXI_TRACE_19_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE19_CLKGATE_AXI_TRACE_19_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name AXI_TRACE11_APB
*/
static struct bus_clk CLK_NAME(axi_trace11_apb)
    = {

	.clk = {
.flags = AXI_TRACE11_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_AXI_TRACE11_APB_BUS_CLK_ID,.name =
		    AXI_TRACE11_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_AXI_TRACE_11_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_AXI_TRACE_11_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_AXI_TRACE_11_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_AXI_TRACE_11_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE11_CLKGATE_AXI_TRACE_11_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name AXI_TRACE12_APB
*/
static struct bus_clk CLK_NAME(axi_trace12_apb)
    = {

	.clk = {
.flags = AXI_TRACE12_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_AXI_TRACE12_APB_BUS_CLK_ID,.name =
		    AXI_TRACE12_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_AXI_TRACE_12_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_AXI_TRACE_12_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_AXI_TRACE_12_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_AXI_TRACE_12_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE12_CLKGATE_AXI_TRACE_12_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name AXI_TRACE13_APB
*/
static struct bus_clk CLK_NAME(axi_trace13_apb)
    = {

	.clk = {
.flags = AXI_TRACE13_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_AXI_TRACE13_APB_BUS_CLK_ID,.name =
		    AXI_TRACE13_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_AXI_TRACE_13_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_AXI_TRACE_13_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_AXI_TRACE_13_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_AXI_TRACE_13_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AXI_TRACE13_CLKGATE_AXI_TRACE_13_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

static struct bus_clk CLK_NAME(hub_switch_apb)
    = {

	.clk = {
.flags = HUB_SWITCH_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_HUB_SWITCH_APB_BUS_CLK_ID,.name =
		    HUB_SWITCH_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_SWITCH_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_SWITCH_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_SWITCH_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_SWITCH_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_SWITCH_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name HSI_APB
*/
static struct bus_clk CLK_NAME(hsi_apb) = {

	.clk = {
.flags = HSI_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_HSI_APB_BUS_CLK_ID,.name =
		    HSI_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_HSI_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_HSI_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_HSI_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_HSI_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_HSI_CLKGATE_HSI_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_HSI_SOFT_RSTN_MASK,};

/*
Bus clock name ETB_APB
*/
static struct bus_clk CLK_NAME(etb_apb) = {

	.clk = {
.flags = ETB_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_ETB_APB_BUS_CLK_ID,.name =
		    ETB_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_ETB_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_ETB_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_ETB_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_ETB_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_ETB_CLKGATE_ETB_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_ETB_SOFT_RSTN_MASK,};

/*
Bus clock name FINAL_FUNNEL_APB
*/
static struct bus_clk CLK_NAME(final_funnel_apb) = {

	.clk = {
.flags = FINAL_FUNNEL_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_FINAL_FUNNEL_APB_BUS_CLK_ID,.name =
		    FINAL_FUNNEL_APB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_FINAL_FUNNEL_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_FINAL_FUNNEL_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_FINAL_FUNNEL_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_FINAL_FUNNEL_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_FINAL_FUNNEL_CLKGATE_FINAL_FUNNEL_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_FINAL_FUNNEL_SOFT_RSTN_MASK,};

/*
Bus clock name APB10
*/
static struct bus_clk CLK_NAME(apb10) = {

	.clk = {
.flags = APB10_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB10_BUS_CLK_ID,.name =
		    APB10_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_APB10_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_APB10_CLKGATE_APB10_CLK_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_APB10_CLKGATE_APB10_STPRSTS_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_APB10_CLKGATE_APB10_HW_SW_GATING_SEL_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_APB10_SOFT_RSTN_MASK,};

/*
Bus clock name APB9
*/
static struct bus_clk CLK_NAME(apb9) = {

	.clk = {
.flags = APB9_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB9_BUS_CLK_ID,.name =
		    APB9_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_APB9_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_APB9_CLKGATE_APB9_CLK_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_APB9_CLKGATE_APB9_STPRSTS_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_APB9_CLKGATE_APB9_HW_SW_GATING_SEL_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_APB9_SOFT_RSTN_MASK,};

/*
Bus clock name ATB_FILTER_APB
*/
static struct bus_clk CLK_NAME(atb_filter_apb) = {

	.clk = {
.flags = ATB_FILTER_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_ATB_FILTER_APB_BUS_CLK_ID,.name =
		    ATB_FILTER_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_ATB_FILTER_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_ATB_FILTER_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_ATB_FILTER_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_ATB_FILTER_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_ATB_CLKGATE_ATB_FILTER_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_ATB_FILTER_SOFT_RSTN_MASK,};

/*
Peri clock name AUDIOH_26M
*/
/*peri clk src list*/
static struct clk *audioh_26m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_26m));
static struct peri_clk CLK_NAME(audioh_26m) = {

	.clk = {
	.flags = AUDIOH_26M_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_AUDIOH_26M_PERI_CLK_ID,.name =
		    AUDIOH_26M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_AUDIOH_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_26M_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_26M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_26M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_26M_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_26M_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset =
		    KHUB_CLK_MGR_REG_AUDIOH_DIV_OFFSET,.
		    div_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    div_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_AUDIOH_26M_TRIGGER_MASK,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_AUDIOH_DIV_AUDIOH_26M_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_AUDIOH_DIV_AUDIOH_26M_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 0,.clk =
		    audioh_26m_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_AUDIOH_SOFT_RSTN_MASK,};

/*
Peri clock name HUB
*/
/*peri clk src list*/
static struct clk *hub_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_208m));
static struct peri_clk CLK_NAME(hub_clk) = {
	.clk = {
	.flags = HUB_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_HUB_PERI_CLK_ID,.name =
		    HUB_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    1,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_HUB_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KHUB_CLK_MGR_REG_HUB_DIV_OFFSET,
//      .div_mask = KHUB_CLK_MGR_REG_HUB_DIV_HUB_DIV_MASK,              /* Not in Capri */
//      .div_shift = KHUB_CLK_MGR_REG_HUB_DIV_HUB_DIV_SHIFT,
	.div_trig_offset =
		    KHUB_CLK_MGR_REG_HUB_SEG_TRG_OFFSET,.
		    div_trig_mask =
		    KHUB_CLK_MGR_REG_HUB_SEG_TRG_HUB_TRIGGER_MASK,.
		    diether_bits = 1,.pll_select_offset =
		    KHUB_CLK_MGR_REG_HUB_DIV_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_HUB_DIV_HUB_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_HUB_DIV_HUB_PLL_SELECT_SHIFT,},.src_clk = {
.count = 2,.src_inx = 1,.clk = hub_peri_clk_src_list,},};

/*
Bus clock name ETB2AXI_APB
*/
static struct bus_clk CLK_NAME(etb2axi_apb) = {

	.clk = {
.flags = ETB2AXI_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_ETB2AXI_APB_BUS_CLK_ID,.name =
		    ETB2AXI_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_ETB2AXI_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_ETB2AXI_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_ETB2AXI_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_ETB2AXI_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_ETB2AXI_CLKGATE_ETB2AXI_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name AUDIOH_APB
*/
static struct bus_clk CLK_NAME(audioh_apb) = {

	.clk = {
.flags = AUDIOH_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_AUDIOH_APB_BUS_CLK_ID,.name =
		    AUDIOH_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_26m),
						      NULL),.
		    ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(audioh_26m),};

/*
Bus clock name SSP3_APB
*/
static struct bus_clk CLK_NAME(ssp3_apb) = {

	.clk = {
.flags = SSP3_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP3_APB_BUS_CLK_ID,.name =
		    SSP3_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_26m),
						      NULL),.
		    ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(audioh_26m),};

/*
Bus clock name SSP4_APB
*/
static struct bus_clk CLK_NAME(ssp4_apb) = {

	.clk = {
.flags = SSP4_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP4_APB_BUS_CLK_ID,.name =
		    SSP4_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_26m),
						      NULL),.
		    ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(audioh_26m),};

/*
Bus clock name SSP5_APB
*/
static struct bus_clk CLK_NAME(ssp5_apb) = {

	.clk = {
.flags = SSP5_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP5_APB_BUS_CLK_ID,.name =
		    SSP5_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_26m),
						      NULL),.
		    ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(audioh_26m),};

/*
Bus clock name SSP6_APB
*/
static struct bus_clk CLK_NAME(ssp6_apb) = {

	.clk = {
.flags = SSP6_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP6_APB_BUS_CLK_ID,.name =
		    SSP6_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_26m),
						      NULL),.
		    ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(audioh_26m),};

/*
Bus clock name VAR_SPM_APB
*/
static struct bus_clk CLK_NAME(var_spm_apb) = {

	.clk = {
.flags = VAR_SPM_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_VAR_SPM_APB_BUS_CLK_ID,.name =
		    VAR_SPM_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_VAR_SPM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_VAR_SPM_CLKGATE_VAR_SPM_APB_CLK_EN_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_VAR_SPM_CLKGATE_VAR_SPM_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_VAR_SPM_CLKGATE_VAR_SPM_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_VAR_SPM_CLKGATE_VAR_SPM_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_VAR_SPM_SOFT_RSTN_MASK,};

/*
Bus clock name NOR
*/
static struct bus_clk CLK_NAME(nor) = {

	.clk = {
.flags = NOR_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_NOR_BUS_CLK_ID,.name =
		    NOR_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_NOR_SOFT_RSTN_MASK,};

/*
Peri clock name NOR_ASYNC
*/
/*peri clk src list*/
/* default value of nor_var_clk is 52M, so using ref_52M clk for source for now
 * as nor_var_clk is not defined*/
static struct clk *nor_async_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_52m));
static struct peri_clk clk_nor_async = {
	.clk = {
		.flags = NOR_ASYNC_PERI_CLK_FLAGS,
		.clk_type = CLK_TYPE_PERI,
		.id = CLK_NOR_ASYNC_PERI_CLK_ID,
		.name = NOR_ASYNC_PERI_CLK_NAME_STR,
		.dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(audioh_apb), NULL),
		.ops = &gen_peri_clk_ops,
		},
	.ccu_clk = &CLK_NAME(khub),
	.clk_gate_offset = KHUB_CLK_MGR_REG_NOR_CLKGATE_OFFSET,
	.clk_en_mask = KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_ASYNC_CLK_EN_MASK,
	.gating_sel_mask =
	    KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_ASYNC_HW_SW_GATING_SEL_MASK,
	.hyst_val_mask = KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_ASYNC_HYST_VAL_MASK,
	.hyst_en_mask = KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_ASYNC_HYST_EN_MASK,
	.stprsts_mask = KHUB_CLK_MGR_REG_NOR_CLKGATE_NOR_ASYNC_STPRSTS_MASK,

	.src_clk = {
		    .count = ARRAY_SIZE(nor_async_peri_clk_src_list),
		    .src_inx = 1,
		    .clk = nor_async_peri_clk_src_list,
		    },
};

/*
Peri clock name AUDIOH_2P4M
*/
/*peri clk src list*/
static struct clk *audioh_2p4m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m));
static struct peri_clk CLK_NAME(audioh_2p4m) = {

	.clk = {
	.flags = AUDIOH_2P4M_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_AUDIOH_2P4M_PERI_CLK_ID,.name =
		    AUDIOH_2P4M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_AUDIOH_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_2P4M_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_2P4M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_2P4M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_2P4M_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_2P4M_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_VOLTAGE_LEVEL_MASK,.src_clk =
	{
.count = 1,.src_inx = 0,.clk =
		    audioh_2p4m_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_AUDIOH_SOFT_RSTN_MASK,};

/*
Peri clock name AUDIOH_156M
*/
/*peri clk src list*/
static struct clk *audioh_156m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m));
static struct peri_clk CLK_NAME(audioh_156m) = {

	.clk = {
	.flags = AUDIOH_156M_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_AUDIOH_156M_PERI_CLK_ID,.name =
		    AUDIOH_156M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(audioh_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_AUDIOH_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_156M_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_156M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_156M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_156M_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_156M_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_AUDIOH_VOLTAGE_LEVEL_MASK,.src_clk =
	{
.count = 1,.src_inx = 0,.clk =
		    audioh_156m_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_AUDIOH_SOFT_RSTN_MASK,};

/*
Peri clock name SSP3_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp3_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp3_audio) = {

	.clk = {
	.flags = SSP3_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP3_AUDIO_PERI_CLK_ID,.name =
		    SSP3_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp3_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_SSP3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_AUDIO_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_DIV_MASK,.
		    div_shift =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_PRE_DIV_SHIFT,.
		    div_trig_offset = KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,
//                                      .div_trig_mask= KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP3_AUDIO_TRIGGER_MASK, // not in Capri per item 18
	.prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP3_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP3_AUDIO_DIV_SSP3_AUDIO_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk = ssp3_audio_peri_clk_src_list,},};

/*
Peri clock name SSP3
*/
/*peri clk src list*/
static struct clk *ssp3_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(ssp3) = {

	.clk = {
	.flags = SSP3_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP3_PERI_CLK_ID,.name =
		    SSP3_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp3_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_SSP3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP3_CLKGATE_SSP3_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_SSP3_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_DIV_MASK,.div_shift =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP3_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP3_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP3_DIV_OFFSET,.pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP3_DIV_SSP3_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    ssp3_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_SSP3_SOFT_RSTN_MASK,};

/*
Peri clock name SSP4_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp4_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp4_audio) = {

	.clk = {
	.flags = SSP4_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP4_AUDIO_PERI_CLK_ID,.name =
		    SSP4_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp4_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP4_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_AUDIO_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_DIV_MASK,.
		    div_shift =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_PRE_DIV_SHIFT,.
		    div_trig_offset = KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,
//                                      .div_trig_mask= KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP4_AUDIO_TRIGGER_MASK, // not in Capri per Item 18
	.prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP4_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP4_AUDIO_DIV_SSP4_AUDIO_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk = ssp4_audio_peri_clk_src_list,},};

/*
Peri clock name SSP5_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp5_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp5_audio) = {

	.clk = {
	.flags = SSP5_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP5_AUDIO_PERI_CLK_ID,.name =
		    SSP5_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp5_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP5_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_AUDIO_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_DIV_MASK,.
		    div_shift =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_PRE_DIV_SHIFT,.
		    div_trig_offset = KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,
//                                      .div_trig_mask= KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP5_AUDIO_TRIGGER_MASK, // not in Capri per Item 18
	.prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP5_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP5_AUDIO_DIV_SSP5_AUDIO_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk = ssp5_audio_peri_clk_src_list,},};

/*
Peri clock name SSP6_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp6_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp6_audio) = {

	.clk = {
	.flags = SSP6_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP6_AUDIO_PERI_CLK_ID,.name =
		    SSP6_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp6_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP6_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_AUDIO_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_DIV_MASK,.
		    div_shift =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_PRE_DIV_SHIFT,.
		    div_trig_offset = KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,
//                                      .div_trig_mask= KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP6_AUDIO_TRIGGER_MASK, // not in Capri per Item 18
	.prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP6_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP6_AUDIO_DIV_SSP6_AUDIO_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 2,.clk = ssp6_audio_peri_clk_src_list,},};

/*
Peri clock name SSP4
*/
/*peri clk src list*/
static struct clk *ssp4_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(ssp4) = {

	.clk = {
	.flags = SSP4_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP4_PERI_CLK_ID,.name =
		    SSP4_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp4_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP4_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP4_CLKGATE_SSP4_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_SSP4_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_DIV_MASK,.div_shift =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP4_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP4_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP4_DIV_OFFSET,.pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP4_DIV_SSP4_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    ssp4_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_SSP4_SOFT_RSTN_MASK,};

/*
Peri clock name SSP5
*/
/*peri clk src list*/
static struct clk *ssp5_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(ssp5) = {

	.clk = {
	.flags = SSP5_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP5_PERI_CLK_ID,.name =
		    SSP5_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp5_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP5_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP5_CLKGATE_SSP5_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_SSP5_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_DIV_MASK,.div_shift =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP5_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP5_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP5_DIV_OFFSET,.pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP5_DIV_SSP5_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    ssp5_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_SSP5_SOFT_RSTN_MASK,};

/*
Peri clock name SSP6
*/
/*peri clk src list*/
static struct clk *ssp6_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(ssp6) = {

	.clk = {
	.flags = SSP6_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP6_PERI_CLK_ID,.name =
		    SSP6_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp6_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_SSP6_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_SSP6_CLKGATE_SSP6_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_SSP6_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_DIV_MASK,.div_shift =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_DIV_SHIFT,.
		    pre_div_offset =
		    KHUB_CLK_MGR_REG_SSP6_DIV_OFFSET,.pre_div_mask =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_SSP6_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_SSP6_DIV_OFFSET,.pll_select_mask =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_SSP6_DIV_SSP6_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    ssp6_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_SSP6_SOFT_RSTN_MASK,};

/*
Peri clock name TMON_1M
*/
/*peri clk src list*/
static struct clk *tmon_1m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(frac_1m));
static struct peri_clk CLK_NAME(tmon_1m) = {

	.clk = {
	.flags = TMON_1M_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_TMON_1M_PERI_CLK_ID,.name =
		    TMON_1M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(tmon_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_TMON_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_1M_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_1M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_1M_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_1M_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_1M_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_TMON_CLKGATE_TMON_VOLTAGE_LEVEL_MASK,.clk_div = {
	.pll_select_offset =
		    KHUB_CLK_MGR_REG_TMON_DIV_DBG_OFFSET,.
		    pll_select_mask =
		    KHUB_CLK_MGR_REG_TMON_DIV_DBG_TMON_1M_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_TMON_DIV_DBG_TMON_1M_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 1,.clk =
		    tmon_1m_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_TMON_SOFT_RSTN_MASK,};

/*
Peri clock name DAP_SWITCH
*/
/*peri clk src list*/
static struct peri_clk CLK_NAME(dap_switch) = {

	.clk = {
.flags = DAP_SWITCH_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_DAP_SWITCH_PERI_CLK_ID,.name =
		    DAP_SWITCH_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khub),.mask_set = 2,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK2_DAP_SWITCH_POLICY0_MASK_MASK,.
	    policy_mask_init =
	    DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_DAP_SWITCH_CLKGATE_DAP_SWITCH_VOLTAGE_LEVEL_MASK,};

/*
Peri clock name BROM
*/
/*peri clk src list*/
static struct clk *brom_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(hub_clk));
static struct peri_clk CLK_NAME(brom) = {

	.clk = {
	.flags = BROM_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_BROM_PERI_CLK_ID,.name =
		    BROM_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    1,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_BROM_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_CLK_EN_MASK,.gating_sel_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_HYST_VAL_MASK,.hyst_en_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_HYST_EN_MASK,.stprsts_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_STPRSTS_MASK,.volt_lvl_mask =
	    KHUB_CLK_MGR_REG_BROM_CLKGATE_BROM_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_HUB_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_HUB_DIV_BROM_DIV_MASK,.div_shift =
		    KHUB_CLK_MGR_REG_HUB_DIV_BROM_DIV_SHIFT,},.src_clk = {
.count = ARRAY_SIZE(brom_peri_clk_src_list),.src_inx = 0,.clk =
		    brom_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN0_BROM_SOFT_RSTN_MASK,};

/*
Peri clock name MDIOMASTER
*/
/*peri clk src list*/
static struct clk *mdiomaster_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal));
static struct peri_clk CLK_NAME(mdiomaster) = {

	.clk = {
	.flags = MDIOMASTER_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_MDIOMASTER_PERI_CLK_ID,.name =
		    MDIOMASTER_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),.mask_set =
	    1,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_MDIOMASTER_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_MDIO_CLKGATE_MDIOMASTER_VOLTAGE_LEVEL_MASK,.
	    src_clk = {
.count = 1,.src_inx = 0,.clk =
		    mdiomaster_peri_clk_src_list,},.soft_reset_offset =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_OFFSET,.clk_reset_mask =
	    KHUB_RST_MGR_REG_SOFT_RSTN1_MDIOMASTER_SOFT_RSTN_MASK,};

/*
CCU clock name KHUBAON
*/
/* CCU freq list */
static u32 khubaon_clk_freq_list0[] = DEFINE_ARRAY_ARGS(26000000, 26000000);
static u32 khubaon_clk_freq_list1[] = DEFINE_ARRAY_ARGS(52000000, 52000000);
static u32 khubaon_clk_freq_list2[] = DEFINE_ARRAY_ARGS(78000000, 78000000);
static u32 khubaon_clk_freq_list3[] = DEFINE_ARRAY_ARGS(104000000, 52000000);
static u32 khubaon_clk_freq_list4[] = DEFINE_ARRAY_ARGS(156000000, 78000000);

static struct ccu_clk CLK_NAME(khubaon) = {

	.clk = {
	.flags = KHUBAON_CCU_CLK_FLAGS,.id =
		    CLK_KHUBAON_CCU_CLK_ID,.name =
		    KHUBAON_CCU_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_CCU,.ops = &gen_ccu_clk_ops,},.ccu_ops =
	    &gen_ccu_ops,.pi_id = PI_MGR_PI_ID_HUB_AON,
//      .pi_id = -1,
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(AON_CLK_BASE_ADDR),.wr_access_offset =
	    KHUBAON_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_OFFSET,.policy_mask2_offset =
	    0,.policy_freq_offset =
	    KHUBAON_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    KHUBAON_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    KHUBAON_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    KHUBAON_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    KHUBAON_CLK_MGR_REG_VLT_PERI_OFFSET,.lvm_en_offset =
	    KHUBAON_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    KHUBAON_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    KHUBAON_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    KHUBAON_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    KHUBAON_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    KHUBAON_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    KHUBAON_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,
#endif
.freq_volt = AON_CCU_FREQ_VOLT_TBL,.freq_count =
	    AON_CCU_FREQ_VOLT_TBL_SZ,.volt_peri =
	    DEFINE_ARRAY_ARGS(VLT_NORMAL_PERI,
					  VLT_HIGH_PERI),.
	    freq_policy =
	    DEFINE_ARRAY_ARGS(AON_CCU_FREQ_POLICY_TBL),.freq_tbl =
	    DEFINE_ARRAY_ARGS(khubaon_clk_freq_list0,
					  khubaon_clk_freq_list1,
					  khubaon_clk_freq_list2,
					  khubaon_clk_freq_list3,
					  khubaon_clk_freq_list4),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(AON_RST_BASE_ADDR),.
	    reset_wr_access_offset = KHUBAON_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
Ref clock name PMU_BSC_VAR
*/
static struct clk *pmu_bsc_var_ref_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(var_312m), CLK_PTR(ref_312m));
static struct ref_clk CLK_NAME(pmu_bsc_var) = {
	.clk = {
	.flags = PMU_BSC_VAR_REF_CLK_FLAGS,.clk_type =
		    CLK_TYPE_REF,.id =
		    CLK_PMU_BSC_VAR_REF_CLK_ID,.name =
		    PMU_BSC_VAR_REF_CLK_NAME_STR,.rate = 13000000,.ops =
		    &gen_ref_clk_ops,},.ccu_clk = &CLK_NAME(khubaon),.clk_div =
	{
	.div_offset =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_OFFSET,.div_mask =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_ASYNC_PRE_DIV_MASK,.
		    div_shift =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_ASYNC_PRE_DIV_SHIFT,.
		    div_trig_offset =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    div_trig_mask =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_ASYNC_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_ASYNC_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_ASYNC_PRE_DIV_ASYNC_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 0,.clk = pmu_bsc_var_ref_clk_src_list,},};

/*
Ref clock name BBL_32K
*/
static struct ref_clk CLK_NAME(bbl_32k) = {
	.clk = {
.flags = BBL_32K_REF_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_BBL_32K_REF_CLK_ID,.name =
		    BBL_32K_REF_CLK_NAME_STR,.rate =
		    32768,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),};

/*
Bus clock name HUBAON.
 This clock has dividers present in seperate register. Since its not used as
 of now, we are declaring this as BUS clock and not initializing divider
 values. Also, clock SW enable bit is present in DIV register for debug.
 So this clock need to be autogated always from B0.
*/
static struct bus_clk CLK_NAME(hubaon)
    = {
	.clk = {
.flags = HUBAON_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_HUBAON_BUS_CLK_ID,.name =
		    HUBAON_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_HUB_CLKGATE_OFFSET,.gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_HUB_CLKGATE_HUBAON_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_HUB_CLKGATE_HUBAON_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_CLKGATE_HUBAON_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_HUB_CLKGATE_HUBAON_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(var_312m),};

/*
Bus clock name HUB_TIMER_APB
*/
static struct bus_clk CLK_NAME(hub_timer_apb) = {

	.clk = {
.flags = HUB_TIMER_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_HUB_TIMER_APB_BUS_CLK_ID,.name =
		    HUB_TIMER_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name ACI_APB
*/
static struct bus_clk CLK_NAME(aci_apb) = {

	.clk = {
.flags = ACI_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_ACI_APB_BUS_CLK_ID,.name =
		    ACI_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_ACI_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_ACI_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_ACI_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_ACI_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_ACI_CLKGATE_ACI_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_ACI_SOFT_RSTN_MASK,};

/*
Bus clock name SIM_APB
*/
static struct bus_clk CLK_NAME(sim_apb) = {

	.clk = {
.flags = SIM_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SIM_APB_BUS_CLK_ID,.name =
		    SIM_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,};

/*
Bus clock name SIM2_APB
*/
static struct bus_clk CLK_NAME(sim2_apb) = {

	.clk = {
.flags = SIM2_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SIM2_APB_BUS_CLK_ID,.name =
		    SIM2_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,};

/*
Bus clock name PWRMGR_AXI
*/
static struct bus_clk CLK_NAME(pwrmgr_axi) = {

	.clk = {
.flags = PWRMGR_AXI_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_PWRMGR_AXI_BUS_CLK_ID,.name =
		    PWRMGR_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_PWRMGR_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_PWRMGR_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_PWRMGR_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_PWRMGR_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_PWRMGR_CLKGATE_PWRMGR_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_PWRMGR_SOFT_RSTN_MASK,};

/*
Bus clock name APB6
*/
static struct bus_clk CLK_NAME(apb6) = {

	.clk = {
.flags = APB6_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB6_BUS_CLK_ID,.name =
		    APB6_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_APB6_CLKGATE_OFFSET,.gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_APB6_CLKGATE_APB6_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_APB6_CLKGATE_APB6_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_APB6_SOFT_RSTN_MASK,};

/*
Bus clock name GPIOKP_APB
*/
static struct bus_clk CLK_NAME(gpiokp_apb) = {

	.clk = {
.flags = GPIOKP_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_GPIOKP_APB_BUS_CLK_ID,.name =
		    GPIOKP_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_GPIOKP_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_GPIOKP_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_GPIOKP_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_GPIOKP_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_GPIOKP_CLKGATE_GPIOKP_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_GPIOKP_SOFT_RSTN_MASK,};

/*
Bus clock name PMU_BSC_APB
*/
static struct bus_clk CLK_NAME(pmu_bsc_apb) = {

	.clk = {
.flags = PMU_BSC_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_PMU_BSC_APB_BUS_CLK_ID,.name =
		    PMU_BSC_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name CHIPREG_APB
*/
static struct bus_clk CLK_NAME(chipreg_apb) = {

	.clk = {
.flags = CHIPREG_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_CHIPREG_APB_BUS_CLK_ID,.name =
		    CHIPREG_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_CHIPREG_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_CHIPREG_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_CHIPREG_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_CHIPREG_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_CHIPREG_CLKGATE_CHIPREG_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_CHIPREG_SOFT_RSTN_MASK,};

/*
Bus clock name FMON_APB
*/
static struct bus_clk CLK_NAME(fmon_apb) = {

	.clk = {
.flags = FMON_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_FMON_APB_BUS_CLK_ID,.name =
		    FMON_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_FMON_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_FMON_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_FMON_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_FMON_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_FMON_CLKGATE_FMON_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_FMON_SOFT_RSTN_MASK,};

/*
Bus clock name HUB_TZCFG_APB
*/
static struct bus_clk CLK_NAME(hub_tzcfg_apb) = {

	.clk = {
.flags = HUB_TZCFG_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_HUB_TZCFG_APB_BUS_CLK_ID,.name =
		    HUB_TZCFG_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_HUB_TZCFG_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_HUB_TZCFG_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_HUB_TZCFG_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_HUB_TZCFG_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TZCFG_CLKGATE_HUB_TZCFG_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_HUB_TZCFG_SOFT_RSTN_MASK,};

/*
Bus clock name SEC_WD_APB
*/
static struct bus_clk CLK_NAME(sec_wd_apb) = {

	.clk = {
.flags = SEC_WD_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SEC_WD_APB_BUS_CLK_ID,.name =
		    SEC_WD_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_SEC_WD_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_SEC_WD_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_SEC_WD_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_SEC_WD_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SEC_WD_CLKGATE_SEC_WD_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SEC_WD_SOFT_RSTN_MASK,};

/*
Bus clock name SYSEMI_SEC_APB
*/
static struct bus_clk CLK_NAME(sysemi_sec_apb) = {

	.clk = {
.flags = SYSEMI_SEC_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SYSEMI_SEC_APB_BUS_CLK_ID,.name =
		    SYSEMI_SEC_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_SEC_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_SEC_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_SEC_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_SEC_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_SEC_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SYSEMI_SOFT_RSTN_MASK,};

/*
Bus clock name SYSEMI_OPEN_APB
*/
static struct bus_clk CLK_NAME(sysemi_open_apb) = {

	.clk = {
.flags = SYSEMI_OPEN_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SYSEMI_OPEN_APB_BUS_CLK_ID,.name =
		    SYSEMI_OPEN_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_OPEN_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_OPEN_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_OPEN_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_OPEN_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SYSEMI_CLKGATE_SYSEMI_OPEN_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SYSEMI_SOFT_RSTN_MASK,};

/*
Bus clock name VCEMI_SEC_APB
*/
static struct bus_clk CLK_NAME(vcemi_sec_apb) = {

	.clk = {
.flags = VCEMI_SEC_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_VCEMI_SEC_APB_BUS_CLK_ID,.name =
		    VCEMI_SEC_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_SEC_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_SEC_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_SEC_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_SEC_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_SEC_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_VCEMI_SOFT_RSTN_MASK,};

/*
Bus clock name VCEMI_OPEN_APB
*/
static struct bus_clk CLK_NAME(vcemi_open_apb) = {

	.clk = {
.flags = VCEMI_OPEN_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_VCEMI_OPEN_APB_BUS_CLK_ID,.name =
		    VCEMI_OPEN_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_OPEN_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_OPEN_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_OPEN_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_OPEN_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_VCEMI_CLKGATE_VCEMI_OPEN_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_VCEMI_SOFT_RSTN_MASK,};

/*
Bus clock name SPM_APB
*/
static struct bus_clk CLK_NAME(spm_apb) = {

	.clk = {
.flags = SPM_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SPM_APB_BUS_CLK_ID,.name =
		    SPM_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_SPM_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_SPM_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_SPM_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_SPM_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SPM_CLKGATE_SPM_APB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SPM_SOFT_RSTN_MASK,};

/*
Bus clock name DAP
*/
static struct bus_clk CLK_NAME(dap) = {

	.clk = {
.flags = DAP_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_DAP_BUS_CLK_ID,.name =
		    DAP_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_DAP_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_DAP_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_DAP_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_DAP_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_DAP_CLKGATE_DAP_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(var_312m),};

/*
Peri clock name SIM
*/
/*peri clk src list*/
static struct clk *sim_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(sim) = {

	.clk = {
	.flags = SIM_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SIM_PERI_CLK_ID,.name =
		    SIM_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sim_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.mask_set = 1,.policy_bit_mask =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_SIM_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_CLK_EN_MASK,.gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_HYST_VAL_MASK,.hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_HYST_EN_MASK,.stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_STPRSTS_MASK,.volt_lvl_mask =
	    KHUBAON_CLK_MGR_REG_SIM_CLKGATE_SIM_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUBAON_CLK_MGR_REG_SIM_DIV_OFFSET,.div_mask =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_DIV_MASK,.
		    div_shift =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_DIV_SHIFT,.
		    pre_div_offset =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_OFFSET,.pre_div_mask =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_SIM_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_SIM_DIV_SIM_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    sim_peri_clk_src_list,},.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SIM_SOFT_RSTN_MASK,};

/*
Peri clock name SIM2
*/
/*peri clk src list*/
static struct clk *sim2_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m), CLK_PTR(ref_312m),
		  CLK_PTR(ref_96m), CLK_PTR(var_96m));
static struct peri_clk CLK_NAME(sim2) = {

	.clk = {
	.flags = SIM2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SIM2_PERI_CLK_ID,.name =
		    SIM2_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sim2_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.mask_set = 1,.policy_bit_mask =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_SIM2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_CLK_EN_MASK,.gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_HYST_VAL_MASK,.hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_HYST_EN_MASK,.stprsts_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_STPRSTS_MASK,.volt_lvl_mask =
	    KHUBAON_CLK_MGR_REG_SIM2_CLKGATE_SIM2_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset = KHUBAON_CLK_MGR_REG_SIM2_DIV_OFFSET,.div_mask =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_DIV_MASK,.
		    div_shift =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_DIV_SHIFT,.
		    pre_div_offset =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_OFFSET,.pre_div_mask =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_PRE_DIV_MASK,.
		    pre_div_shift =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_PRE_DIV_SHIFT,.
		    prediv_trig_offset =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    prediv_trig_mask =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_SIM_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_SIM2_DIV_SIM2_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 5,.src_inx = 0,.clk =
		    sim2_peri_clk_src_list,},.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_SIM2_SOFT_RSTN_MASK,};

/*
Peri clock name PSCS
*/
/*peri clk src list*/
static struct clk *pscs_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_312m));
static struct peri_clk CLK_NAME(pscs) = {

	.clk = {
	.flags = PSCS_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_PSCS_PERI_CLK_ID,.name =
		    PSCS_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.mask_set = 1,.policy_bit_mask =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_SIM_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_CLK_EN_MASK,.gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_HYST_VAL_MASK,.hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_HYST_EN_MASK,.stprsts_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_STPRSTS_MASK,.volt_lvl_mask =
	    KHUBAON_CLK_MGR_REG_PSCS_CLKGATE_PSCS_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset = KHUBAON_CLK_MGR_REG_PSCS_DIV_OFFSET,.div_mask =
		    KHUBAON_CLK_MGR_REG_PSCS_DIV_PSCS_DIV_MASK,.
		    div_shift =
		    KHUBAON_CLK_MGR_REG_PSCS_DIV_PSCS_DIV_SHIFT,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_PSCS_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_PSCS_DIV_PSCS_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_PSCS_DIV_PSCS_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 1,.clk =
		    pscs_peri_clk_src_list,},.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_PSCS_SOFT_RSTN_MASK,};

/*
Peri clock name HUB_TIMER
*/
/*peri clk src list*/
static struct clk *hub_timer_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(bbl_32k), CLK_PTR(frac_1m), CLK_PTR(dft_19_5m));
static struct peri_clk CLK_NAME(hub_timer) = {

	.clk = {
	.flags = HUB_TIMER_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_HUB_TIMER_PERI_CLK_ID,.name =
		    HUB_TIMER_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(hub_timer_apb),
					      NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.mask_set = 1,.policy_bit_mask =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_HUB_TIMER_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUBAON_CLK_MGR_REG_HUB_TIMER_CLKGATE_HUB_TIMER_VOLTAGE_LEVEL_MASK,.
	    clk_div = {
	.div_offset =
		    KHUBAON_CLK_MGR_REG_HUB_TIMER_DIV_OFFSET,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_HUB_TIMER_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_HUB_TIMER_DIV_HUB_TIMER_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_HUB_TIMER_DIV_HUB_TIMER_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk =
		    hub_timer_peri_clk_src_list,},.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_HUB_TIMER_SOFT_RSTN_MASK,};

/*
Peri clock name PMU_BSC
*/
/*peri clk src list*/
static struct clk *pmu_bsc_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(pmu_bsc_var), CLK_PTR(bbl_32k));
static struct peri_clk CLK_NAME(pmu_bsc) = {
	.clk = {
	.flags = PMU_BSC_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_PMU_BSC_PERI_CLK_ID,.name =
		    PMU_BSC_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(pmu_bsc_apb),
					      NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(khubaon),.mask_set = 1,.policy_bit_mask =
	    KHUBAON_CLK_MGR_REG_POLICY0_MASK1_PMU_BSC_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_OFFSET,.clk_en_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUBAON_CLK_MGR_REG_PMU_BSC_CLKGATE_PMU_BSC_VOLTAGE_LEVEL_MASK,.
	    clk_div = {
	.div_offset = KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_OFFSET,.div_mask =
		    KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_PMU_BSC_DIV_MASK,.
		    div_shift =
		    KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_PMU_BSC_DIV_SHIFT,.
		    div_trig_offset =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    div_trig_mask =
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_PMU_BSC_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_OFFSET,.
		    pll_select_mask =
		    KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_PMU_BSC_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_PMU_BSC_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk =
		    pmu_bsc_peri_clk_src_list,},.soft_reset_offset =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_OFFSET,.clk_reset_mask =
	    KHUBAON_RST_MGR_REG_SOFT_RSTN0_PMU_BSC_SOFT_RSTN_MASK,};

/*
CCU clock name KPM
*/
/* CCU freq list */
static u32 kpm_clk_freq_list0[] =
DEFINE_ARRAY_ARGS(26000000, 26000000, 26000000);
static u32 kpm_clk_freq_list1[] =
DEFINE_ARRAY_ARGS(52000000, 52000000, 26000000);
static u32 kpm_clk_freq_list2[] =
DEFINE_ARRAY_ARGS(104000000, 52000000, 26000000);
static u32 kpm_clk_freq_list3[] =
DEFINE_ARRAY_ARGS(156000000, 52000000, 26000000);
static u32 kpm_clk_freq_list4[] =
DEFINE_ARRAY_ARGS(156000000, 78000000, 39000000);
static u32 kpm_clk_freq_list5[] =
DEFINE_ARRAY_ARGS(208000000, 104000000, 52000000);
static u32 kpm_clk_freq_list6[] =
DEFINE_ARRAY_ARGS(312000000, 104000000, 52000000);
static u32 kpm_clk_freq_list7[] =
DEFINE_ARRAY_ARGS(312000000, 156000000, 78000000);

static struct ccu_clk CLK_NAME(kpm) = {

	.clk = {
	.flags = KPM_CCU_CLK_FLAGS,.id = CLK_KPM_CCU_CLK_ID,.name =
		    KPM_CCU_CLK_NAME_STR,.clk_type = CLK_TYPE_CCU,.ops =
		    &gen_ccu_clk_ops,},.ccu_ops = &gen_ccu_ops,.pi_id =
	    PI_MGR_PI_ID_ARM_SUB_SYSTEM,
//      .pi_id = -1,
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(KONA_MST_CLK_BASE_ADDR),.wr_access_offset =
	    KPM_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    KPM_CLK_MGR_REG_POLICY0_MASK_OFFSET,.policy_mask2_offset =
	    0,.policy_freq_offset =
	    KPM_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    KPM_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    KPM_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    KPM_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    KPM_CLK_MGR_REG_VLT_PERI_OFFSET,.lvm_en_offset =
	    KPM_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    KPM_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    KPM_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    KPM_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    KPM_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    KPM_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    KPM_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,
#endif
.freq_volt = KPM_CCU_FREQ_VOLT_TBL,.freq_count =
	    KPM_CCU_FREQ_VOLT_TBL_SZ,.volt_peri =
	    DEFINE_ARRAY_ARGS(VLT_NORMAL_PERI,
					  VLT_HIGH_PERI),.
	    freq_policy =
	    DEFINE_ARRAY_ARGS(KPM_CCU_FREQ_POLICY_TBL),.freq_tbl =
	    DEFINE_ARRAY_ARGS(kpm_clk_freq_list0,
					  kpm_clk_freq_list1,
					  kpm_clk_freq_list2,
					  kpm_clk_freq_list3,
					  kpm_clk_freq_list4,
					  kpm_clk_freq_list5,
					  kpm_clk_freq_list6,
					  kpm_clk_freq_list7),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(KONA_MST_RST_BASE_ADDR),.
	    reset_wr_access_offset = KPM_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
Bus clock name USB_OTG_AHB
*/

#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs usb_otg_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_ECONOMY,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  },

};
#endif

static struct bus_clk CLK_NAME(usb_otg_ahb) = {

	.clk = {
	.flags = USB_OTG_AHB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_USB_OTG_AHB_BUS_CLK_ID,.name =
		    USB_OTG_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &usb_otg_dfs,
#endif
.clk_gate_offset =
	    KPM_CLK_MGR_REG_USB_OTG_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_USB_OTG_CLKGATE_USB_OTG_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_USB_OTG_CLKGATE_USB_OTG_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_USB_OTG_CLKGATE_USB_OTG_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_USB_OTG_SOFT_RSTN_MASK,};

/*
Bus clock name SDIO2_AHB
*/
static struct bus_clk CLK_NAME(sdio2_ahb) = {

	.clk = {
.flags = SDIO2_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SDIO2_AHB_BUS_CLK_ID,.name =
		    SDIO2_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name SDIO3_AHB
*/
static struct bus_clk CLK_NAME(sdio3_ahb) = {

	.clk = {
.flags = SDIO3_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SDIO3_AHB_BUS_CLK_ID,.name =
		    SDIO3_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name NAND_AHB
*/
static struct bus_clk CLK_NAME(nand_ahb) = {

	.clk = {
.flags = NAND_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_NAND_AHB_BUS_CLK_ID,.name =
		    NAND_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_NAND_SOFT_RSTN_MASK,};

/*
Bus clock name SDIO1_AHB
*/
static struct bus_clk CLK_NAME(sdio1_ahb) = {

	.clk = {
.flags = SDIO1_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SDIO1_AHB_BUS_CLK_ID,.name =
		    SDIO1_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name SDIO4_AHB
*/
static struct bus_clk CLK_NAME(sdio4_ahb) = {

	.clk = {
.flags = SDIO4_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SDIO4_AHB_BUS_CLK_ID,.name =
		    SDIO4_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_AHB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name SYS_SWITCH_AXI
*/
static struct bus_clk CLK_NAME(sys_switch_axi) = {
	.clk = {
.flags = SYS_SWITCH_AXI_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SYS_SWITCH_AXI_BUS_CLK_ID,.name =
		    SYS_SWITCH_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_AXI_SYS_SWITCH_CLKGATE_OFFSET,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_AXI_SYS_SWITCH_CLKGATE_SYS_SWITCH_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_AXI_SYS_SWITCH_CLKGATE_SYS_SWITCH_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_AXI_SYS_SWITCH_CLKGATE_SYS_SWITCH_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_AXI_SYS_SWITCH_CLKGATE_SYS_SWITCH_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(var_312m),};

/*
Bus clock name MASTER_SWITCH_AHB
*/
static struct bus_clk CLK_NAME(master_switch_ahb) = {
	.clk = {
.flags = MASTER_SWITCH_AHB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_MASTER_SWITCH_AHB_BUS_CLK_ID,.name =
		    MASTER_SWITCH_AHB_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AHB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AHB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AHB_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(sys_switch_axi),};

/*
Bus clock name MASTER_SWITCH_AXI
*/
static struct bus_clk CLK_NAME(master_switch_axi) = {
	.clk = {
.flags = MASTER_SWITCH_AXI_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_MASTER_SWITCH_AXI_BUS_CLK_ID,.name =
		    MASTER_SWITCH_AXI_BUS_CLK_NAME_STR,.
		    dep_clks = DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_OFFSET,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_AXI_MST_SWITCH_CLKGATE_MASTER_SWITCH_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(master_switch_ahb),};
/*
Bus clock name ARMCORE_AXI
*/
static struct bus_clk CLK_NAME(armcore_axi)
    = {
	.clk = {
.flags = ARMCORE_AXI_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_ARMCORE_AXI_BUS_CLK_ID,.name =
		    ARMCORE_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_ARMCORE_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_ARMCORE_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_ARMCORE_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_ARMCORE_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_AXI_CORE_CLKGATE_ARMCORE_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk =
	    CLK_PTR(sys_switch_axi),.soft_reset_offset =
	    KPM_RST_MGR_REG_AXI_APB0_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AXI_APB0_SOFTRST_ARMCORE_SOFT_RSTN_MASK,};

/*
Bus clock name APB4
*/
static struct bus_clk CLK_NAME(apb4)
    = {
	.clk = {
.flags = APB4_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB4_BUS_CLK_ID,.name =
		    APB4_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_APB4_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_APB4_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_APB4_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_APB4_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_APB4_CLKGATE_APB4_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = CLK_PTR(sys_switch_axi),};

/*
Bus clock name APB8
*/
static struct bus_clk CLK_NAME(apb8)
    = {
	.clk = {
.flags = APB8_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB8_BUS_CLK_ID,.name =
		    APB8_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_APB8_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_APB8_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_APB8_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_APB8_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_APB8_CLKGATE_APB8_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk =
	    CLK_PTR(sys_switch_axi),.soft_reset_offset =
	    KPM_RST_MGR_REG_APB8_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_APB8_SOFTRST_APB8_SOFT_RSTN_MASK,};

/*
Bus clock name DMA_AXI
*/
static struct bus_clk CLK_NAME(dma_axi)
    = {
	.clk = {
.flags = DMA_AXI_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_DMA_AXI_BUS_CLK_ID,.name =
		    DMA_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_DMA_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_DMA_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_DMA_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_DMA_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_DMAC_CLKGATE_DMA_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk =
	    CLK_PTR(sys_switch_axi),.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_DMA_SOFT_RSTN_MASK,};

/*
Bus clock name USBH_AHB
*/
static struct bus_clk CLK_NAME(usbh_ahb) = {
	.clk = {
.flags = USBH_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_USBH_AHB_BUS_CLK_ID,.name =
		    USBH_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_USBH2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(master_switch_ahb),};

/*
Bus clock name SPI_AHB
*/
static struct bus_clk CLK_NAME(spi_ahb)
    = {
	.clk = {
.flags = SPI_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SPI_AHB_BUS_CLK_ID,.name =
		    SPI_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SPI_SLAVE_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SPI_SLAVE_CLKGATE_SPI_SLAVE_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_SPI_SLAVE_CLKGATE_SPI_SLAVE_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SPI_SLAVE_CLKGATE_SPI_SLAVE_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(master_switch_ahb),};
/*
Bus clock name USBHSIC_AHB
*/
static struct bus_clk CLK_NAME(usbhsic_ahb) = {
	.clk = {
.flags = USBHSIC_AHB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_USBHSIC_AHB_BUS_CLK_ID,.name =
		    USBHSIC_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(master_switch_ahb),};

/*
Bus clock name USB_IC_AHB
*/
static struct bus_clk CLK_NAME(usb_ic_ahb) = {
	.clk = {
.flags = USB_IC_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_USB_IC_AHB_BUS_CLK_ID,.name =
		    USB_IC_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.clk_gate_offset =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(master_switch_ahb),};

/*DFS def for SDIO */
#ifdef CONFIG_KONA_PI_MGR
static struct dfs_rate_thold sdio2_rate_thold[2] = {
	{FREQ_MHZ(26), PI_OPP_ECONOMY},
	{-1, PI_OPP_NORMAL},
};

static struct clk_dfs sdio2_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_RATE,
	.policy_param = (u32)&sdio2_rate_thold,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif
/*
Peri clock name SDIO2
*/
/*peri clk src list*/
static struct clk *sdio2_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_52m), CLK_PTR(ref_52m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(sdio2) = {

	.clk = {
		.flags = SDIO2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SDIO2_PERI_CLK_ID,.name = SDIO2_PERI_CLK_NAME_STR,
#ifdef CONFIG_BCM_HWCAPRI_1508
		    .dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sdio2_ahb),
				      CLK_PTR(8phase_en_pll1), NULL),
#else
		    .dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(sdio2_ahb), NULL),
#endif
	.ops = &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &sdio2_clk_dfs,
#endif
	    .policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPM_CLK_MGR_REG_SDIO2_DIV_OFFSET,.div_mask =
		    KPM_CLK_MGR_REG_SDIO2_DIV_SDIO2_DIV_MASK,.
		    div_shift =
		    KPM_CLK_MGR_REG_SDIO2_DIV_SDIO2_DIV_SHIFT,.
		    div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_SDIO2_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_SDIO2_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_SDIO2_DIV_SDIO2_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_SDIO2_DIV_SDIO2_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = ARRAY_SIZE(sdio2_peri_clk_src_list),.src_inx = 0,.clk =
		    sdio2_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_SDIO2_SOFT_RSTN_MASK,};

/*
Peri clock name SDIO2_SLEEP
*/
/*peri clk src list*/
static struct clk *sdio2_sleep_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(ref_32k));
static struct peri_clk CLK_NAME(sdio2_sleep) = {

	.clk = {
	.flags = SDIO2_SLEEP_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SDIO2_SLEEP_PERI_CLK_ID,.name =
		    SDIO2_SLEEP_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set =
	    0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_SLEEP_CLK_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_SLEEP_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO2_CLKGATE_SDIO2_VOLTAGE_LEVEL_MASK,.src_clk = {
.count = ARRAY_SIZE(sdio2_sleep_peri_clk_src_list),.src_inx =
		    0,.clk = sdio2_sleep_peri_clk_src_list,},};

#ifdef CONFIG_KONA_PI_MGR
static struct dfs_rate_thold sdio3_rate_thold[2] = {
	{FREQ_MHZ(26), PI_OPP_ECONOMY},
	{-1, PI_OPP_NORMAL},
};

static struct clk_dfs sdio3_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_RATE,
	.policy_param = (u32)&sdio3_rate_thold,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

/*
Peri clock name SDIO3
*/
/*peri clk src list*/
static struct clk *sdio3_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_52m), CLK_PTR(ref_52m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(sdio3) = {

	.clk = {
		.flags = SDIO3_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SDIO3_PERI_CLK_ID,.name = SDIO3_PERI_CLK_NAME_STR,
#ifdef CONFIG_BCM_HWCAPRI_1508
		    .dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sdio3_ahb),
				      CLK_PTR(8phase_en_pll1), NULL),
#else
		    .dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(sdio3_ahb), NULL),
#endif
	.ops = &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &sdio3_clk_dfs,
#endif
	    .policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPM_CLK_MGR_REG_SDIO3_DIV_OFFSET,.div_mask =
		    KPM_CLK_MGR_REG_SDIO3_DIV_SDIO3_DIV_MASK,.
		    div_shift =
		    KPM_CLK_MGR_REG_SDIO3_DIV_SDIO3_DIV_SHIFT,.
		    div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_SDIO3_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_SDIO3_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_SDIO3_DIV_SDIO3_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_SDIO3_DIV_SDIO3_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = ARRAY_SIZE(sdio3_peri_clk_src_list),.src_inx = 0,.clk =
		    sdio3_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_SDIO3_SOFT_RSTN_MASK,};

/*
Peri clock name SDIO3_SLEEP
*/
/*peri clk src list*/
static struct clk *sdio3_sleep_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(ref_32k));
static struct peri_clk CLK_NAME(sdio3_sleep) = {

	.clk = {
	.flags = SDIO3_SLEEP_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SDIO3_SLEEP_PERI_CLK_ID,.name =
		    SDIO3_SLEEP_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set =
	    0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_SLEEP_CLK_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_SLEEP_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO3_CLKGATE_SDIO3_VOLTAGE_LEVEL_MASK,.src_clk = {
.count = ARRAY_SIZE(sdio3_sleep_peri_clk_src_list),.src_inx =
		    0,.clk = sdio3_sleep_peri_clk_src_list,},};

/*
Peri clock name NAND
*/
/*peri clk src list*/
static struct clk *nand_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_208m), CLK_PTR(ref_208m));
static struct peri_clk CLK_NAME(nand) = {

	.clk = {
	.flags = NAND_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_NAND_PERI_CLK_ID,.name =
		    NAND_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(nand_ahb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set =
	    0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_NAND_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_NAND_CLKGATE_NAND_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPM_CLK_MGR_REG_NAND_DIV_OFFSET,.div_mask =
		    KPM_CLK_MGR_REG_NAND_DIV_NAND_DIV_MASK,.div_shift =
		    KPM_CLK_MGR_REG_NAND_DIV_NAND_DIV_SHIFT,.
		    div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_NAND_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_NAND_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_NAND_DIV_NAND_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_NAND_DIV_NAND_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 3,.src_inx = 0,.clk = nand_peri_clk_src_list,},};

/*
Peri clock name SDIO1
*/
/*peri clk src list*/

#ifdef CONFIG_KONA_PI_MGR
static struct dfs_rate_thold sdio1_rate_thold[2] = {
	{FREQ_MHZ(26), PI_OPP_ECONOMY},
	{-1, PI_OPP_NORMAL},
};

static struct clk_dfs sdio1_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_RATE,
	.policy_param = (u32)&sdio1_rate_thold,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

static struct clk *sdio1_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_52m), CLK_PTR(ref_52m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(sdio1) = {

	.clk = {
		.flags = SDIO1_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SDIO1_PERI_CLK_ID,.name = SDIO1_PERI_CLK_NAME_STR,
#ifdef CONFIG_BCM_HWCAPRI_1508
		    .dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sdio1_ahb),
				      CLK_PTR(8phase_en_pll1), NULL),
#else
		    .dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(sdio1_ahb), NULL),
#endif
	.ops = &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &sdio1_clk_dfs,
#endif
	    .policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO1_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPM_CLK_MGR_REG_SDIO1_DIV_OFFSET,.div_mask =
		    KPM_CLK_MGR_REG_SDIO1_DIV_SDIO1_DIV_MASK,.
		    div_shift =
		    KPM_CLK_MGR_REG_SDIO1_DIV_SDIO1_DIV_SHIFT,.
		    div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_SDIO1_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_SDIO1_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_SDIO1_DIV_SDIO1_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_SDIO1_DIV_SDIO1_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = ARRAY_SIZE(sdio1_peri_clk_src_list),.src_inx = 0,.clk =
		    sdio1_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_SDIO1_SOFT_RSTN_MASK,};
#ifdef CONFIG_KONA_PI_MGR
static struct dfs_rate_thold sdio4_rate_thold[2] = {
	{FREQ_MHZ(26), PI_OPP_ECONOMY},
	{-1, PI_OPP_NORMAL},
};

static struct clk_dfs sdio4_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_RATE,
	.policy_param = (u32)&sdio4_rate_thold,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

/*
Peri clock name SDIO4
*/
/*peri clk src list*/
static struct clk *sdio4_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_52m), CLK_PTR(ref_52m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(sdio4) = {
	.clk = {
		.flags = SDIO4_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SDIO4_PERI_CLK_ID,.name = SDIO4_PERI_CLK_NAME_STR,
#ifdef CONFIG_BCM_HWCAPRI_1508
		    .dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(sdio4_ahb),
						  CLK_PTR(8phase_en_pll1),
						  NULL),
#else
		    .dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(sdio4_ahb), NULL),
#endif
	.ops = &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &sdio4_clk_dfs,
#endif
	    .policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO4_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPM_CLK_MGR_REG_SDIO4_DIV_OFFSET,.div_mask =
		    KPM_CLK_MGR_REG_SDIO4_DIV_SDIO4_DIV_MASK,.
		    div_shift =
		    KPM_CLK_MGR_REG_SDIO4_DIV_SDIO4_DIV_SHIFT,.
		    div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_SDIO4_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_SDIO4_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_SDIO4_DIV_SDIO4_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_SDIO4_DIV_SDIO4_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = ARRAY_SIZE(sdio4_peri_clk_src_list),.src_inx = 0,.clk =
		    sdio4_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_SDIO4_SOFT_RSTN_MASK,};

/*
Peri clock name SDIO1_SLEEP
*/
static struct clk *sdio1_sleep_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(ref_32k));
static struct peri_clk CLK_NAME(sdio1_sleep) = {

	.clk = {
	.flags = SDIO1_SLEEP_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SDIO1_SLEEP_PERI_CLK_ID,.name =
		    SDIO1_SLEEP_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(sdio1_ahb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set =
	    0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO1_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_SLEEP_CLK_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_SLEEP_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO1_CLKGATE_SDIO1_VOLTAGE_LEVEL_MASK,.src_clk = {
.count = ARRAY_SIZE(sdio1_sleep_peri_clk_src_list),.src_inx =
		    0,.clk = sdio1_sleep_peri_clk_src_list,},};

/*
Peri clock name SDIO4_SLEEP
*/
static struct clk *sdio4_sleep_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(ref_32k));
static struct peri_clk CLK_NAME(sdio4_sleep) = {
	.clk = {
	.flags = SDIO4_SLEEP_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SDIO4_SLEEP_PERI_CLK_ID,.name =
		    SDIO4_SLEEP_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kpm),.mask_set =
	    0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_SDIO4_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(0, 0, 0, 0),.clk_gate_offset =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_SLEEP_CLK_EN_MASK,.
	    stprsts_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_SLEEP_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_SDIO4_CLKGATE_SDIO4_VOLTAGE_LEVEL_MASK,.src_clk = {
.count = ARRAY_SIZE(sdio4_sleep_peri_clk_src_list),.src_inx =
		    0,.clk = sdio4_sleep_peri_clk_src_list,},};

 /*
    Peri clock name USB_IC
  */
/*peri clk src list*/
static struct clk *usb_ic_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(usb_ic) = {
	.clk = {
	.flags = USB_IC_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_USB_IC_PERI_CLK_ID,.name =
		    USB_IC_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(usb_ic_ahb), NULL),.rate =
		    0,.ops = &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.mask_set = 0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_USB_IC_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_CLK_EN_MASK,.gating_sel_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask = 0,.hyst_en_mask = 0,.stprsts_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_STPRSTS_MASK,.volt_lvl_mask =
	    KPM_CLK_MGR_REG_USB_IC_CLKGATE_USB_IC_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.pll_select_offset =
		    KPM_CLK_MGR_REG_USB_IC_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_USB_IC_DIV_USB_IC_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_USB_IC_DIV_USB_IC_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 2,.clk = usb_ic_peri_clk_src_list,},};

/*
Peri clock name USBH_48M
*/
/*peri clk src list*/
static struct clk *usbh_48m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(usbh_48m) = {
	.clk = {
	.flags = USBH_48M_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_USBH_48M_PERI_CLK_ID,.name =
		    USBH_48M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(usbh_ahb), NULL),.rate =
		    0,.ops = &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.mask_set = 0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_HSIC2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_48M_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_48M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_48M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_48M_HYST_EN_MASK,.stprsts_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_48M_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_HSIC2_48M_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_HSIC2_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_HSIC2_DIV_HSIC2_48M_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_HSIC2_DIV_HSIC2_48M_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 2,.clk =
		    usbh_48m_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_HSIC2_SOFT_RSTN_MASK,};

/*
Peri clock name USBH_12M
*/
/*peri clk src list*/
static struct clk *usbh_12m_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(usbh_12m) = {
	.clk = {
	.flags = USBH_12M_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_USBH_12M_PERI_CLK_ID,.name =
		    USBH_12M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(usbh_ahb), NULL),.rate =
		    0,.ops = &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(kpm),.mask_set = 0,.policy_bit_mask =
	    KPM_CLK_MGR_REG_POLICY0_MASK_HSIC2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_OFFSET,.clk_en_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_12M_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_12M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_12M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_12M_HYST_EN_MASK,.stprsts_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_12M_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPM_CLK_MGR_REG_HSIC2_CLKGATE_HSIC2_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_trig_offset =
		    KPM_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPM_CLK_MGR_REG_DIV_TRIG_HSIC2_48M_TRIGGER_MASK,.
		    pll_select_offset =
		    KPM_CLK_MGR_REG_HSIC2_DIV_OFFSET,.pll_select_mask =
		    KPM_CLK_MGR_REG_HSIC2_DIV_HSIC2_48M_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPM_CLK_MGR_REG_HSIC2_DIV_HSIC2_48M_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 2,.clk =
		    usbh_12m_peri_clk_src_list,},.soft_reset_offset =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_OFFSET,.clk_reset_mask =
	    KPM_RST_MGR_REG_AHB_MST_SOFTRST_HSIC2_SOFT_RSTN_MASK,};

/*
CCU clock name KPS
*/
/* CCU freq list */
static u32 kps_clk_freq_list0[] =
DEFINE_ARRAY_ARGS(26000000, 26000000, 26000000, 26000000, 26000000);
static u32 kps_clk_freq_list1[] =
DEFINE_ARRAY_ARGS(52000000, 26000000, 26000000, 26000000, 26000000);
static u32 kps_clk_freq_list2[] =
DEFINE_ARRAY_ARGS(78000000, 39000000, 39000000, 39000000, 39000000);
static u32 kps_clk_freq_list3[] =
DEFINE_ARRAY_ARGS(104000000, 52000000, 52000000, 52000000, 52000000);
static u32 kps_clk_freq_list4[] =
DEFINE_ARRAY_ARGS(156000000, 52000000, 52000000, 52000000, 52000000);
static u32 kps_clk_freq_list5[] =
DEFINE_ARRAY_ARGS(156000000, 78000000, 78000000, 78000000, 78000000);

static struct ccu_clk CLK_NAME(kps) = {

	.clk = {
	.flags = KPS_CCU_CLK_FLAGS,.id = CLK_KPS_CCU_CLK_ID,.name =
		    KPS_CCU_CLK_NAME_STR,.clk_type = CLK_TYPE_CCU,.ops =
		    &gen_ccu_clk_ops,},.ccu_ops = &gen_ccu_ops,.pi_id =
	    PI_MGR_PI_ID_ARM_SUB_SYSTEM,
//      .pi_id = -1,
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(KONA_SLV_CLK_BASE_ADDR),.wr_access_offset =
	    KPS_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    KPS_CLK_MGR_REG_POLICY0_MASK_OFFSET,.policy_mask2_offset =
	    KPS_CLK_MGR_REG_POLICY0_MASK2_OFFSET,.policy_freq_offset =
	    KPS_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    KPS_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    KPS_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    KPS_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    KPS_CLK_MGR_REG_VLT_PERI_OFFSET,.lvm_en_offset =
	    KPS_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    KPS_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    KPS_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    KPS_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    KPS_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    KPS_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    KPS_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,
#endif
.freq_volt = KPS_CCU_FREQ_VOLT_TBL,.freq_count =
	    KPS_CCU_FREQ_VOLT_TBL_SZ,.volt_peri =
	    DEFINE_ARRAY_ARGS(VLT_NORMAL_PERI,
					  VLT_HIGH_PERI),.
	    freq_policy =
	    DEFINE_ARRAY_ARGS(KPS_CCU_FREQ_POLICY_TBL),.freq_tbl =
	    DEFINE_ARRAY_ARGS(kps_clk_freq_list0,
					  kps_clk_freq_list1,
					  kps_clk_freq_list2,
					  kps_clk_freq_list3,
					  kps_clk_freq_list4,
					  kps_clk_freq_list5),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(KONA_SLV_RST_BASE_ADDR),.
	    reset_wr_access_offset = KPS_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
Peri clock name CAPH_SRCMIXER
*/
/*DFS def for CAPH */
#ifdef CONFIG_KONA_PI_MGR
static struct dfs_rate_thold caph_rate_thold[2] = {
	{FREQ_MHZ(26), PI_OPP_ECONOMY},
	{-1, PI_OPP_NORMAL},
};

static struct clk_dfs caph_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_RATE,
	.policy_param = (u32)&caph_rate_thold,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

/*peri clk src list*/
static struct clk *caph_srcmixer_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m));
static struct peri_clk CLK_NAME(caph_srcmixer) = {

	.clk = {
		.flags = CAPH_SRCMIXER_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id = CLK_CAPH_SRCMIXER_PERI_CLK_ID,.name = CAPH_SRCMIXER_PERI_CLK_NAME_STR,.dep_clks = DEFINE_ARRAY_ARGS(CLK_PTR(kps), CLK_PTR(kpm), NULL),	/*Don't allow arm subsys to enter retention when CapH is active */
	.ops = &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(khub),
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &caph_clk_dfs,
#endif
	    .mask_set = 1,.policy_bit_mask =
	    KHUB_CLK_MGR_REG_POLICY0_MASK1_CAPH_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_OFFSET,.clk_en_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_SRCMIXER_CLK_EN_MASK,.
	    gating_sel_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_SRCMIXER_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_SRCMIXER_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_SRCMIXER_HYST_EN_MASK,.
	    stprsts_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_SRCMIXER_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KHUB_CLK_MGR_REG_CAPH_CLKGATE_CAPH_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KHUB_CLK_MGR_REG_CAPH_DIV_OFFSET,.div_mask =
		    KHUB_CLK_MGR_REG_CAPH_DIV_CAPH_SRCMIXER_DIV_MASK,.
		    div_shift =
		    KHUB_CLK_MGR_REG_CAPH_DIV_CAPH_SRCMIXER_DIV_SHIFT,.
		    div_trig_offset =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET,.
		    div_trig_mask =
		    KHUB_CLK_MGR_REG_PERIPH_SEG_TRG_CAPH_SRCMIXER_TRIGGER_MASK,.
		    pll_select_offset =
		    KHUB_CLK_MGR_REG_CAPH_DIV_OFFSET,.pll_select_mask =
		    KHUB_CLK_MGR_REG_CAPH_DIV_CAPH_SRCMIXER_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KHUB_CLK_MGR_REG_CAPH_DIV_CAPH_SRCMIXER_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 0,.clk = caph_srcmixer_peri_clk_src_list,},};

/*
Bus clock name UARTB_APB
*/
static struct bus_clk CLK_NAME(uartb_apb) = {

	.clk = {
.flags = UARTB_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_UARTB_APB_BUS_CLK_ID,.name =
		    UARTB_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name UARTB2_APB
*/
static struct bus_clk CLK_NAME(uartb2_apb) = {

	.clk = {
.flags = UARTB2_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_UARTB2_APB_BUS_CLK_ID,.name =
		    UARTB2_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name UARTB3_APB
*/
static struct bus_clk CLK_NAME(uartb3_apb) = {

	.clk = {
.flags = UARTB3_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_UARTB3_APB_BUS_CLK_ID,.name =
		    UARTB3_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name UARTB4_APB
*/
static struct bus_clk CLK_NAME(uartb4_apb) = {

	.clk = {
.flags = UARTB4_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_UARTB4_APB_BUS_CLK_ID,.name =
		    UARTB4_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name DMAC_MUX_APB
*/
static struct bus_clk CLK_NAME(dmac_mux_apb) = {

	.clk = {
.flags = DMAC_MUX_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_DMAC_MUX_APB_BUS_CLK_ID,.name =
		    DMAC_MUX_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_DMAC_MUX_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_DMAC_MUX_CLKGATE_DMAC_MUX_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_DMAC_MUX_CLKGATE_DMAC_MUX_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_DMAC_MUX_CLKGATE_DMAC_MUX_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,.soft_reset_offset =
	    KPS_RST_MGR_REG_APB2_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB2_SOFTRST_DMAC_MUX_SOFT_RSTN_MASK,};

/*
Bus clock name BSC1_APB
*/
static struct bus_clk CLK_NAME(bsc1_apb) = {

	.clk = {
.flags = BSC1_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_BSC1_APB_BUS_CLK_ID,.name =
		    BSC1_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_APB_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Bus clock name BSC2_APB
*/
static struct bus_clk CLK_NAME(bsc2_apb) = {

	.clk = {
.flags = BSC2_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_BSC2_APB_BUS_CLK_ID,.name =
		    BSC2_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_APB_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Bus clock name BSC3_APB
*/
static struct bus_clk CLK_NAME(bsc3_apb) = {

	.clk = {
.flags = BSC3_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_BSC3_APB_BUS_CLK_ID,.name =
		    BSC3_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_APB_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Bus clock name PWM_APB
*/
static struct bus_clk CLK_NAME(pwm_apb) = {

	.clk = {
.flags = PWM_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_PWM_APB_BUS_CLK_ID,.name =
		    PWM_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_APB_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Bus clock name SSP0_APB
*/
static struct bus_clk CLK_NAME(ssp0_apb) = {

	.clk = {
.flags = SSP0_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP0_APB_BUS_CLK_ID,.name =
		    SSP0_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name SSP2_APB
*/
static struct bus_clk CLK_NAME(ssp2_apb) = {

	.clk = {
.flags = SSP2_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SSP2_APB_BUS_CLK_ID,.name =
		    SSP2_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name SWITCH_AXI
*/
static struct bus_clk CLK_NAME(switch_axi) = {

	.clk = {
.flags = SWITCH_AXI_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_SWITCH_AXI_BUS_CLK_ID,.name =
		    SWITCH_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_AXI_SWITCH_CLKGATE_OFFSET,.gating_sel_mask =
	    KPS_CLK_MGR_REG_AXI_SWITCH_CLKGATE_SWITCH_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_AXI_SWITCH_CLKGATE_SWITCH_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_AXI_SWITCH_CLKGATE_SWITCH_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_AXI_SWITCH_CLKGATE_SWITCH_AXI_STPRSTS_MASK,.
	    freq_tbl_index = 0,.src_clk = NULL,};

/*
Bus clock name DAP
*/
static struct bus_clk CLK_NAME(kps_dap)
    = {

	.clk = {
.flags = KPS_DAP_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_KPS_DAP_BUS_CLK_ID,.name =
		    KPS_DAP_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_DAP_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_DAP_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_DAP_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_DAP_HYST_EN_MASK,.stprsts_mask =
	    KPS_CLK_MGR_REG_DAP_CLKGATE_DAP_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

/*
Bus clock name EXT_AXI
*/
static struct bus_clk CLK_NAME(ext_axi)
    = {

	.clk = {
.flags = EXT_AXI_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_EXT_AXI_BUS_CLK_ID,.name =
		    EXT_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_AXI_EXT_CLKGATE_OFFSET,.gating_sel_mask =
	    KPS_CLK_MGR_REG_AXI_EXT_CLKGATE_EXT_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_AXI_EXT_CLKGATE_EXT_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_AXI_EXT_CLKGATE_EXT_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_AXI_EXT_CLKGATE_EXT_AXI_STPRSTS_MASK,.
	    freq_tbl_index = 0,.src_clk = NULL,};

/*
Bus clock name SPUM_OPEN_APB
*/
static struct bus_clk CLK_NAME(spum_open_apb) = {

	.clk = {
	.flags = SPUM_OPEN_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SPUM_OPEN_APB_BUS_CLK_ID,.name =
		    SPUM_OPEN_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_SPUM_OPEN_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_SPUM_OPEN_APB_HW_SW_GATING_SEL_MASK,
// .hyst_val_mask = KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_SPUM_OPEN_APB_HYST_VAL_MASK,
// .hyst_en_mask = KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_SPUM_OPEN_APB_HYST_EN_MASK,
.stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_APB_CLKGATE_SPUM_OPEN_APB_STPRSTS_MASK,.
	    freq_tbl_index = 4,.src_clk = NULL,};

/*
Bus clock name SPUM_SEC_APB
*/
static struct bus_clk CLK_NAME(spum_sec_apb) = {

	.clk = {
	.flags = SPUM_SEC_APB_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SPUM_SEC_APB_BUS_CLK_ID,.name =
		    SPUM_SEC_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_SPUM_SEC_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_SPUM_SEC_APB_HW_SW_GATING_SEL_MASK,
// .hyst_val_mask =
//KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_SPUM_SEC_APB_HYST_VAL_MASK,
// .hyst_en_mask =
//KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_SPUM_SEC_APB_HYST_EN_MASK,
.stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_APB_CLKGATE_SPUM_SEC_APB_STPRSTS_MASK,.
	    freq_tbl_index = 4,.src_clk = NULL,};

/*
Bus clock name APB1
*/
static struct bus_clk CLK_NAME(apb1) = {

	.clk = {
.flags = APB1_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB1_BUS_CLK_ID,.name =
		    APB1_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_APB1_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_APB1_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_APB1_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_APB1_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_APB1_CLKGATE_APB1_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name APB7
*/
static struct bus_clk CLK_NAME(apb7)
    = {

	.clk = {
.flags = APB7_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB7_BUS_CLK_ID,.name =
		    APB7_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_APB7_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_APB7_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_APB7_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_APB7_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_APB7_CLKGATE_APB7_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name TIMERS_APB
*/
static struct bus_clk CLK_NAME(timers_apb) = {

	.clk = {
.flags = TIMERS_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_TIMERS_APB_BUS_CLK_ID,.name =
		    TIMERS_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_APB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_APB_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name APB2
*/
static struct bus_clk CLK_NAME(apb2) = {

	.clk = {
.flags = APB2_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_APB2_BUS_CLK_ID,.name =
		    APB2_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_APB2_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_APB2_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_APB2_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_APB2_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_APB2_CLKGATE_APB2_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Bus clock name SPUM_OPEN_AXI
*/
static struct bus_clk CLK_NAME(spum_open_axi) = {
	.clk = {
.flags = SPUM_OPEN_AXI_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SPUM_OPEN_AXI_BUS_CLK_ID,.name =
		    SPUM_OPEN_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_AXI_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(switch_axi),};

/*
Bus clock name SPUM_SEC_AXI
*/
static struct bus_clk CLK_NAME(spum_sec_axi) = {
	.clk = {
.flags = SPUM_SEC_AXI_BUS_CLK_FLAGS,.clk_type =
		    CLK_TYPE_BUS,.id =
		    CLK_SPUM_SEC_AXI_BUS_CLK_ID,.name =
		    SPUM_SEC_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_AXI_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_AXI_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = CLK_PTR(switch_axi),};

/*
Peri clock name UARTB
*/
/*peri clk src list*/
static struct clk *uartb_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_156m), CLK_PTR(ref_156m));
static struct peri_clk CLK_NAME(uartb) = {

	.clk = {
	.flags = UARTB_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_UARTB_PERI_CLK_ID,.name =
		    UARTB_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(uartb_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_UARTB_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_UARTB_CLKGATE_UARTB_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_UARTB_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_UARTB_DIV_UARTB_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_UARTB_DIV_UARTB_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_UARTB_TRIGGER_MASK,.
		    diether_bits = 8,.pll_select_offset =
		    KPS_CLK_MGR_REG_UARTB_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_UARTB_DIV_UARTB_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_UARTB_DIV_UARTB_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 1,.clk =
		    uartb_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_UARTB_SOFT_RSTN_MASK,};

/*
Peri clock name UARTB2
*/
/*peri clk src list*/
static struct clk *uartb2_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_156m), CLK_PTR(ref_156m));
static struct peri_clk CLK_NAME(uartb2) = {

	.clk = {
	.flags = UARTB2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_UARTB2_PERI_CLK_ID,.name =
		    UARTB2_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(uartb2_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_UARTB2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_UARTB2_CLKGATE_UARTB2_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset = KPS_CLK_MGR_REG_UARTB2_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_UARTB2_DIV_UARTB2_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_UARTB2_DIV_UARTB2_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_UARTB2_TRIGGER_MASK,.
		    diether_bits = 8,.pll_select_offset =
		    KPS_CLK_MGR_REG_UARTB2_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_UARTB2_DIV_UARTB2_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_UARTB2_DIV_UARTB2_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 1,.clk =
		    uartb2_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_UARTB2_SOFT_RSTN_MASK,};

/*
Peri clock name UARTB3
*/
/*peri clk src list*/
static struct clk *uartb3_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_156m), CLK_PTR(ref_156m));
static struct peri_clk CLK_NAME(uartb3) = {

	.clk = {
	.flags = UARTB3_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_UARTB3_PERI_CLK_ID,.name =
		    UARTB3_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(uartb3_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_UARTB3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_UARTB3_CLKGATE_UARTB3_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset = KPS_CLK_MGR_REG_UARTB3_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_UARTB3_DIV_UARTB3_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_UARTB3_DIV_UARTB3_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_UARTB3_TRIGGER_MASK,.
		    diether_bits = 8,.pll_select_offset =
		    KPS_CLK_MGR_REG_UARTB3_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_UARTB3_DIV_UARTB3_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_UARTB3_DIV_UARTB3_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 1,.clk =
		    uartb3_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_UARTB3_SOFT_RSTN_MASK,};

/*
Peri clock name UARTB4
*/
/*peri clk src list*/
static struct clk *uartb4_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_156m), CLK_PTR(ref_156m));
static struct peri_clk CLK_NAME(uartb4) = {

	.clk = {
	.flags = UARTB4_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_UARTB4_PERI_CLK_ID,.name =
		    UARTB4_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(uartb4_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_UARTB4_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_UARTB4_CLKGATE_UARTB4_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_offset = KPS_CLK_MGR_REG_UARTB4_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_UARTB4_DIV_UARTB4_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_UARTB4_DIV_UARTB4_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_UARTB4_TRIGGER_MASK,.
		    diether_bits = 8,.pll_select_offset =
		    KPS_CLK_MGR_REG_UARTB4_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_UARTB4_DIV_UARTB4_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_UARTB4_DIV_UARTB4_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 1,.clk = uartb4_peri_clk_src_list,},};

/*
Peri clock name SSP0_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp0_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp0_audio) = {

	.clk = {
	.flags = SSP0_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP0_AUDIO_PERI_CLK_ID,.name =
		    SSP0_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp0_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SSP0_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_AUDIO_HYST_EN_MASK,.stprsts_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_SSP0_AUDIO_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_SSP0_AUDIO_DIV_SHIFT,.
		    div_trig_offset = KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,
//                                      .div_trig_mask= KPS_CLK_MGR_REG_DIV_TRIG_SSP0_AUDIO_TRIGGER_MASK,   //not in Capri
	.prediv_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.
		    prediv_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SSP0_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_OFFSET,.
		    pll_select_mask =
		    KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_SSP0_AUDIO_PRE_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SSP0_AUDIO_DIV_SSP0_AUDIO_PRE_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 2,.clk = ssp0_audio_peri_clk_src_list,},};

/*
Peri clock name SSP2_AUDIO
*/
/*peri clk src list*/
static struct clk *ssp2_audio_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(ref_312m), CLK_PTR(ref_cx40));
static struct peri_clk CLK_NAME(ssp2_audio) = {

	.clk = {
	.flags = SSP2_AUDIO_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SSP2_AUDIO_PERI_CLK_ID,.name =
		    SSP2_AUDIO_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp2_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SSP2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_AUDIO_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_AUDIO_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_AUDIO_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_AUDIO_HYST_EN_MASK,.stprsts_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_AUDIO_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_VOLTAGE_LEVEL_MASK,.clk_div = {
		.div_offset = KPS_CLK_MGR_REG_SSP2_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_DIV_MASK,.div_shift =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_DIV_SHIFT,.div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,
//                                      .div_trig_mask= KPS_CLK_MGR_REG_DIV_TRIG_SSP2_AUDIO_TRIGGER_MASK,   //not in Capri
	.prediv_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.
		    prediv_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SSP2_AUDIO_PRE_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SSP2_DIV_OFFSET,.
		    pll_select_mask =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 3,.src_inx = 0,.clk = ssp2_audio_peri_clk_src_list,},};

/*
Peri clock name BSC1
*/
/*peri clk src list*/
static struct clk *bsc1_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_104m), CLK_PTR(ref_104m),
		  CLK_PTR(var_13m), CLK_PTR(ref_13m));
static struct peri_clk CLK_NAME(bsc1) = {

	.clk = {
	.flags = BSC1_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_BSC1_PERI_CLK_ID,.name =
		    BSC1_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(bsc1_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_BSC1_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_BSC1_CLKGATE_BSC1_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_BSC1_DIV_OFFSET,.div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_BSC1_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_BSC1_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_BSC1_DIV_BSC1_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_BSC1_DIV_BSC1_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 5,.src_inx = 3,.clk =
		    bsc1_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB2_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB2_SOFTRST_BSC1_SOFT_RSTN_MASK,};

/*
Peri clock name BSC2
*/
/*peri clk src list*/
static struct clk *bsc2_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_104m), CLK_PTR(ref_104m),
		  CLK_PTR(var_13m), CLK_PTR(ref_13m));
static struct peri_clk CLK_NAME(bsc2) = {

	.clk = {
	.flags = BSC2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_BSC2_PERI_CLK_ID,.name =
		    BSC2_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(bsc2_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_BSC2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_BSC2_CLKGATE_BSC2_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_BSC2_DIV_OFFSET,.div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_BSC2_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_BSC2_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_BSC2_DIV_BSC2_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_BSC2_DIV_BSC2_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 5,.src_inx = 3,.clk =
		    bsc2_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB2_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB2_SOFTRST_BSC2_SOFT_RSTN_MASK,};

/*
Peri clock name BSC3
*/
/*peri clk src list*/
static struct clk *bsc3_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_104m), CLK_PTR(ref_104m),
		  CLK_PTR(var_13m), CLK_PTR(ref_13m));
static struct peri_clk CLK_NAME(bsc3) = {

	.clk = {
	.flags = BSC3_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_BSC3_PERI_CLK_ID,.name =
		    BSC3_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(bsc3_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    2,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK2_BSC3_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_BSC3_CLKGATE_BSC3_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_BSC3_DIV_OFFSET,.div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG2_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG2_BSC3_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_BSC3_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_BSC3_DIV_BSC3_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_BSC3_DIV_BSC3_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 5,.src_inx = 3,.clk =
		    bsc3_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB2_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB2_SOFTRST_BSC3_SOFT_RSTN_MASK,};

/*
Peri clock name PWM
*/
/*peri clk src list*/
static struct peri_clk CLK_NAME(pwm) = {

	.clk = {
.flags = PWM_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_PWM_PERI_CLK_ID,.name =
		    PWM_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(pwm_apb),
						      NULL),.
		    ops = &gen_peri_clk_ops,.rate =
		    FREQ_MHZ(26),},.ccu_clk = &CLK_NAME(kps),.mask_set =
	    0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_PWM_POLICY0_MASK_MASK,.
	    policy_mask_init =
	    DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_STPRSTS_MASK,.
	    volt_lvl_mask =
	    KPS_CLK_MGR_REG_PWM_CLKGATE_PWM_VOLTAGE_LEVEL_MASK,.
	    soft_reset_offset =
	    KPS_RST_MGR_REG_APB2_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB2_SOFTRST_PWM_SOFT_RSTN_MASK,};

/*
Peri clock name SSP0
*/
#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs ssp0_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_ECONOMY,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

/*
Peri clock name SSP0
*/
/*peri clk src list*/
static struct clk *ssp0_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_104m), CLK_PTR(ref_104m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(ssp0) = {

	.clk = {
	.flags = SSP0_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP0_PERI_CLK_ID,.name =
		    SSP0_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp0_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &ssp0_dfs,
#endif
	    .policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SSP0_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_SSP0_CLKGATE_SSP0_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_SSP0_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SSP0_DIV_SSP0_DIV_MASK,.div_shift =
		    KPS_CLK_MGR_REG_SSP0_DIV_SSP0_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SSP0_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SSP0_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_SSP0_DIV_SSP0_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SSP0_DIV_SSP0_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 5,.src_inx = 0,.clk =
		    ssp0_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_SSP0_SOFT_RSTN_MASK,};

/*
Peri clock name SSP2
*/
/*peri clk src list*/
#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs ssp2_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_ECONOMY,
	.opp_weightage = {
			  [PI_OPP_ECONOMY] = 25,
			  [PI_OPP_NORMAL] = 0,
			  },

};
#endif

static struct clk *ssp2_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(crystal), CLK_PTR(var_104m), CLK_PTR(ref_104m),
		  CLK_PTR(var_96m), CLK_PTR(ref_96m));
static struct peri_clk CLK_NAME(ssp2) = {

	.clk = {
	.flags = SSP2_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SSP2_PERI_CLK_ID,.name =
		    SSP2_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(ssp2_apb), NULL),.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &ssp2_dfs,
#endif
	    .policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SSP2_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_SSP2_CLKGATE_SSP2_VOLTAGE_LEVEL_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_SSP2_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_DIV_MASK,.div_shift =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SSP2_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SSP2_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SSP2_DIV_SSP2_PLL_SELECT_SHIFT,},.src_clk =
	{
.count = 5,.src_inx = 0,.clk =
		    ssp2_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_SSP2_SOFT_RSTN_MASK,};
/*
Peri clock name TIMERS
*/
/*peri clk src list*/
static struct clk *timers_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(ref_1m), CLK_PTR(ref_32k));
static struct peri_clk CLK_NAME(timers) = {
	.clk = {
	.flags = TIMERS_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_TIMERS_PERI_CLK_ID,.name =
		    TIMERS_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(timers_apb), NULL),.rate =
		    0,.ops = &gen_peri_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),.mask_set = 0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_TIMERS_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_CLK_EN_MASK,.gating_sel_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_STPRSTS_MASK,.volt_lvl_mask =
	    KPS_CLK_MGR_REG_TIMERS_CLKGATE_TIMERS_VOLTAGE_LEVEL_MASK,.clk_div =
	{
	.div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_TIMERS_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_TIMERS_DIV_OFFSET,.pll_select_mask =
		    KPS_CLK_MGR_REG_TIMERS_DIV_TIMERS_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_TIMERS_DIV_TIMERS_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 2,.src_inx = 0,.clk =
		    timers_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB1_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB1_SOFTRST_TIMERS_SOFT_RSTN_MASK,};

/*
Peri clock name SPUM_OPEN
*/

#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs spum_open_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_NORMAL,
};
#endif

/*peri clk src list*/
static struct clk *spum_open_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(var_312m),
		  CLK_PTR(ref_312m),
		  CLK_PTR(var_208m),
		  CLK_PTR(ref_208m));
static struct peri_clk CLK_NAME(spum_open) = {
	.clk = {
	.flags = SPUM_OPEN_PERI_CLK_FLAGS,.clk_type =
		    CLK_TYPE_PERI,.id =
		    CLK_SPUM_OPEN_PERI_CLK_ID,.name =
		    SPUM_OPEN_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(spum_open_axi),
					      NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),.mask_set = 0,
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &spum_open_dfs,
#endif
	    .policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SPUM_OPEN_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_OPEN_CLKGATE_SPUM_OPEN_STPRSTS_MASK,.clk_div =
	{
	.div_offset = KPS_CLK_MGR_REG_SPUM_OPEN_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SPUM_OPEN_DIV_SPUM_OPEN_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_SPUM_OPEN_DIV_SPUM_OPEN_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SPUM_OPEN_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SPUM_OPEN_DIV_OFFSET,.
		    pll_select_mask =
		    KPS_CLK_MGR_REG_SPUM_OPEN_DIV_SPUM_OPEN_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SPUM_OPEN_DIV_SPUM_OPEN_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 4,.src_inx = 0,.clk =
		    spum_open_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB7_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB7_SOFTRST_SPUM_OPEN_SOFT_RSTN_MASK,};
#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs spum_sec_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_NORMAL,
};
#endif
/*
Peri clock name SPUM_SEC
*/
/*peri clk src list*/
static struct clk *spum_sec_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(var_312m),
		  CLK_PTR(ref_312m),
		  CLK_PTR(var_208m),
		  CLK_PTR(ref_208m));
static struct peri_clk CLK_NAME(spum_sec) = {
	.clk = {
	.flags = SPUM_SEC_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_SPUM_SEC_PERI_CLK_ID,.name =
		    SPUM_SEC_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(CLK_PTR(spum_sec_axi),
					      NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(kps),
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &spum_sec_dfs,
#endif
	    .mask_set = 0,.policy_bit_mask =
	    KPS_CLK_MGR_REG_POLICY0_MASK_SPUM_SEC_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_HYST_VAL_MASK,.
	    hyst_en_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_HYST_EN_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_SPUM_SEC_CLKGATE_SPUM_SEC_STPRSTS_MASK,.clk_div = {
	.div_offset = KPS_CLK_MGR_REG_SPUM_SEC_DIV_OFFSET,.div_mask =
		    KPS_CLK_MGR_REG_SPUM_SEC_DIV_SPUM_SEC_DIV_MASK,.
		    div_shift =
		    KPS_CLK_MGR_REG_SPUM_SEC_DIV_SPUM_SEC_DIV_SHIFT,.
		    div_trig_offset =
		    KPS_CLK_MGR_REG_DIV_TRIG_OFFSET,.div_trig_mask =
		    KPS_CLK_MGR_REG_DIV_TRIG_SPUM_SEC_TRIGGER_MASK,.
		    pll_select_offset =
		    KPS_CLK_MGR_REG_SPUM_SEC_DIV_OFFSET,.
		    pll_select_mask =
		    KPS_CLK_MGR_REG_SPUM_SEC_DIV_SPUM_SEC_PLL_SELECT_MASK,.
		    pll_select_shift =
		    KPS_CLK_MGR_REG_SPUM_SEC_DIV_SPUM_SEC_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 4,.src_inx = 0,.clk =
		    spum_sec_peri_clk_src_list,},.soft_reset_offset =
	    KPS_RST_MGR_REG_APB7_SOFTRST_OFFSET,.clk_reset_mask =
	    KPS_RST_MGR_REG_APB7_SOFTRST_SPUM_SEC_SOFT_RSTN_MASK,};

/*
BUS clock name MPHI_AHB
*/
#ifdef CONFIG_KONA_PI_MGR
static struct clk_dfs mphi_ahb_clk_dfs = {
	.dfs_policy = CLK_DFS_POLICY_STATE,
	.policy_param = PI_OPP_NORMAL,
};
#endif

static struct bus_clk CLK_NAME(mphi_ahb) = {

	.clk = {
	.flags = MPHI_AHB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_MPHI_AHB_BUS_CLK_ID,.name =
		    MPHI_AHB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops = &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(kps),
#ifdef CONFIG_KONA_PI_MGR
	    .clk_dfs = &mphi_ahb_clk_dfs,
#endif
.clk_gate_offset =
	    KPS_CLK_MGR_REG_MPHI_CLKGATE_OFFSET,.clk_en_mask =
	    KPS_CLK_MGR_REG_MPHI_CLKGATE_MPHI_AHB_CLK_EN_MASK,.
	    gating_sel_mask =
	    KPS_CLK_MGR_REG_MPHI_CLKGATE_MPHI_AHB_HW_SW_GATING_SEL_MASK,.
	    stprsts_mask =
	    KPS_CLK_MGR_REG_MPHI_CLKGATE_MPHI_AHB_STPRSTS_MASK,.
	    freq_tbl_index = -1,.src_clk = NULL,};

#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
/*
CCU clock name ESUB
*/

/* CCU freq list */
static u32 esub_clk_freq_list0[] = DEFINE_ARRAY_ARGS(26000000, 13000000);
static u32 esub_clk_freq_list1[] = DEFINE_ARRAY_ARGS(78000000, 39000000);
static u32 esub_clk_freq_list2[] = DEFINE_ARRAY_ARGS(156000000, 78000000);
static u32 esub_clk_freq_list3[] = DEFINE_ARRAY_ARGS(156000000, 78000000);
static u32 esub_clk_freq_list4[] = DEFINE_ARRAY_ARGS(156000000, 78000000);
static u32 esub_clk_freq_list5[] = DEFINE_ARRAY_ARGS(208000000, 104000000);
static u32 esub_clk_freq_list6[] = DEFINE_ARRAY_ARGS(208000000, 104000000);
static u32 esub_clk_freq_list7[] = DEFINE_ARRAY_ARGS(208000000, 104000000);

static struct ccu_clk CLK_NAME(esub) = {

	.clk = {
	.flags = ESUB_CCU_CLK_FLAGS,.id = CLK_ESUB_CCU_CLK_ID,.name =
		    ESUB_CCU_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_CCU,.ops = &gen_ccu_clk_ops,},.ccu_ops =
	    &gen_ccu_ops,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	    .pi_id = PI_MGR_PI_ID_ESUB,
#else
	    .pi_id = -1,
#endif
	    .ccu_clk_mgr_base =
	    HW_IO_PHYS_TO_VIRT(ESUB_CLK_BASE_ADDR),.wr_access_offset =
	    ESUB_CLK_MGR_REG_WR_ACCESS_OFFSET,.policy_mask1_offset =
	    ESUB_CLK_MGR_REG_POLICY0_MASK_OFFSET,.policy_mask2_offset =
	    0,.policy_freq_offset =
	    ESUB_CLK_MGR_REG_POLICY_FREQ_OFFSET,.policy_ctl_offset =
	    ESUB_CLK_MGR_REG_POLICY_CTL_OFFSET,.inten_offset =
	    ESUB_CLK_MGR_REG_INTEN_OFFSET,.intstat_offset =
	    ESUB_CLK_MGR_REG_INTSTAT_OFFSET,.vlt_peri_offset =
	    ESUB_CLK_MGR_REG_VLT_PERI_OFFSET,.lvm_en_offset =
	    ESUB_CLK_MGR_REG_LVM_EN_OFFSET,.lvm0_3_offset =
	    ESUB_CLK_MGR_REG_LVM0_3_OFFSET,.vlt0_3_offset =
	    ESUB_CLK_MGR_REG_VLT0_3_OFFSET,.vlt4_7_offset =
	    ESUB_CLK_MGR_REG_VLT4_7_OFFSET,
#ifdef CONFIG_DEBUG_FS
	    .policy_dbg_offset =
	    ESUB_CLK_MGR_REG_POLICY_DBG_OFFSET,.policy_dbg_act_freq_shift =
	    ESUB_CLK_MGR_REG_POLICY_DBG_ACT_FREQ_SHIFT,.
	    policy_dbg_act_policy_shift =
	    ESUB_CLK_MGR_REG_POLICY_DBG_ACT_POLICY_SHIFT,.clk_mon_offset =
	    ESUB_CLK_MGR_REG_CLKMON_OFFSET,
#endif
.freq_volt = ESUB_CCU_FREQ_VOLT_TBL,.freq_count =
	    ESUB_CCU_FREQ_VOLT_TBL_SZ,.volt_peri =
	    DEFINE_ARRAY_ARGS(VLT_NORMAL_PERI,
					  VLT_HIGH_PERI),.
	    freq_policy =
	    DEFINE_ARRAY_ARGS(ESUB_CCU_FREQ_POLICY_TBL),.freq_tbl =
	    DEFINE_ARRAY_ARGS(esub_clk_freq_list0,
					  esub_clk_freq_list1,
					  esub_clk_freq_list2,
					  esub_clk_freq_list3,
					  esub_clk_freq_list4,
					  esub_clk_freq_list5,
					  esub_clk_freq_list6,
					  esub_clk_freq_list7),.
	    ccu_reset_mgr_base =
	    HW_IO_PHYS_TO_VIRT(ESUB_RST_BASE_ADDR),.
	    reset_wr_access_offset = ESUB_RST_MGR_REG_WR_ACCESS_OFFSET,};

/*
PLL Clk name esub_pll
*/

static u32 esub_vc0_thold[] = { FREQ_MHZ(1500), PLL_VCO_RATE_MAX };
static u32 esub_cfg_val[] = { 0x8000000, 0x8102000 };

static struct pll_cfg_ctrl_info esub_pll_cfg_ctrl = {
	.pll_cfg_ctrl_offset = ESUB_CLK_MGR_REG_PLLE_CTRL_OFFSET,
	.pll_cfg_ctrl_mask = ESUB_CLK_MGR_REG_PLLE_CTRL_I_PLL_CNTRL_PLLE_MASK,
	.pll_cfg_ctrl_shift = ESUB_CLK_MGR_REG_PLLE_CTRL_I_PLL_CNTRL_PLLE_SHIFT,

	.vco_thold = esub_vc0_thold,
	.pll_config_value = esub_cfg_val,
	.thold_count = ARRAY_SIZE(esub_vc0_thold),
};

static struct pll_clk CLK_NAME(esub_pll) = {

	.clk = {
	.flags = ESUB_PLL_CLK_FLAGS,.id = CLK_ESUB_PLL_CLK_ID,.name = ESUB_PLL_CLK_NAME_STR,.clk_type = CLK_TYPE_PLL,.ops = &gen_pll_clk_ops,},.ccu_clk = &CLK_NAME(esub),.pll_ctrl_offset = ESUB_CLK_MGR_REG_PLLE_PWRDN_OFFSET,.idle_pwrdwn_sw_ovrride_mask = 0,	// no such bit in Capri
.soft_post_resetb_offset =
	    ESUB_CLK_MGR_REG_PLLE_POST_RESETB_OFFSET,.
	    soft_post_resetb_mask =
	    ESUB_CLK_MGR_REG_PLLE_POST_RESETB_I_POST_RESETB_PLLE_MASK,.
	    soft_resetb_offset =
	    ESUB_CLK_MGR_REG_PLLE_RESETB_OFFSET,.soft_resetb_mask =
	    ESUB_CLK_MGR_REG_PLLE_RESETB_I_PLL_RESETB_PLLE_MASK,.
	    pwrdwn_offset =
	    ESUB_CLK_MGR_REG_PLLE_PWRDN_OFFSET,.pwrdwn_mask =
	    ESUB_CLK_MGR_REG_PLLE_PWRDN_I_PLL_PWRDWN_PLLE_MASK,.
	    ndiv_pdiv_offset =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_PDIV_OFFSET,.ndiv_int_mask =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_PDIV_I_NDIV_INT_PLLE_MASK,.
	    ndiv_int_shift =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_PDIV_I_NDIV_INT_PLLE_SHIFT,.
	    ndiv_int_max = 512,.pdiv_mask =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_PDIV_I_PDIV_PLLE_MASK,.
	    pdiv_shift =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_PDIV_I_PDIV_PLLE_SHIFT,.
	    pdiv_max = 8,.pll_lock_offset =
	    ESUB_CLK_MGR_REG_PLL_LOCK_OFFSET,.pll_lock =
	    ESUB_CLK_MGR_REG_PLL_LOCK_PLL_LOCK_PLLE_MASK,.
	    ndiv_frac_offset =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_FRAC_OFFSET,.ndiv_frac_mask =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_FRAC_I_NDIV_FRAC_PLLE_SHIFT,.
	    ndiv_frac_shift =
	    ESUB_CLK_MGR_REG_PLLE_NDIV_FRAC_I_NDIV_FRAC_PLLE_MASK,.
	    cfg_ctrl_info = &esub_pll_cfg_ctrl,};

/*esw_sys - channel 0*/
static struct pll_chnl_clk CLK_NAME(esw_sys_ch0) = {

	.clk = {
.flags = ESW_SYS_CH0_CLK_FLAGS,.id =
		    CLK_ESW_SYS_CH0_CLK_ID,.name =
		    ESW_SYS_CH0_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_PLL_CHNL,.ops =
		    &gen_pll_chnl_clk_ops,},.ccu_clk =
	    &CLK_NAME(esub),.pll_clk =
	    &CLK_NAME(esub_pll),.cfg_reg_offset =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_CH0_OFFSET,.mdiv_mask =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_CH0_I_CH0_MDIV_PLLE_MASK,.
	    mdiv_shift =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_CH0_I_CH0_MDIV_PLLE_SHIFT,.
	    mdiv_max = 256,.pll_enableb_offset =
	    ESUB_CLK_MGR_REG_PLLE_ENABLEB_OFFSET,.out_en_mask =
	    1 << (0 +
			  ESUB_CLK_MGR_REG_PLLE_ENABLEB_I_ENABLEB_PLLE_SHIFT),.
	    pll_load_ch_en_offset =
	    ESUB_CLK_MGR_REG_PLLE_LOAD_CH_EN_OFFSET,.load_en_mask =
	    1 << (0 +
			  ESUB_CLK_MGR_REG_PLLE_LOAD_CH_EN_I_LOAD_EN_CH_PLLE_SHIFT),.
	    pll_hold_ch_offset =
	    ESUB_CLK_MGR_REG_PLLE_CTRL0_OFFSET,.hold_en_mask =
	    1 << (0 + ESUB_CLK_MGR_REG_PLLE_CTRL0_I_HOLD_CH_PLLE_SHIFT),};

/*esw_sys_125M - channel 1*/
static struct pll_chnl_clk CLK_NAME(esw_sys_125m_ch1) = {

	.clk = {
.flags = ESW_SYS_125M_CH1_CLK_FLAGS,.id =
		    CLK_ESW_SYS_125M_CH1_CLK_ID,.name =
		    ESW_SYS_125M_CH1_CLK_NAME_STR,.clk_type =
		    CLK_TYPE_PLL_CHNL,.ops =
		    &gen_pll_chnl_clk_ops,},.ccu_clk =
	    &CLK_NAME(esub),.pll_clk =
	    &CLK_NAME(esub_pll),.cfg_reg_offset =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_125M_CH1_OFFSET,.
	    mdiv_mask =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_125M_CH1_I_CH1_MDIV_PLLE_MASK,.
	    mdiv_shift =
	    ESUB_CLK_MGR_REG_PLLE_MDIV_ESW_SYS_125M_CH1_I_CH1_MDIV_PLLE_SHIFT,.
	    mdiv_max = 256,.pll_enableb_offset =
	    ESUB_CLK_MGR_REG_PLLE_ENABLEB_OFFSET,.out_en_mask =
	    1 << (1 +
			  ESUB_CLK_MGR_REG_PLLE_ENABLEB_I_ENABLEB_PLLE_SHIFT),.
	    pll_load_ch_en_offset =
	    ESUB_CLK_MGR_REG_PLLE_LOAD_CH_EN_OFFSET,.load_en_mask =
	    1 << (1 +
			  ESUB_CLK_MGR_REG_PLLE_LOAD_CH_EN_I_LOAD_EN_CH_PLLE_SHIFT),.
	    pll_hold_ch_offset =
	    ESUB_CLK_MGR_REG_PLLE_CTRL0_OFFSET,.hold_en_mask =
	    1 << (1 + ESUB_CLK_MGR_REG_PLLE_CTRL0_I_HOLD_CH_PLLE_SHIFT),};

/*
Ref clock name ESW_GPIO_125M
*/
static struct ref_clk CLK_NAME(esw_gpio_125m) = {

	.clk = {
.flags = ESW_GPIO_125M_CLK_FLAGS,.clk_type = CLK_TYPE_REF,.id =
		    CLK_REF_ESW_GPIO_125M_CLK_ID,.name =
		    REF_ESW_GPIO_125M_CLK_NAME_STR,.rate =
		    125000000,.ops = &gen_ref_clk_ops,},.ccu_clk =
	    &CLK_NAME(esub),};

/*
Bus clock name ESUB_AXI
*/
static struct bus_clk CLK_NAME(esub_axi) = {

	.clk = {
.flags = ESUB_AXI_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_ESUB_AXI_BUS_CLK_ID,.name =
		    ESUB_AXI_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(esub),.clk_gate_offset =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_OFFSET,.clk_en_mask =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_ESUB_AXI_CLK_EN_MASK,.
	    gating_sel_mask =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_ESUB_AXI_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_ESUB_AXI_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_ESUB_AXI_HYST_EN_MASK,.
	    stprsts_mask =
	    ESUB_CLK_MGR_REG_ESUB_AXI_CLKGATE_ESUB_AXI_STPRSTS_MASK,.
	    freq_tbl_index = 1,.src_clk = NULL,};

/*
Bus clock name ESUB_APB
*/
static struct bus_clk CLK_NAME(esub_apb) = {

	.clk = {
.flags = ESUB_APB_BUS_CLK_FLAGS,.clk_type = CLK_TYPE_BUS,.id =
		    CLK_ESUB_APB_BUS_CLK_ID,.name =
		    ESUB_APB_BUS_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.ops =
		    &gen_bus_clk_ops,},.ccu_clk =
	    &CLK_NAME(esub),.clk_gate_offset =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_OFFSET,.clk_en_mask =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_ESUB_APB_CLK_EN_MASK,.
	    gating_sel_mask =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_ESUB_APB_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_ESUB_APB_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_ESUB_APB_HYST_EN_MASK,.
	    stprsts_mask =
	    ESUB_CLK_MGR_REG_ESUB_APB_CLKGATE_ESUB_APB_STPRSTS_MASK,.
	    freq_tbl_index = 2,.src_clk = NULL,};

/*
Peri clock name ESW_SYS
*/
/*peri clk src list*/
static struct clk *esw_sys_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(esw_sys_ch0), CLK_PTR(var_208m), CLK_PTR(crystal));
static struct peri_clk CLK_NAME(esw_sys) = {

	.clk = {
	.flags = ESW_SYS_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_ESW_SYS_PERI_CLK_ID,.name =
		    ESW_SYS_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(esub),.mask_set =
	    1,.policy_bit_mask =
	    ESUB_CLK_MGR_REG_POLICY0_MASK_ESW_SYS_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_OFFSET,.clk_en_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_CLK_EN_MASK,.
	    gating_sel_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_HYST_EN_MASK,.
	    stprsts_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_STPRSTS_MASK,.
	    volt_lvl_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_CLKGATE_ESW_SYS_VOLTAGE_LEVEL_MASK,.
	    clk_div = {
	.div_offset = ESUB_CLK_MGR_REG_ESW_SYS_DIV_OFFSET,.div_mask =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_ESW_SYS_DIV_MASK,.
		    div_shift =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_ESW_SYS_DIV_SHIFT,.
		    div_trig_offset =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_OFFSET,.div_trig_mask =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_ESW_SYS_TRIGGER_MASK,.
		    pll_select_offset =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_OFFSET,.
		    pll_select_mask =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_ESW_SYS_PLL_SELECT_MASK,.
		    pll_select_shift =
		    ESUB_CLK_MGR_REG_ESW_SYS_DIV_ESW_SYS_PLL_SELECT_SHIFT,},.
	    src_clk = {
.count = 3,.src_inx = 0,.clk =
		    esw_sys_peri_clk_src_list,},.soft_reset_offset =
	    ESUB_RST_MGR_REG_ESW_SYS_SOFT_RSTN_OFFSET,.clk_reset_mask =
	    ESUB_RST_MGR_REG_ESW_SYS_SOFT_RSTN_ESW_SYS_SOFT_RSTN_MASK,};

/*
Peri clock name ESW_25M_CLK
*/
/*peri clk src list*/
static struct clk *esw_25m_clk_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(esw_gpio_125m), CLK_PTR(esw_sys_125m_ch1));
static struct peri_clk CLK_NAME(esw_25m) = {

	.clk = {
	.flags = ESW_25M_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_ESW_25M_PERI_CLK_ID,.name =
		    ESW_25M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(esub),.mask_set =
	    1,.policy_bit_mask =
	    ESUB_CLK_MGR_REG_POLICY0_MASK_ESW_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_OFFSET,.clk_en_mask =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_ESW_25M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_ESW_25M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_ESW_25M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_ESW_25M_HYST_EN_MASK,.
	    stprsts_mask =
	    ESUB_CLK_MGR_REG_ESW_25M_CLKGATE_ESW_25M_STPRSTS_MASK,.src_clk = {
.count = 2,.src_inx = 0,.clk = esw_25m_clk_peri_clk_src_list,},};

/*
Peri clock name ESW_125M_CLK
*/
/*peri clk src list*/
static struct clk *esw_125m_clk_peri_clk_src_list[] =
DEFINE_ARRAY_ARGS(CLK_PTR(esw_gpio_125m), CLK_PTR(esw_sys_125m_ch1));
static struct peri_clk CLK_NAME(esw_125m) = {

	.clk = {
	.flags = ESW_125M_PERI_CLK_FLAGS,.clk_type = CLK_TYPE_PERI,.id =
		    CLK_ESW_125M_PERI_CLK_ID,.name =
		    ESW_125M_PERI_CLK_NAME_STR,.dep_clks =
		    DEFINE_ARRAY_ARGS(NULL),.rate = 0,.ops =
		    &gen_peri_clk_ops,},.ccu_clk = &CLK_NAME(esub),.mask_set =
	    1,.policy_bit_mask =
	    ESUB_CLK_MGR_REG_POLICY0_MASK_ESW_POLICY0_MASK_MASK,.
	    policy_mask_init = DEFINE_ARRAY_ARGS(1, 1, 1, 1),.clk_gate_offset =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_OFFSET,.clk_en_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_ESW_SYS_125M_CLK_EN_MASK,.
	    gating_sel_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_ESW_SYS_125M_HW_SW_GATING_SEL_MASK,.
	    hyst_val_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_ESW_SYS_125M_HYST_VAL_MASK,.
	    hyst_en_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_ESW_SYS_125M_HYST_EN_MASK,.
	    stprsts_mask =
	    ESUB_CLK_MGR_REG_ESW_SYS_125M_CLKGATE_ESW_SYS_125M_STPRSTS_MASK,.
	    src_clk = {
.count = 2,.src_inx = 0,.clk = esw_125m_clk_peri_clk_src_list,},};
#endif

/*Island specifc handlers*/

int clk_set_pll_pwr_on_idle(int pll_id, int enable)
{
	u32 reg_val = 0;
	int ret = 0;
	/* enable write access */
	switch (pll_id) {
	case ROOT_CCU_PLL0A:
		ccu_write_access_enable(&CLK_NAME(root), true);
		reg_val =
		    readl(CLK_NAME(root).ccu_clk_mgr_base +
			  ROOT_CLK_MGR_REG_PLL0A_OFFSET);
		if (enable)
			reg_val |=
			    ROOT_CLK_MGR_REG_PLL0A_PLL0_IDLE_PWRDWN_SW_OVRRIDE_MASK;
		else
			reg_val &=
			    ~ROOT_CLK_MGR_REG_PLL0A_PLL0_IDLE_PWRDWN_SW_OVRRIDE_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_PLL0A_OFFSET);
		ccu_write_access_enable(&CLK_NAME(root), false);
		break;

	case ROOT_CCU_PLL1A:
		ccu_write_access_enable(&CLK_NAME(root), true);
		reg_val =
		    readl(CLK_NAME(root).ccu_clk_mgr_base +
			  ROOT_CLK_MGR_REG_PLL1A_OFFSET);
		if (enable)
			reg_val |=
			    ROOT_CLK_MGR_REG_PLL1A_PLL1_IDLE_PWRDWN_SW_OVRRIDE_MASK;
		else
			reg_val &=
			    ~ROOT_CLK_MGR_REG_PLL1A_PLL1_IDLE_PWRDWN_SW_OVRRIDE_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_PLL1A_OFFSET);
		ccu_write_access_enable(&CLK_NAME(root), false);

		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

EXPORT_SYMBOL(clk_set_pll_pwr_on_idle);

int clk_set_crystal_pwr_on_idle(int enable)
{
	u32 reg_val = 0;
	/* enable write access */
	ccu_write_access_enable(&CLK_NAME(root), true);

	reg_val = readl(KONA_ROOT_CLK_VA + ROOT_CLK_MGR_REG_CRYSTALCTL_OFFSET);
	if (enable)
		reg_val |=
		    ROOT_CLK_MGR_REG_CRYSTALCTL_CRYSTAL_IDLE_PWRDWN_SW_OVRRIDE_MASK;
	else
		reg_val &=
		    ~ROOT_CLK_MGR_REG_CRYSTALCTL_CRYSTAL_IDLE_PWRDWN_SW_OVRRIDE_MASK;

	writel(reg_val, KONA_ROOT_CLK_VA + ROOT_CLK_MGR_REG_CRYSTALCTL_OFFSET);
	/* disable write access */
	ccu_write_access_enable(&CLK_NAME(root), false);

	return 0;
}

EXPORT_SYMBOL(clk_set_crystal_pwr_on_idle);

int root_ccu_clk_init(struct clk *clk)
{

	if (clk->clk_type != CLK_TYPE_CCU)
		return -EPERM;

	if (clk->init)
		return 0;

	clk_dbg("%s - %s\n", __func__, clk->name);

	clk_set_pll_pwr_on_idle(ROOT_CCU_PLL0A, 1);
	clk_set_pll_pwr_on_idle(ROOT_CCU_PLL1A, 1);
	clk_set_crystal_pwr_on_idle(1);
#if 0
	/* enable write access */
	ccu_write_access_enable(ccu_clk, true);

	/* initialize PLL1 OFFSET */

	/* disable write access */
	ccu_write_access_enable(ccu_clk, false);
#endif
	return 0;
}

/*Override ccu_clk_set_freq_policy for KPROC as the there is 5 operating points*/

/* post divider for ARM PLL */
static inline void __proc_clk_set_pll0_div(void __iomem *base, u32 div)
{

	/*For policy 6 divder should be for VCO/3/4 */

	writel(div, base + KPROC_CLK_MGR_REG_PLLARMC_OFFSET);
	writel((div << KPROC_CLK_MGR_REG_PLLARMC_PLLARM_MDIV_SHIFT) |
	       KPROC_CLK_MGR_REG_PLLARMC_PLLARM_LOAD_EN_MASK,
	       base + KPROC_CLK_MGR_REG_PLLARMC_OFFSET);
	writel(div, base + KPROC_CLK_MGR_REG_PLLARMC_OFFSET);

}

static inline void __proc_clk_set_pll1_div(void __iomem *base, int div)
{
	writel(div, base + KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET);
	writel((div << KPROC_CLK_MGR_REG_PLLARMC_PLLARM_MDIV_SHIFT) |
	       KPROC_CLK_MGR_REG_PLLARMC_PLLARM_LOAD_EN_MASK,
	       base + KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET);
	writel(div, base + KPROC_CLK_MGR_REG_PLLARMCTRL5_OFFSET);
}

static int ccu_clk_set_voltage(struct ccu_clk *ccu_clk, u32 volt_id, u8 voltage)
{
	u32 shift, reg_val;
	void __iomem *reg_addr;

	if (volt_id >= ccu_clk->freq_count)
		return -EINVAL;
	clk_dbg("%s:%s ccu , volt_id = %d volatge = 0x%x\n", __func__,
		ccu_clk->clk.name, volt_id, voltage);
	ccu_clk->freq_volt[volt_id] = voltage & CCU_VLT_MASK;

	switch (volt_id) {
	case CCU_VLT0:
		shift = CCU_VLT0_SHIFT;
		reg_addr = CCU_VLT0_3_REG(ccu_clk);
		break;
	case CCU_VLT1:
		shift = CCU_VLT1_SHIFT;
		reg_addr = CCU_VLT0_3_REG(ccu_clk);
		break;
	case CCU_VLT2:
		shift = CCU_VLT2_SHIFT;
		reg_addr = CCU_VLT0_3_REG(ccu_clk);
		break;
	case CCU_VLT3:
		shift = CCU_VLT3_SHIFT;
		reg_addr = CCU_VLT0_3_REG(ccu_clk);
		break;
	case CCU_VLT4:
		shift = CCU_VLT4_SHIFT;
		reg_addr = CCU_VLT4_7_REG(ccu_clk);
		break;
	case CCU_VLT5:
		shift = CCU_VLT5_SHIFT;
		reg_addr = CCU_VLT4_7_REG(ccu_clk);
		break;
	case CCU_VLT6:
		shift = CCU_VLT6_SHIFT;
		reg_addr = CCU_VLT4_7_REG(ccu_clk);
		break;
	case CCU_VLT7:
		shift = CCU_VLT7_SHIFT;
		reg_addr = CCU_VLT4_7_REG(ccu_clk);
		break;
	default:
		return -EINVAL;
	}
	reg_val = readl(reg_addr);
	reg_val =
	    (reg_val & ~(CCU_VLT_MASK << shift)) | ((voltage & CCU_VLT_MASK) <<
						    shift);

	clk_dbg("%s:%s ccu , reg_val = 0x%x reg_addr = 0x%x\n", __func__,
		ccu_clk->clk.name, reg_val, (u32)reg_addr);
	writel(reg_val, reg_addr);

	return 0;
}

static int kproc_ccu_set_freq_policy(struct ccu_clk *ccu_clk,
				     int policy_id, int target_freq_id,
				     int target_opp_inx)
{
	u32 reg_val = 0;
	u32 shift;
	u32 a9_voltage[] =
	    DEFINE_ARRAY_ARGS(PROC_VLT_ID_ECO, PROC_VLT_ID_ECO, PROC_VLT_ID_ECO,
			      PROC_VLT_ID_ECO,
			      PROC_VLT_ID_ECO1, PROC_VLT_ID_NORMAL,
			      PROC_VLT_ID_TURBO1, PROC_VLT_ID_TURBO);
	int ret = 0;
#if defined(CONFIG_BCM_HWCAPRI_1605) || defined(CONFIG_BCM_HWCAPRI_1605_A2)
	int saved_pllarma = 0;
	int saved_pll_debug = 0;
#endif

	if (target_freq_id >= ccu_clk->freq_count) {
		ret = -EINVAL;
		goto err_out;
	}

	ccu_write_access_enable(ccu_clk, true);
	clk_dbg("%s:%s ccu , freq_id = %d policy_id = %d, opp_inx= %d\n",
		__func__, ccu_clk->clk.name, target_freq_id, policy_id,
		target_opp_inx);

	switch (policy_id) {
	case CCU_POLICY0:
		shift = CCU_FREQ_POLICY0_SHIFT;
		break;
	case CCU_POLICY1:
		shift = CCU_FREQ_POLICY1_SHIFT;
		break;
	case CCU_POLICY2:
		shift = CCU_FREQ_POLICY2_SHIFT;
		break;
	case CCU_POLICY3:
		shift = CCU_FREQ_POLICY3_SHIFT;
		break;
	default:
		ret = -EINVAL;
		goto err_out;
	}

#ifdef CONFIG_BCM_HWCAPRI_1605
	/* HwCAPRI-1605 div3 workaround for A0/A1 */
	outer_lock_all();
#endif

#if defined(CONFIG_BCM_HWCAPRI_1605) || defined(CONFIG_BCM_HWCAPRI_1605_A2)
	/* HwCAPRI-1605 div3 workaround for A0/A1/A2 */

	/* Change PLL settings to avoid glitches */
	ccu_write_access_enable(&CLK_NAME(kproc), true);

	reg_val = readl(CLK_NAME(kproc).ccu_clk_mgr_base +
			KPROC_CLK_MGR_REG_PLLARMA_OFFSET);
	saved_pllarma = reg_val;
	reg_val &=
	    ~KPROC_CLK_MGR_REG_PLLARMA_PLLARM_IDLE_PWRDWN_SW_OVRRIDE_MASK;
	writel(reg_val, CLK_NAME(kproc).ccu_clk_mgr_base +
	       KPROC_CLK_MGR_REG_PLLARMA_OFFSET);

	reg_val = readl(CLK_NAME(kproc).ccu_clk_mgr_base +
			KPROC_CLK_MGR_REG_PLL_DEBUG_OFFSET);
	saved_pll_debug = reg_val;
	reg_val |= KPROC_CLK_MGR_REG_PLL_DEBUG_PLLARM_TIMER_LOCK_EN_MASK;
	writel(reg_val, CLK_NAME(kproc).ccu_clk_mgr_base +
	       KPROC_CLK_MGR_REG_PLL_DEBUG_OFFSET);

	ccu_write_access_enable(&CLK_NAME(kproc), false);
#endif
#ifdef CONFIG_BCM_HWCAPRI_1766
	if (cur_freq_id < kproc_fid7) {
		switch (target_freq_id) {
		case kproc_fid7:
			if (cur_freq_id < kproc_fid4) {
				/* workaround for Capri AX */
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				reg_val |= kproc_fid4 << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			}

			writel(0x1, KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);

			clk_dbg("%s: FID ==7\n", __func__);

			if (target_opp_inx == PI_PROC_OPP_TURBO) {
				clk_dbg("%s: 1200M\n", __func__);

				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 7 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_clk_set_voltage(ccu_clk, kproc_fid7,
						    a9_voltage[7]);
				__proc_clk_set_pll1_div(ccu_clk->
							ccu_clk_mgr_base, 2);
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			} else if (target_opp_inx == PI_PROC_OPP_TURBO1) {
				clk_dbg("%s: 800M\n", __func__);

				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 6 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_clk_set_voltage(ccu_clk, kproc_fid7,
						    a9_voltage[6]);
				__proc_clk_set_pll1_div(ccu_clk->
							ccu_clk_mgr_base, 3);
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			} else {
				clk_dbg("%s: 600M\n", __func__);
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 7 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_clk_set_voltage(ccu_clk, kproc_fid7,
						    a9_voltage[5]);
				__proc_clk_set_pll1_div(ccu_clk->
							ccu_clk_mgr_base, 4);
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			}
			break;

		case kproc_fid4:

			clk_dbg
			    ("%s: FID == 4 target_freq_id = %d. cur_id = %d\n",
			     __func__, target_freq_id, cur_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid3:
			clk_dbg
			    ("%s: FID == 3 target_freq_id = %d. cur_id = %d\n",
			     __func__, target_freq_id, cur_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		default:
			ret = -EINVAL;
			goto err_unlock;
		}

	} else if (cur_freq_id == kproc_fid7) {
		switch (target_freq_id) {
		case kproc_fid7:

			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= kproc_fid4 << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);

			writel(0x1, KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);

			clk_dbg("%s: FID ==7\n", __func__);

			if (cur_opp_inx != target_opp_inx) {
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 4 write to register */
				reg_val |= kproc_fid4 << shift;
				ccu_policy_engine_stop(ccu_clk);
				if (target_opp_inx == PI_PROC_OPP_NORMAL)
					ccu_clk_set_voltage(ccu_clk, kproc_fid4,
							    a9_voltage[5]);
				else if (target_opp_inx == PI_PROC_OPP_TURBO1)
					ccu_clk_set_voltage(ccu_clk, kproc_fid4,
							    a9_voltage[6]);
				else
					ccu_clk_set_voltage(ccu_clk, kproc_fid4,
							    a9_voltage[7]);

				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 7 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);

				if (target_opp_inx == PI_PROC_OPP_NORMAL) {
					clk_dbg("%s: 600M\n", __func__);
					ccu_clk_set_voltage(ccu_clk, kproc_fid7,
							    a9_voltage[5]);
					__proc_clk_set_pll1_div(ccu_clk->
								ccu_clk_mgr_base,
								4);
				} else if (target_opp_inx == PI_PROC_OPP_TURBO1) {
					clk_dbg("%s: 800M\n", __func__);
					ccu_clk_set_voltage(ccu_clk, kproc_fid7,
							    a9_voltage[6]);
					__proc_clk_set_pll1_div(ccu_clk->
								ccu_clk_mgr_base,
								3);
				} else {
					clk_dbg("%s: 1200M\n", __func__);
					ccu_clk_set_voltage(ccu_clk, kproc_fid7,
							    a9_voltage[7]);
					__proc_clk_set_pll1_div(ccu_clk->
								ccu_clk_mgr_base,
								2);
				}

				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
				ccu_clk_set_voltage(ccu_clk, kproc_fid4,
						    a9_voltage[4]);
			}
			break;

		case kproc_fid4:
			clk_dbg("%s: FID ==4\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			writel(0x0,
			       KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);
			break;

		case kproc_fid3:
/* workaround for Capri AX */
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= kproc_fid4 << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			writel(0x0,
			       KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);
			clk_dbg("%s: FID ==3 target_freq_id = %d\n", __func__,
				target_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		default:
			ret = -EINVAL;
			goto err_unlock;
		}
	} else {
		pr_err("WRONG FIQ_ID %s: cur_freq_id=%d\n", __func__,
		       cur_freq_id);
	}
#else
	if (cur_freq_id < kproc_fid6) {
		switch (target_freq_id) {
		case kproc_fid7:
			if (cur_freq_id < kproc_fid4) {
				/* workaround for Capri AX */
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				reg_val |= kproc_fid4 << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			}

			writel(0x1, KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);

			clk_dbg("%s: FID ==7\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid6:

			if (target_opp_inx == PI_PROC_OPP_TURBO1) {
				clk_dbg("%s: 800M\n", __func__);

				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 6 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_clk_set_voltage(ccu_clk, kproc_fid6,
						    a9_voltage[6]);
				__proc_clk_set_pll0_div(ccu_clk->
							ccu_clk_mgr_base, 3);
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			} else {
				clk_dbg("%s: 600M\n", __func__);
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 6 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_clk_set_voltage(ccu_clk, kproc_fid6,
						    a9_voltage[5]);
				__proc_clk_set_pll0_div(ccu_clk->
							ccu_clk_mgr_base, 4);
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			}
			break;

		case kproc_fid4:

			clk_dbg
			    ("%s: FID == 4 target_freq_id = %d. cur_id = %d\n",
			     __func__, target_freq_id, cur_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid3:
			clk_dbg
			    ("%s: FID == 3 target_freq_id = %d. cur_id = %d\n",
			     __func__, target_freq_id, cur_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		default:
			ret = -EINVAL;
			goto err_unlock;
		}

	} else if (cur_freq_id == kproc_fid6) {
		switch (target_freq_id) {
		case kproc_fid7:

			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= kproc_fid4 << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);

			writel(0x1, KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);

			clk_dbg("%s: FID ==7\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid6:
			if (cur_opp_inx != target_opp_inx) {
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 4 write to register */
				reg_val |= kproc_fid4 << shift;
				ccu_policy_engine_stop(ccu_clk);
				if (target_opp_inx == PI_PROC_OPP_NORMAL)
					ccu_clk_set_voltage(ccu_clk, kproc_fid4,
							    a9_voltage[5]);
				else
					ccu_clk_set_voltage(ccu_clk, kproc_fid4,
							    a9_voltage[6]);

				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				/*freq_id 6 write to register */
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);

				if (target_opp_inx == PI_PROC_OPP_NORMAL) {
					clk_dbg("%s: 600M\n", __func__);
					ccu_clk_set_voltage(ccu_clk, kproc_fid6,
							    a9_voltage[5]);
					__proc_clk_set_pll0_div(ccu_clk->
								ccu_clk_mgr_base,
								4);
				} else {
					clk_dbg("%s: 800M\n", __func__);
					ccu_clk_set_voltage(ccu_clk, kproc_fid6,
							    a9_voltage[6]);
					__proc_clk_set_pll0_div(ccu_clk->
								ccu_clk_mgr_base,
								3);
				}

				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
				ccu_clk_set_voltage(ccu_clk, kproc_fid4,
						    a9_voltage[4]);
			}
			break;

		case kproc_fid4:
			clk_dbg("%s: FID == 4\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid3:
			clk_dbg("%s: FID == 3\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		default:
			ret = -EINVAL;
			goto err_unlock;
		}
	} else if (cur_freq_id == kproc_fid7) {
		switch (target_freq_id) {
		case kproc_fid6:
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= kproc_fid4 << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);

			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			/*freq_id 6 write to register */
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			if (target_opp_inx == PI_PROC_OPP_NORMAL) {
				clk_dbg("%s: 600M\n", __func__);
				ccu_clk_set_voltage(ccu_clk, kproc_fid6,
						    a9_voltage[5]);
				__proc_clk_set_pll0_div(ccu_clk->
							ccu_clk_mgr_base, 4);
			} else {
				clk_dbg("%s: 800M\n", __func__);
				ccu_clk_set_voltage(ccu_clk, kproc_fid6,
						    a9_voltage[6]);
				__proc_clk_set_pll0_div(ccu_clk->
							ccu_clk_mgr_base, 3);
			}
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			writel(0x0,
			       KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);
			break;

		case kproc_fid4:
			clk_dbg("%s: FID ==4\n", __func__);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			writel(0x0,
			       KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);
			break;

		case kproc_fid3:
/* workaround for Capri AX */
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= kproc_fid4 << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			writel(0x0,
			       KONA_PROC_CLK_VA +
			       KPROC_CLK_MGR_REG_PL310_TRIGGER_OFFSET);
			clk_dbg("%s: FID ==3 target_freq_id = %d\n", __func__,
				target_freq_id);
			reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
			reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
			reg_val |= target_freq_id << shift;
			ccu_policy_engine_stop(ccu_clk);
			writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
			ccu_policy_engine_resume(ccu_clk,
						 ccu_clk->clk.
						 flags & CCU_TARGET_LOAD ?
						 CCU_LOAD_TARGET :
						 CCU_LOAD_ACTIVE);
			break;

		case kproc_fid7:
			if (cur_policy_id != 1) {
				/* need to change to from policy 7 to 5 */
				clk_dbg("%s: FID ==7 target_freq_id = %d\n",
					__func__, target_freq_id);
				reg_val = readl(CCU_POLICY_FREQ_REG(ccu_clk));
				reg_val &= ~(CCU_FREQ_POLICY_MASK << shift);
				reg_val |= target_freq_id << shift;
				ccu_policy_engine_stop(ccu_clk);
				writel(reg_val, CCU_POLICY_FREQ_REG(ccu_clk));
				ccu_policy_engine_resume(ccu_clk,
							 ccu_clk->clk.
							 flags & CCU_TARGET_LOAD
							 ? CCU_LOAD_TARGET :
							 CCU_LOAD_ACTIVE);
			}
			break;
		default:
			ret = -EINVAL;
			goto err_unlock;
		}
	} else {
		pr_err("WRONG FIQ_ID %s: cur_freq_id=%d\n", __func__,
		       cur_freq_id);
	}
#endif

	cur_freq_id = target_freq_id;
	cur_policy_id = policy_id;
	cur_opp_inx = target_opp_inx;

err_unlock:
#if defined(CONFIG_BCM_HWCAPRI_1605) || defined(CONFIG_BCM_HWCAPRI_1605_A2)
	/* HwCAPRI-1605 div3 workaround for A0/A1/A2 */

	/* Restore saved PLL settings */
	ccu_write_access_enable(&CLK_NAME(kproc), true);

	writel(saved_pllarma, CLK_NAME(kproc).ccu_clk_mgr_base +
	       KPROC_CLK_MGR_REG_PLLARMA_OFFSET);

	writel(saved_pll_debug, CLK_NAME(kproc).ccu_clk_mgr_base +
	       KPROC_CLK_MGR_REG_PLL_DEBUG_OFFSET);

	ccu_write_access_enable(&CLK_NAME(kproc), false);
#endif

#ifdef CONFIG_BCM_HWCAPRI_1605
	/* HwCAPRI-1605 div3 workaround for A0/A1 */
	outer_unlock_all();
#endif

err_out:
	ccu_write_access_enable(ccu_clk, false);
	return ret;
}

/* table for registering clock */
static struct __init clk_lookup capri_clk_tbl[] = {
	/* All the CCUs are registered first */
	BRCM_REGISTER_CLK(KPROC_CCU_CLK_NAME_STR, NULL, kproc),
	BRCM_REGISTER_CLK(ROOT_CCU_CLK_NAME_STR, NULL, root),
	BRCM_REGISTER_CLK(KHUB_CCU_CLK_NAME_STR, NULL, khub),
	BRCM_REGISTER_CLK(KHUBAON_CCU_CLK_NAME_STR, NULL, khubaon),
	BRCM_REGISTER_CLK(KPM_CCU_CLK_NAME_STR, NULL, kpm),
	BRCM_REGISTER_CLK(KPS_CCU_CLK_NAME_STR, NULL, kps),
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	BRCM_REGISTER_CLK(ESUB_CCU_CLK_NAME_STR, NULL, esub),
#endif
	BRCM_REGISTER_CLK(BMDM_CCU_CLK_NAME_STR, NULL, bmdm),
	BRCM_REGISTER_CLK(DSP_CCU_CLK_NAME_STR, NULL, dsp),
	/* CCU registration end */

	BRCM_REGISTER_CLK(ARM_CORE_CLK_NAME_STR, NULL, a9_core),
	BRCM_REGISTER_CLK(ARM_SWITCH_CLK_NAME_STR, NULL, arm_switch),
	BRCM_REGISTER_CLK(APB0_CLK_NAME_STR, NULL, apb0),
	BRCM_REGISTER_CLK(A9_PLL_CLK_NAME_STR, NULL, a9_pll),
	BRCM_REGISTER_CLK(A9_PLL_CHNL0_CLK_NAME_STR, NULL, a9_pll_chnl0),
	BRCM_REGISTER_CLK(A9_PLL_CHNL1_CLK_NAME_STR, NULL, a9_pll_chnl1),

	BRCM_REGISTER_CLK(REF_8PHASE_EN_PLL1_CLK_NAME_STR,
			  NULL, 8phase_en_pll1),
	BRCM_REGISTER_CLK(FRAC_1M_REF_CLK_NAME_STR, NULL, frac_1m),
	BRCM_REGISTER_CLK(REF_96M_VARVDD_REF_CLK_NAME_STR, NULL,
			  ref_96m_varvdd),
	BRCM_REGISTER_CLK(REF_96M_REF_CLK_NAME_STR, NULL, ref_96m),
	BRCM_REGISTER_CLK(VAR_96M_REF_CLK_NAME_STR, NULL, var_96m),
	BRCM_REGISTER_CLK(VAR_500M_VARVDD_REF_CLK_NAME_STR, NULL,
			  var_500m_varvdd),
	BRCM_REGISTER_CLK(REF_208M_REF_CLK_NAME_STR, NULL, ref_208m),
	BRCM_REGISTER_CLK(REF_156M_REF_CLK_NAME_STR, NULL, ref_156m),
	BRCM_REGISTER_CLK(REF_104M_REF_CLK_NAME_STR, NULL, ref_104m),
	BRCM_REGISTER_CLK(REF_52M_REF_CLK_NAME_STR, NULL, ref_52m),
	BRCM_REGISTER_CLK(REF_13M_REF_CLK_NAME_STR, NULL, ref_13m),
	BRCM_REGISTER_CLK(REF_26M_REF_CLK_NAME_STR, NULL, ref_26m),
	BRCM_REGISTER_CLK(VAR_312M_REF_CLK_NAME_STR, NULL, var_312m),
	BRCM_REGISTER_CLK(VAR_208M_REF_CLK_NAME_STR, NULL, var_208m),
	BRCM_REGISTER_CLK(VAR_156M_REF_CLK_NAME_STR, NULL, var_156m),
	BRCM_REGISTER_CLK(VAR_104M_REF_CLK_NAME_STR, NULL, var_104m),
	BRCM_REGISTER_CLK(VAR_52M_REF_CLK_NAME_STR, NULL, var_52m),
	BRCM_REGISTER_CLK(VAR_13M_REF_CLK_NAME_STR, NULL, var_13m),
	BRCM_REGISTER_CLK(DIG_CH0_PERI_CLK_NAME_STR, NULL, dig_ch0),
	BRCM_REGISTER_CLK(DIG_CH1_PERI_CLK_NAME_STR, NULL, dig_ch1),
	BRCM_REGISTER_CLK(DIG_CH2_PERI_CLK_NAME_STR, NULL, dig_ch2),
	BRCM_REGISTER_CLK(DIG_CH3_PERI_CLK_NAME_STR, NULL, dig_ch3),
	/*ref_312m clock should be the last clock to be auto gated in root CCU */
	BRCM_REGISTER_CLK(REF_312M_REF_CLK_NAME_STR, NULL, ref_312m),
	 /*AON*/ BRCM_REGISTER_CLK(DFT_19_5M_REF_CLK_NAME_STR, NULL, dft_19_5m),
	BRCM_REGISTER_CLK(REF_CX40_VARVDD_REF_CLK_NAME_STR, NULL,
			  ref_cx40_varvdd),
	BRCM_REGISTER_CLK(REF_CX40_REF_CLK_NAME_STR, NULL, ref_cx40),
	BRCM_REGISTER_CLK(REF_1M_REF_CLK_NAME_STR, NULL, ref_1m),
	BRCM_REGISTER_CLK(REF_32K_REF_CLK_NAME_STR, NULL, ref_32k),
	BRCM_REGISTER_CLK_DIRECT(TPIU_PERI_CLK_NAME_STR, NULL, &clk_tpiu),
	BRCM_REGISTER_CLK_DIRECT(PTI_PERI_CLK_NAME_STR, NULL, &clk_pti),
	BRCM_REGISTER_CLK(PMU_BSC_VAR_REF_CLK_NAME_STR, NULL, pmu_bsc_var),
	BRCM_REGISTER_CLK(BBL_32K_REF_CLK_NAME_STR, NULL, bbl_32k),
	BRCM_REGISTER_CLK(HUB_TIMER_APB_BUS_CLK_NAME_STR, NULL, hub_timer_apb),
	BRCM_REGISTER_CLK(ACI_APB_BUS_CLK_NAME_STR, NULL, aci_apb),
	BRCM_REGISTER_CLK(SIM_APB_BUS_CLK_NAME_STR, NULL, sim_apb),
	BRCM_REGISTER_CLK(SIM2_APB_BUS_CLK_NAME_STR, NULL, sim2_apb),
	BRCM_REGISTER_CLK(PWRMGR_AXI_BUS_CLK_NAME_STR, NULL, pwrmgr_axi),
	BRCM_REGISTER_CLK(APB6_BUS_CLK_NAME_STR, NULL, apb6),
	BRCM_REGISTER_CLK(GPIOKP_APB_BUS_CLK_NAME_STR, NULL, gpiokp_apb),
	BRCM_REGISTER_CLK(PMU_BSC_APB_BUS_CLK_NAME_STR, NULL, pmu_bsc_apb),
	/* 3500d000.i2c */
	BRCM_REGISTER_CLK("bus_clk", "bsc-i2c.2", pmu_bsc_apb),
	BRCM_REGISTER_CLK(CHIPREG_APB_BUS_CLK_NAME_STR, NULL, chipreg_apb),
	BRCM_REGISTER_CLK(FMON_APB_BUS_CLK_NAME_STR, NULL, fmon_apb),
	BRCM_REGISTER_CLK(HUB_TZCFG_APB_BUS_CLK_NAME_STR, NULL, hub_tzcfg_apb),
	BRCM_REGISTER_CLK(SEC_WD_APB_BUS_CLK_NAME_STR, NULL, sec_wd_apb),
	BRCM_REGISTER_CLK(SYSEMI_SEC_APB_BUS_CLK_NAME_STR,
			  NULL, sysemi_sec_apb),
	BRCM_REGISTER_CLK(SYSEMI_OPEN_APB_BUS_CLK_NAME_STR,
			  NULL, sysemi_open_apb),
	BRCM_REGISTER_CLK(VCEMI_SEC_APB_BUS_CLK_NAME_STR, NULL, vcemi_sec_apb),
	BRCM_REGISTER_CLK(VCEMI_OPEN_APB_BUS_CLK_NAME_STR,
			  NULL, vcemi_open_apb),
	BRCM_REGISTER_CLK(SPM_APB_BUS_CLK_NAME_STR, NULL, spm_apb),
	BRCM_REGISTER_CLK(DAP_BUS_CLK_NAME_STR, NULL, dap),
	BRCM_REGISTER_CLK(SIM_PERI_CLK_NAME_STR, NULL, sim),
	BRCM_REGISTER_CLK(SIM2_PERI_CLK_NAME_STR, NULL, sim2),
	BRCM_REGISTER_CLK(PSCS_PERI_CLK_NAME_STR, NULL, pscs),
	BRCM_REGISTER_CLK(HUB_TIMER_PERI_CLK_NAME_STR, NULL, hub_timer),
	BRCM_REGISTER_CLK(PMU_BSC_PERI_CLK_NAME_STR, NULL, pmu_bsc),
	BRCM_REGISTER_CLK("peri_clk", "bsc-i2c.2", pmu_bsc),	/* 3500d000.i2c */
	/*hubaon clk should be the last clock to be auto gated in AON CCU */
	BRCM_REGISTER_CLK(HUBAON_BUS_CLK_NAME_STR, NULL, hubaon),

	BRCM_REGISTER_CLK(NOR_APB_BUS_CLK_NAME_STR, NULL, nor_apb),
	BRCM_REGISTER_CLK(TMON_APB_BUS_CLK_NAME_STR, NULL, tmon_apb),
	BRCM_REGISTER_CLK(APB5_BUS_CLK_NAME_STR, NULL, apb5),
	BRCM_REGISTER_CLK(CTI_APB_BUS_CLK_NAME_STR, NULL, cti_apb),
	BRCM_REGISTER_CLK(FUNNEL_APB_BUS_CLK_NAME_STR, NULL, funnel_apb),
	BRCM_REGISTER_CLK(IPC_APB_BUS_CLK_NAME_STR, NULL, ipc_apb),
	BRCM_REGISTER_CLK(SRAM_MPU_APB_BUS_CLK_NAME_STR,
			  NULL, sram_mpu_apb),
	BRCM_REGISTER_CLK(TPIU_APB_BUS_CLK_NAME_STR, NULL, tpiu_apb),
	BRCM_REGISTER_CLK(VC_ITM_APB_BUS_CLK_NAME_STR, NULL, vc_itm_apb),
	BRCM_REGISTER_CLK(SEC_VIOL_TRAP7_APB_BUS_CLK_NAME_STR,
			  NULL, sec_viol_trap7_apb),
	BRCM_REGISTER_CLK(SEC_VIOL_TRAP6_APB_BUS_CLK_NAME_STR,
			  NULL, sec_viol_trap6_apb),
	BRCM_REGISTER_CLK(SEC_VIOL_TRAP5_APB_BUS_CLK_NAME_STR,
			  NULL, sec_viol_trap5_apb),
	BRCM_REGISTER_CLK(SEC_VIOL_TRAP4_APB_BUS_CLK_NAME_STR,
			  NULL, sec_viol_trap4_apb),
	BRCM_REGISTER_CLK(AXI_TRACE19_APB_BUS_CLK_NAME_STR,
			  NULL, axi_trace19_apb),
	BRCM_REGISTER_CLK(AXI_TRACE11_APB_BUS_CLK_NAME_STR,
			  NULL, axi_trace11_apb),
	BRCM_REGISTER_CLK(AXI_TRACE12_APB_BUS_CLK_NAME_STR,
			  NULL, axi_trace12_apb),
	BRCM_REGISTER_CLK(AXI_TRACE13_APB_BUS_CLK_NAME_STR,
			  NULL, axi_trace13_apb),
	BRCM_REGISTER_CLK(HSI_APB_BUS_CLK_NAME_STR, NULL, hsi_apb),
	BRCM_REGISTER_CLK(ETB_APB_BUS_CLK_NAME_STR, NULL, etb_apb),
	BRCM_REGISTER_CLK(FINAL_FUNNEL_APB_BUS_CLK_NAME_STR, NULL,
			  final_funnel_apb),
	BRCM_REGISTER_CLK(APB10_BUS_CLK_NAME_STR, NULL, apb10),
	BRCM_REGISTER_CLK(APB9_BUS_CLK_NAME_STR, NULL, apb9),
	BRCM_REGISTER_CLK(ATB_FILTER_APB_BUS_CLK_NAME_STR, NULL,
			  atb_filter_apb),
	BRCM_REGISTER_CLK(AUDIOH_26M_PERI_CLK_NAME_STR, NULL, audioh_26m),
	BRCM_REGISTER_CLK(ETB2AXI_APB_BUS_CLK_NAME_STR, NULL, etb2axi_apb),
	BRCM_REGISTER_CLK(AUDIOH_APB_BUS_CLK_NAME_STR, NULL, audioh_apb),
	BRCM_REGISTER_CLK(SSP3_APB_BUS_CLK_NAME_STR, NULL, ssp3_apb),
	BRCM_REGISTER_CLK(SSP4_APB_BUS_CLK_NAME_STR, NULL, ssp4_apb),
	BRCM_REGISTER_CLK(SSP5_APB_BUS_CLK_NAME_STR, NULL, ssp5_apb),
	BRCM_REGISTER_CLK(SSP6_APB_BUS_CLK_NAME_STR, NULL, ssp6_apb),
	BRCM_REGISTER_CLK(VAR_SPM_APB_BUS_CLK_NAME_STR, NULL, var_spm_apb),
	BRCM_REGISTER_CLK(NOR_BUS_CLK_NAME_STR, NULL, nor),
	BRCM_REGISTER_CLK(NOR_ASYNC_PERI_CLK_NAME_STR, NULL, nor_async),
	BRCM_REGISTER_CLK(AUDIOH_2P4M_PERI_CLK_NAME_STR, NULL, audioh_2p4m),
	BRCM_REGISTER_CLK(AUDIOH_156M_PERI_CLK_NAME_STR, NULL, audioh_156m),
	BRCM_REGISTER_CLK(SSP3_AUDIO_PERI_CLK_NAME_STR, NULL, ssp3_audio),
	BRCM_REGISTER_CLK(SSP3_PERI_CLK_NAME_STR, NULL, ssp3),
	BRCM_REGISTER_CLK(SSP4_AUDIO_PERI_CLK_NAME_STR, NULL, ssp4_audio),
	BRCM_REGISTER_CLK(SSP4_PERI_CLK_NAME_STR, NULL, ssp4),
	BRCM_REGISTER_CLK(SSP5_AUDIO_PERI_CLK_NAME_STR, NULL, ssp5_audio),
	BRCM_REGISTER_CLK(SSP5_PERI_CLK_NAME_STR, NULL, ssp5),
	BRCM_REGISTER_CLK(SSP6_AUDIO_PERI_CLK_NAME_STR, NULL, ssp6_audio),
	BRCM_REGISTER_CLK(SSP6_PERI_CLK_NAME_STR, NULL, ssp6),
	BRCM_REGISTER_CLK(TMON_1M_PERI_CLK_NAME_STR, NULL, tmon_1m),
	BRCM_REGISTER_CLK(CAPH_SRCMIXER_PERI_CLK_NAME_STR, NULL, caph_srcmixer),
	BRCM_REGISTER_CLK(DAP_SWITCH_PERI_CLK_NAME_STR, NULL, dap_switch),
	BRCM_REGISTER_CLK(BROM_PERI_CLK_NAME_STR, NULL, brom),
	BRCM_REGISTER_CLK(MDIOMASTER_PERI_CLK_NAME_STR, NULL, mdiomaster),
	BRCM_REGISTER_CLK(HUB_SWITCH_APB_BUS_CLK_NAME_STR,
			  NULL, hub_switch_apb),
	BRCM_REGISTER_CLK(HUB_PERI_CLK_NAME_STR, NULL, hub_clk),

	BRCM_REGISTER_CLK(USB_OTG_AHB_BUS_CLK_NAME_STR, NULL, usb_otg_ahb),
	BRCM_REGISTER_CLK("bus_clk", "sdhci.1", sdio2_ahb),	/* 3f190000.sdhci */
	BRCM_REGISTER_CLK("bus_clk", "sdhci.2", sdio3_ahb),	/* 3f1a0000.sdhci */
	BRCM_REGISTER_CLK("bus_clk", "sdhci.3", sdio4_ahb),	/* 3f1b0000.sdhci */
	BRCM_REGISTER_CLK(MASTER_SWITCH_AHB_BUS_CLK_NAME_STR, NULL,
			  master_switch_ahb),
	BRCM_REGISTER_CLK(MASTER_SWITCH_AXI_BUS_CLK_NAME_STR, NULL,
			  master_switch_axi),
	BRCM_REGISTER_CLK(ARMCORE_AXI_BUS_CLK_NAME_STR,
			  NULL, armcore_axi),
	BRCM_REGISTER_CLK(APB4_BUS_CLK_NAME_STR,
			  NULL, apb4),
	BRCM_REGISTER_CLK(APB8_BUS_CLK_NAME_STR,
			  NULL, apb8),
	BRCM_REGISTER_CLK(DMA_AXI_BUS_CLK_NAME_STR,
			  NULL, dma_axi),
	BRCM_REGISTER_CLK(SPI_AHB_BUS_CLK_NAME_STR, NULL, spi_ahb),
	BRCM_REGISTER_CLK(USBH_AHB_BUS_CLK_NAME_STR, NULL, usbh_ahb),
	BRCM_REGISTER_CLK(USBHSIC_AHB_BUS_CLK_NAME_STR, NULL, usbhsic_ahb),
	BRCM_REGISTER_CLK(USB_IC_AHB_BUS_CLK_NAME_STR, NULL, usb_ic_ahb),
	BRCM_REGISTER_CLK(NAND_AHB_BUS_CLK_NAME_STR, NULL, nand_ahb),
	BRCM_REGISTER_CLK("bus_clk", "sdhci.0", sdio1_ahb),	/* 3f180000.sdhci */
	BRCM_REGISTER_CLK("peri_clk", "sdhci.1", sdio2),	/* 3f190000.sdhci */
	BRCM_REGISTER_CLK("sleep_clk", "sdhci.1", sdio2_sleep),	/* 3f190000.sdhci */
	BRCM_REGISTER_CLK("peri_clk", "sdhci.2", sdio3),	/* 3f1a0000.sdhci */
	BRCM_REGISTER_CLK("sleep_clk", "sdhci.2", sdio3_sleep),	/* 3f1a0000.sdhci */
	BRCM_REGISTER_CLK(NAND_PERI_CLK_NAME_STR, NULL, nand),
	BRCM_REGISTER_CLK("peri_clk", "sdhci.0", sdio1),	/* 3f180000.sdhci */
	BRCM_REGISTER_CLK("sleep_clk", "sdhci.0", sdio1_sleep),	/* 3f180000.sdhci */
	BRCM_REGISTER_CLK("peri_clk", "sdhci.3", sdio4),	/* 3f1b0000.sdhci */
	BRCM_REGISTER_CLK("sleep_clk", "sdhci.3", sdio4_sleep),	/* 3f1b0000.sdhci */
	BRCM_REGISTER_CLK(USB_IC_PERI_CLK_NAME_STR, NULL, usb_ic),
	BRCM_REGISTER_CLK(USBH_48M_PERI_CLK_NAME_STR, NULL, usbh_48m),
	BRCM_REGISTER_CLK(USBH_12M_PERI_CLK_NAME_STR, NULL, usbh_12m),
	/*sys_switch_axi clk should be the last
	   clock to be auto gated in KPM CCU */
	BRCM_REGISTER_CLK(SYS_SWITCH_AXI_BUS_CLK_NAME_STR,
			  NULL, sys_switch_axi),

	BRCM_REGISTER_CLK(UARTB_APB_BUS_CLK_NAME_STR, NULL, uartb_apb),
	BRCM_REGISTER_CLK(UARTB2_APB_BUS_CLK_NAME_STR, NULL, uartb2_apb),
	BRCM_REGISTER_CLK(UARTB3_APB_BUS_CLK_NAME_STR, NULL, uartb3_apb),
	BRCM_REGISTER_CLK(UARTB4_APB_BUS_CLK_NAME_STR, NULL, uartb4_apb),
	BRCM_REGISTER_CLK(DMAC_MUX_APB_BUS_CLK_NAME_STR, NULL, dmac_mux_apb),
	BRCM_REGISTER_CLK("bus_clk", "bsc-i2c.0", bsc1_apb),	/* 3e016000.i2c */
	BRCM_REGISTER_CLK("bus_clk", "bsc-i2c.1", bsc2_apb),	/* 3e017000.i2c */
	BRCM_REGISTER_CLK("bus_clk", "bsc-i2c.3", bsc3_apb),	/* 3e018000.i2c */
	BRCM_REGISTER_CLK(PWM_APB_BUS_CLK_NAME_STR, NULL, pwm_apb),
	BRCM_REGISTER_CLK(SSP0_APB_BUS_CLK_NAME_STR, NULL, ssp0_apb),
	BRCM_REGISTER_CLK(KPS_DAP_BUS_CLK_NAME_STR, NULL, kps_dap),
	BRCM_REGISTER_CLK(EXT_AXI_BUS_CLK_NAME_STR, NULL, ext_axi),
	BRCM_REGISTER_CLK(SSP2_APB_BUS_CLK_NAME_STR, NULL, ssp2_apb),
	BRCM_REGISTER_CLK(SPUM_OPEN_APB_BUS_CLK_NAME_STR, NULL, spum_open_apb),
	BRCM_REGISTER_CLK(SPUM_SEC_APB_BUS_CLK_NAME_STR, NULL, spum_sec_apb),
	BRCM_REGISTER_CLK(MPHI_AHB_BUS_CLK_NAME_STR, NULL, mphi_ahb),
	BRCM_REGISTER_CLK(APB1_BUS_CLK_NAME_STR, NULL, apb1),
	BRCM_REGISTER_CLK(APB7_BUS_CLK_NAME_STR, NULL, apb7),
	BRCM_REGISTER_CLK(TIMERS_APB_BUS_CLK_NAME_STR, NULL, timers_apb),
	BRCM_REGISTER_CLK(APB2_BUS_CLK_NAME_STR, NULL, apb2),
	BRCM_REGISTER_CLK(SPUM_OPEN_AXI_BUS_CLK_NAME_STR, NULL, spum_open_axi),
	BRCM_REGISTER_CLK(SPUM_SEC_AXI_BUS_CLK_NAME_STR, NULL, spum_sec_axi),
	BRCM_REGISTER_CLK(UARTB_PERI_CLK_NAME_STR, NULL, uartb),
	BRCM_REGISTER_CLK(UARTB2_PERI_CLK_NAME_STR, NULL, uartb2),
	BRCM_REGISTER_CLK(UARTB3_PERI_CLK_NAME_STR, NULL, uartb3),
	BRCM_REGISTER_CLK(UARTB4_PERI_CLK_NAME_STR, NULL, uartb4),
	BRCM_REGISTER_CLK(SSP0_AUDIO_PERI_CLK_NAME_STR, NULL, ssp0_audio),
	BRCM_REGISTER_CLK(SSP2_AUDIO_PERI_CLK_NAME_STR, NULL, ssp2_audio),
	BRCM_REGISTER_CLK("peri_clk", "bsc-i2c.0", bsc1),	/* 3e016000.i2c */
	BRCM_REGISTER_CLK("peri_clk", "bsc-i2c.1", bsc2),	/* 3e017000.i2c */
	BRCM_REGISTER_CLK("peri_clk", "bsc-i2c.3", bsc3),	/* 3e018000.i2c */
	BRCM_REGISTER_CLK(PWM_PERI_CLK_NAME_STR, NULL, pwm),
	BRCM_REGISTER_CLK(SSP0_PERI_CLK_NAME_STR, NULL, ssp0),
	BRCM_REGISTER_CLK(SSP2_PERI_CLK_NAME_STR, NULL, ssp2),
	BRCM_REGISTER_CLK(TIMERS_PERI_CLK_NAME_STR, NULL, timers),
	BRCM_REGISTER_CLK(SPUM_OPEN_PERI_CLK_NAME_STR, NULL, spum_open),
	BRCM_REGISTER_CLK(SPUM_SEC_PERI_CLK_NAME_STR, NULL, spum_sec),
	/*switch_axi clk should be the last
	   clock to be auto gated in KPS CCU */
	BRCM_REGISTER_CLK(SWITCH_AXI_BUS_CLK_NAME_STR,
			  NULL, switch_axi),
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	BRCM_REGISTER_CLK(ESUB_PLL_CLK_NAME_STR, NULL, esub_pll),
	BRCM_REGISTER_CLK(ESW_SYS_CH0_CLK_NAME_STR, NULL, esw_sys_ch0),
	BRCM_REGISTER_CLK(ESW_SYS_125M_CH1_CLK_NAME_STR, NULL,
			  esw_sys_125m_ch1),
	BRCM_REGISTER_CLK(REF_ESW_GPIO_125M_CLK_NAME_STR, NULL, esw_gpio_125m),
	BRCM_REGISTER_CLK(ESUB_AXI_BUS_CLK_NAME_STR, NULL, esub_axi),
	BRCM_REGISTER_CLK(ESUB_APB_BUS_CLK_NAME_STR, NULL, esub_apb),
	BRCM_REGISTER_CLK(ESW_SYS_PERI_CLK_NAME_STR, NULL, esw_sys),
	BRCM_REGISTER_CLK(ESW_25M_PERI_CLK_NAME_STR, NULL, esw_25m),
	BRCM_REGISTER_CLK(ESW_125M_PERI_CLK_NAME_STR, NULL, esw_125m),
#endif
};

#ifdef CONFIG_BCM_HWCAPRI_1508
#if 0
static int remove_dep_clks(void)
{
	int clk_inx, inx;

	struct clk *clks_arr[] = {
		&clk_sdio1.clk,
		&clk_sdio2.clk,
		&clk_sdio3.clk,
		&clk_sdio4.clk,
		&clk_tpiu,
		&clk_pti
	};
	for (clk_inx = 0; clk_inx < ARRAY_SIZE(clks_arr); clk_inx++) {
		for (inx = 0; inx < MAX_DEP_CLKS
		     && clks_arr[clk_inx]->dep_clks[inx]; inx++) ;
		clk_dbg("clk: %s index: %d\n", clks_arr[clk_inx]->name, inx);
		if (inx > 0)
			clks_arr[clk_inx]->dep_clks[inx - 1] = NULL;
	}
	return 0;
}
#endif
#endif

int __init capri_clock_init(void)
{
	void __iomem *base, *hub_base;

/*only clk mgr write_access and reset mgr access functions
is needed for root ccu*/
	root_ccu_ops.write_access = gen_ccu_ops.write_access;
	root_ccu_ops.rst_write_access = gen_ccu_ops.rst_write_access;
	cur_freq_id = 0;
	cur_opp_inx = 0;
	cur_policy_id = 0;

	kproc_ccu_ops = gen_ccu_ops;
	kproc_ccu_ops.set_freq_policy = kproc_ccu_set_freq_policy;

	enable_special_autogating = 0;

	dig_ch_peri_clk_ops = gen_peri_clk_ops;
	dig_ch_peri_clk_ops.init = dig_clk_init;
	en_8ph_pll1_ref_clk_ops = gen_ref_clk_ops;
	en_8ph_pll1_ref_clk_ops.init = en_8ph_pll1_clk_init;
	en_8ph_pll1_ref_clk_ops.enable = en_8ph_pll1_clk_enable;

/* Remove dependency on 8ph_en clock */
#ifdef CONFIG_BCM_HWCAPRI_1508
/*		remove_dep_clks();  */
#endif

	if (clk_register(capri_clk_tbl, ARRAY_SIZE(capri_clk_tbl)))
		pr_err("%s clk_register failed !!!!\n", __func__);

     /*********************  TEMPORARY *************************************
     * Work arounds for clock module . this could be because of ASIC
     * errata or other limitations or special requirements.
     * -- To be revised based on future fixes.
     *********************************************************************/
	/*clock_module_temp_fixes(); */
	base = HW_IO_PHYS_TO_VIRT(ROOT_CLK_BASE_ADDR);
	hub_base = HW_IO_PHYS_TO_VIRT(HUB_CLK_BASE_ADDR);
	writel(0xA5A501, base);
	writel(0x1, base + ROOT_CLK_MGR_REG_VAR8PH_DIVMODE_OFFSET);
	writel(0xA5A500, base);

	writel(0xA5A501, hub_base);
	writel(0x0, hub_base + KHUB_CLK_MGR_REG_TPIU_CLKGATE_OFFSET);
	writel(0xA5A500, hub_base);
	writel(0x600, KONA_CHIPREG_VA + CHIPREG_ARM_PERI_CONTROL_OFFSET);

	return 0;
}

#ifndef CONFIG_KONA_POWER_MGR
early_initcall(capri_clock_init);
#endif

/**
 * log active clocks during suspend
 */

int capri_clock_print_act_clks(void)
{
	int i;

	pr_info("\n*** ACTIVE CLKS DURING SUSPEND ***\n");
	pr_info("\tCLK \t\t USE_COUNT\n");

	for (i = 0; i < ARRAY_SIZE(ccu_clks); i++)
		ccu_print_sleep_prevent_clks(clk_get(NULL, ccu_clks[i]));

	pr_info("**********************************\n");
	return 0;
}

EXPORT_SYMBOL(capri_clock_print_act_clks);

static int misc_clk_status(struct clk *clk)
{
	u32 reg_val = 0;
	int status;
	u32 reg_offset, reg_mask, reg_shift;
	void __iomem *reg_base;

	BUG_ON(clk->clk_type != CLK_TYPE_MISC);

	clk_dbg("%s, clock: %s\n", __func__, clk->name);

	switch (clk->id) {
	case CLK_TPIU_PERI_CLK_ID:
		reg_base = KONA_CHIPREG_VA;
		reg_offset = CHIPREG_ARM_PERI_CONTROL_OFFSET;
		reg_mask = CHIPREG_ARM_PERI_CONTROL_TPIU_CLK_IS_IDLE_MASK;
		reg_shift = CHIPREG_ARM_PERI_CONTROL_TPIU_CLK_IS_IDLE_SHIFT;
		break;
	case CLK_PTI_PERI_CLK_ID:
		reg_base = KONA_CHIPREG_VA;
		reg_offset = CHIPREG_ARM_PERI_CONTROL_OFFSET;
		reg_mask = CHIPREG_ARM_PERI_CONTROL_PTI_CLK_IS_IDLE_MASK;
		reg_shift = CHIPREG_ARM_PERI_CONTROL_PTI_CLK_IS_IDLE_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	reg_val = readl(reg_base + reg_offset);
	status = (reg_val & reg_mask) >> reg_shift;
	if (clk->flags & INVERT_ENABLE) {
		if (status)
			status = 0;
		else
			status = 1;
	}
	return status;
}

static int clk_debug_misc_status(void *data, u64 *val)
{
	struct clk *clock = data;

	*val = misc_clk_status(clock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(misc_clock_status_fops, clk_debug_misc_status,
			NULL, "%llu\n");

static int clk_debug_misc_enable(void *data, u64 val)
{
	struct clk *clock = data;
	if (val == 1)
		clk_enable(clock);
	else if (val == 0)
		clk_disable(clock);
	else
		clk_dbg("Invalid value\n");

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(misc_clock_enable_fops, NULL, clk_debug_misc_enable,
			"%llu\n");

int clk_switch_src_pll0_pll1(int pll1_disable)
{
	u32 reg_val;
	u32 orig_reg_val;
	u32 orig_reg_val1;
	u32 orig_reg_val2;
	int insurance;
	unsigned long flags;

	static u32 pll1_disabled;
	static u32 orig_var_48m_div_clk_src;
	static u32 orig_var_208m_div_clk_src;
	static u32 orig_var_312m_div_clk_src;
	static u32 orig_dig_pre_div_clk_src;
	static u32 orig_var_tpiu_div_clk_src;

	/* Lock the CCU for exclusive access */
	clk_lock(&CLK_NAME(root).clk, &flags);
	ccu_write_access_enable(&CLK_NAME(root), true);
	CCU_ACCESS_EN(&CLK_NAME(root), 1);

	if (pll1_disable && !pll1_disabled) {
		/* Switch VAR_96M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_48M_DIV_OFFSET);
		orig_var_48m_div_clk_src = reg_val &
		    ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_MASK;
		if (orig_var_48m_div_clk_src ==
		    ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_CMD_PLL1_96M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_CMD_PLL0_96M_CLK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_48M_DIV_OFFSET);
		}

		/* Switch VAR_208M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_208M_DIV_OFFSET);
		orig_var_208m_div_clk_src = reg_val &
		    ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_MASK;
		if (orig_var_208m_div_clk_src ==
		    ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_CMD_PLL1_208M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_CMD_PLL0_208M_CLK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_208M_DIV_OFFSET);
		}

		/* Switch VAR_312M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_312M_DIV_OFFSET);
		orig_var_312m_div_clk_src = reg_val &
		    ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_MASK;
		if (orig_var_312m_div_clk_src ==
		    ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_CMD_PLL1_312M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_CMD_PLL0_312M_CLK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_312M_DIV_OFFSET);
		}

		/* Trigger VAR_96M/208M/312M clock source switch */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET);
		reg_val |= (ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_96M_TRIGGER_MASK
			    |
			    ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_208M_TRIGGER_MASK
			    |
			    ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_312M_TRIGGER_MASK);
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET);

		/* Run the clocks run briefly for the switch to happen */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);
		orig_reg_val = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_HW_SW_GATING_SEL_MASK;
		reg_val |= ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_CLK_EN_MASK;
		writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);

		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);
		orig_reg_val1 = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_HW_SW_GATING_SEL_MASK;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_CLK_EN_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);

		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);
		orig_reg_val2 = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_HW_SW_GATING_SEL_MASK;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_CLK_EN_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);

		/* Wait until the trigger bits are cleared */
		insurance = 10000;
		while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
			      ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET) &
			(ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_96M_TRIGGER_MASK |
			 ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_208M_TRIGGER_MASK |
			 ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_312M_TRIGGER_MASK))
		       && insurance) {
			udelay(1);
			insurance--;
		}
		BUG_ON(insurance == 0);

		/* Restore the clocks to their original settings */
		writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);
		writel(orig_reg_val1, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);
		writel(orig_reg_val2, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);

		/* Switch VAR_TPIU clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_TPIU_DIV_OFFSET);
		orig_var_tpiu_div_clk_src = reg_val &
		    ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_MASK;
		if (orig_var_tpiu_div_clk_src ==
		    ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_CMD_PLL1_312M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_CMD_VAR_TPIU_CLK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_TPIU_DIV_OFFSET);

			/* Trigger the switchover */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET);
			reg_val |=
			    ROOT_CLK_MGR_REG_TPIU_TRG_VAR_TPIU_VARVDD_TRIGGER_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET);

			/* Run the clock run briefly for the switch to happen */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);
			orig_reg_val = reg_val;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_HW_SW_GATING_SEL_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_CLK_EN_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);

			/* Wait until the trigger bits are cleared */
			insurance = 10000;
			while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
				      ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET) &
				ROOT_CLK_MGR_REG_TPIU_TRG_VAR_TPIU_VARVDD_TRIGGER_MASK)
			       && insurance) {
				udelay(1);
				insurance--;
			}
			BUG_ON(insurance == 0);

			/* Restore the clock to its original settings */
			writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);
		}

		/* Switch DIG_PRE clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET);
		orig_dig_pre_div_clk_src = reg_val &
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK;
		if (orig_dig_pre_div_clk_src ==
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_CMD_PLL1_312M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_CMD_PLL0_312M_CLK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_PRE_DIV_OFFSET);

			/* Trigger the switchover */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_DIG_TRG_OFFSET);
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_TRG_OFFSET);

			/* Run the clock run briefly for the switch to happen */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);
			orig_reg_val = reg_val;
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_HW_SW_GATING_SEL_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_CLK_EN_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);

			/* Wait until the trigger bits are cleared */
			insurance = 10000;
			while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
				      ROOT_CLK_MGR_REG_DIG_TRG_OFFSET) &
				ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK)
			       && insurance) {
				udelay(1);
				insurance--;
			}
			BUG_ON(insurance == 0);

			/* Restore the clock to its original settings */
			writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);
		}

		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_PLL1A_OFFSET);
#ifdef CONFIG_BCM_HWCAPRI_1508
		/* Enable PLL1 software override */
		reg_val |= ROOT_CLK_MGR_REG_PLL1A_PLL1_SW_OVRRIDE_EN_MASK;
#else
		/* Put PLL1 into low power mode */
		reg_val |= ROOT_CLK_MGR_REG_PLL1A_PLL1_PWRDWN_MASK;
#endif
		writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_PLL1A_OFFSET);

		pll1_disabled = 1;
	} else if (!pll1_disable && pll1_disabled) {
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_PLL1A_OFFSET);
#ifdef CONFIG_BCM_HWCAPRI_1508
		/* Disable PLL1 software override */
		reg_val &= ~ROOT_CLK_MGR_REG_PLL1A_PLL1_SW_OVRRIDE_EN_MASK;
#else
		/* Put PLL1 into active mode */
		reg_val &= ~ROOT_CLK_MGR_REG_PLL1A_PLL1_PWRDWN_MASK;
#endif
		writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_PLL1A_OFFSET);

		/* Restore VAR_96M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_48M_DIV_OFFSET);
		if ((reg_val &
		     ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_MASK) ==
		    ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_CMD_PLL0_96M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_48M_DIV_VAR_96M_PLL_SELECT_MASK;
			reg_val |= orig_var_48m_div_clk_src;
			writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_48M_DIV_OFFSET);
		}

		/* Restore VAR_208M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_208M_DIV_OFFSET);
		if ((reg_val &
		     ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_MASK) ==
		    ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_CMD_PLL0_208M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_208M_DIV_VAR_208M_PLL_SELECT_MASK;
			reg_val |= orig_var_208m_div_clk_src;
			writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_208M_DIV_OFFSET);
		}

		/* Restore VAR_312M clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_312M_DIV_OFFSET);
		if ((reg_val &
		     ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_MASK) ==
		    ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_CMD_PLL0_312M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_VAR_312M_DIV_VAR_312M_PLL_SELECT_MASK;
			reg_val |= orig_var_312m_div_clk_src;
			writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_312M_DIV_OFFSET);
		}

		/* Trigger VAR_96M/208M/312M clock source switch */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET);
		reg_val |= (ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_96M_TRIGGER_MASK
			    |
			    ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_208M_TRIGGER_MASK
			    |
			    ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_312M_TRIGGER_MASK);
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET);

		/* Run the clocks run briefly for the switch to happen */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);
		orig_reg_val = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_HW_SW_GATING_SEL_MASK;
		reg_val |= ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_VAR_96M_CLK_EN_MASK;
		writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);

		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);
		orig_reg_val1 = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_HW_SW_GATING_SEL_MASK;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_VAR_208M_CLK_EN_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);

		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);
		orig_reg_val2 = reg_val;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_HW_SW_GATING_SEL_MASK;
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_VAR_312M_CLK_EN_MASK;
		writel(reg_val,
		       CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);

		/* Wait until the trigger bits are cleared */
		insurance = 10000;
		while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
			      ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_OFFSET) &
			(ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_96M_TRIGGER_MASK |
			 ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_208M_TRIGGER_MASK |
			 ROOT_CLK_MGR_REG_REFCLK_SEG_TRG_VAR_312M_TRIGGER_MASK))
		       && insurance) {
			udelay(1);
			insurance--;
		}
		BUG_ON(insurance == 0);

		/* Restore the clocks to their original settings */
		writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_48M_CLKGATE_OFFSET);
		writel(orig_reg_val1, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_208M_CLKGATE_OFFSET);
		writel(orig_reg_val2, CLK_NAME(root).ccu_clk_mgr_base +
		       ROOT_CLK_MGR_REG_VAR_312M_CLKGATE_OFFSET);

		/* Restore VAR_TPIU clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_TPIU_DIV_OFFSET);
		if ((reg_val &
		     ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_MASK)
		    ==
		    ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_CMD_VAR_TPIU_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_TPIU_DIV_VAR_TPIU_VARVDD_PLL_SELECT_MASK;
			reg_val |= orig_var_tpiu_div_clk_src;
			writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_TPIU_DIV_OFFSET);

			/* Trigger the switchover */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET);
			reg_val |=
			    ROOT_CLK_MGR_REG_TPIU_TRG_VAR_TPIU_VARVDD_TRIGGER_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET);

			/* Run the clock run briefly for the switch to happen */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);
			orig_reg_val = reg_val;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_HW_SW_GATING_SEL_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_CLK_EN_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);

			/* Wait until the trigger bits are cleared */
			insurance = 10000;
			while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
				      ROOT_CLK_MGR_REG_TPIU_TRG_OFFSET) &
				ROOT_CLK_MGR_REG_TPIU_TRG_VAR_TPIU_VARVDD_TRIGGER_MASK)
			       && insurance) {
				udelay(1);
				insurance--;
			}
			BUG_ON(insurance == 0);

			/* Restore the clock to its original settings */
			writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);
		}

		/* Restore DIG_PRE clock source */
		reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
				ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);
		if ((reg_val &
		     ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK)
		    ==
		    ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_CMD_PLL0_312M_CLK)
		{
			reg_val &=
			    ~ROOT_CLK_MGR_REG_DIG_PRE_DIV_DIGITAL_PRE_PLL_SELECT_MASK;
			reg_val |= orig_dig_pre_div_clk_src;
			writel(reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);

			/* Trigger the switchover */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_DIG_TRG_OFFSET);
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_TRG_OFFSET);

			/* Run the clock run briefly for the switch to happen */
			reg_val = readl(CLK_NAME(root).ccu_clk_mgr_base +
					ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);
			orig_reg_val = reg_val;
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_HW_SW_GATING_SEL_MASK;
			reg_val |=
			    ROOT_CLK_MGR_REG_DIG_CLKGATE_DIGITAL_CH1_CLK_EN_MASK;
			writel(reg_val,
			       CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);

			/* Wait until the trigger bits are cleared */
			insurance = 10000;
			while ((readl(CLK_NAME(root).ccu_clk_mgr_base +
				      ROOT_CLK_MGR_REG_DIG_TRG_OFFSET) &
				ROOT_CLK_MGR_REG_DIG_TRG_DIGITAL_PRE_TRIGGER_MASK)
			       && insurance) {
				udelay(1);
				insurance--;
			}
			BUG_ON(insurance == 0);

			/* Restore the clock to its original settings */
			writel(orig_reg_val, CLK_NAME(root).ccu_clk_mgr_base +
			       ROOT_CLK_MGR_REG_DIG_CLKGATE_OFFSET);
		}

		/* Phew- finally done */
		pll1_disabled = 0;
	}

	/* Disable write access */
	CCU_ACCESS_EN(&CLK_NAME(root), 0);
	ccu_write_access_enable(&CLK_NAME(root), false);
	clk_unlock(&CLK_NAME(root).clk, &flags);

	return 0;
}

EXPORT_SYMBOL(clk_switch_src_pll0_pll1);

int __init clock_debug_add_misc_clock(struct clk *c)
{
	struct dentry *dent_clk_dir = 0, *dent_enable = 0, *dent_status = 0,
	    *dent_use_cnt = 0;

	if (c->clk_type != CLK_TYPE_MISC)
		return 0;
	/* Add TPIU and PTI clocks to root CCU */
	if (clk_root.dent_ccu_dir) {
		dent_clk_dir = debugfs_create_dir(c->name,
						  clk_root.dent_ccu_dir);
		if (!dent_clk_dir)
			return -ENOMEM;

		dent_enable = debugfs_create_file("enable", S_IRUGO | S_IWUSR,
						  dent_clk_dir, c,
						  &misc_clock_enable_fops);
		if (!dent_enable)
			return -ENOMEM;

		dent_status = debugfs_create_file("status", S_IRUGO,
						  dent_clk_dir, c,
						  &misc_clock_status_fops);
		if (!dent_status)
			return -ENOMEM;

		dent_use_cnt = debugfs_create_u32("use_cnt", S_IRUGO,
						  dent_clk_dir,
						  (unsigned int *)&c->use_cnt);
		if (!dent_use_cnt)
			return -ENOMEM;

	}

	return 0;
}

static void enable_spec_autogating(unsigned short enable)
{
	struct clk *autogating_clk;

	autogating_clk = clk_get(NULL, HUB_PERI_CLK_NAME_STR);
	if (enable)
		clk_enable_autogate(autogating_clk);
	else
		clk_disable_autogate(autogating_clk);

}

static ssize_t idle_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%hu\n", enable_special_autogating);
}

static ssize_t idle_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 || (value != 0 && value != 1)) {
		pr_err("idle_store: Invalid value\n");
		return -EINVAL;
	}
	enable_special_autogating = value;
	enable_spec_autogating(value);
	return n;
}

static struct kobj_attribute autogating_attr =
__ATTR(autogating, 0644, idle_show, idle_store);

#if 1
int __init clock_late_init(void)
{
	int ret = 0;
#ifdef CONFIG_DEBUG_FS
	int i;
	ret = clock_debug_init();
	if (ret) {
		pr_info("%s:dfs clock init failed\n", __func__);
		return ret;
	}
	for (i = 0; i < ARRAY_SIZE(capri_clk_tbl); i++) {
		if (capri_clk_tbl[i].clk->clk_type == CLK_TYPE_CCU) {
			ret = clock_debug_add_ccu(capri_clk_tbl[i].clk);
			if (ret) {
				pr_info("%s:dfs add ccu failed\n", __func__);
				return ret;
			}
		} else if (capri_clk_tbl[i].clk->clk_type == CLK_TYPE_MISC) {
			ret = clock_debug_add_misc_clock(capri_clk_tbl[i].clk);
			if (ret) {
				pr_info("%s:add misc clk failed\n", __func__);
				return ret;
			}
		} else {
			ret = clock_debug_add_clock(capri_clk_tbl[i].clk);
			if (ret) {
				pr_info("%s:dfs add clock failed\n", __func__);
				return ret;
			}
		}
	}
#endif
	ret = sysfs_create_file(power_kobj, &autogating_attr.attr);
	if (ret) {
		pr_info("%s:sysfs_create_file failed\n", __func__);
		return ret;
	}

	return 0;
}

late_initcall(clock_late_init);
#endif
