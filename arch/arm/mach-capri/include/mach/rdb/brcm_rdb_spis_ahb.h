/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2012  Broadcom Corporation                                                        */
/*                                                                                              */
/*     Unless you and Broadcom execute a separate written software license agreement governing  */
/*     use of this software, this software is licensed to you under the terms of the GNU        */
/*     General Public License version 2 (the GPL), available at                                 */
/*                                                                                              */
/*          http://www.broadcom.com/licenses/GPLv2.php                                          */
/*                                                                                              */
/*     with the following added to such license:                                                */
/*                                                                                              */
/*     As a special exception, the copyright holders of this software give you permission to    */
/*     link this software with independent modules, and to copy and distribute the resulting    */
/*     executable under terms of your choice, provided that you also meet, for each linked      */
/*     independent module, the terms and conditions of the license of that module.              */
/*     An independent module is a module which is not derived from this software.  The special  */
/*     exception does not apply to any modifications of the software.                           */
/*                                                                                              */
/*     Notwithstanding the above, under no circumstances may you combine this software in any   */
/*     way with any other Broadcom software provided under a license other than the GPL,        */
/*     without Broadcom's express prior written consent.                                        */
/*                                                                                              */
/*     Date     : Generated on 3/20/2012 13:8:57                                             */
/*     RDB file : /projects/CHIP/revA0                                                                   */
/************************************************************************************************/

#ifndef __BRCM_RDB_SPIS_AHB_H__
#define __BRCM_RDB_SPIS_AHB_H__

#define SPIS_AHB_AHB_DMA_ADDR_OFFSET                                      0x00000000
#define SPIS_AHB_AHB_DMA_ADDR_TYPE                                        UInt32
#define SPIS_AHB_AHB_DMA_ADDR_RESERVED_MASK                               0x00000000
#define    SPIS_AHB_AHB_DMA_ADDR_DMA_ADDR_SHIFT                           0
#define    SPIS_AHB_AHB_DMA_ADDR_DMA_ADDR_MASK                            0xFFFFFFFF

#define SPIS_AHB_AHB_DMA_LEN_OFFSET                                       0x00000010
#define SPIS_AHB_AHB_DMA_LEN_TYPE                                         UInt32
#define SPIS_AHB_AHB_DMA_LEN_RESERVED_MASK                                0xFFFF0000
#define    SPIS_AHB_AHB_DMA_LEN_DMA_LEN_SHIFT                             0
#define    SPIS_AHB_AHB_DMA_LEN_DMA_LEN_MASK                              0x0000FFFF

#define SPIS_AHB_AHB_DMA_CONTROL_OFFSET                                   0x00000018
#define SPIS_AHB_AHB_DMA_CONTROL_TYPE                                     UInt32
#define SPIS_AHB_AHB_DMA_CONTROL_RESERVED_MASK                            0xFFFFFFF0
#define    SPIS_AHB_AHB_DMA_CONTROL_ENDIAN_SHIFT                          3
#define    SPIS_AHB_AHB_DMA_CONTROL_ENDIAN_MASK                           0x00000008
#define    SPIS_AHB_AHB_DMA_CONTROL_LOCK_SHIFT                            2
#define    SPIS_AHB_AHB_DMA_CONTROL_LOCK_MASK                             0x00000004
#define    SPIS_AHB_AHB_DMA_CONTROL_DIR_SHIFT                             1
#define    SPIS_AHB_AHB_DMA_CONTROL_DIR_MASK                              0x00000002
#define    SPIS_AHB_AHB_DMA_CONTROL_EN_SHIFT                              0
#define    SPIS_AHB_AHB_DMA_CONTROL_EN_MASK                               0x00000001

#define SPIS_AHB_SPI_SLAVE_ADDR_OFFSET                                    0x0000001C
#define SPIS_AHB_SPI_SLAVE_ADDR_TYPE                                      UInt32
#define SPIS_AHB_SPI_SLAVE_ADDR_RESERVED_MASK                             0xFFFFFF80
#define    SPIS_AHB_SPI_SLAVE_ADDR_SPI_SLAVE_ADDR_SHIFT                   0
#define    SPIS_AHB_SPI_SLAVE_ADDR_SPI_SLAVE_ADDR_MASK                    0x0000007F

#define SPIS_AHB_AHB_DMA_MODE_OFFSET                                      0x00000020
#define SPIS_AHB_AHB_DMA_MODE_TYPE                                        UInt32
#define SPIS_AHB_AHB_DMA_MODE_RESERVED_MASK                               0xFFFFFFFE
#define    SPIS_AHB_AHB_DMA_MODE_DMA_MODE_SHIFT                           0
#define    SPIS_AHB_AHB_DMA_MODE_DMA_MODE_MASK                            0x00000001

#define SPIS_AHB_AHB_STATUS_OFFSET                                        0x00000024
#define SPIS_AHB_AHB_STATUS_TYPE                                          UInt32
#define SPIS_AHB_AHB_STATUS_RESERVED_MASK                                 0xFFFFFF00
#define    SPIS_AHB_AHB_STATUS_RFIFO_HALF_EMPTY_SHIFT                     7
#define    SPIS_AHB_AHB_STATUS_RFIFO_HALF_EMPTY_MASK                      0x00000080
#define    SPIS_AHB_AHB_STATUS_RFIFO_EMPTY_SHIFT                          6
#define    SPIS_AHB_AHB_STATUS_RFIFO_EMPTY_MASK                           0x00000040
#define    SPIS_AHB_AHB_STATUS_WFIFO_HALF_FULL_SHIFT                      5
#define    SPIS_AHB_AHB_STATUS_WFIFO_HALF_FULL_MASK                       0x00000020
#define    SPIS_AHB_AHB_STATUS_WFIFO_FULL_SHIFT                           4
#define    SPIS_AHB_AHB_STATUS_WFIFO_FULL_MASK                            0x00000010
#define    SPIS_AHB_AHB_STATUS_DMA_DONE_SHIFT                             3
#define    SPIS_AHB_AHB_STATUS_DMA_DONE_MASK                              0x00000008
#define    SPIS_AHB_AHB_STATUS_DMA_ERROR_SHIFT                            2
#define    SPIS_AHB_AHB_STATUS_DMA_ERROR_MASK                             0x00000004
#define    SPIS_AHB_AHB_STATUS_RFIFO_OVERFLOW_SHIFT                       1
#define    SPIS_AHB_AHB_STATUS_RFIFO_OVERFLOW_MASK                        0x00000002
#define    SPIS_AHB_AHB_STATUS_WFIFO_UNDERRUN_SHIFT                       0
#define    SPIS_AHB_AHB_STATUS_WFIFO_UNDERRUN_MASK                        0x00000001

#define SPIS_AHB_AHB_IRQ_ENABLE_OFFSET                                    0x00000028
#define SPIS_AHB_AHB_IRQ_ENABLE_TYPE                                      UInt32
#define SPIS_AHB_AHB_IRQ_ENABLE_RESERVED_MASK                             0xFFFFFF00
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_HALF_EMPTY_EN_SHIFT              7
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_HALF_EMPTY_EN_MASK               0x00000080
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_EMPTY_EN_SHIFT                   6
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_EMPTY_EN_MASK                    0x00000040
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_HALF_FULL_EN_SHIFT               5
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_HALF_FULL_EN_MASK                0x00000020
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_FULL_EN_SHIFT                    4
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_FULL_EN_MASK                     0x00000010
#define    SPIS_AHB_AHB_IRQ_ENABLE_DMA_DONE_EN_SHIFT                      3
#define    SPIS_AHB_AHB_IRQ_ENABLE_DMA_DONE_EN_MASK                       0x00000008
#define    SPIS_AHB_AHB_IRQ_ENABLE_DMA_ERROR_EN_SHIFT                     2
#define    SPIS_AHB_AHB_IRQ_ENABLE_DMA_ERROR_EN_MASK                      0x00000004
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_OVERFLOW_EN_SHIFT                1
#define    SPIS_AHB_AHB_IRQ_ENABLE_RFIFO_OVERFLOW_EN_MASK                 0x00000002
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_UNDERRUN_EN_SHIFT                0
#define    SPIS_AHB_AHB_IRQ_ENABLE_WFIFO_UNDERRUN_EN_MASK                 0x00000001

#define SPIS_AHB_AHB_WFIFO_CONTROL_OFFSET                                 0x00000040
#define SPIS_AHB_AHB_WFIFO_CONTROL_TYPE                                   UInt32
#define SPIS_AHB_AHB_WFIFO_CONTROL_RESERVED_MASK                          0xFFFFFFFE
#define    SPIS_AHB_AHB_WFIFO_CONTROL_WFIFO_CONTROL_SHIFT                 0
#define    SPIS_AHB_AHB_WFIFO_CONTROL_WFIFO_CONTROL_MASK                  0x00000001

#define SPIS_AHB_AHB_WFIFO_DEPTH_OFFSET                                   0x00000044
#define SPIS_AHB_AHB_WFIFO_DEPTH_TYPE                                     UInt32
#define SPIS_AHB_AHB_WFIFO_DEPTH_RESERVED_MASK                            0xFFFFFF00
#define    SPIS_AHB_AHB_WFIFO_DEPTH_WFIFO_DEPTH_SHIFT                     0
#define    SPIS_AHB_AHB_WFIFO_DEPTH_WFIFO_DEPTH_MASK                      0x000000FF

#define SPIS_AHB_AHB_WFIFO_READ_WORD_DATA_OFFSET                          0x0000004C
#define SPIS_AHB_AHB_WFIFO_READ_WORD_DATA_TYPE                            UInt32
#define SPIS_AHB_AHB_WFIFO_READ_WORD_DATA_RESERVED_MASK                   0x00000000
#define    SPIS_AHB_AHB_WFIFO_READ_WORD_DATA_WFIFO_READ_WORD_DATA_SHIFT   0
#define    SPIS_AHB_AHB_WFIFO_READ_WORD_DATA_WFIFO_READ_WORD_DATA_MASK    0xFFFFFFFF

#define SPIS_AHB_AHB_WFIFO_READ_BYTE_DATA_OFFSET                          0x00000050
#define SPIS_AHB_AHB_WFIFO_READ_BYTE_DATA_TYPE                            UInt32
#define SPIS_AHB_AHB_WFIFO_READ_BYTE_DATA_RESERVED_MASK                   0xFFFFFF00
#define    SPIS_AHB_AHB_WFIFO_READ_BYTE_DATA_WFIFO_READ_BYTE_DATA_SHIFT   0
#define    SPIS_AHB_AHB_WFIFO_READ_BYTE_DATA_WFIFO_READ_BYTE_DATA_MASK    0x000000FF

#define SPIS_AHB_AHB_RFIFO_CONTROL_OFFSET                                 0x00000080
#define SPIS_AHB_AHB_RFIFO_CONTROL_TYPE                                   UInt32
#define SPIS_AHB_AHB_RFIFO_CONTROL_RESERVED_MASK                          0xFFFFFFFE
#define    SPIS_AHB_AHB_RFIFO_CONTROL_RFIFO_CONTROL_SHIFT                 0
#define    SPIS_AHB_AHB_RFIFO_CONTROL_RFIFO_CONTROL_MASK                  0x00000001

#define SPIS_AHB_AHB_RFIFO_DEPTH_OFFSET                                   0x00000084
#define SPIS_AHB_AHB_RFIFO_DEPTH_TYPE                                     UInt32
#define SPIS_AHB_AHB_RFIFO_DEPTH_RESERVED_MASK                            0xFFFFFF00
#define    SPIS_AHB_AHB_RFIFO_DEPTH_RFIFO_DEPTH_SHIFT                     0
#define    SPIS_AHB_AHB_RFIFO_DEPTH_RFIFO_DEPTH_MASK                      0x000000FF

#define SPIS_AHB_AHB_RFIFO_WRITE_WORD_DATA_OFFSET                         0x0000008C
#define SPIS_AHB_AHB_RFIFO_WRITE_WORD_DATA_TYPE                           UInt32
#define SPIS_AHB_AHB_RFIFO_WRITE_WORD_DATA_RESERVED_MASK                  0x00000000
#define    SPIS_AHB_AHB_RFIFO_WRITE_WORD_DATA_RFIFO_WRITE_WORD_DATA_SHIFT 0
#define    SPIS_AHB_AHB_RFIFO_WRITE_WORD_DATA_RFIFO_WRITE_WORD_DATA_MASK  0xFFFFFFFF

#define SPIS_AHB_AHB_RFIFO_WRITE_BYTE_DATA_OFFSET                         0x00000090
#define SPIS_AHB_AHB_RFIFO_WRITE_BYTE_DATA_TYPE                           UInt32
#define SPIS_AHB_AHB_RFIFO_WRITE_BYTE_DATA_RESERVED_MASK                  0xFFFFFF00
#define    SPIS_AHB_AHB_RFIFO_WRITE_BYTE_DATA_RFIFO_WRITE_BYTE_DATA_SHIFT 0
#define    SPIS_AHB_AHB_RFIFO_WRITE_BYTE_DATA_RFIFO_WRITE_BYTE_DATA_MASK  0x000000FF

#endif /* __BRCM_RDB_SPIS_AHB_H__ */


