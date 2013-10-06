/******************************************************************************/
/* Copyright 2009, 2010 Broadcom Corporation.  All rights reserved.           */
/*     Unless you and Broadcom execute a separate written software license    */
/*     agreement governing use of this software, this software is licensed to */
/*     you under the terms of the GNU General Public License version 2        */
/*    (the GPL), available at                                                 */
/*                                                                            */
/*          http://www.broadcom.com/licenses/GPLv2.php                        */
/*                                                                            */
/*     with the following added to such license:                              */
/*                                                                            */
/*     As a special exception, the copyright holders of this software give    */
/*     you permission to link this software with independent modules, and to  */
/*     copy and distribute the resulting executable under terms of your       */
/*     choice, provided that you also meet, for each linked independent       */
/*     module, the terms and conditions of the license of that module.        */
/*     An independent module is a module which is not derived from this       */
/*     software.  The special exception does not apply to any modifications   */
/*     of the software.                                                       */
/*                                                                            */
/*     Notwithstanding the above, under no circumstances may you combine this */
/*     software in any way with any other Broadcom software provided under a  */
/*     license other than the GPL, without Broadcom's express prior written   */
/*     consent.                                                               */
/******************************************************************************/

/**
*
*  @file   csl_caph_render.c
*
*  @brief  CSL layer driver for caph render
*
****************************************************************************/
#include "mobcom_types.h"
#include "resultcode.h"
#include "msconsts.h"
#include "audio_consts.h"
#include "csl_caph.h"
#include "csl_caph_dma.h"
#include "csl_caph_hwctrl.h"
#include "csl_audio_render.h"
#include "audio_trace.h"
#include <linux/completion.h>

/***************************************************************************/
/*                       G L O B A L   S E C T I O N                       */
/***************************************************************************/

/***************************************************************************/
/*lobal variable definitions                                               */
/***************************************************************************/

/***************************************************************************/
/*                        L O C A L   S E C T I O N                        */
/***************************************************************************/

/***************************************************************************/
/*local macro declarations                                                 */
/***************************************************************************/
/***************************************************************************/
/*local typedef declarations                                               */
/***************************************************************************/

/***************************************************************************/
/*local variable definitions                                               */
/***************************************************************************/
static CSL_CAPH_Render_Drv_t sRenderDrv[CSL_CAPH_STREAM_TOTAL] = { {0} };
static atomic_t gNumActiveStreams = ATOMIC_INIT(0);
struct completion completeDynDma[CSL_CAPH_STREAM_TOTAL];

/***************************************************************************/
/*local function declarations                                              */
/***************************************************************************/
static CSL_CAPH_STREAM_e GetStreamIDByDmaCH(CSL_CAPH_DMA_CHNL_e dmaCH);
static void AUDIO_DMA_CB(CSL_CAPH_DMA_CHNL_e chnl);
static void audio_sil_frm_detect(CSL_CAPH_Render_Drv_t *audDrv);

/***************************************************************************/
/*local function definitions                                               */
/***************************************************************************/

/****************************************************************************
*
*  Function Name:UInt32 csl_caph_render_init(CSL_CAPH_DEVICE_e source,
*                                                        CSL_CAPH_DEVICE_e sink)
*
*  Description: init CAPH render block
*
****************************************************************************/

/* lready know source and sink, hmm */

UInt32 csl_audio_render_init(CSL_CAPH_DEVICE_e source, CSL_CAPH_DEVICE_e sink)
{
	UInt32 streamID = CSL_CAPH_STREAM_NONE;
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	aTrace(LOG_AUDIO_CSL,
			"csl_caph_render_init::source=0x%x sink=0x%x.\n",
			source, sink);

	/* allocate a unique streamID */
	streamID = (UInt32) csl_caph_hwctrl_AllocateStreamID();
	audDrv = &sRenderDrv[streamID];

	memset(audDrv, 0, sizeof(CSL_CAPH_Render_Drv_t));

	audDrv->streamID = streamID;
	audDrv->source = source;
	audDrv->sink = sink;
	spin_lock_init(&audDrv->readyStatusLock);
	spin_lock_init(&audDrv->configLock);
	return audDrv->streamID;
}

/****************************************************************************
*
*  Function Name:Result_t csl_audio_render_deinit
*
*  Description: De-initialize CSL render layer
*
****************************************************************************/
Result_t csl_audio_render_deinit(UInt32 streamID)
{
	CSL_CAPH_Render_Drv_t	*audDrv = NULL;
	unsigned long flags;

	aTrace(LOG_AUDIO_CSL,
			"csl_caph_render_deinit::streamID=0x%lx\n", streamID);

	audDrv = GetRenderDriverByType(streamID);

	if (audDrv == NULL)
		return RESULT_ERROR;

	audDrv->streamID = 0;
	audDrv->source = CSL_CAPH_DEV_NONE;
	audDrv->sink = CSL_CAPH_DEV_NONE;
	audDrv->pathID = 0;
	audDrv->dmaCH = 0;
	audDrv->blockIndex = 0;

	spin_lock_irqsave(&audDrv->configLock, flags);
	audDrv->dmaCB = NULL;
	spin_unlock_irqrestore(&audDrv->configLock, flags);

	audDrv->ringBuffer = NULL;
	audDrv->numBlocks = 0;
	audDrv->blockSize = 0;

	spin_lock_irqsave(&audDrv->readyStatusLock, flags);
	audDrv->readyBlockStatus = CSL_CAPH_READY_NONE;
	spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: Result_t csl_audio_render_configure
*
*  Description: Configure the CAPH render
*
****************************************************************************/
Result_t csl_audio_render_configure(AUDIO_SAMPLING_RATE_t sampleRate,
				    AUDIO_NUM_OF_CHANNEL_t numChannels,
				    AUDIO_BITS_PER_SAMPLE_t bitsPerSample,
				    UInt8 *ringBuffer,
				    UInt32 numBlocks,
				    UInt32 blockSize,
				    CSL_AUDRENDER_CB csl_audio_render_cb,
				    UInt32 streamID,
					int mixMode)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	CSL_CAPH_HWCTRL_STREAM_REGISTER_t stream;
	unsigned long flags;

#ifdef DSP_FPGA_TEST
	AP_SharedMem_t *pSharedMem = SHAREDMEM_GetDsp_SharedMemPtr();
#endif

	aTrace(LOG_AUDIO_CSL,
	"csl_caph_render_configure:: streamID = 0x%lx, sampleRate =0x%x,"
	"numChannels = 0x%x, numbBuffers = 0x%lx, blockSize = 0x%lx,"
	"bitsPerSample %d, cb = %lx.\r\n",
		streamID, sampleRate, numChannels, numBlocks, blockSize,
		(int)bitsPerSample,
		(long unsigned int)csl_audio_render_cb);

	BUG_ON(streamID <= CSL_CAPH_STREAM_NONE ||
		streamID >= CSL_CAPH_STREAM_TOTAL);

	init_completion(&completeDynDma[streamID]);

	audDrv = GetRenderDriverByType(streamID);

	if (audDrv == NULL)
		return RESULT_ERROR;

	/*audDrv->numChannels = numChannels;
	audDrv->bitsPerSample = bitsPerSample;
	audDrv->sampleRate = sampleRate;*/

#ifdef DSP_FPGA_TEST
	if (audDrv->source == CSL_CAPH_DEV_DSP)
		ringBuffer =
		    (UInt8
		     *) (&(((AP_SharedMem_t *) pSharedMem)->
			   shared_aud_out_buf_48k[0][0]));
#endif
	/* make sure ringbuffer, numblocks and block size */
	/* are legal for Rhea  */

	memset(&stream, 0, sizeof(CSL_CAPH_HWCTRL_STREAM_REGISTER_t));
	stream.streamID = (CSL_CAPH_STREAM_e) audDrv->streamID;
	stream.src_sampleRate = sampleRate;
	/* stream.snk_sampleRate = sampleRate; */
	stream.chnlNum = numChannels;
	stream.bitPerSample = bitsPerSample;
	stream.pBuf = ringBuffer;
	stream.pBuf2 = NULL;
	stream.size = 2 * blockSize; /*HW only supports 2 buffers*/
	stream.dmaCB = AUDIO_DMA_CB;
	stream.mixMode = mixMode;
	audDrv->pathID = csl_caph_hwctrl_RegisterStream(&stream);
	if (audDrv->pathID == 0) {
		audio_xassert(0, audDrv->pathID);
		return RESULT_ERROR;
	}

	spin_lock_irqsave(&audDrv->configLock, flags);
	audDrv->dmaCB = csl_audio_render_cb;
	spin_unlock_irqrestore(&audDrv->configLock, flags);

	audDrv->dmaCH = csl_caph_hwctrl_GetdmaCH(audDrv->pathID);
	if (((numBlocks > 2) || (numBlocks * blockSize >= 0x10000)) &&
	    audDrv->dmaCH > CSL_CAPH_DMA_CH2) {
		csl_caph_hwctrl_SetLongDma(audDrv->pathID);
		audDrv->dmaCH = csl_caph_hwctrl_GetdmaCH(audDrv->pathID);
	}
	audDrv->ringBuffer = ringBuffer;
	audDrv->numBlocks = numBlocks;
	audDrv->blockSize = blockSize;

	/* assume everytime it starts, the first 2 buffers will be filled
	when the interrupt comes, it will start from buffer 2
	*/
	audDrv->readyBlockStatus = CSL_CAPH_READY_NONE;
	audDrv->blockIndex = 1;
#if defined(DYNAMIC_DMA_PLAYBACK)
	if (blockSize == DYNDMA_COPY_SIZE
		&& numBlocks == DYNDMA_NUM_BLOCKS) {
		atomic_set(&audDrv->renderNumBlocks, DYNDMA_NUM_BLOCKS);
		atomic_set(&audDrv->numDMABlocks, 0);
		atomic_set(&audDrv->dmaState, DYNDMA_NORMAL);
	} else {
		atomic_set(&audDrv->dmaState, DYNDMA_DUMMY);
	}
	audDrv->numBlocks2 = numBlocks;
	atomic_set(&audDrv->availBytes, 0);
	audDrv->first = 1;
#endif
	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: csl_audio_render_start
*
*  Description: Start the data transfer of audio path render
*
****************************************************************************/
Result_t csl_audio_render_start(UInt32 streamID)
{
	CSL_CAPH_Render_Drv_t	*audDrv = NULL;

	aTrace(LOG_AUDIO_CSL,
			"csl_audio_render_start::streamID=0x%lx\n", streamID);

	audDrv = GetRenderDriverByType(streamID);

	if (audDrv == NULL)
		return RESULT_ERROR;

	atomic_inc(&gNumActiveStreams);

	(void)csl_caph_hwctrl_StartPath(audDrv->pathID);

	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: csl_audio_render_stop
*
*  Description: Stop the data transfer of audio path render
*
****************************************************************************/
Result_t csl_audio_render_stop(UInt32 streamID)
{
	CSL_CAPH_HWCTRL_CONFIG_t config;
	BUG_ON(streamID < CSL_CAPH_STREAM_NONE
		|| streamID >= CSL_CAPH_STREAM_TOTAL);
	aTrace(LOG_AUDIO_CSL,
			"csl_audio_render_stop::streamID=0x%lx\n", streamID);
	config.streamID = (CSL_CAPH_STREAM_e) streamID;
	(void)csl_caph_hwctrl_DisablePath(config);
#if defined(DYNAMIC_DMA_PLAYBACK)
	/*When stopping the playback,
	release the completion as
	Copy() callback might be waiting*/
	if (streamID != 0)
		complete(&completeDynDma[streamID]);
#endif
	atomic_dec(&gNumActiveStreams);
	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: csl_audio_render_pause
*
*  Description: Pause the data transfer of audio path render
*
****************************************************************************/
Result_t csl_audio_render_pause(UInt32 streamID)
{
	CSL_CAPH_HWCTRL_CONFIG_t config;

	aTrace(LOG_AUDIO_CSL,
			"csl_audio_render_pause::streamID=0x%lx\n", streamID);
	memset(&config, 0, sizeof(CSL_CAPH_HWCTRL_CONFIG_t));
	config.streamID = (CSL_CAPH_STREAM_e) streamID;
	(void)csl_caph_hwctrl_PausePath(config);

	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: csl_audio_render_resume
*
*  Description: Resume the data transfer of audio path render
*
****************************************************************************/
Result_t csl_audio_render_resume(UInt32 streamID)
{
	CSL_CAPH_HWCTRL_CONFIG_t config;

	aTrace(LOG_AUDIO_CSL,
			"csl_audio_render_resume::streamID=0x%lx\n", streamID);
	memset(&config, 0, sizeof(CSL_CAPH_HWCTRL_CONFIG_t));
	config.streamID = (CSL_CAPH_STREAM_e) streamID;
	(void)csl_caph_hwctrl_ResumePath(config);

	return RESULT_OK;
}

/****************************************************************************
*
*  Function Name: csl_audio_render_buffer_ready
*
*  Description: set the SW_READY to aadmac when a new buffer is ready
*
****************************************************************************/
Result_t csl_audio_render_buffer_ready(UInt32 streamID, int size, int offset)
{
#if defined(DYNAMIC_DMA_PLAYBACK)
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	unsigned long flags;
	int avail = 0, nCopyBlocks, ringBufSize;

	/*aTrace(LOG_AUDIO_CSL,
		"%s streamID=0x%lx size %x offset %x\n",
		__func__, streamID, size, offset);*/

	audDrv = GetRenderDriverByType(streamID);

	if (audDrv == NULL)
		return RESULT_WRONG_STATE;

	audio_sil_frm_detect(audDrv);

	if (audDrv->dmaCH <= CSL_CAPH_DMA_CH2 && size == DYNDMA_COPY_SIZE) {
		/*only for dyndma streams*/
		ringBufSize = audDrv->numBlocks * audDrv->blockSize;
		audDrv->readyBlockIndex++;
		avail = atomic_read(&audDrv->availBytes);
		/*the 1st call may be later than DMA CB, especially if full
		  logs are enabled*/
		if (audDrv->first == 1) {
			audDrv->first = 0;
			avail = atomic_read(&audDrv->availBytes);
			avail += offset;
			audDrv->readyBlockIndex = avail/size;
		} else {
			avail += size;
		}

		if (avail > ringBufSize) {
			aTrace(LOG_AUDIO_CSL, "%s avail 0x%x reset to %x\n",
			__func__, avail, ringBufSize);
			avail = ringBufSize;
		}
		atomic_set(&audDrv->availBytes, avail);
		nCopyBlocks = ringBufSize / size;
		if (audDrv->readyBlockIndex >= nCopyBlocks)
			audDrv->readyBlockIndex = 0;

		/*may need to wait for complete low buffer, but could be too
		  late*/
		if (atomic_read(&audDrv->dmaState) == DYNDMA_LOW_DONE) {
			atomic_set(&audDrv->dmaState, DYNDMA_LOW_RDY);
			aTrace(LOG_AUDIO_CSL, "%s stream %d idx %d low rdy\n",
			__func__, (int)streamID, (int)audDrv->readyBlockIndex);
		}
		/*aTrace(LOG_AUDIO_CSL,
		"%s avail 0x%x index %d size %d offset 0x%x", __func__,
		avail, audDrv->readyBlockIndex, size, offset);*/
	}

	spin_lock_irqsave(&audDrv->readyStatusLock, flags);

	/* when to set sw rdy:
	 - non-dyndma (pcmout1, aplay, legacy audio hal etc).
	 - dyndma, and enough data for one dma period.
	 - dyndma, switch big dma to small, waiting big buffer to be
	   consumed by small dma (size=0).
	 - draining state, multiple period data still remain (size=0).
	*/
	if (audDrv->dmaCH > CSL_CAPH_DMA_CH2 || size == 0 ||
		size != DYNDMA_COPY_SIZE ||
		atomic_read(&audDrv->availBytes)
		>= audDrv->blockSize) {
		/* Set the block(s) that are ready for processing */
		csl_caph_dma_set_ddrfifo_status(audDrv->dmaCH,
			audDrv->readyBlockStatus);
		/* Clear the ready block status */
		audDrv->readyBlockStatus = CSL_CAPH_READY_NONE;
	}

	spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);
#else
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	unsigned long flags;

	/*aTrace(LOG_AUDIO_CSL,
		"csl_audio_render_buffer_ready:streamID=0x%lx\n", streamID);*/

	audDrv = GetRenderDriverByType(streamID);

	if (audDrv == NULL)
		return RESULT_WRONG_STATE;

	audio_sil_frm_detect(audDrv);

	spin_lock_irqsave(&audDrv->readyStatusLock, flags);

	/* Set the block(s) that are ready for processing */
	csl_caph_dma_set_ddrfifo_status(audDrv->dmaCH,
					audDrv->readyBlockStatus);

	/* Clear the ready block status */
	audDrv->readyBlockStatus = CSL_CAPH_READY_NONE;

	spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);
#endif
	return RESULT_OK;
}

/* ==========================================================================
//
// Function Name: GetRenderDriverByType
//
// Description: Get the audio render driver reference from the steamID.
//
// =========================================================================*/
CSL_CAPH_Render_Drv_t *GetRenderDriverByType(UInt32 streamID)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	if (sRenderDrv[streamID].streamID != 0) {
		/* valid streamID starts with value of 1*/
		audDrv = &sRenderDrv[streamID];
	}

	return audDrv;
}

/* ==========================================================================
//
// Function Name: audio_sil_frm_detect
//
// Description: Detection for silent frames
//
// =========================================================================*/
static void audio_sil_frm_detect(CSL_CAPH_Render_Drv_t *audDrv)
{
	UInt8 *addr;
	UInt16 *ptr;
	int left_sig = 0;
	int right_sig = 0;
	UInt16 dither_lr;
	int i;

	dither_lr = csl_caph_audio_is_hs_path_dither_enabled();

	if (atomic_read(&gNumActiveStreams) == 1 && !dither_lr) {

		if (csl_caph_dma_get_sdm_reset_mode(audDrv->dmaCH) ==
			SDM_RESET_MODE_ENABLED) {

			addr = audDrv->ringBuffer +
				audDrv->blockIndex * audDrv->blockSize;

			ptr = phys_to_virt((UInt32)addr);
			for (i = 0; i < audDrv->blockSize; i += 1) {

				if ((i & 0x1) == 0 && !left_sig) {
					if (ptr[i] != 0x0 ||
						(dither_lr &
						CSL_AUDIO_CHANNEL_LEFT)) {
						csl_caph_dma_sil_frm_cnt_reset(
						audDrv->dmaCH,
						CSL_AUDIO_CHANNEL_LEFT);
						left_sig = 1;
					}

				} else if (!right_sig) {
					if (ptr[i] != 0x0 ||
						(dither_lr &
						CSL_AUDIO_CHANNEL_RIGHT)) {
						csl_caph_dma_sil_frm_cnt_reset(
						audDrv->dmaCH,
						CSL_AUDIO_CHANNEL_RIGHT);
						right_sig = 1;
					}
				}

				if (left_sig && right_sig)
					break;
			}

			if (i == audDrv->blockSize) {
				if (!left_sig)
					csl_caph_dma_sil_frm_cnt_incr(
						audDrv->dmaCH,
						CSL_AUDIO_CHANNEL_LEFT);
				if (!right_sig)
					csl_caph_dma_sil_frm_cnt_incr(
						audDrv->dmaCH,
						CSL_AUDIO_CHANNEL_RIGHT);
			}
		}
	} else {
		csl_caph_dma_sil_frm_cnt_reset(
			audDrv->dmaCH,
			(CSL_AUDIO_CHANNEL_LEFT | CSL_AUDIO_CHANNEL_RIGHT));
	}
}

/* ==========================================================================
//
// Function Name: AUDIO_DMA_CB
//
// Description: The callback function when there is DMA request
//
// =========================================================================*/
#if defined(DYNAMIC_DMA_PLAYBACK)
static void audio_dma_cb_complete(CSL_CAPH_Render_Drv_t *audDrv)
{
	/* when to release completion:
	 - dyndma
	 - small dma size
	 - or changing dma size from big to small*/
	if (audDrv->dmaCH <= CSL_CAPH_DMA_CH2 &&
	   (audDrv->numBlocks > 2 ||
	   atomic_read(&audDrv->renderNumBlocks) > 2)) {
		BUG_ON(audDrv->streamID <= CSL_CAPH_STREAM_NONE
			|| audDrv->streamID
			>= CSL_CAPH_STREAM_TOTAL);
		complete(&completeDynDma[audDrv->streamID]);
	}
}

static void AUDIO_DMA_CB(CSL_CAPH_DMA_CHNL_e chnl)
{
	int temp, avail, blocks;
	UInt32 streamID;
	CSL_CAPH_Render_Drv_t *audDrv;
	CSL_CAPH_DMA_CHNL_FIFO_STATUS_e buffer_status = CSL_CAPH_READY_NONE;
	CSL_CAPH_DMA_CHNL_FIFO_STATUS_e fifo_status;
	UInt8 *addr;
	unsigned long flags;

	/*aTrace(LOG_AUDIO_CSL, "%s %d\n", __func__, chnl);*/
	streamID = GetStreamIDByDmaCH(chnl);

	/* Collect a snapshot of the status */
	fifo_status = csl_caph_dma_read_ddrfifo_sw_status(chnl);

	audDrv = GetRenderDriverByType(streamID);
	if (audDrv == NULL)
		return;

	if (audDrv->dmaCH <= CSL_CAPH_DMA_CH2) {
		audDrv->dynDmaSize = audDrv->blockSize;
		temp = atomic_read(&audDrv->renderNumBlocks);
		if (atomic_read(&audDrv->dmaState) == DYNDMA_NORMAL
			&& temp != audDrv->numBlocks2) {
			atomic_set(&audDrv->dmaState, DYNDMA_TRIGGER);
			audDrv->numBlocks2 = temp;
		}
	}

	if (atomic_read(&audDrv->dmaState) == DYNDMA_TRIGGER
		&& audDrv->blockIndex == (audDrv->numBlocks>>1)) {
		/*wait for low long buffer*/
		atomic_set(&audDrv->dmaState, DYNDMA_LOW_DONE);
		aTrace(LOG_AUDIO_CSL, "%s chnl %d low done\n", __func__, chnl);
	} else if (atomic_read(&audDrv->dmaState) == DYNDMA_LOW_RDY
		&& audDrv->blockIndex == 0) {
		/*wait for high long buffer*/
		CSL_CAPH_DMA_CONFIG_t dma_cfg;

		blocks = atomic_read(&audDrv->numDMABlocks);
		if (blocks == 2 || blocks == 8)	{
			/* if numDMABlocks != 0, then
			switch got triggered in between
			another switch (rare case), ignore
			the current switch, and continue normally
			2 (current)->8->2 - ignore switching to 8 blocks
			8 (current)->2->8 - ignore switching to 2 blocks*/
			atomic_set(&audDrv->numDMABlocks, 0);
			atomic_set(&audDrv->dmaState, DYNDMA_NORMAL);
			/*aError("state changed to NORMAL"
			audDrv->numBlocks2 %d",audDrv->numBlocks2); */
		}  else {
		if ((fifo_status & CSL_CAPH_READY_LOW) ==
			CSL_CAPH_READY_NONE) {
			audio_dma_cb_complete(audDrv);
			atomic_sub(audDrv->blockSize, &audDrv->availBytes);
		}
		if ((fifo_status & CSL_CAPH_READY_HIGH) ==
			CSL_CAPH_READY_NONE) {
			audio_dma_cb_complete(audDrv);
			atomic_sub(audDrv->blockSize, &audDrv->availBytes);
		}
		atomic_set(&audDrv->dmaState, DYNDMA_NORMAL);
		audDrv->blockSize =
		    (audDrv->numBlocks*audDrv->blockSize)/audDrv->numBlocks2;
		audDrv->numBlocks = audDrv->numBlocks2;
		audDrv->blockIndex = 1;

		spin_lock_irqsave(&audDrv->readyStatusLock, flags);
		audDrv->readyBlockStatus = CSL_CAPH_READY_NONE;
		spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

		dma_cfg.mem_addr = audDrv->ringBuffer;
		dma_cfg.dma_ch = chnl;
		dma_cfg.mem_size = 2*audDrv->blockSize;

		csl_caph_dma_set_buffer(&dma_cfg);
		csl_caph_dma_set_hibuffer_address(audDrv->dmaCH,
			audDrv->ringBuffer + audDrv->blockSize);
		csl_caph_dma_set_ddrfifo_status(audDrv->dmaCH,
			CSL_CAPH_READY_HIGHLOW);

		buffer_status |= CSL_CAPH_READY_HIGHLOW;
		avail = atomic_read(&audDrv->availBytes);

		spin_lock_irqsave(&audDrv->configLock, flags);
		if (audDrv->dmaCB)
			audDrv->dmaCB(audDrv->streamID, buffer_status);
		spin_unlock_irqrestore(&audDrv->configLock, flags);

		aTrace(LOG_AUDIO_CSL, "AUDIO_DMA_CB::chnl %d numBlocks %d "
			"addr %p idx %d blkSize %d avail %x\n",
			chnl, (int)audDrv->numBlocks, dma_cfg.mem_addr,
			(int)audDrv->blockIndex, (int)audDrv->blockSize,
			avail);
		return;
		}
	}

	/*in case both low and high are done,
	  blockIndex should still dictate the sequence*/
	if ((fifo_status & CSL_CAPH_READY_LOW) == CSL_CAPH_READY_NONE
		&& (audDrv->blockIndex & 1)) {
		audio_dma_cb_complete(audDrv);
		avail = atomic_read(&audDrv->availBytes);
		avail -= audDrv->blockSize;
		atomic_set(&audDrv->availBytes, avail);
		buffer_status |= CSL_CAPH_READY_LOW;

		spin_lock_irqsave(&audDrv->readyStatusLock, flags);
		audDrv->readyBlockStatus |= CSL_CAPH_READY_LOW;
		spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

		spin_lock_irqsave(&audDrv->configLock, flags);
		if (audDrv->dmaCB)
			audDrv->dmaCB(audDrv->streamID, buffer_status);
		spin_unlock_irqrestore(&audDrv->configLock, flags);

		audDrv->blockIndex++;
		if (audDrv->blockIndex >= audDrv->numBlocks)
			audDrv->blockIndex = 0;

		addr = audDrv->ringBuffer +
			audDrv->blockIndex * audDrv->blockSize;
		csl_caph_dma_set_lobuffer_address(chnl, addr);

		/*aTrace(LOG_AUDIO_CSL, "AUDIO_DMA_CB %d idx %d addr %p low "
		" avail %x\n", chnl, (int)audDrv->blockIndex, addr, avail);*/
	}

	if ((fifo_status & CSL_CAPH_READY_HIGH) == CSL_CAPH_READY_NONE
		&& (audDrv->blockIndex & 1) == 0) {
		audio_dma_cb_complete(audDrv);
		buffer_status |= CSL_CAPH_READY_HIGH;
		avail = atomic_read(&audDrv->availBytes);
		avail -= audDrv->blockSize;
		atomic_set(&audDrv->availBytes, avail);

		spin_lock_irqsave(&audDrv->readyStatusLock, flags);
		audDrv->readyBlockStatus |= CSL_CAPH_READY_HIGH;
		spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

		spin_lock_irqsave(&audDrv->configLock, flags);
		if (audDrv->dmaCB)
			audDrv->dmaCB(audDrv->streamID, buffer_status);
		spin_unlock_irqrestore(&audDrv->configLock, flags);

		audDrv->blockIndex++;
		if (audDrv->blockIndex >= audDrv->numBlocks)
			audDrv->blockIndex = 0;

		addr = audDrv->ringBuffer +
			audDrv->blockIndex * audDrv->blockSize;
		csl_caph_dma_set_hibuffer_address(chnl, addr);

		/*aTrace(LOG_AUDIO_CSL, "AUDIO_DMA_CB %d idx %d addr %p high"
		"avail %x\n", chnl, (int)audDrv->blockIndex, addr, avail);*/
	}
}
#else
static void AUDIO_DMA_CB(CSL_CAPH_DMA_CHNL_e chnl)
{
	UInt32 streamID = 0;
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	CSL_CAPH_DMA_CHNL_FIFO_STATUS_e buffer_status = CSL_CAPH_READY_NONE;
	CSL_CAPH_DMA_CHNL_FIFO_STATUS_e fifo_status = CSL_CAPH_READY_NONE;
	UInt8 *addr = NULL;
	unsigned long flags;

	/* aTrace(LOG_AUDIO_CSL, "AUDIO_DMA_CB::
	   DMA callback. chnl = %d\n", chnl); */

	streamID = GetStreamIDByDmaCH(chnl);

	/* Collect a snapshot of the status */
	fifo_status = csl_caph_dma_read_ddrfifo_sw_status(chnl);

	audDrv = GetRenderDriverByType(streamID);
	if (audDrv == NULL)
		return;

	/* when system is heavily loaded, both buffers could be emptied by hw.
	in this case need to fill the data on the HIGH buffer and prime the LOW
    buffer to recover. */
	if ((fifo_status & CSL_CAPH_READY_HIGH) == CSL_CAPH_READY_NONE) {
		buffer_status |= CSL_CAPH_READY_HIGH;

		/* Check for need to process both HIGH and LOW blocks
			or just HIGH block */
		spin_lock_irqsave(&audDrv->readyStatusLock, flags);

		if (fifo_status == CSL_CAPH_READY_NONE)
			audDrv->readyBlockStatus |= CSL_CAPH_READY_HIGHLOW;
		else
			audDrv->readyBlockStatus |= CSL_CAPH_READY_HIGH;

		spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

		/* Handle special case to clear LOW block if undderrun.
		 * Set LOW addr to last used block used and clear */
		if (fifo_status == CSL_CAPH_READY_NONE) {
			addr = audDrv->ringBuffer +
			   audDrv->blockIndex * audDrv->blockSize;
			memset(phys_to_virt((UInt32)addr),
				0, audDrv->blockSize);
			csl_caph_dma_set_lobuffer_address(chnl, addr);
		}

		spin_lock_irqsave(&audDrv->configLock, flags);
		if (audDrv->dmaCB)
			audDrv->dmaCB(audDrv->streamID, buffer_status);
		spin_unlock_irqrestore(&audDrv->configLock, flags);

		audDrv->blockIndex++;
		if (audDrv->blockIndex >= audDrv->numBlocks)
			audDrv->blockIndex = 0;
		addr = audDrv->ringBuffer +
			audDrv->blockIndex * audDrv->blockSize;
		csl_caph_dma_set_hibuffer_address(chnl, addr);

	} else if ((fifo_status & CSL_CAPH_READY_LOW) == CSL_CAPH_READY_NONE) {
		buffer_status |= CSL_CAPH_READY_LOW;

		spin_lock_irqsave(&audDrv->readyStatusLock, flags);
		audDrv->readyBlockStatus |= CSL_CAPH_READY_LOW;
		spin_unlock_irqrestore(&audDrv->readyStatusLock, flags);

		spin_lock_irqsave(&audDrv->configLock, flags);
		if (audDrv->dmaCB)
			audDrv->dmaCB(audDrv->streamID, buffer_status);
		spin_unlock_irqrestore(&audDrv->configLock, flags);

		audDrv->blockIndex++;
		if (audDrv->blockIndex >= audDrv->numBlocks)
			audDrv->blockIndex = 0;
		addr = audDrv->ringBuffer +
			audDrv->blockIndex * audDrv->blockSize;
		csl_caph_dma_set_lobuffer_address(chnl, addr);
	}
}
#endif

/* ==========================================================================
//
// Function Name: GetStreamIDByDmaCH
//
// Description: Get the audio streamID from the dma channel.
//
// =========================================================================*/
static CSL_CAPH_STREAM_e GetStreamIDByDmaCH(CSL_CAPH_DMA_CHNL_e dmaCH)
{
	CSL_CAPH_STREAM_e streamID = CSL_CAPH_STREAM_NONE;
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	UInt32 i = 0;

	for (i = 0; i < CSL_CAPH_STREAM_TOTAL; i++) {
		audDrv = GetRenderDriverByType(i);
		if (audDrv != NULL) {
			/* aTrace(LOG_AUDIO_CSL,
			   "GetStreamIDByDmaCH::audDrv->dmaCH = %d,
			   dmaCH = %d, i = %d\n",
			   audDrv->dmaCH, dmaCH, i);
			 */

			if (audDrv->dmaCH == dmaCH) {
				streamID = audDrv->streamID;
				break;
			}
		}
	}

	return streamID;
}

/* ==========================================================================
//
// Function Name: csl_audio_render_get_current_position
//
// Description: Get the current render position for this streamID
//
// =========================================================================*/
UInt16 csl_audio_render_get_current_position(UInt32 streamID)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	if (streamID != CSL_CAPH_STREAM_NONE)
		audDrv = &sRenderDrv[streamID];
	else
		return 0;

	if (audDrv->dmaCH != CSL_CAPH_DMA_NONE)
		return csl_caph_dma_read_currmempointer(audDrv->dmaCH);
	else
		return 0;
}

/* ==========================================================================
//
// Function Name: csl_audio_render_get_current_buffer
//
// Description: Get the current render buffer for this streamID
//
// =========================================================================*/
UInt16 csl_audio_render_get_current_buffer(UInt32 streamID)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	if (streamID != CSL_CAPH_STREAM_NONE)
		audDrv = &sRenderDrv[streamID];
	else
		return 0;

	if (audDrv->dmaCH != CSL_CAPH_DMA_NONE)
		return csl_caph_dma_check_dmabuffer(audDrv->dmaCH);
	else
		return 0;
}

/* ==========================================================================
//
// Description: Configure dma size
//
// =========================================================================*/
void csl_audio_render_set_dma_size(int num_blocks, int stream)
{
	int temp;
	CSL_CAPH_Render_Drv_t *audDrv = NULL;
	if (num_blocks != 2 && num_blocks != 8) {
		aTrace(LOG_AUDIO_CSL, "%s num_blocks %d\n",
			__func__, num_blocks);
		return;
	}
	BUG_ON(stream < CSL_CAPH_STREAM_NONE ||
		stream >= CSL_CAPH_STREAM_TOTAL);

	audDrv = GetRenderDriverByType(stream);
	if (audDrv == NULL) {
		aError("%s audDrv "
		"is NULL stream %d",
		__func__, stream);
		return;
	}

	temp = atomic_read(&audDrv->renderNumBlocks);
	if (temp == num_blocks)
		return;

	atomic_set(&audDrv->renderNumBlocks, num_blocks);
	if (atomic_read(&audDrv->dmaState) !=
		DYNDMA_NORMAL) {
		/*aTrace(LOG_AUDIO_CSL,"switch is in"
		"progress, numDMABlocks %d", num_blocks);*/
		atomic_set(&audDrv->numDMABlocks, num_blocks);
	}
	aTrace(LOG_AUDIO_CSL, "%s num_blocks %d->%d\n",
		__func__, temp, num_blocks);
}

int csl_audio_render_get_num_blocks(int stream)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	BUG_ON(stream < CSL_CAPH_STREAM_NONE ||
		stream >= CSL_CAPH_STREAM_TOTAL);
	if (stream == 0)
		/* default value */
		return DYNDMA_NUM_BLOCKS;

	audDrv = GetRenderDriverByType(stream);
	if (audDrv == NULL) {
			aError("%s audDrv "
			"is NULL stream %d",
			__func__, stream);
			return 0;
	}
	return atomic_read(&audDrv->renderNumBlocks);
}

int csl_audio_render_get_dma_size(int stream)
{
	CSL_CAPH_Render_Drv_t *audDrv = NULL;

	BUG_ON(stream < CSL_CAPH_STREAM_NONE ||
		stream >= CSL_CAPH_STREAM_TOTAL);
	if (stream == 0)
		return 0;

	audDrv = GetRenderDriverByType(stream);
	if (audDrv == NULL) {
			aError("%s audDrv "
			"is NULL stream %d",
			__func__, stream);
			return 0;
	}
	return audDrv->dynDmaSize;
}
