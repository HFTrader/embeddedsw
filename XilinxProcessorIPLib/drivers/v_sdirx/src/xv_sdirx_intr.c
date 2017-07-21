/******************************************************************************
*
* Copyright (C) 2017 Xilinx, Inc. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xv_sdirx_intr.c
*
* This file contains interrupt related functions for Xilinx SDI RX core.
* Please see xv_sdirx.h for more details of the driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver	Who	   Date		Changes
* ----- ------ -------- --------------------------------------------------
* 1.0	jsr    07/17/17 Initial release.
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xv_sdirx.h"

/************************** Constant Definitions *****************************/


/***************** Macros (Inline Functions) Definitions *********************/


/**************************** Type Definitions *******************************/


/************************** Function Prototypes ******************************/

static void SdiRx_VidLckIntrHandler(XV_SdiRx *InstancePtr);
static void SdiRx_VidUnLckIntrHandler(XV_SdiRx *InstancePtr);

/************************** Variable Definitions *****************************/


/************************** Function Definitions *****************************/

/*****************************************************************************/
/**
*
* This function is the interrupt handler for the SDI RX driver.
*
* This handler reads the pending interrupt for video lock or video unlock
* interrupts, determines the source of the interrupts, clears the
* interrupts and calls callbacks accordingly.
*
* The application is responsible for connecting this function to the interrupt
* system. Application beyond this driver is also responsible for providing
* callbacks to handle interrupts and installing the callbacks using
* XV_SdiRx_SetCallback() during initialization phase. An example delivered
* with this driver demonstrates how this could be done.
*
* @param	InstancePtr is a pointer to the XV_SdiRx instance that just
*		interrupted.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XV_SdiRx_IntrHandler(void *InstancePtr)
{
	u32 Data;
	XV_SdiRx *SdiRxPtr = (XV_SdiRx *)InstancePtr;

	/* Verify arguments */
	Xil_AssertVoid(SdiRxPtr != NULL);
	Xil_AssertVoid(SdiRxPtr->IsReady == XIL_COMPONENT_IS_READY);

	/* Video Lock */
	Data = XV_SdiRx_ReadReg(SdiRxPtr->Config.BaseAddress,
	(XV_SDIRX_INT_STS_OFFSET));

	/* Check for IRQ flag set */
	if (Data & XV_SDIRX_INT_STS_VID_LOCK_MASK) {
		/* Clear event flag */
		XV_SdiRx_WriteReg(SdiRxPtr->Config.BaseAddress,
					(XV_SDIRX_INT_CLR_OFFSET),
					(XV_SDIRX_INT_STS_VID_LOCK_MASK));
		XV_SdiRx_WriteReg(SdiRxPtr->Config.BaseAddress,
					(XV_SDIRX_INT_CLR_OFFSET), 0x0);
		/* Jump to Video lock interrupt handler */
		SdiRx_VidLckIntrHandler(SdiRxPtr);
	}

	/* Check for IRQ flag set */
	if (Data & XV_SDIRX_INT_STS_VID_UNLOCK_MASK) {
		/* Clear event flag */
		XV_SdiRx_WriteReg(SdiRxPtr->Config.BaseAddress,
					(XV_SDIRX_INT_CLR_OFFSET),
					(XV_SDIRX_INT_STS_VID_UNLOCK_MASK));
		XV_SdiRx_WriteReg(SdiRxPtr->Config.BaseAddress,
					(XV_SDIRX_INT_CLR_OFFSET), 0x0);
		/* Jump to Video unlock interrupt handler */
		SdiRx_VidUnLckIntrHandler(SdiRxPtr);
	}
}

/*****************************************************************************/
/**
*
* This function installs an asynchronous callback function for the given
* HandlerType:
*
* <pre>
* HandlerType				Callback Function Type
* -------------------------		---------------------------------------
* (XV_SDIRX_HANDLER_STREAM_DOWN)	StreamDownCallback
* (XV_SDIRX_HANDLER_STREAM_UP)		StreamUpCallback
* </pre>
*
* @param	InstancePtr is a pointer to the SDI RX core instance.
* @param	HandlerType specifies the type of handler.
* @param	CallbackFunc is the address of the callback function.
* @param	CallbackRef is a user data item that will be passed to the
*		callback function when it is invoked.
*
* @return
*		- XST_SUCCESS if callback function installed successfully.
*		- XST_INVALID_PARAM when HandlerType is invalid.
*
* @note		Invoking this function for a handler that already has been
*		installed replaces it with the new handler.
*
******************************************************************************/
int XV_SdiRx_SetCallback(XV_SdiRx *InstancePtr, u32 HandlerType,
				void *CallbackFunc, void *CallbackRef)
{
	u32 Status;

	/* Verify arguments. */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(HandlerType >= (XV_SDIRX_HANDLER_STREAM_DOWN));
	Xil_AssertNonvoid(CallbackFunc != NULL);
	Xil_AssertNonvoid(CallbackRef != NULL);

	/* Check for handler type */
	switch (HandlerType) {
	/* Stream down */
	case (XV_SDIRX_HANDLER_STREAM_DOWN):
		InstancePtr->StreamDownCallback = (XV_SdiRx_Callback)CallbackFunc;
		InstancePtr->StreamDownRef = CallbackRef;
		InstancePtr->IsStreamDownCallbackSet = (TRUE);
		Status = (XST_SUCCESS);
		break;

	/* Stream up */
	case (XV_SDIRX_HANDLER_STREAM_UP):
		InstancePtr->StreamUpCallback = (XV_SdiRx_Callback)CallbackFunc;
		InstancePtr->StreamUpRef = CallbackRef;
		InstancePtr->IsStreamUpCallbackSet = (TRUE);
		Status = (XST_SUCCESS);
		break;

	default:
		Status = (XST_INVALID_PARAM);
		break;
	}

	return Status;
}

/*****************************************************************************/
/**
*
* This function enables the selected interrupt.
*
* @param	InstancePtr is a pointer to the XV_SdiRx core instance.
* @param	Interrupt to be enabled.
*
* @return	None.
*
* @note         None.
*
******************************************************************************/
void XV_SdiRx_IntrEnable(XV_SdiRx *InstancePtr, u32 Interrupt)
{
	u32 Data;

	Data = XV_SdiRx_ReadReg(InstancePtr->Config.BaseAddress,
				(XV_SDIRX_INT_MSK_OFFSET));
	Data &= ~Interrupt;

	XV_SdiRx_WriteReg((InstancePtr)->Config.BaseAddress,
				(XV_SDIRX_INT_MSK_OFFSET), (Data));
}

/*****************************************************************************/
/**
*
* This function disables the selected interrupt.
*
* @param	InstancePtr is a pointer to the XV_SdiRx core instance.
* @param	Interrupt to be disabled.
*
* @return	None.
*
* @note         None.
*
******************************************************************************/
void XV_SdiRx_IntrDisable(XV_SdiRx *InstancePtr, u32 Interrupt)
{
	u32 Data;

	Data = XV_SdiRx_ReadReg(InstancePtr->Config.BaseAddress,
				(XV_SDIRX_INT_MSK_OFFSET));
	Data |= Interrupt;

	XV_SdiRx_WriteReg((InstancePtr)->Config.BaseAddress,
				(XV_SDIRX_INT_MSK_OFFSET), (Data));
}

/*****************************************************************************/
/**
*
* This function is the interrupt handler for the SDI Video Lock Event.
*
* @param	InstancePtr is a pointer to the XV_SdiRx core instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void SdiRx_VidLckIntrHandler(XV_SdiRx *InstancePtr)
{
	XVidC_VideoStream *SdiStream = &InstancePtr->Stream[0].Video;
	XVidC_VideoTiming const *Timing;
	XVidC_FrameRate FrameRate;
	u32 Data0 = 0;
	u32 Data1 = 0;

	Data0 = XV_SdiRx_ReadReg(InstancePtr->Config.BaseAddress,
				(XV_SDIRX_MODE_DET_STS_OFFSET));
	Data1 = XV_SdiRx_ReadReg(InstancePtr->Config.BaseAddress,
				(XV_SDIRX_TS_DET_STS_OFFSET));

	if(((Data0 & XV_SDIRX_MODE_DET_STS_MODE_LOCKED_MASK)
			== XV_SDIRX_MODE_DET_STS_MODE_LOCKED_MASK)
		&& ((Data1 & XV_SDIRX_TS_DET_STS_T_LOCKED_MASK)
			== XV_SDIRX_TS_DET_STS_T_LOCKED_MASK)) {
		InstancePtr->Transport.IsLevelB3G = (Data0
			& XV_SDIRX_MODE_DET_STS_LVL_B_3G_MASK)
			>> XV_SDIRX_MODE_DET_STS_LVL_B_3G_SHIFT;
		InstancePtr->Transport.TMode = Data0 & XV_SDIRX_MODE_DET_STS_MODE_MASK;

		if(InstancePtr->Transport.TMode > XSDIVID_MODE_12G) {
			InstancePtr->Transport.TMode = XSDIVID_MODE_12G;
		}

		InstancePtr->Transport.ActiveStreams
			= (Data0 & XV_SDIRX_MODE_DET_STS_ACT_STRM_MASK)
				>> XV_SDIRX_MODE_DET_STS_ACT_STRM_SHIFT;

		InstancePtr->Transport.TScan
			= (Data1 & XV_SDIRX_TS_DET_STS_T_SCAN_MASK)
				>> XV_SDIRX_TS_DET_STS_T_SCAN_SHIFT;

		InstancePtr->Transport.TFamily
			= (Data1 & XV_SDIRX_TS_DET_STS_T_FAMILY_MASK)
				>> XV_SDIRX_TS_DET_STS_T_FAMILY_SHIFT;

		InstancePtr->Transport.TRate
			= (Data1 & XV_SDIRX_TS_DET_STS_T_RATE_MASK)
				>> XV_SDIRX_TS_DET_STS_T_RATE_SHIFT;

		Data0 = XV_SdiRx_ReadReg(InstancePtr->Config.BaseAddress,
					(XV_SDIRX_STS_SB_RX_TDATA_OFFSET));
		InstancePtr->Transport.IsFractional
			= (Data0 & XV_SDIRX_STS_SB_RX_TDATA_SDICTRL_BIT_RATE_MASK)
				>> XV_SDIRX_STS_SB_RX_TDATA_SDICTRL_BIT_RATE_SHIFT;

		/* Toggle reset on Stat_reset register */
		XV_SdiRx_WriteReg((InstancePtr)->Config.BaseAddress,
					(XV_SDIRX_ST_RST_OFFSET),
					(XV_SDIRX_ST_RST_CLR_ERR_MASK
					| XV_SDIRX_ST_RST_CLR_EDH_MASK));

		XV_SdiRx_WriteReg((InstancePtr)->Config.BaseAddress,
					(XV_SDIRX_ST_RST_OFFSET), 0x0);

		for(int StreamId = 0; StreamId < XV_SDIRX_MAX_DATASTREAM; StreamId++) {
			InstancePtr->Stream[StreamId].PayloadId
				= XV_SdiRx_GetPayloadId(InstancePtr, StreamId);
		}

		SdiStream->PixPerClk = XVIDC_PPC_2;
		SdiStream->ColorDepth = XVIDC_BPC_10;
		SdiStream->ColorFormatId = XVIDC_CSF_YCRCB_422;
		SdiStream->IsInterlaced = FALSE;
		SdiStream->VmId = XVIDC_VM_NOT_SUPPORTED;

		if(InstancePtr->Transport.IsFractional) {
			switch(InstancePtr->Transport.TRate) {
			case XV_SDIRX_FR_23_98HZ: FrameRate = XVIDC_FR_24HZ; break;
			case XV_SDIRX_FR_47_95HZ: FrameRate = XVIDC_FR_48HZ; break;
			case XV_SDIRX_FR_29_97HZ: FrameRate = XVIDC_FR_30HZ; break;
			case XV_SDIRX_FR_59_94HZ: FrameRate = XVIDC_FR_60HZ; break;
			default: FrameRate = XVIDC_FR_60HZ; break;
			}
		} else {
			switch(InstancePtr->Transport.TRate) {
			case XV_SDIRX_FR_24HZ: FrameRate = XVIDC_FR_24HZ; break;
			case XV_SDIRX_FR_25HZ: FrameRate = XVIDC_FR_25HZ; break;
			case XV_SDIRX_FR_30HZ: FrameRate = XVIDC_FR_30HZ; break;
			case XV_SDIRX_FR_48HZ: FrameRate = XVIDC_FR_48HZ; break;
			case XV_SDIRX_FR_50HZ: FrameRate = XVIDC_FR_50HZ; break;
			case XV_SDIRX_FR_60HZ: FrameRate = XVIDC_FR_60HZ; break;
			default: FrameRate = XVIDC_FR_60HZ; break;
			}
		}

		switch(InstancePtr->Transport.TMode) {
		case XV_SDIRX_MODE_SD:
			if(InstancePtr->Transport.TFamily == XV_SDIRX_NTSC) {
				SdiStream->VmId =  XVIDC_VM_720x480_60_I;
				FrameRate = XVIDC_FR_60HZ;

			} else {
				SdiStream->VmId =  XVIDC_VM_720x576_50_I;
				FrameRate = XVIDC_FR_50HZ;
			}
			SdiStream->IsInterlaced = TRUE;
			break;


		case XV_SDIRX_MODE_HD:
			switch(FrameRate) {
			case XVIDC_FR_24HZ:
				if(InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_296) {
					SdiStream->VmId = XVIDC_VM_1280x720_24_P;
				} else if(InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_2048_2) {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_2048x1080_24_P :
							XVIDC_VM_2048x1080_48_I);
				} else {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_1920x1080_24_P :
							XVIDC_VM_1920x1080_48_I);
				}
				SdiStream->IsInterlaced
					= (~InstancePtr->Transport.TScan)
						& 0x1;
				break;

			case XVIDC_FR_25HZ:
				if(InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_296) {
					SdiStream->VmId = XVIDC_VM_1280x720_25_P;
				} else if(InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_2048_2) {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_2048x1080_25_P :
							XVIDC_VM_2048x1080_50_I);
				} else {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_1920x1080_25_P :
							XVIDC_VM_1920x1080_50_I);
				}
				SdiStream->IsInterlaced = (~InstancePtr->Transport.TScan)
								& 0x1;
				break;

			case XVIDC_FR_30HZ:
				if(InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_296) {
					SdiStream->VmId = XVIDC_VM_1280x720_30_P;
				} else if(InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_2048_2) {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_2048x1080_30_P :
							XVIDC_VM_2048x1080_60_I);
				} else {
					SdiStream->VmId = ((InstancePtr->Transport.TScan) ?
							XVIDC_VM_1920x1080_30_P :
							XVIDC_VM_1920x1080_60_I);

				}
				SdiStream->IsInterlaced = (~InstancePtr->Transport.TScan)
								& 0x1;
				break;

			case XVIDC_FR_50HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
							== XV_SDIRX_SMPTE_ST_274) ?
							XVIDC_VM_1920x1080_50_P :
							XVIDC_VM_1280x720_50_P);
				break;

			case XVIDC_FR_60HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
						== XV_SDIRX_SMPTE_ST_274) ?
						XVIDC_VM_1920x1080_60_P :
						XVIDC_VM_1280x720_60_P);
				break;

			default:
				SdiStream->VmId = XVIDC_VM_1920x1080_60_P;
				break;
			}
			break;

		case XV_SDIRX_MODE_3G:
			if (InstancePtr->Transport.IsLevelB3G) {
				switch(FrameRate) {
				case XVIDC_FR_24HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_96_I : XVIDC_VM_1920x1080_96_I); break;
				case XVIDC_FR_25HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_100_I : XVIDC_VM_1920x1080_100_I); break;
				case XVIDC_FR_30HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_120_I : XVIDC_VM_1920x1080_120_I); break;
				default:
					SdiStream->VmId = XVIDC_VM_1920x1080_120_I; break;
				}
			} else {
				switch(FrameRate) {
				case XVIDC_FR_24HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_24_P : XVIDC_VM_1920x1080_24_P); break;
				case XVIDC_FR_25HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_25_P : XVIDC_VM_1920x1080_25_P); break;
				case XVIDC_FR_30HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_30_P : XVIDC_VM_1920x1080_30_P); break;
				case XVIDC_FR_48HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_48_P : XVIDC_VM_1920x1080_48_P); break;
				case XVIDC_FR_50HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_50_P : XVIDC_VM_1920x1080_50_P); break;
				case XVIDC_FR_60HZ:
					SdiStream->VmId = ((InstancePtr->Transport.TFamily == XV_SDIRX_SMPTE_ST_2048_2) ?
					XVIDC_VM_2048x1080_60_P : XVIDC_VM_1920x1080_60_P); break;
				default:
					SdiStream->VmId = XVIDC_VM_1920x1080_60_P; break;
				}
			}

			SdiStream->IsInterlaced = (~InstancePtr->Transport.TScan) & 0x1;
			break;

		case XV_SDIRX_MODE_6G:
			switch(FrameRate) {
			case XVIDC_FR_24HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_24_P
					: XVIDC_VM_3840x2160_24_P);
				break;
			case XVIDC_FR_25HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_25_P
					: XVIDC_VM_3840x2160_25_P);
				break;
			case XVIDC_FR_30HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_30_P
					: XVIDC_VM_3840x2160_30_P);
				break;
			default:
				SdiStream->VmId = XVIDC_VM_3840x2160_30_P; break;
			}
			break;

		case XV_SDIRX_MODE_12G:
			switch(FrameRate) {
			case XVIDC_FR_48HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_48_P :
					XVIDC_VM_3840x2160_48_P);
				break;
			case XVIDC_FR_50HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_50_P :
					XVIDC_VM_3840x2160_50_P);
				break;

			case XVIDC_FR_60HZ:
				SdiStream->VmId = ((InstancePtr->Transport.TFamily
					== XV_SDIRX_SMPTE_ST_2048_2) ? XVIDC_VM_4096x2160_60_P :
					XVIDC_VM_3840x2160_60_P);
				break;

			default:
				SdiStream->VmId = XVIDC_VM_3840x2160_60_P;
				break;
			}
			break;

		default:
			/* Unknown video format */
			break;
		}

		if(SdiStream->VmId < XVIDC_VM_NUM_SUPPORTED) {
			Timing = XVidC_GetTimingInfo(SdiStream->VmId);
			SdiStream->Timing = *Timing;
		}

		/* Call stream up callback */
		if (InstancePtr->IsStreamUpCallbackSet) {
			InstancePtr->StreamUpCallback(InstancePtr->StreamUpRef);
		}
	} else {
		/* WARNING: rx_mode_locked and rx_t_locked are not locked at the same
		 * time when IRQ!
		 */
	}
}

/*****************************************************************************/
/**
*
* This function is the interrupt handler for the SDI Video Unlock Event.
*
* @param	InstancePtr is a pointer to the XV_SdiRx core instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void SdiRx_VidUnLckIntrHandler(XV_SdiRx *InstancePtr)
{
	/* Assert reset */

	/* Clear variables */
	XV_SdiRx_ResetStream(InstancePtr);

	/* Call stream up callback */
	if (InstancePtr->IsStreamDownCallbackSet) {
		InstancePtr->StreamDownCallback(InstancePtr->StreamDownRef);
	}
}