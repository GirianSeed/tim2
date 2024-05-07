// $Revision: 11 $
// $Date: 99/11/10 13:10 $

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4200)
#endif
#include "tim2.h"
#ifdef WIN32
#pragma warning(pop)
#endif

// prototypes
int  main(int argc, char *argv[]);
void *ReadTim2File(const char *pszFileName);
int  Tim2Verify(void *pTim);
int  CheckPaddingBytes(void *pPadding, int s);
int  CheckFileHeaer(TIM2_FILEHEADER *pFileHdr);
void CheckPictureHeader(TIM2_PICTUREHEADER *pPictHdr, int align, int *pErrors, int *pWarnings);
int  GetLog2(unsigned int n);


// global variables
static const char szTitle[] = "TIM2 verifier\n";
static const char szUsage[] = "usage:    tim2verify <input TIM2>\n";


// main function
int main(int argc, char *argv[])
{
	int i;
	char *p;

	// display title
	printf(szTitle);
	if(argc<2) {
		// display help message
		printf(szUsage);
		return(0);
	}

	for(i=1; i<argc; i++) {
		// read TIM2 file
		p = ReadTim2File(argv[i]);
		if(p==NULL) {
			// if read error occurs, exit w/ error
			return(1);
		}

		// verify TIM2 file
		printf("Verifying : \"%s\"\n", argv[i]);
		Tim2Verify(p);

		// free memory
		free(p);
	}
	return(0);
}



// read TIM2 file
void *ReadTim2File(const char *pszFileName)
{
	FILE *fp;
	char *p;
	long l;

	// open file
	fp = fopen(pszFileName, "rb");
	if(fp==NULL) {
		return(NULL);
	}

	// get file size
	fseek(fp, 0, SEEK_END);
	l = ftell(fp);
	p = malloc(l);

	// read whole file
	fseek(fp, 0, SEEK_SET);
	fread(p, 1, l, fp);
	fclose(fp);
	return(p);
}



// verify TIM2 data
int Tim2Verify(void *pTim2)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	TIM2_PICTUREHEADER *pPictHdr;
	int i;
	int align;
	int nErrors, nWarnings;

	// check file header
	printf("---- Check TIM2 file header ----\n");
	nErrors = CheckFileHeaer(pFileHdr);
	if(nErrors) {
		// If error exists in the file header, no further check, immediately exit
		return(0);
	}

	if(pFileHdr->FormatId==0x00) {
		// if 16 byte alignment mode
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + sizeof(TIM2_FILEHEADER));
		align = 16;
	} else {
		// if 128 byte alignment mode
		CheckPaddingBytes(((char *)pFileHdr + sizeof(TIM2_FILEHEADER)), (0x80 - sizeof(TIM2_FILEHEADER)));
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + 0x80);
		align = 128;
	}

	for(i=0; i<pFileHdr->Pictures; i++) {
		static char *postfix[] = {"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"};
		printf("---- check %d %s picture data ---\n", i+1, postfix[(i + 1) % 10]);
		CheckPictureHeader(pPictHdr, align, &nErrors, &nWarnings);

		printf("%d Error(s), %d Warning(s)\n", nErrors, nWarnings);
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	printf("---- Done ----\n");
	return(0);
}


// check if paddings are all zeros
int CheckPaddingBytes(void *pPadding, int s)
{
	int i;
	char *p = (char *)pPadding;

	for(i=0; i<s; i++) {
		if(p[i]) {
			printf("Fatal: There is non-zero value in padding field (pad)\n");
			return(1);
		}
	}
	return(0);
}


// check TIM2 file header
int CheckFileHeaer(TIM2_FILEHEADER *pFileHdr)
{
	int nError = 0;

	// check TIM2 signeture
	if(pFileHdr->FileId[0]=='T' || pFileHdr->FileId[1]=='I' || pFileHdr->FileId[2]=='M' || pFileHdr->FileId[3]=='2') {
		// if TIM2
	} else if(pFileHdr->FileId[0]=='C' || pFileHdr->FileId[1]=='L' || pFileHdr->FileId[2]=='T' || pFileHdr->FileId[3]=='2') {
		// if CLUT2
	} else {
		// if illegal ID
		printf("Fatal: File ID is illegal (FileId: 0x%02X,0x%02X,0x%02X,0x%02X)\n",
					pFileHdr->FileId[0], pFileHdr->FileId[1], pFileHdr->FileId[2], pFileHdr->FileId[3]);
		nError++;
	}

	// check TIM2 file format version and format ID
	if(pFileHdr->FormatVersion==0x03) {
		if(pFileHdr->FormatId) {
			printf("Fatal: Invlalid alignment mode. 128 byte alignment can be used in version 4 or later only. (FormadId: 0x%02X\n",
					pFileHdr->FormatId);
			nError++;
		}
	} else if(pFileHdr->FormatVersion==0x04) {
		if(!(pFileHdr->FormatId==0x00 || pFileHdr->FormatId==0x01)) {
			printf("Fatal: illegal alignment mode. (FormadId: 0x%02X)\n",
					pFileHdr->FormatId);
			nError++;
		}
	} else {
		printf("Fatal: Unsupported version by this verifier. (FormatVersion: 0x%02X)\n",
					pFileHdr->FormatVersion);
		nError++;
	}

	if(pFileHdr->Pictures==0) {
		printf("Fatal: illegal TIM2 picture data count (Pictures: %d)\n",
					pFileHdr->Pictures);
		nError++;
	}

	// check if all paddings are 0x00
	CheckPaddingBytes(&pFileHdr->pad, 8);
	return(nError);
}



// check contents of picture header
void CheckPictureHeader(TIM2_PICTUREHEADER *pPictHdr, int align, int *pErrors, int *pWarnings)
{
	static const char mmsize[] = {
		sizeof(TIM2_PICTUREHEADER),			// no texture (CLUT2 data)
		sizeof(TIM2_PICTUREHEADER),			// LV0 texture only (no MIPMAP header)
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV1 MIPMAP
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV2 MIPMAP
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV0-LV3 MIPMAP
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV0-LV4 MIPMAP
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV0-LV5 MIPMAP
		sizeof(TIM2_PICTUREHEADER) + 48		// LV0-LV6 MIPMAP
	};
	int nError = 0;
	int nWarning = 0;

	void *pUserSpace;
	TIM2_MIPMAPHEADER *pMmHdr;
	TIM2_EXHEADER *pExHdr;

	// first check and get headers
	// get a pointer to MIPMAP header
	if(pPictHdr->MipMapTextures<2) {
		pMmHdr = NULL;
	} else {
		pMmHdr = (TIM2_MIPMAPHEADER *)((char *)pPictHdr + sizeof(TIM2_PICTUREHEADER));
	}
	// check if extended header exists
	if(pPictHdr->HeaderSize==mmsize[pPictHdr->MipMapTextures]) {
		// no user space
		pUserSpace = 0;
		pExHdr = NULL;
	} else {
		// user space exists
		pUserSpace = (void *)((char *)pPictHdr + mmsize[pPictHdr->MipMapTextures]);
		pExHdr = (TIM2_EXHEADER *)pUserSpace;

		if(pExHdr->ExHeaderId[0]!='e' ||
			pExHdr->ExHeaderId[1]!='X' ||
			pExHdr->ExHeaderId[2]!='t' ||
			pExHdr->ExHeaderId[3]!=0x00) {

			// no extended header
			pExHdr = NULL;
		} else {
			// extended header exists
			if(pExHdr->UserSpaceSize!=((sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize))) {
				// if comment string exists
				unsigned char *pComment;
				pComment = (unsigned char *)pUserSpace + sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize;

				while(*pComment) {
					if(*pComment<0x20 || *pComment>=0x80) {
						printf("Fatal: Unusable character in comment string (0x%02X)\n",
									*pComment);
						nError++;
						break;
					}
					pComment++;
				};
			}
		}
	}


	// check it!

	// check TotalSize member
	if(pPictHdr->TotalSize % align) {
		printf("Fatal: TotalSize is not aligned to the boundary (TotalSize: %d)\n",
					pPictHdr->TotalSize);
		nError++;
	} else if(pPictHdr->TotalSize!=(pPictHdr->HeaderSize + pPictHdr->ImageSize + pPictHdr->ClutSize)) {
		printf("Fatal: TotalSize is different from the sum of HeaderSize, ImageSize and ClutSize (TotalSize: %d)\n",
					pPictHdr->TotalSize);
		nError++;
	}

	// check ClutSize member
	if(pPictHdr->ClutSize % align) {
		printf("Fatal: ClutSize is not aligned to the boundary (ClutSize: %d)\n",
					pPictHdr->ClutSize);
		nError++;
	} else {
		// check CLUT data size from CLUT pixel format and color number
		unsigned int nClutSize;
		switch(pPictHdr->ClutType & 0x3F) {
			case TIM2_RGB16:
				nClutSize = pPictHdr->ClutColors * 2;
				break;
			case TIM2_RGB24:
				nClutSize = pPictHdr->ClutColors * 3;
				break;
			case TIM2_RGB32:
				nClutSize = pPictHdr->ClutColors * 4;
				break;
			default:
				nClutSize = 0;
				break;
		}
		nClutSize = (nClutSize + (align - 1)) & (~(align - 1));		// adjust for alignment
		if(pPictHdr->ClutSize!=nClutSize) {
			printf("Fatal: ClutSize conflicts with the request from ClutColors, color number and pixel format (ClutSize: %d)\n",
						pPictHdr->ClutSize);
			nError++;
		}
	}

	// check ImageSize member
	if(pPictHdr->ImageSize % align) {
		printf("Fatal: ImageSize is not aligned to the boundary (ImageSize: %d)\n",
					pPictHdr->ImageSize);
		nError++;
	} else {
		// recalculate image size
		int n, w, h;
		unsigned int nImageSize;

		// check if image size is equal to the amount of all MIPMAP image size
		w = pPictHdr->ImageWidth;
		h = pPictHdr->ImageHeight;
		nImageSize = 0;
		for(n=0; n<pPictHdr->MipMapTextures; n++) {
			switch(pPictHdr->ImageType) {
				case TIM2_RGB16:	nImageSize += w * h * 2;	break;
				case TIM2_RGB24:	nImageSize += w * h * 3;	break;
				case TIM2_RGB32:	nImageSize += w * h * 4;	break;
				case TIM2_IDTEX4:	nImageSize += w * h / 2;	break;
				case TIM2_IDTEX8:	nImageSize += w * h;		break;
			}
			nImageSize = (nImageSize + 15) & (~15);				// align to 16 byte boundary
			w = w / 2;
			h = h / 2;
		}
		nImageSize = (nImageSize + (align - 1)) & (~(align - 1));	// alignment adjust
		if(pPictHdr->ImageSize!=nImageSize) {
			printf("Fatal: ImageSize conflicts with ImageType,ImageWidth,ImageHeight (ImageSize: %d)\n",
						pPictHdr->ImageSize);
			nError++;
		}

		// check if image size is equal to the sum of all texture data size
		if(pMmHdr) {
			nImageSize = 0;
			for(n=0; n<pPictHdr->MipMapTextures; n++) {
				nImageSize += pMmHdr->MMImageSize[n];
			}

			if(pPictHdr->ImageSize!=nImageSize) {
				printf("Fatal: ImageSize is not equal to the sum of all MIPMAP texture size (ImageSize: %d)\n",
							pPictHdr->ImageSize);
				nError++;
			}
		}

		// FIXME: texture data size of each MIPMAP level should be checked
	}

	// check HeaderSize member
	if(pPictHdr->HeaderSize % align) {
		printf("Fatal: HeaderSize is not aligned to the boundary (HeaderSize: %d)\n",
					pPictHdr->HeaderSize);
		nError++;
	} else {
		// FIXME: more extended error cheking can be performed if we consider about user space size in the extended header
		;
	}

	// check ClutType, ImageType, ClutColors members
	{
		// combination check of ClutType and ImageType
		int n = 0;
		switch((pPictHdr->ClutType<<8) | pPictHdr->ImageType) {
			case (  TIM2_NONE              | TIM2_RGB16):	// 16 bit direct color
			case (  TIM2_NONE              | TIM2_RGB24):	// 24 bit direct color
			case (  TIM2_NONE              | TIM2_RGB32):	// 32 bit direct color
				n = 1;
				break;

			case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16 bits, compound
			case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24 bits, compound
			case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32 bits, compound
				n = 32;
				break;

			case (( TIM2_RGB16        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 16 bits, sequential
			case (( TIM2_RGB24        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 24 bits, sequential
			case (( TIM2_RGB32        <<8) | TIM2_IDTEX4):	// 16 colors, CSM1, 32 bits, sequential
			case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colors, CSM2, 16 bits, sequential
			case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colors, CSM2, 24 bits, sequential
			case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX4):	// 16 colors, CSM2, 32 bits, sequential
				n = 16;
				break;

			case (( TIM2_RGB16        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 16 bits, compound
			case (( TIM2_RGB24        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 24 bits, compound
			case (( TIM2_RGB32        <<8) | TIM2_IDTEX8):	// 256 colors, CSM1, 32 bits, compound
			case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 16 bits, sequential
			case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 24 bits, sequential
			case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX8):	// 256 colors, CSM2, 32 bits, sequential
				n = 256;
				break;
		}
		if(n==0) {
			printf("Fatal: illegal combination of ClutType and ImageType (ClutType: %02X, ImageType: 0x%02X)\n",
						pPictHdr->ClutType, pPictHdr->ImageType);
			nError++;
		} else if(pPictHdr->ClutColors % n) {
			printf("Fatal: ClutColors doesn't satisfy the request from ClutType and ImageType (ClutColors: %d, ClutType: 0x%02X, ImageType: 0x%02X)\n",
						pPictHdr->ClutColors, pPictHdr->ClutType, pPictHdr->ImageType);
			nError++;
		} else if((pPictHdr->ClutType & 0x3F)>3) {
			printf("Fatal: illegal ClutType value (ClutType: 0x%02X)\n",
						pPictHdr->ClutType);
			nError++;
		} else {
			// check ImageType member
			// ImageType value range is different between TIM2 and CLUT2
			if(pPictHdr->MipMapTextures) {
				// if TIM2
				if(pPictHdr->ImageType==0 || pPictHdr->ImageType>5) {
					printf("Fatal: illegal ImageType value (ImageType: 0x%02X)\n",
						pPictHdr->ImageType);
					nError++;
				}
			} else {
				// if CLUT2
				if(pPictHdr->ImageType!=0x04 && pPictHdr->ImageType!=0x05) {
					printf("Fatal: illegal ImageType value (ImageType: 0x%02X)\n",
						pPictHdr->ImageType);
					nError++;
				}
			}
		}
	}


	// check PictFormat member
	if(pPictHdr->PictFormat) {
		printf("Fatal: illegal PictFormat value (PictFormat: 0x%02X)\n",
					pPictHdr->PictFormat);
		nError++;
	}

	// verify MipMapTextures, ImageWidth, ImageHeight members
	if(pPictHdr->MipMapTextures==0) {
		// if CLUT2
		if(pPictHdr->ImageWidth) {
			printf("Fatal: Inavlid ImageWidth value. (ImageWidth: %d)\n",
						pPictHdr->ImageWidth);
			nError++;
		}
		if(pPictHdr->ImageHeight) {
			printf("Fatal: Inavlid ImageHeight value. (ImageHeight: %d)\n",
						pPictHdr->ImageHeight);
			nError++;
		}
	} else if(pPictHdr->MipMapTextures>7) {
		// too many MIPMAPs
		printf("Fatal: illegal MipMapTextures value. (MipMapTextures: %d)\n",
					pPictHdr->MipMapTextures);
		nError++;
	} else {
		// if TIM2

		// check ImageWidth member
		if(pPictHdr->ImageWidth>1024) {
			printf("Fatal: illegal ImageWidth value (ImageWidth: %d)\n",
						pPictHdr->ImageWidth);
			nError++;
		} else {
			int n;
			n = 0;
			switch(pPictHdr->ImageType) {
				case TIM2_RGB32:
				case TIM2_RGB24:
				case TIM2_RGB16:
					n = 1;
					break;
				case TIM2_IDTEX8:
					n = 2;
					break;
				case TIM2_IDTEX4:
					n = 4;
					break;
			}
			if(n) {
				n = n << (pPictHdr->MipMapTextures - 1);
				if(pPictHdr->ImageWidth % n) {
					printf("Fatal: ImageWidth doesn't satisfy the request from MipMapTextures and ImageType (ImageWidth: %d, MipMapTextres: %d, ImageType 0x%02X)\n",
								pPictHdr->ImageWidth, pPictHdr->MipMapTextures, pPictHdr->ImageType);
					nError++;
				}
			} else {
				// if ImageType value is illegal, skip this checking
			}
		}

		// check ImageHeight member
		if(pPictHdr->ImageHeight>1024) {
			printf("Fatal: illegal ImageHeight value (ImageHeight: %d)\n",
						pPictHdr->ImageHeight);
			nError++;
		}
	}


	// check GsTex0 member
	if(pPictHdr->ImageType==TIM2_RGB16 ||
		pPictHdr->ImageType==TIM2_RGB24 ||
		pPictHdr->ImageType==TIM2_RGB32) {

		// in direct color mode, check if all CLUT related members are 0
		if((pPictHdr->GsTex0>>37) & 0x03FFFFFF) {
			printf("Warning: Some of GsTex0.CLD, CSA, CSM, CBP are non-zero (GsTex0: 0x%08X%08X)\n",
						(int)(pPictHdr->GsTex0>>32), (int)(pPictHdr->GsTex0 & 0xFFFFFFFF));
			nWarning++;
		}
	} else {
		// check CLD, CSA, CSM members
		if(((pPictHdr->GsTex0>>61) & 7)>5) {
			printf("Warning: illegal GsTex0.CLD value (GsTex0.CLD: %d)\n",
						(int)((pPictHdr->GsTex0>>61) & 7));
			nWarning++;
		}
		if(((pPictHdr->GsTex0>>56) & 31) && (pPictHdr->ImageType==TIM2_IDTEX8)) {
			printf("Warning: illegal GsTex0.CSA value (GsTex0.CSA: %d)\n",
						(int)((pPictHdr->GsTex0>>56) & 31));
			nWarning++;
		}
		if(((pPictHdr->GsTex0>>55) & 1) ^ ((pPictHdr->ClutType>>7) & 1)) {
			printf("Warning: GsTex0.CSM value conflicts with the CSM mode assumed by ClutType (GsTex0.CSM: %d)\n",
						(int)((pPictHdr->GsTex0>>55) & 1));
			nWarning++;
		}

		// check CPSM member
		switch(pPictHdr->ClutType & 0x3F) {
			case TIM2_RGB16:
				// if 16 bit color
				if((((pPictHdr->GsTex0>>51) & 15)!=0x2 && ((pPictHdr->GsTex0>>51) & 15)!=0xA)) {
					printf("Warning: GsTex0.CPSM value confilicts with the pixel format assumed by ClutType (GsTex0.CPSM: %d, ClutType: 0x%02X)\n",
							(int)((pPictHdr->GsTex0>>51) & 15), pPictHdr->ClutType);
					nWarning++;
				}
				break;

			case TIM2_RGB24:
				// if 24 bit color
				if(((pPictHdr->GsTex0>>51) & 15)!=0) {
					printf("Warning: GsTex0.CPSM value conflicts with the pixel format assumed by ClutType (GsTex0.CPSM: %d, ClutType: 0x%02X)\n",
							(int)((pPictHdr->GsTex0>>51) & 15), pPictHdr->ClutType);
					nWarning++;
				}
				if(((pPictHdr->GsTex0>>34) & 1)!=1) {
					printf("Warning: illegal GsTex0.TCC. In 24 bit color mode, no alpha-channel data in CLUT (GsTex0.TCC: %d, ClutType: 0x%02X)\n",
							(int)((pPictHdr->GsTex0>>34) & 1), pPictHdr->ClutType);
					nWarning++;
				}
				break;

			case TIM2_RGB32:
				// 32 bit color
				if(((pPictHdr->GsTex0>>51) & 15)!=0) {
					printf("Warning: GsTex0.CPSM value conflicts with the pixel format assumed by ClutType (GsTex0.CPSM: %d, ClutType: 0x%02X)\n",
							(int)((pPictHdr->GsTex0>>51) & 15), pPictHdr->ClutType);
					nWarning++;
				}
				break;
		}
	}

	if(pPictHdr->MipMapTextures==0) {
		// if CLUT2
		if((pPictHdr->GsTex0 & 0xFFFFFFFF)!=0 ||
			((pPictHdr->GsTex0>>32) & 1)!=0) {
			printf("Warning: Some of GsTex0.TH, TW, PSM, TBW, TBP0 are non-zero (GsTex0: 0x%08X%08X)\n",
						(int)(pPictHdr->GsTex0>>32), (int)(pPictHdr->GsTex0));
			nWarning++;
		}
	} else {
		// if TIM2
		// check TW, TH members
		if(((pPictHdr->GsTex0>>26) & 15)!=GetLog2(pPictHdr->ImageWidth)) {
			printf("Warning: GsTex0.TW conflicts with the image width assumed by ImageWidth (GsTex0.TW: %d, ImageWidth: %d)\n",
						(int)((pPictHdr->GsTex0>>26) & 15), pPictHdr->ImageWidth);
			nWarning++;
		}
		if(((pPictHdr->GsTex0>>30) & 15)!=GetLog2(pPictHdr->ImageHeight)) {
			printf("Warning: GsTex0.TH confilicts with the image height assumed by ImageHeight (GsTex0.TH: %d, ImageHeight: %d)\n",
						(int)((pPictHdr->GsTex0>>30) & 15), pPictHdr->ImageHeight);
			nWarning++;
		}

		// check PSM member
		switch((pPictHdr->GsTex0>>20) & 0x3F) {
			case 0x00:	// PSMCT32
				if(pPictHdr->ImageType!=TIM2_RGB32) {
					printf("Warning: GsTex0.PSM conflicts with the pixel format assumed by ImageType (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
					nWarning++;
				}
				break;

			case 0x01:	// PSMCT24
				if(pPictHdr->ImageType!=TIM2_RGB24) {
					printf("Warning: GsTex0.PSM conflicts with the pixel format assumed by ImageType (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
					nWarning++;
				}
				break;

			case 0x02:	// PSMCT16
			case 0x0A:	// PSMCT16S
				if(pPictHdr->ImageType!=TIM2_RGB16) {
					printf("Warning: GsTex0.PSM conflicts with the pixel format assumed by ImageType (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
					nWarning++;
				}
				break;

			case 0x13:	// PSMCT8
			case 0x1B:	// PSMCT8H
				if(pPictHdr->ImageType!=TIM2_IDTEX8) {
					printf("Warning: GsTex0.PSM conflicts with the pixel format assumed by ImageType (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
					nWarning++;
				}
				break;

			case 0x14:	// PSMCT4
			case 0x24:	// PSMCT4HH
			case 0x2C:	// PSMCT4HL
				if(pPictHdr->ImageType!=TIM2_IDTEX4) {
					printf("Warning: GsTex0.PSM conflicts with the pixel format assumed by ImageType (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
					nWarning++;
				}
				break;

			default:
				printf("Fatal: illegal GsTex0.PSM value (GsTex0.PSM: %d, ImageType: 0x%02X)\n",
								(int)((pPictHdr->GsTex0>>20) & 0x3F), pPictHdr->ImageType);
				nError++;
				break;
		}
	}

	// check GsTex1
	if(((pPictHdr->GsTex1>>32) & 0xFFFFF000)!=0 ||
		(pPictHdr->GsTex1 & 0xFFFF7C02)!=0) {
		printf("Fatal: Reserved area of GsTex1 is non-zero (GsTex1: 0x%08X%08X)\n",
					(int)(pPictHdr->GsTex1>>32), (int)(pPictHdr->GsTex1 & 0xFFFFFFFF));
		nError++;
	}
	if(((pPictHdr->GsTex1>>6) & 7)>5) {
		printf("Fatal: illegal GsTex1.MMIN value (GsTex1.MMIN: %d)\n",
					(int)((pPictHdr->GsTex1>>6) & 7));
		nError++;
	}
	if(pPictHdr->MipMapTextures &&
		((pPictHdr->GsTex1>>2) & 7)!=(pPictHdr->MipMapTextures - 1)) {
		printf("Warning: Max MIPMAP level assumed by GsTex1.MXL conflicts with  MipMapTextures (GsTex1.MXL: %d, MipMapTextures: %d)\n",
					(int)((pPictHdr->GsTex1>>2) & 7), pPictHdr->MipMapTextures);
		nWarning++;
	}

	// check GsTexaFbaPabe member
	if(pPictHdr->GsTexaFbaPabe & 0x3F007F00) {
		printf("Fatal: Reserved area of GsTexaFbaPabe is non-zero (GsTexaFbaPabe: 0x%08X)\n",
					pPictHdr->GsTexaFbaPabe);
		nError++;
	}

	// check GsTexClut member
	if(pPictHdr->GsTexClut & 0xFFC00000) {
		printf("Fatal: Reserved area of GsTexClut is non-zero (GsTexClut: 0x%08X)\n",
					pPictHdr->GsTexClut);
		nError++;
	}
	if((pPictHdr->ClutType & 0x80)==0) {
		// if CSM1
		if(pPictHdr->GsTexClut!=0) {
			printf("Warning: GsTexClut is non-zero. If CSM1 is assumed by ClutType, GsTexClut is invalid (GsTexClut: 0x%08X, ClutType: 0x%02X)\n",
						pPictHdr->GsTexClut, pPictHdr->ClutType);
			nWarning++;
		}
	}

	// if MIPMAP header exists, check it
	if(pMmHdr) {
		if((pMmHdr->GsMiptbp1>>60) & 0xF) {
			printf("Fatal: Reserved area of GsMiptbp1 is non-zero (GsMiptbp1: 0x%08X%08X)\n",
						(int)(pMmHdr->GsMiptbp1>>32), (int)(pMmHdr->GsMiptbp1 & 0xFFFFFFFF));
			nError++;
		}
		if(pPictHdr->MipMapTextures<3 &&
			((pMmHdr->GsMiptbp1>>20) & 0x000FFFFF)) {
			printf("Warning: Some of GsMiptbp1.TBP2, TBW2 are non-zero (GsMiptbp1.TBP2,TBW: 0x%05X)\n",
						(int)((pMmHdr->GsMiptbp1>>20) & 0x000FFFFF));
			nWarning++;
		}
		if(pPictHdr->MipMapTextures<4 &&
			((pMmHdr->GsMiptbp1>>40) & 0x000FFFFF)) {
			printf("Warning: Some of GsMiptbp1.TBP3,TBW3 are non-zero (GsMiptbp1.TBP3,TBW3: 0x%05X)\n",
						(int)((pMmHdr->GsMiptbp1>>40) & 0x000FFFFF));
			nWarning++;
		}
		if((pMmHdr->GsMiptbp2>>60) & 0xF) {
			printf("Fatal: Reserved area of GsMiptbp2 is non-zero (GsMiptbp2: 0x%08X%08X)\n",
						(int)(pMmHdr->GsMiptbp2>>32), (int)(pMmHdr->GsMiptbp2 & 0xFFFFFFFF));
			nError++;
		}
		if(pPictHdr->MipMapTextures<5 &&
			((pMmHdr->GsMiptbp2>>0) & 0x000FFFFF)) {
			printf("Warning: Some of GsMiptbp2.TBP4, TBW4 are non-zero (GsMiptbp2.TBP4,TBW4: 0x%05X)\n",
						(int)((pMmHdr->GsMiptbp2>>0) & 0x000FFFFF));
			nWarning++;
		}
		if(pPictHdr->MipMapTextures<6 &&
			((pMmHdr->GsMiptbp2>>20) & 0x000FFFFF)) {
			printf("Warning: Some of GsMiptbp2.TBP5, TBW5 are non-zero (GsMiptbp2.TBP5,TBW5: 0x%05X)\n",
						(int)((pMmHdr->GsMiptbp2>>20) & 0x000FFFFF));
			nWarning++;
		}
		if(pPictHdr->MipMapTextures<7 &&
			((pMmHdr->GsMiptbp2>>40) & 0x000FFFFF)) {
			printf("Warning: Some of GsMiptbp2.TBP6, TBW6 are non-zero (GsMiptbp2.TBP6,TBW6: 0x%05X)\n",
						(int)((pMmHdr->GsMiptbp2>>40) & 0x000FFFFF));
			nWarning++;
		}

		// FIXME:	check if all padding after MIPMAP header are zero
	}


	
	// return error and worning count
	if(pErrors)		*pErrors   = nError;
	if(pWarnings)	*pWarnings = nWarning;
	return;
}


// get bit width
int GetLog2(unsigned int n)
{
	int i;
	for(i=31; i>0; i--) {
		if(n & (1<<i)) {
			break;
		}
	}
	if(n>(unsigned int)(1<<i)) {
		i++;
	}
	return(i);
}




