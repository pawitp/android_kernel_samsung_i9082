/*
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if !defined( DEBUG_SYM_H )
#define DEBUG_SYM_H

/* ---- Include Files ----------------------------------------------------- */

#include <linux/types.h>

/* ---- Constants and Types ---------------------------------------------- */

typedef struct opaque_vc_mem_access_handle_t *VC_MEM_ACCESS_HANDLE_T;

typedef uint32_t    VC_MEM_ADDR_T;

#define TO_VC_MEM_ADDR(ptr)    ((VC_MEM_ADDR_T)(unsigned long)(ptr))

/* ---- Variable Externs ------------------------------------------------- */

/* ---- Function Prototypes ---------------------------------------------- */

/*
 * The following were taken from vcinclude/hardware_vc4_bigisland.h
 */

#define ALIAS_NORMAL(x)             ((void*)(((unsigned long)(x)&~0xc0000000uL)|0x00000000uL)) // normal cached data (uses main 128K L2 cache)
#define IS_ALIAS_PERIPHERAL(x)      (((unsigned long)(x)>>29)==0x3uL)

/*
 * Get access to the videocore memory space. Returns zero if the memory was
 * opened successfully, or a negative value (-errno) if the access could not
 * be obtained.
 */
int OpenVideoCoreMemory( VC_MEM_ACCESS_HANDLE_T *handle  );

/*
 * Get access to the videocore space from a file. The file might be /dev/mem, or 
 * it might be saved image on disk. 
 */
int OpenVideoCoreMemoryFile( const char *filename, VC_MEM_ACCESS_HANDLE_T *vcHandlePtr );

/*
 * Returns the number of symbols which were detected.
 */
unsigned NumVideoCoreSymbols( VC_MEM_ACCESS_HANDLE_T handle );

/*
 * Returns the name, address and size of the i'th symbol.
 */
int GetVideoCoreSymbol( VC_MEM_ACCESS_HANDLE_T handle,
                        unsigned idx,
                        char *nameBuf,
                        size_t nameBufSize,
                        VC_MEM_ADDR_T *vcMemAddr,
                        size_t *vcMemSize );

/*
 * Looks up the named, symbol. If the symbol is found, it's value and size
 * are returned.
 *
 * Returns  true if the lookup was successful.
 */
int LookupVideoCoreSymbol( VC_MEM_ACCESS_HANDLE_T handle,
                           const char *symbol,
                           VC_MEM_ADDR_T *vcMemAddr,
                           size_t *vcMemSize );

/*
 * Looks up the named, symbol. If the symbol is found, and it's size is equal
 * to the sizeof a uint32_t, then true is returned.
 */
int LookupVideoCoreUInt32Symbol( VC_MEM_ACCESS_HANDLE_T handle,
                                 const char *symbol,
                                 VC_MEM_ADDR_T *vcMemAddr );

/*
 * Reads 'numBytes' from the videocore memory starting at 'vcMemAddr'. The
 * results are stored in 'buf'.
 *
 * Returns true if the read was successful.
 */
int ReadVideoCoreMemory( VC_MEM_ACCESS_HANDLE_T handle,
                         void *buf,
                         VC_MEM_ADDR_T vcMemAddr,
                         size_t numBytes );

/*
 * Reads an unsigned 32-bit value from videocore memory.
 */
static __inline int ReadVideoCoreUInt32( VC_MEM_ACCESS_HANDLE_T handle,
                                       uint32_t *val,
                                       VC_MEM_ADDR_T vcMemAddr )
{
    return ReadVideoCoreMemory( handle, val, vcMemAddr, sizeof( val ));
}

/*
 * Reads a block of memory using the address associated with a symbol.
 */
int ReadVideoCoreMemoryBySymbol( VC_MEM_ACCESS_HANDLE_T vcHandle,
                                 const char            *symbol,
                                 void                  *buf,
                                 size_t                 numBytes );
/*
 * Reads an unsigned 32-bit value from videocore memory.
 */
static __inline int ReadVideoCoreUInt32BySymbol( VC_MEM_ACCESS_HANDLE_T handle,
                                               const char *symbol,
                                               uint32_t *val )
{
    return ReadVideoCoreMemoryBySymbol( handle, symbol, val, sizeof( val ));
}

/*
 * Looksup a string symbol by name, and reads the contents into a user
 * supplied buffer.
 */
int ReadVideoCoreStringBySymbol( VC_MEM_ACCESS_HANDLE_T handle,
                                 const char *symbol,
                                 char *buf,
                                 size_t bufSize );

/*
 * Writes 'numBytes' into the videocore memory starting at 'vcMemAddr'. The
 * data is taken from 'buf'.
 *
 * Returns true if the write was successful.
 */
int WriteVideoCoreMemory( VC_MEM_ACCESS_HANDLE_T handle,
                          void *buf,
                          VC_MEM_ADDR_T vcMemAddr,
                          size_t numBytes );

/*
 * Writes an unsigned 32-bit value into videocore memory.
 */
static __inline int WriteVideoCoreUInt32( VC_MEM_ACCESS_HANDLE_T handle,
                                        uint32_t val,
                                        VC_MEM_ADDR_T vcMemAddr )
{
    return WriteVideoCoreMemory( handle, &val, vcMemAddr, sizeof( val ));
}

/*
 * Closes the memory space opened previously via OpenVideoCoreMemory.
 */
void CloseVideoCoreMemory( VC_MEM_ACCESS_HANDLE_T handle );

/*
 * Returns the size of the videocore memory space.
 */
size_t GetVideoCoreMemorySize( VC_MEM_ACCESS_HANDLE_T handle );

/*
 * Returns the videocore memory physical address.
 */
void *GetVideoCoreMemoryPhysicalAddress( VC_MEM_ACCESS_HANDLE_T handle );


#endif /* DEBUG_SYM_H */

