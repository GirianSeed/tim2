#include <eekernel.h>
#include <eeregs.h>
#include <sifdev.h>
#include <libgraph.h>
#include <libdma.h>
#include <libpkt.h>
#include <libdev.h>

#include <stdio.h>
#include <stdlib.h>


// definition of TIM2 file header structure
#include "tim2.h"

// additional portable data types
typedef unsigned char   u_char8;
typedef unsigned short  u_short16;
typedef unsigned int    u_short32;
typedef unsigned int    u_int32;
typedef unsigned int    u_long32;
typedef unsigned long   u_long64;


// scratch pad for DMA
#define ADR_DMA_SPR(_p)			(u_int *)(((unsigned int)(_p)&0x0FFFFFFF)|0x80000000)

// physical address for DMA
#define ADR_DMA_PERIPH(_p)		(u_int *)(((unsigned int)(_p)&0x0FFFFFFF)|0x00000000)

// uncached are for CPU
#define ADR_UNCACHED(_p)		(void *)(((unsigned int)(_p)&0x0FFFFFFF)|0x20000000)



#define GIF_PACKET_SIZE		65536		// size fo GIF packet buffer

// definition of screen size
#define SCREEN_WIDTH	640				// screen width
#define SCREEN_HEIGHT	448				// screen height

#define BASE_TBP		10240			// VRAM top address for texture buffer
#define BASE_CBP		(16384 - 32)	// VRAM top address for CLUT





// prototypes
static void InitSystem(void);
static void SendTIM2(void);
static void DrawImage(int odev, u_short tbp, u_short cbp);
static void FlushTextureCache(void);
static int  GetLog2(u_long32 n);

// static void BUG_WALKAROUND(sceGifPacket *pkt);


// global variables
static char GifPktbuf[GIF_PACKET_SIZE] __attribute__((aligned (64)));	// GIF packet buffer
static char g_Tim2Buffer[4*1024*1024];		// TIM2 data buffer

static sceGsDBuff g_db;						// double buffer information
static sceGifPacket g_PkGif;				// GIF packet handler
static sceDmaChan *g_DmaGif;				// DMA gif channel

static u_char  g_bInterlace;				// interlace flag

static sceGsTex0 g_GsTex0;					// TEX0 register value in TIM2

static u_int    g_nTexSizeW, g_nTexSizeH;	// TIM2 texture size

static u_char	g_bMipmapLv;				// max MIPMAP level in TIM2 (0-6)
static u_long64 g_GsMiptbp1;				// MIPTBP1 register value in TIM2
static u_long64 g_GsMiptbp2;				// MIPTBP2 register value in TIM2

static u_int   g_nLod = 0;					// LOD value for MIPMAP
static u_int   g_nFilter = 2;				// filter mode for MIPMAP
static u_int   g_nClutNo = 0;				// CLUT number


int main(int argc, char *argv[])
{
	u_int fr;					// frame counter

	// hardware initialization, etc.
	InitSystem();


	// read TIM2 file
	SendTIM2();

	// wait till next odd field
	while(sceGsSyncV(0)==0);

	// main loop
	fr = 0;						// double buffer management for frame
	while(1) {
		// pixel adjustment for interlace
		int odev;

		odev = sceGsSyncV(0) ^ 1;
		*T0_COUNT = 0;									// reset HSYNC counter
		if(fr & 1) {
			sceGsSetHalfOffset(&g_db.draw1, 2048, 2048, odev);
		} else {
			sceGsSetHalfOffset(&g_db.draw0, 2048, 2048, odev);
		}
		sceGsSwapDBuff(&g_db, fr);

		// draw TIM2 image into VRAM
		DrawImage(odev, BASE_TBP, BASE_CBP);

		fr++;
	};
	return(0);
}




// load TIM2 image into VRAM
static void SendTIM2(void)
{
	int h;

	// load TIM2 file from target
	h  = sceOpen("sim:test.tm2", SCE_RDONLY);
	sceRead(h, g_Tim2Buffer, sizeof(g_Tim2Buffer));
	sceClose(h);

	if(!Tim2CheckFileHeaer(g_Tim2Buffer)) {
		// broken TIM2 file
	} else {
		// get picture header
		TIM2_PICTUREHEADER *ph;
		ph = Tim2GetPictureHeader(g_Tim2Buffer, 0);
		if(!ph) {
			// error in getting picture header
		} else {
			// load picture data into VRAM
			Tim2LoadPicture(ph, BASE_TBP, BASE_CBP);

			// get some information from picture header
			g_GsTex0 = *(sceGsTex0 *)&ph->GsTex0;		// get TEX0 register value from picture header
			g_nTexSizeW = ph->ImageWidth;				// picture size X
			g_nTexSizeH = ph->ImageHeight;				// picture size Y

			// MEMO: correct TW, TH members
			// following operation is not requested if the values are correctly set into GsTex0 member of picture header
			// improper for sample program, might to be removed
			g_GsTex0.TW = GetLog2(g_nTexSizeW);
			g_GsTex0.TH = GetLog2(g_nTexSizeH);


			// get MIPMAP level
			g_bMipmapLv = ph->MipMapTextures - 1;		// MIXMAP level
			if(g_bMipmapLv) {
				// use MIPMAP
				TIM2_MIPMAPHEADER *pm;
				pm = (TIM2_MIPMAPHEADER *)(ph + 1);		// MIPMAP header just after picture header
				g_GsMiptbp1 = pm->GsMiptbp1;
				g_GsMiptbp2 = pm->GsMiptbp2;
			} else {
				// don't use MIPMAP
				g_GsMiptbp1 = 0;
				g_GsMiptbp2 = 0;
			}

			// flush texture cache
			FlushTextureCache();
		}
	}
	return;
}






// hardware initialization, etc.
static void InitSystem(void)
{
	sceDmaReset(1);						// start after DMA reset
	sceGsResetPath();					// reset VIF1, VU1, GIF
	sceDevVif0Reset();					// reset VIF0
	sceDevVu0Reset();					// reset VU0

	*T0_MODE = T_MODE_CLKS_M | T_MODE_CUE_M;	// initialize HSYNC counter

	// environment set for DMA
	{
		sceDmaEnv env;							// DMA common environment

		sceDmaGetEnv(&env);						// get DMA common environment
		env.notify = 1<<SCE_DMA_GIF;			// notify only GIF channel
		sceDmaPutEnv(&env);						// set DMA common environment
	}

	// get register address of GIF channel
	g_DmaGif = sceDmaGetChan(SCE_DMA_GIF);

	// initialize GIF packet
	sceGifPkInit(&g_PkGif, (u_long128 *)GifPktbuf);

	// setup double buffer
	sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);	// initialize GS
	sceGsSetDefDBuff(&g_db, SCE_GS_PSMCT32, SCREEN_WIDTH, SCREEN_HEIGHT/2,
							0, 0, 1);

	g_bInterlace = 1;

	// set clear color for double buffer
	g_db.clear0.rgbaq.R = 0x0;			// clear color for dbuf0
	g_db.clear0.rgbaq.G = 0x80;
	g_db.clear0.rgbaq.B = 0x80;
	g_db.clear1.rgbaq.R = 0x0;			// clear color for dbuf1
	g_db.clear1.rgbaq.G = 0x80;
	g_db.clear1.rgbaq.B = 0x80;

	// don't use alpha test
	g_db.clear0.testb.ATE  = 0;
	g_db.clear1.testb.ATE  = 0;


	// create GS initialize packet and send it
	{
		// GIF tag for A+D
		const u_long64 giftagAD[2] = {
			SCE_GIF_SET_TAG(0, 1, 0, 0, 0, 1),
			0x000000000000000EL
		};
		int n;

		sceGifPkReset(&g_PkGif);						// initialize GIF packet

		sceGifPkCnt(&g_PkGif, 0, 0, 0);
		sceGifPkOpenGifTag(&g_PkGif, *(u_long128*)&giftagAD);

		// add alpha environment packet (no alpha)
		n = sceGsSetDefAlphaEnv((sceGsAlphaEnv *)g_PkGif.pCurrent, 0);
		sceGifPkReserve(&g_PkGif, n*4);

		sceGifPkCloseGifTag(&g_PkGif);
		sceGifPkEnd(&g_PkGif, 0, 0, 0);

		sceGifPkTerminate(&g_PkGif);					// close GIF packet

		// send GS initialize packet, and wait till done
	//	BUG_WALKAROUND(&g_PkGif);
		FlushCache(WRITEBACK_DCACHE);				// flush D cache
		sceDmaSend(g_DmaGif, ADR_DMA_PERIPH(g_PkGif.pBase));
		sceGsSyncPath(0, 0);						// wait till transfer complete
	}
	return;
}


// drawing operation
static void DrawImage(int odev, u_short tbp, u_short cbp)
{
	const float fog = 1.0f;

	// intialize GIF packet
	sceGifPkReset(&g_PkGif);								// reset GIF packet
	sceGifPkEnd(&g_PkGif, 0, 0, 0);							// set DMA tag

	// draw sprite
	{
		// GIF tag for sprite
		const u_long64 giftagSPR[2] = {
			SCE_GIF_SET_TAG(0, 1, 0, 0, SCE_GIF_REGLIST, 8),
			0x00000000F4343810L					// PRIM,RGBAQ,CLAMP_1,UV,XYZF2,UV,XYZF2
		};
		const u_long64 giftagAD[2] = {
			SCE_GIF_SET_TAG(0, 1, 0, 0, SCE_GIF_PACKED, 1),		// GIF tag for A+D
			0x000000000000000EL
		};

		int tw, th;
		int dx, dy, dw, dh;


		// calculate sprite size for draw
		tw = g_nTexSizeW;
		if(SCREEN_WIDTH<g_nTexSizeW) {
			tw = SCREEN_WIDTH;
		}
		dx = (2048 - tw/2)<<4;
		tw = tw<<4;
		dw = tw;

		th = g_nTexSizeH;
		if(SCREEN_HEIGHT<g_nTexSizeH) {
			th = SCREEN_HEIGHT;
		}
		if(g_bInterlace) {
			dy = (2048 - SCREEN_HEIGHT/4)<<4;
			th = th<<4;
			dh = th/2;
		} else {
			dy = (2048 - SCREEN_HEIGHT/2)<<4;
			th = th<<4;
			dh = th;
		}


		// set GS registers
		sceGifPkOpenGifTag(&g_PkGif, *(u_long128*)&giftagAD);	// set GIF tag

		// set TEX1 register
		sceGifPkAddGsData(&g_PkGif,
				SCE_GS_SET_TEX1(1, g_bMipmapLv, 0, g_nFilter, 0, 0, g_nLod));	// D(TEX1_1)
		sceGifPkAddGsData(&g_PkGif, SCE_GS_TEX1_1);		// A(TEX1_1)


		// set TEX0 register
		if(g_GsTex0.PSM==SCE_GS_PSMT4) {
			// if 16 color texture
			sceGsTex0 Load;						// TEX0 (for loading CLUT)
			sceGsTex0 Change;					// TEX0 (for changing CSA)

			// copy the setting of TEX0 register of GS, then modify it for loading CLUT and changing CSA
			Load        = g_GsTex0;				// copy original TEX0_1
			Load.PSM    = SCE_GS_PSMT8;			// fake as 8 bit texture
			Load.CBP   += g_nClutNo/16;			// offset adjust for CBP
			Load.CSA    = 0;					// CSA use the top area
			Load.CLD    = 1;					// load CLUT buffer

			Change      = g_GsTex0;				// copy original TEX0_1
			Change.CBP += g_nClutNo/16;			// offset adjust for CBP
			Change.CSA  = (g_nClutNo & 0x0F);	// CSA use the top area
			Change.CLD  = 0;					// don't load CLUT buffer

			// fake it as 256 color CLUT, and load into CLUT buffer
			sceGifPkAddGsData(&g_PkGif, *(u_long64 *)&Load);	// D(TEX0_1)
			sceGifPkAddGsData(&g_PkGif, SCE_GS_TEX0_1);			// A(TEX0_1)

			// set TEX0 register again, to change only CSA
			sceGifPkAddGsData(&g_PkGif, *(u_long64 *)&Change);	// D(TEX0_1)
			sceGifPkAddGsData(&g_PkGif, SCE_GS_TEX0_1);			// A(TEX0_1)
		} else {
			// not 16 colors
			sceGsTex0 t0;						// TEX0

			t0 = g_GsTex0;						// copy original TEX0_1
			t0.CBP += g_nClutNo*4;				// offset adjust for CBP

			sceGifPkAddGsData(&g_PkGif, *(u_long64 *)&t0);		// D(TEX0_1)
			sceGifPkAddGsData(&g_PkGif, SCE_GS_TEX0_1);			// A(TEX0_1)
		}

		// set MIPTBP1 register
		sceGifPkAddGsData(&g_PkGif, g_GsMiptbp1);		// D(MIPTBP1_1)
		sceGifPkAddGsData(&g_PkGif, SCE_GS_MIPTBP1_1);	// A(MIPTBP1_1)

		// set MIPTBP2 register
		sceGifPkAddGsData(&g_PkGif, g_GsMiptbp2);		// D(MIPTBP2_1)
		sceGifPkAddGsData(&g_PkGif, SCE_GS_MIPTBP2_1);	// A(MIPTBP2_1)

		sceGifPkCloseGifTag(&g_PkGif);					// close GIF tag


		// draw sprite
		sceGifPkOpenGifTag(&g_PkGif, *(u_long128*)&giftagSPR);	// set GIF tag

		// set PRIM register (0x0)
		// TriangleStrip, Flat, Texture, Fog off, Alpah On/Off
		// anti-alias Off, UV setting, context 1, flagment value control
		// don't use alpha channel
		sceGifPkAddGsData(&g_PkGif,
				SCE_GS_SET_PRIM(SCE_GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0));

		// set RGBAQ register (0x1)
		sceGifPkAddGsData(&g_PkGif,
				SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, *(u_int *)&fog));

		// set CLAMP register (0x8)
		sceGifPkAddGsData(&g_PkGif,
				SCE_GS_SET_CLAMP(0, 0, 0, 0, 0, 0));

		// set UV and XY value (upper-left, lower-right)
		sceGifPkAddGsData(&g_PkGif, SCE_GS_SET_UV(0,      0));			// 0x3
		sceGifPkAddGsData(&g_PkGif, SCE_GS_SET_XYZ(dx,    dy,    1));	// 0x4

		sceGifPkAddGsData(&g_PkGif, SCE_GS_SET_UV(tw,     th));			// 0x3
		sceGifPkAddGsData(&g_PkGif, SCE_GS_SET_XYZ(dx+dw, dy+dh, 1));	// 0x4
		sceGifPkAddGsData(&g_PkGif, 0);				// 0x0f (alignment adjust)

		sceGifPkCloseGifTag(&g_PkGif);				// close GIF tag
	}
	sceGifPkTerminate(&g_PkGif);					// close DMA tag
//	BUG_WALKAROUND(&g_PkGif);
	FlushCache(WRITEBACK_DCACHE);				// flush D cache
	sceDmaSend(g_DmaGif, ADR_DMA_PERIPH(g_PkGif.pBase));
	sceDmaSync(g_DmaGif, 0, 0);
	return;
}



// flush texture cache
static void FlushTextureCache(void)
{
	const u_long64 giftagAD[2] = {
		SCE_GIF_SET_TAG(0, 1, 0, 0, SCE_GIF_PACKED, 1),		// GIF tag for A+D
		0x000000000000000EL
	};

	// initialize GIF packet
	sceGifPkReset(&g_PkGif);

	// flush texture
	sceGifPkEnd(&g_PkGif, 0, 0, 0);
	sceGifPkOpenGifTag(&g_PkGif, *(u_long128*)&giftagAD);	// set GIF tag
	sceGifPkAddGsData(&g_PkGif, 0x00000000);		// D
	sceGifPkAddGsData(&g_PkGif, SCE_GS_TEXFLUSH);	// A(TEXFLUSH)
	sceGifPkCloseGifTag(&g_PkGif);					// close GIF tag

	// close GIF packet
	sceGifPkTerminate(&g_PkGif);


	// send GS initialize packet, and wait till done
//	BUG_WALKAROUND(&g_PkGif);
	FlushCache(WRITEBACK_DCACHE);				// flush D cache
	sceDmaSend(g_DmaGif, ADR_DMA_PERIPH(g_PkGif.pBase));
	sceDmaSync(g_DmaGif, 0, 0);
	return;
}


/*
// avoid buf in EE rev1.0
static void BUG_WALKAROUND(sceGifPacket *pkt)
{
	int dma_size, i;

	dma_size = ((u_int)pkt->pCurrent - (u_int)pkt->pBase - 0x10)/0x10;
	for(i=0; i<dma_size%4; i++) {
		sceGifPkAddCode(pkt, 0x00000000);
		sceGifPkAddCode(pkt, SCE_VIF1_SET_NOP(0));
		sceGifPkAddCode(pkt, SCE_VIF1_SET_NOP(0));
		sceGifPkAddCode(pkt, SCE_VIF1_SET_NOP(0));
	}
	return;
}

*/


// get bit width
static int GetLog2(u_long32 n)
{
	int i;
	for(i=31; i>0; i--) {
		if(n & (1<<i)) {
			break;
		}
	}
	if(n>(1<<i)) {
		i++;
	}
	return(i);
}


