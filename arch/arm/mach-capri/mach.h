#ifndef __CAPRI_H
#define __CAPRI_H

#include <linux/init.h>
#include <linux/platform_device.h>

extern struct platform_device capri_ipc_device;

void __init capri_map_io(void);
void __init board_pmu_init(void);

#endif /* __CAPRI_H */
