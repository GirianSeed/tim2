// TIM2 File Parser
// version info
// $Revision: 22 $
// $Date: 99/11/18 21:58 $

#include <stdio.h>

// TIM2 file header structure

#ifdef R5900

// if PS2 environment
#include <eekernel.h>
#include <sifdev.h>
#include <libgraph.h>
#include "tim2.h"

// prototypes
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int sx, int sy, u_long128 *pImage);
static int  Tim2CalcBufWidth(int psm, int w);
static int  Tim2CalcBufSize(int psm, int w, int h);
static int  Tim2GetLog2(int n);

#else	// R5900

// if non-PS2

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4200)
#endif	// WIN32
#include "tim2.h"
#ifdef WIN32
#pragma warning(pop)
#endif	// WIN32

#endif	// R5900



// check TIM2 file header
// arguments:
// pTim2	pointer to the head of TIM2 data
// return value:
// 			0 : ERROR
// 			1 : OK (TIM2)
// 			2 : OK (CLUT2)
int Tim2CheckFileHeaer(void *pTim2)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	int i;

	// check TIM2 signature
	if(pFileHdr->FileId[0]=='T' || pFileHdr->FileId[1]=='I' || pFileHdr->FileId[2]=='M' || pFileHdr->FileId[3]=='2') {
		// if TIM2
		i = 1;
	} else if(pFileHdr->FileId[0]=='C' || pFileHdr->FileId[1]=='L' || pFileHdr->FileId[2]=='T' || pFileHdr->FileId[3]=='2') {
		// if CLUT2
		i = 2;
	} else {
		// if illegal signature
		printf("Tim2CheckFileHeaer: TIM2 is broken %02X,%02X,%02X,%02X\n",
					pFileHdr->FileId[0], pFileHdr->FileId[1], pFileHdr->FileId[2], pFileHdr->FileId[3]);
		return(0);
	}

	// check TIM2 file format version, format ID
	if(!(pFileHdr->FormatVersion==0x03 ||
	    (pFileHdr->FormatVersion==0x04 && (pFileHdr->FormatId==0x00 || pFileHdr->FormatId==0x01)))) {
		printf("Tim2CheckFileHeaer: TIM2 is broken (2)\n");
		return(0);
	}
	return(i);
}



// get imgno-th picture header
// arguments:
// pTim2	pointer to the head of TIM2 data
//	imgno	picture header number to be retrieved
// return value:
// 			pointer to the picture header
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *pTim2, int imgno)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	TIM2_PICTUREHEADER *pPictHdr;
	int i;

	// check picture number
	if(imgno>=pFileHdr->Pictures) {
		printf("Tim2GetPictureHeader: Illegal image no.(%d)\n", imgno);
		return(NULL);
	}

	if(pFileHdr->FormatId==0x00) {
		// 16 byte alignment if format ID is 0x00
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + sizeof(TIM2_FILEHEADER));
	} else {
		// 128 byte alignment if format ID is 0x01
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + 0x80);
	}

	// skip to the assumed picture
	for(i=0; i<imgno; i++) {
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	return(pPictHdr);
}


// check if the picture data is CLUT2
// arguments:
// ph		pointer to a TIM2 picture header
// return value:
// 			0 if TIM2
// 			1 if CLUT2
int Tim2IsClut2(TIM2_PICTUREHEADER *ph)
{
	// check MipMapTextures member of picture header
	if(ph->MipMapTextures==0) {
		// if texture number is 0, it is CLUT2
		return(1);
	} else {
		// if one or more texture(s), it is TIM2
		return(0);
	}
}


// get the size of assumed MIPMAP level texture
// arguments:
// ph		pointer to a TIM2 picture header
// mipmap	MIPMAP texture level
// pWidth	pointer to an int-variable used to receive X size
// pHeight		pointer to an int-variable user to receive Y size
// return value:
// 			size of the MIPMAP texture
int Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph, int mipmap, int *pWidth, int *pHeight)
{
	int w, h, n;
	w = ph->ImageWidth>>mipmap;
	h = ph->ImageHeight>>mipmap;
	if(pWidth) {
		*pWidth  = w;
	}
	if(pHeight) {
		*pHeight = h;
	}

	n = w * h;
	switch(ph->ImageType) {
		case TIM2_RGB16:	n *= 2;		break;
		case TIM2_RGB24:	n *= 3;		break;
		case TIM2_RGB32:	n *= 4;		break;
		case TIM2_IDTEX4:	n /= 2;		break;
		case TIM2_IDTEX8:				break;
	}

	// each MIPMAP texture is aligned to 16 byte boundary, regardless of
	// the setting by FormatId
	n = (n + 15) & ~15;
	return(n);
}


// get address and size of assumed MIPMAP header
// arguments:
// ph		pointer to a TIM2 picture header
// pSize	pointer to an int-variable to receive MIPMAP header size
// 			NULL if needless
// return value:
// 			NULL if no MIPMAP header
// 			non NULL value means the pointer to a MIPMAP header
TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize)
{
	TIM2_MIPMAPHEADER *pMmHdr;

	static const char mmsize[] = {
		0,		// no texture (CLUT2 data)
		0,		// LV0 texture only (no MIPMAP header)
		32,		// LV0-LV1 MipMap
		32,		// LV0-LV2 MipMap
		32,		// LV0-LV3 MipMap
		48,		// LV0-LV4 MipMap
		48,		// LV0-LV5 MipMap
		48		// LV0-LV6 MipMap
	};

	if(ph->MipMapTextures>1) {
		pMmHdr = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	} else {
		pMmHdr = NULL;
	}

	if(pSize) {
		// no extended header...
		*pSize = mmsize[ph->MipMapTextures];
	}
	return(pMmHdr);
}


// get the address and the size of a user space
// arguments:
// ph		pointer to a TIM2 picture header
// pSize	pointer to an int-variable used to receive the size of a user space
// 			NULL if needless
// return value:
// 			NULL if no user space
// 			non NULL value means the top address of the user space
void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;

	static const char mmsize[] = {
		sizeof(TIM2_PICTUREHEADER),			// no texture (CLUT2 data)
		sizeof(TIM2_PICTUREHEADER),			// LV0 texture only (no MIPMAP header)
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV1
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV2
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV3
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV0-LV4
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV0-LV5
		sizeof(TIM2_PICTUREHEADER) + 48		// LV0-LV6
	};

	// check header size
	if(ph->HeaderSize==mmsize[ph->MipMapTextures]) {
		// if no user space
		if(pSize) *pSize = 0;
		return(NULL);
	}

	// user space exists
	pUserSpace = (void *)((char *)ph + mmsize[ph->MipMapTextures]);
	if(pSize) {
		// calculate user space size
		// if there is an extended header, get total size from the header
		TIM2_EXHEADER *pExHdr;

		pExHdr = (TIM2_EXHEADER *)pUserSpace;
		if(pExHdr->ExHeaderId[0]!='e' ||
			pExHdr->ExHeaderId[1]!='X' ||
			pExHdr->ExHeaderId[2]!='t' ||
			pExHdr->ExHeaderId[3]!=0x00) {

			// no extended header exists
			*pSize = (ph->HeaderSize - mmsize[ph->MipMapTextures]);
		} else {
			// extended header exists
			*pSize = pExHdr->UserSpaceSize;
		}
	}
	return(pUserSpace);
}


// get the address and size of a user data
// arguments:
// ph		pointer to a TIM2 picture header
// pSize	pointer to an int-variable used to receive the size of the user data
// 			NULL if needless
// return value:
// 			NULL if no user data
// 			non NULL means the top address of the user data
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, pSize);
	if(pUserSpace==NULL) {
		// no user space
		return(NULL);
	}

	// check if extended header exists
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// no extended header
		return(pUserSpace);
	}

	// extended header found
	if(pSize) {
		// return the size of the user data
		*pSize = pExHdr->UserDataSize;
	}
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER));
}


// get the top address of comment string in a user space
// argument:
// ph		pointer to a TIM2 picture header
// return value:
// 			NULL if no comment string
// 			non NULL means the top address of a comment string (ASCIZ)
char *Tim2GetComment(TIM2_PICTUREHEADER *ph)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, NULL);
	if(pUserSpace==NULL) {
		// no user space
		return(NULL);
	}

	// check if extended header exists
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// no extended header
		return(NULL);
	}

	// extended header found
	if(pExHdr->UserSpaceSize==((sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize))) {
		// if valid user space size is equal to the amount of extended header and user data,
		// no comment string exists
		return(NULL);
	}

	// comment string found
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize);
}



// get the top address of an image data of the assumed MIPMAP level
// arguments:
// ph		pointer to a TIM2 picture header
// mipmap	MIPMAP texture level
// return value:
//				NULL if no assumed image data
// 			non NULL means the top address of the image data
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap)
{
	void *pImage;
	TIM2_MIPMAPHEADER *pm;
	int i;

	if(mipmap>=ph->MipMapTextures) {
		// no MIPMAP texture of the assumed level
		return(NULL);
	}

	// calculate the top address of the image data
	pImage = (void *)((char *)ph + ph->HeaderSize);
	if(ph->MipMapTextures==1) {
		// if LV0 texture only
		return(pImage);
	}

	// MIPMAP textures exist
	pm = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	for(i=0; i<mipmap; i++) {
		pImage = (void *)((char *)pImage + pm->MMImageSize[i]);
	}
	return(pImage);
}


// get the top address of a CLUT data
// argument:
// ph		pointer to a TIM2 picture header
// return value:
// 			NULL if no CLUT data
// 			non NULL means the top address of the CLUT data
void *Tim2GetClut(TIM2_PICTUREHEADER *ph)
{
	void *pClut;
	if(ph->ClutColors==0) {
		// if the color number of CLUT data part
		pClut = NULL;
	} else {
		// CLUT data exists
		pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);
	}
	return(pClut);
}


// get CLUT color
// arguments:
// ph		pointer to a TIM2 picture header
// clut	CLUT set to retrieve
// no		index number to retrive in the set
// return value:
// 			a color in RGBA32 format
// 			if error exists in clut, no etc., return 0x00000000 (black)
unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no)
{
	unsigned char *pClut;
	int n;
	unsigned char r, g, b, a;

	pClut = Tim2GetClut(ph);
	if(pClut==NULL) {
		// no CLUT data
		return(0);
	}

	// first, calculate the sequential index of the color
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:			return(0);					// illegal pixel color format
	}
	if(n>ph->ClutColors) {
		// if assumed CLUT set, index color doesn't exist
		return(0);
	}

	// order might be compounded according to the CLUT format
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16 bits, compounded
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24 bits, compounded
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32 bits, compounded
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 16 bits, compounded
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 24 bits, compounded
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 32 bits, compounded
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;				// make 8-15 into 16-23
				} else if((n & 31)<24) {
					n -= 8;				// make 16-23 into 8-15
				}
			}
			break;
	}

	// get color data according to the CLUT pixel format
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// if 16 bits color
			r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
			g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
			b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
			a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);
			break;

		case TIM2_RGB24:
			// if 24 bits color
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			break;

		case TIM2_RGB32:
			// if 32 bits color
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			break;

		default:
			// if illegal pixel format
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// set CLUT color
// arguments
// ph		pointer to a TIM2 picture header
// clut	CLUT set no
// no		index to set a color
// color	color to be set (in RGBA32 format)
// return value
// 			previous color in RGBA32 format
// 			if error exists in clut, no etc., return 0x00000000 (black)
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no, unsigned int newcolor)
{
	unsigned char *pClut;
	unsigned char r, g, b, a;
	int n;

	pClut = Tim2GetClut(ph);
	if(pClut==NULL) {
		// no CLUT data
		return(0);
	}

	// first, calculate the sequential index of the color
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:			return(0);					// illegal pixel color format
	}
	if(n>ph->ClutColors) {
		// if assumed CLUT set, index color doesn't exist
		return(0);
	}

	// order might be compounded according to the CLUT format
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16 bits, compounded
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24 bits, compounded
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32 bits, compounded
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 16 bits, compounded
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 24 bits, compounded
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 32 bits, compounded
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;				// make 8-15 into 16-23
				} else if((n & 31)<24) {
					n -= 8;				// make 16-23 into 8-15
				}
			}
			break;
	}

	// get color data according to the CLUT pixel format
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// if 16 bits color
			{
				unsigned char rr, gg, bb, aa;
				r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
				g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
				b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
				a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);

				rr = (unsigned char)((newcolor>>3)  & 0x1F);
				gg = (unsigned char)((newcolor>>11) & 0x1F);
				bb = (unsigned char)((newcolor>>19) & 0x1F);
				aa = (unsigned char)((newcolor>>31) & 1);

				pClut[n*2]     = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr))    & 0xFF);
				pClut[n*2 + 1] = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr)>>8) & 0xFF);
			}
			break;

		case TIM2_RGB24:
			// if 24 bits color
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			pClut[n*3]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*3 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*3 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			break;

		case TIM2_RGB32:
			// if 32 bits color
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			pClut[n*4]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*4 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*4 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			pClut[n*4 + 3] = (unsigned char)((newcolor>>24) & 0xFF);
			break;

		default:
			// if illegal pixel format
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// get texel data (might not be a direct color)
// arguments:
// ph		pointer to a TIM2 picture header
// mipmap	MIPMAP texture level
// x		X coordinate to get the data
// y		Y coordinate to get the data
// return value:
// 			color info (4 or 8 bits index number. otherwise 16, 24 or 32 bits direct color)
unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y)
{
	unsigned char *pImage;
	int t;
	int w, h;

	pImage = Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// no assumed level texture data
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// illegal texel cooridnates
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			return((pImage[t*2 + 1]<<8) | pImage[t*2]);

		case TIM2_RGB24:
			return((pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3]);

		case TIM2_RGB32:
			return((pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4]);

		case TIM2_IDTEX4:
			if(x & 1) {
				return(pImage[t/2]>>4);
			} else {
				return(pImage[t/2] & 0x0F);
			}
		case TIM2_IDTEX8:
			return(pImage[t]);
	}

	// illegal pixel format
	return(0);
}


// set texel data (might not be a direct color)
// arguments:
// ph		pointer to a TIM2 picture header
// mipmap	MIPMAP texture level
// x		X coordinate to set the data
// y		Y coordinate to set the data
// newtexel	color info to be set (4 or 8 bits index number. otherwise 16, 24 or 32 bits direct color)
// return value:
// 			previous color info (4 or 8 bits index number. otherwise 16, 24 or 32 bits direct color)
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y, unsigned int newtexel)
{
	unsigned char *pImage;
	int t;
	int w, h;
	unsigned int oldtexel;

	pImage = Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// no assumed level texture data
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// illegal texel cooridnates
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			oldtexel = (pImage[t*2 + 1]<<8) | pImage[t*2];
			pImage[t*2]     = (unsigned char)((newtexel)    & 0xFF);
			pImage[t*2 + 1] = (unsigned char)((newtexel>>8) & 0xFF);
			return(oldtexel);

		case TIM2_RGB24:
			oldtexel = (pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3];
			pImage[t*3]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*3 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*3 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			return(oldtexel);

		case TIM2_RGB32:
			oldtexel = (pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4];
			pImage[t*4]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*4 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*4 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			pImage[t*4 + 3] = (unsigned char)((newtexel>>24) & 0xFF);
			return(oldtexel);

		case TIM2_IDTEX4:
			if(x & 1) {
				oldtexel = pImage[t/2]>>4;
				pImage[t/2] = (unsigned char)((newtexel<<4) | oldtexel);
			} else {
				oldtexel = pImage[t/2] & 0x0F;
				pImage[t/2] = (unsigned char)((oldtexel<<4) | newtexel);
			}
			return(oldtexel);
		case TIM2_IDTEX8:
			oldtexel = pImage[t];
			pImage[t] = (unsigned char)newtexel;
			return(oldtexel);
	}

	// illegal pixel format
	return(0);
}


// get texture color
// arguments:
// ph		pointer to a TIM2 picture header
// mipmap	MIPMAP texture level
// clut	CLUT set number used to convert indexed color
// x		X coordinate to get texel color
// y		Y coordinate to get texel color
// return value:
// 			texel color in RGBA32 format
// 			if error exists in clut, return 0x00000000 (black)
// 			action will be invalid if x or y assumption is illegal
unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph, int mipmap, int clut, int x, int y)
{
	unsigned int t;
	if(Tim2GetImage(ph, mipmap)==NULL) {
		// no assumed level texture data
		return(0);
	}
	t = Tim2GetTexel(ph, mipmap, (x>>mipmap), (y>>mipmap));
	switch(ph->ImageType) {
		case TIM2_RGB16:
			{
				unsigned char r, g, b, a;
				r = (unsigned char)((t<<3) & 0xF8);
				g = (unsigned char)((t>>2) & 0xF8);
				b = (unsigned char)((t>>7) & 0xF8);
				a = (unsigned char)((t>>8) & 0x80);
				return((a<<24) | (g<<16) | (b<<8) | r);
			}

		case TIM2_RGB24:
			return((0x80<<24) | (t & 0x00FFFFFF));

		case TIM2_RGB32:
			return(t);

		case TIM2_IDTEX4:
		case TIM2_IDTEX8:
			return(Tim2GetClutColor(ph, clut, t));
	}
	return(0);
}






// from here, can be used only on PS2 ee-gcc
#ifdef R5900

// load TIM2 picture data into GS local memory
// arguments:
// ph		pointer to a TIM2 picture header
// tbp			page base address of the source texture buffer (use the value in the header if -1)
// cbp			page base address of the source CLUT buffer (use the avlue in the header if -1)
// return value:
// 			NULL means an error
// 			non NULL means the next buffer address
// caution:
// 			even if CSM2 is assumed as a CLUT storage mode, forced converting to CSM1, then transferred to GS
unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph, unsigned int tbp, unsigned int cbp)
{
	// transfer CLUT data
	tbp = Tim2LoadImage(ph, tbp);
	Tim2LoadClut(ph, cbp);
	return(tbp);
}


// load TIM2 picture image data part into GS local memory
// arguments:
// ph		pointer to a TIM2 picture header
// tbp			page base address of the source texture buffer (use the value in the header if -1)
// return value:
// 			NULL means an error
// 			non NULL means the next buffer address
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp)
{
	// transfer image data
	if(ph->MipMapTextures>0) {
		static const int psmtbl[] = {
			SCE_GS_PSMCT16,
			SCE_GS_PSMCT24,
			SCE_GS_PSMCT32,
			SCE_GS_PSMT4,
			SCE_GS_PSMT8
		};
		int i;
		int psm;
		u_long128 *pImage;
		int w, h;
		int tbw;

		psm = psmtbl[ph->ImageType - 1];		// get pixel format
		((sceGsTex0 *)&ph->GsTex0)->PSM  = psm;

		w = ph->ImageWidth;						// X size of the image
		h = ph->ImageHeight;					// y size of the image

		// calculate the top address of the image data
		pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
		if(tbp==-1) {
			// if no tbp assumed, get tbp and tbw from GsTex0 in the picture header
			tbp = ((sceGsTex0 *)&ph->GsTex0)->TBP0;
			tbw = ((sceGsTex0 *)&ph->GsTex0)->TBW;

		} else {
			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);		// if tbp assumed, override tbp, tbw of GsTex0 member
			tbw = Tim2CalcBufWidth(psm, w);
			// update the value to be set into TEX0 register of GS
			((sceGsTex0 *)&ph->GsTex0)->TBP0 = tbp;
			((sceGsTex0 *)&ph->GsTex0)->TBW  = tbw;
			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);		// transfer the image data to GS
			tbp += Tim2CalcBufSize(psm, w, h);		// update tbp value
		}

		if(ph->MipMapTextures>1) {
			// if MIPMAP textures exist
			TIM2_MIPMAPHEADER *pm;

			pm = (TIM2_MIPMAPHEADER *)(ph + 1);		// MIPMAP header is placed just after the picture header

			// get LV0 texture size
			// if tbp assumed by an argument, ignore miptbp etc. in the picture header, and recaluclate
			if(tbp!=-1) {
				pm->GsMiptbp1 = 0;
				pm->GsMiptbp2 = 0;
			}

			pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
			// transfer image data of each MIPMAP lavel
			for(i=1; i<ph->MipMapTextures; i++) {
				// texture size of the higher level will be half of the origianl
				w = w / 2;
				h = h / 2;

				pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
				if(tbp==-1) {
					// if tbp is not assumed, use tbp, tbw in the MIPMAP header
					int miptbp;
					if(i<4) {
						// if MIPMAP level 1, 2, or 3
						miptbp = (pm->GsMiptbp1>>((i-1)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp1>>((i-1)*20 + 14)) & 0x3F;
					} else {
						// if MIPMAP level 4, 5 or 6
						miptbp = (pm->GsMiptbp2>>((i-4)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp2>>((i-4)*20 + 14)) & 0x3F;
					}
					Tim2LoadTexture(psm, miptbp, tbw, w, h, pImage);
				} else {
					// if texture page is assumed, modify the MIPAMP header
					tbw = Tim2CalcBufWidth(psm, w);		// calculate the width of the texture
					if(i<4) {
						// if MIPMAP level 1, 2 or 3
						pm->GsMiptbp1 |= ((u_long)tbp)<<((i-1)*20);
						pm->GsMiptbp1 |= ((u_long)tbw)<<((i-1)*20 + 14);
					} else {
						// if MIPMAP level 4, 5 or 6
						pm->GsMiptbp2 |= ((u_long)tbp)<<((i-4)*20);
						pm->GsMiptbp2 |= ((u_long)tbw)<<((i-4)*20 + 14);
					}
					Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);
					tbp += Tim2CalcBufSize(psm, w, h);			// update tbp value
				}
			}
		}
	}
	return(tbp);
}



// load TIM2 picture CLUT data part into GS local memory
// arguments:
// ph		pointer to a TIM2 picture header
// cbp			page base address of the source CLUT buffer (use the avlue in the header if -1)
// return value:
// 			0 means an error
// 			non zero means success
// caution:
// 			even if CSM2 is assumed as a CLUT storage mode, forced converting to CSM1, then transferred to GS
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp)
{
	int i;
	sceGsLoadImage li;
	u_long128 *pClut;
	int	cpsm;

	// get CLUT pixel format
	if(ph->ClutType==TIM2_NONE) {
		// no CLUT data exists
		return(1);
	} else if((ph->ClutType & 0x3F)==TIM2_RGB16) {
		cpsm = SCE_GS_PSMCT16;
	} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
		cpsm = SCE_GS_PSMCT24;
	} else {
		cpsm = SCE_GS_PSMCT32;
	}
	((sceGsTex0 *)&ph->GsTex0)->CPSM = cpsm;	// pixel format setting of CLUT
	((sceGsTex0 *)&ph->GsTex0)->CSM  = 0;		// CLUT storage mode (always CSM1)
	((sceGsTex0 *)&ph->GsTex0)->CSA  = 0;		// CLUT entry offset (always 0)
	((sceGsTex0 *)&ph->GsTex0)->CLD  = 1;		// CLUT buffer load control (always load)

	if(cbp==-1) {
		// if no cbp assumed, get the value from GsTex0 member of the picture header
		cbp = ((sceGsTex0 *)&ph->GsTex0)->CBP;
	} else {
		// if cbp assumed, override the GsTex0 member value
		((sceGsTex0 *)&ph->GsTex0)->CBP = cbp;
	}

	// calculate the top address of the CLUT data
	pClut = (u_long128 *)((char *)ph + ph->HeaderSize + ph->ImageSize);

	// transfer CLUT data into GS local memory
	// CLUT data format etc. are defined by CLUT type and texture type
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16bits, compound
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24bits, compound
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32bits, compound
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 16bits, compound
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 24bits, compound
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 32bits, compound
			// if 256 colors CLUT and storage mode is CSM1 or..
			// 16 colors CLUT and storage mode is CSM1 and compounded flag is ON
			// pixels are already placed in the right place, so directly transferable
			Tim2LoadTexture(cpsm, cbp, 1, 16, (ph->ClutColors / 16), pClut);
			break;

		case (( TIM2_RGB16        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16bits, sequential
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24bits, sequential
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32bits, sequential
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colros, CSM2, 16bits, sequential
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colors, CSM2, 24bits, sequential
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colors, CSM2, 32bits, sequential
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 16bits, sequential
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 24bits, sequential
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 32bits, sequential
			// 16 colors CLUT, storage mode is CSM1, and compunded flag is OFF or..
			// 16 colros CLUT and storage mode is CSM2 or..
			// 256 colors CLUT and storage mode is CSM2

			// sequential placement (CSM2) is ineffective in performance, so compound into CSM1 then transfer
			for(i=0; i<(ph->ClutColors/16); i++) {
				sceGsSetDefLoadImage(&li, cbp, 1, cpsm, (i & 1)*8, (i>>1)*2, 8, 2);
				FlushCache(WRITEBACK_DCACHE);			// flush D cache
				sceGsExecLoadImage(&li, pClut);			// transfer CLUT data to GS
				sceGsSyncPath(0, 0);					// wait till transfer terminates

				if((ph->ClutType & 0x3F)==TIM2_RGB16) {
					// next 16 colors, address update
					pClut = (u_long128 *)((char *)pClut + 2*16);	// 16bit colors
				} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
					pClut = (u_long128 *)((char *)pClut + 3*16);	// 24bit colors
				} else {
					pClut = (u_long128 *)((char *)pClut + 4*16);	// 32bit colors
				}
			}
			break;

		default:
			printf("Illegal clut and texture combination. ($%02X,$%02X)\n", ph->ClutType, ph->ImageType);
			return(0);
	}
	return(1);
}


// write out 'snap shot' in TIM2 format
// arguments:
// d0		frame buffer of even raster frame
// d1		frame buffer of odd raster frame (NULL if non-interaced)
// pszfname	TIM2 file name
// return value:
// 			0 means an error
// 			non zero means success
int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1, char *pszFname)
{
	int i;

	int h;					// file handle
	int nWidth, nHeight;	// size of the image
	int nImageType;			// image type
	int psm;				// pixel format
	int nBytes;				// bytes per raster

	// get image size and pixel format
	nWidth  = d0->display.DW / (d0->display.MAGH + 1);
	nHeight = d0->display.DH + 1;
	psm     = d0->dispfb.PSM;

	// from pixel format, get bytes per raster and TIM2 pixel type
	switch(psm) {
		case SCE_GS_PSMCT32:	nBytes = nWidth*4;	nImageType = TIM2_RGB32;	break;
		case SCE_GS_PSMCT24:	nBytes = nWidth*3;	nImageType = TIM2_RGB24;	break;
		case SCE_GS_PSMCT16:	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		case SCE_GS_PSMCT16S:	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		default:
			// unknown pixel format, return with error
			// GS_PSGPU24 format is not supported
			printf("Illegal pixel format.\n");
			return(0);
	}


	// open TIM2 file
	h = sceOpen(pszFname, SCE_WRONLY | SCE_CREAT);
	if(h==-1) {
		// open failure
		printf("file create failure.\n");
		return(0);
	}

	// write out file header
	{
		TIM2_FILEHEADER fhdr;

		fhdr.FileId[0] = 'T';				// set file ID
		fhdr.FileId[1] = 'I';
		fhdr.FileId[2] = 'M';
		fhdr.FileId[3] = '2';
		fhdr.FormatVersion = 3;				// format version 4
		fhdr.FormatId  = 0;					// 16 byte alignment
		fhdr.Pictures  = 1;					// only one picture
		for(i=0; i<8; i++) {
			fhdr.pad[i] = 0x00;				// clear padding area by 0x00
		}

		sceWrite(h, &fhdr, sizeof(TIM2_FILEHEADER));	// write file header
	}

	// write out picture header
	{
		TIM2_PICTUREHEADER phdr;
		int nImageSize;

		nImageSize = nBytes * nHeight;
		phdr.TotalSize   = sizeof(TIM2_PICTUREHEADER) + nImageSize;	// total size
		phdr.ClutSize    = 0;							// no CLUT
		phdr.ImageSize   = nImageSize;					// image size
		phdr.HeaderSize  = sizeof(TIM2_PICTUREHEADER);	// header size
		phdr.ClutColors  = 0;							// CLUT colors
		phdr.PictFormat  = 0;							// picture format
		phdr.MipMapTextures = 1;						// MIPMAP texture number
		phdr.ClutType    = TIM2_NONE;					// no CLUT
		phdr.ImageType   = nImageType;					// image type
		phdr.ImageWidth  = nWidth;						// image width
		phdr.ImageHeight = nHeight;						// image height

		// clear the settings of GS registers
		phdr.GsTex0        = 0;
		((sceGsTex0 *)&phdr.GsTex0)->TBW = Tim2CalcBufWidth(psm, nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->PSM = psm;
		((sceGsTex0 *)&phdr.GsTex0)->TW  = Tim2GetLog2(nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->TH  = Tim2GetLog2(nHeight);
		phdr.GsTex1        = 0;
		phdr.GsTexaFbaPabe = 0;
		phdr.GsTexClut     = 0;

		sceWrite(h, &phdr, sizeof(TIM2_PICTUREHEADER));	// write out picture header
	}


	// write out image data
	for(i=0; i<nHeight; i++) {
		u_char buf[4096];			// reserve 4KB as a raster buffer
		sceGsStoreImage si;

		if(d1) {
			// if interace
			if(!(i & 1)) {
				// if even raster in interace mode
				sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
									d0->dispfb.DBX, (d0->dispfb.DBY + i/2),
									nWidth, 1);
			} else {
				// if odd raster in interace mode
				sceGsSetDefStoreImage(&si, d1->dispfb.FBP*32, d1->dispfb.FBW, psm,
									d1->dispfb.DBX, (d1->dispfb.DBY + i/2),
									nWidth, 1);
			}
		} else {
			// if non-interaced
			sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
									d0->dispfb.DBX, (d0->dispfb.DBY + i),
									nWidth, 1);
		}
		FlushCache(WRITEBACK_DCACHE);				// flush D cache

		sceGsExecStoreImage(&si, (u_long128 *)buf);	// start transfer from VRAM
		sceGsSyncPath(0, 0);						// wait till transfer ends

		sceWrite(h, buf, nBytes);					// write one raster data
	}

	// write out completes
	sceClose(h);						// close file
	return(1);
}






// transfer texture data
// arguments:
// psm			texture pixel format
// tbp			base point of the texture buffer
// tbw			width of the texture buffer
// w 		width of the transfer area
// h		height of the transfer area
// pImage	address where texture image is stored
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int w, int h, u_long128 *pImage)
{
	sceGsLoadImage li;
	int i, l, n;
	u_long128 *p;

	switch(psm) {
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMCT32:
			n = w*4;
			break;

		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT24:
			n = w*3;
			break;

		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
			n = w*2;
			break;

		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT8:
			n = w;
			break;

		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMT4:
			n = w/2;
			break;

		default:
			return;
	}

	// not to exceed the max. DMA transfer limit of 512KB, split then transfer
	l = 32764 * 16 / n;
	for(i=0; i<h; i+=l) {
		p = (u_long128 *)((char *)pImage + n*i);
		if((i+l)>h) {
			l = h - i;
		}

		sceGsSetDefLoadImage(&li, tbp, tbw, psm, 0, i, w, l);
		FlushCache(WRITEBACK_DCACHE);		// flush D cache
		sceGsExecLoadImage(&li, p);			// invoke transfer to VRAM
		sceGsSyncPath(0, 0);				// wait till transfer completes
	}
	return;
}



// calculate buffer size from pixel format and texture size
// arugments:
// psm			texture pixel format
// w		width
// return value:
// 			buffer size per line
// 			unit is 256 bytes (64 words)
static int Tim2CalcBufWidth(int psm, int w)
{
//	return(w / 64);

	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			return((w+63) / 64);

		case SCE_GS_PSMT8:
		case SCE_GS_PSMT4:
			w = (w+63) / 64;
			if(w & 1) {
				w++;
			}
			return(w);
	}
	return(0);
}



// calculate the buffer size form pixel format and texture size
// arugments:
// psm			texture pixel format
// w		width
// h		line number
// return value:
// 			buffer size per line
// 			unit is 256 bytes (64 words)
// caution:
// 			if following texture has different pixel format,
// 			tbp must be aligned to the next 2KB page boundary
static int Tim2CalcBufSize(int psm, int w, int h)
{
	return(w * h / 64);
/*
	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
			// 1 word per pixel
			return(((w+63)/64) * ((h+31)/32));

		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			// 1/2 word per pixel
			return(((w+63)/64) * ((h+63)/64));

		case SCE_GS_PSMT8:
			// 1/4 word per pixel
			return(((w+127)/128) * ((h+63)/64));

		case SCE_GS_PSMT4:
			// 1/8 word per pixel
			return(((w+127)/128) * ((h+127)/128));
	}
	// error?
	return(0);
*/
}


// get bit width
static int Tim2GetLog2(int n)
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




