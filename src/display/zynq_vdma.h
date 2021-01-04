#ifndef ZYNQ_VDMA_H_INCLUDED
#define ZYNQ_VDMA_H_INCLUDED

#include <src/zynq/xaxivdma.h>
#include <src/zynq/xdebug.h>
#include <src/zynq/xil_cache.h>
#include <src/zynq/xil_exception.h>
#include <src/zynq/xil_types.h>
#include <src/zynq/xparameters.h>
#include <src/zynq/xscugic.h>
#include <src/zynq/xvtc.h>
#include "hardware.h"	/* зависящие от процессора функции работы с портами */

typedef struct {
	char label[64];  /* Label describing the resolution */
	uint_fast32_t width;		 /* Width(horizon) of the active video frame */
	uint_fast32_t height; 	 /* Height(vertical) of the active video frame */
	uint_fast32_t hps; 		 /* Start time of Horizontal sync pulse, in pixel clocks (active width + H. front porch) */
	uint_fast32_t hpe; 		 /* End time of Horizontal sync pulse, in pixel clocks (active width + H. front porch + H. sync width) */
	uint_fast32_t hmax; 		 /* Total number of pixel clocks per line (active width + H. front porch + H. sync width + H. back porch) */
	uint_fast32_t hpol; 		 /* hsync pulse polarity */
	uint_fast32_t vps;		 /* Start time of Vertical sync pulse, in lines (active height + V. front porch) */
	uint_fast32_t vpe;		 /* End time of Vertical sync pulse, in lines (active height + V. front porch + V. sync width) */
	uint_fast32_t vmax; 		 /* Total number of lines per frame (active height + V. front porch + V. sync width + V. back porch) */
	uint_fast32_t vpol; 		 /* vsync pulse polarity */
	double freq; 	 /* Pixel Clock frequency */
} VideoMode;

static const VideoMode VMODE_800x480 = {
	.label = "800x480@50Hz",
	.width = 800,
	.height = 480,
	.hps = 800 + 210,
	.hpe = 800 + 210 + 40,
	.hmax = 800 + 210 + 40 + 46,
	.hpol = 0,
	.vps = 480 + 22,
	.vpe = 480 + 22 + 10,
	.vmax = 480 + 22 + 10 + 23,
	.vpol = 0,
	.freq = 50
};

typedef enum {
	DISPLAY_STOPPED = 0,
	DISPLAY_RUNNING = 1
} DisplayState;

#define DISPLAY_NUM_FRAMES		3

typedef struct {
	u32 dynClkAddr; 		  		  /* Physical Base address of the dynclk core */
	XAxiVdma *vdma; 				  /* VDMA driver struct */
	XAxiVdma_DmaSetup vdmaConfig;     /* VDMA channel configuration */
	XVtc vtc; 					      /* VTC driver struct */
	VideoMode vMode; 				  /* Current video mode */
	u8 *framePtr[DISPLAY_NUM_FRAMES]; 		  /* Array of pointers to the frame buffers */
	u32 stride; 					  /* The line stride of the frame buffers, in bytes */
	double pxlFreq; 				  /* Frequency of clock currently being generated, maybe not exactly with vMode.freq */
	u32 curFrame; 					  /* Current frame being displayed */
	DisplayState state; 			  /* Indicates if the Display is currently running */
} DisplayCtrl;

int DisplayStop(DisplayCtrl *dispPtr);
int DisplayStart(DisplayCtrl *dispPtr);
int DisplayInitialize(DisplayCtrl *dispPtr, XAxiVdma *vdma, u16 vtcId, u32 dynClkAddr, u8 *framePtr[DISPLAY_NUM_FRAMES], u32 stride, VideoMode VMODE);
int DisplaySetMode(DisplayCtrl *dispPtr, const VideoMode *newMode);
int DisplayChangeFrame(DisplayCtrl *dispPtr, u32 frameIndex);


#define AXI_VDMA_DEV_ID		XPAR_AXIVDMA_0_DEVICE_ID

#define VDMA_INTR_ID		XPAR_FABRIC_AXI_VDMA_0_MM2S_INTROUT_INTR


#define IMAGE_WIDTH     	800
#define IMAGE_HEIGHT		480
#define BYTES_PER_PIXEL		4

#define NUMBER_OF_READ_FRAMES  1


#define MEM_BASE_ADDR		0x5000000
#define BUFFER0_BASE		(MEM_BASE_ADDR)
#define BUFFER1_BASE		(MEM_BASE_ADDR +     IMAGE_WIDTH * IMAGE_HEIGHT * BYTES_PER_PIXEL)
#define BUFFER2_BASE		(MEM_BASE_ADDR + 2 * IMAGE_WIDTH * IMAGE_HEIGHT * BYTES_PER_PIXEL)


XAxiVdma AxiVdma;

//void Vdma_Setup_Intr_System(XScuGic *GicInstancePtr, XAxiVdma *InstancePtr, u16 IntrId);
void ReadErrorCallBack(void *CallbackRef, u32 Mask);
void Vdma_Init(XAxiVdma *InstancePtr, u32 DeviceId);
int ReadSetup(XAxiVdma *InstancePtr);
int Vdma_Start(XAxiVdma *InstancePtr);







#endif /* ZYNQ_VDMA_H_INCLUDED */
