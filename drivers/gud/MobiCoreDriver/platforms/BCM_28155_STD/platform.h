/*
 * Header file of MobiCore Driver Kernel Module Platform
 * specific structures
 *
 * Internal structures of the McDrvModule
 *
 * Header file the MobiCore Driver Kernel Module,
 * its internal structures and defines.
 *
 * <-- Copyright Giesecke & Devrient GmbH 2009-2012 -->
 */

#ifndef _MC_DRV_PLATFORM_H_
#define _MC_DRV_PLATFORM_H_

/* MobiCore Interrupt. */
#define MC_INTR_SSIQ 77

/* Enable mobicore mem traces */
#define MC_MEM_TRACES

/* Enable Runtime Power Management */
#ifdef CONFIG_PM_RUNTIME
 #define MC_PM_RUNTIME
#endif

/* Use the arch_extension sec pseudo op before switching to secure world */
#if defined(__GNUC__) && \
	defined(__GNUC_MINOR__) && \
	defined(__GNUC_PATCHLEVEL__) && \
	((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)) \
		>= 40502
#define MC_ARCH_EXTENSION_SEC
#endif

#endif /* _MC_DRV_PLATFORM_H_ */
