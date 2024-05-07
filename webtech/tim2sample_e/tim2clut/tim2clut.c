// $Revision: 7 $
// $Date: 99/11/12 20:18 $

#include <stdio.h>
#include <malloc.h>
#include <memory.h>


#pragma warning(push)
#pragma warning(disable: 4200)
#include "tim2.h"
#pragma warning(pop)

// prototypes
int main(int argc, char *argv[]);
void *ReadTim2File(const char *pszFileName);
int WriteClut2File(void *pTim, const char *pszFileName);
int CountClutData(void *pTim);


// global variables
static const char szTitle[] = "TIM2 -> CLUT2 converter\n";
static const char szUsage[] = "usage:    tim2clut <input TIM2> <output CLUT2>\n";


int main(int argc, char *argv[])
{
	char *p;

	printf(szTitle);
	if(argc!=3) {
		printf(szUsage);
		return(1);
	}

	p = ReadTim2File(argv[1]);
	if(p) {
		if(CountClutData(p)) {
			WriteClut2File(p, argv[2]);
		} else {
			// If no CLUT data, exit w/o createing file
			;
		}
	}
	if(p) {
		free(p);
	}
	return(0);
}




// read TIM2 file
void *ReadTim2File(const char *pszFileName)
{
	FILE *fp;
	long l;
	char *p;

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



// write TIM2 file
int WriteClut2File(void *pTim, const char *pszFileName)
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
	char pad[128];

	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim;
	TIM2_PICTUREHEADER *pPictHdr;
	FILE *fp;
	int i;
	int align;

	memset(pad, 0x00, 128);

	// open file
	fp = fopen(pszFileName, "wb");
	if(fp==NULL) {
		return(1);
	}

	// write out TIM2 file header
	pFileHdr->FileId[0] = 'C';
	pFileHdr->FileId[1] = 'L';
	pFileHdr->FileId[2] = 'T';
	pFileHdr->FileId[3] = '2';
	pFileHdr->Pictures = (unsigned short)CountClutData(pTim);

	if(pFileHdr->FormatId==0x00) {
		// 16 byte alignment if format ID is 0x00
		fwrite(pFileHdr, 1, sizeof(TIM2_FILEHEADER), fp);
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + sizeof(TIM2_FILEHEADER));
		align = 16;
	} else {
		// 128 byte alignment if format ID is 0x01
		fwrite(pFileHdr, 1, 128, fp);
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + 0x80);
		align = 128;
	}

	// replace all ClutType members of all pictures
	for(i=0; i<pFileHdr->Pictures; i++) {
		TIM2_PICTUREHEADER *pNextPict;
		char *pUserSpace, *pClut;
		int nUserSpaceSize;
		int nHeaderSize;
		int j;

		// pre-calculate a pointer to next picture header
		pNextPict      = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);

		if(pPictHdr->ClutColors) {
			// caluclate starting address and size of user space
			pUserSpace     = (char *)pPictHdr + mmsize[pPictHdr->MipMapTextures];
			nUserSpaceSize = pPictHdr->HeaderSize - mmsize[pPictHdr->MipMapTextures];
			pClut          = (char *)pPictHdr + pPictHdr->HeaderSize + pPictHdr->ImageSize;

			// recalculate the size w/o MIPMAP header
			nHeaderSize = (sizeof(TIM2_PICTUREHEADER) + nUserSpaceSize + (align - 1)) & (~(align - 1));
			pPictHdr->HeaderSize = (unsigned short)nHeaderSize;
			pPictHdr->TotalSize = pPictHdr->HeaderSize + pPictHdr->ClutSize;

			// reset unused members
			pPictHdr->ImageSize = 0;
			pPictHdr->ImageWidth = 0;
			pPictHdr->ImageHeight = 0;
			pPictHdr->MipMapTextures = 0;

			// write out picture header
			fwrite(pPictHdr, 1, sizeof(TIM2_PICTUREHEADER), fp);

			// next, user space
			j = pPictHdr->HeaderSize - sizeof(TIM2_PICTUREHEADER);
			if(j<=nUserSpaceSize) {
				fwrite(pUserSpace, 1, j, fp);
			} else {
				fwrite(pUserSpace, 1, nUserSpaceSize, fp);
				fwrite(pad, 1, (j - nUserSpaceSize), fp);
			}

			// write out CLUT data
			fwrite(pClut, 1, pPictHdr->ClutSize, fp);
		}

		// advance pointer to next picture header
		pPictHdr = pNextPict;
	}
	fclose(fp);
	return(0);
}


// calculate CLUT data count in TIM2 file
int CountClutData(void *pTim)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim;
	TIM2_PICTUREHEADER *pPictHdr;
	int nCluts = 0;
	int i;

	if(pFileHdr->FormatId==0x00) {
		// 16 byte alignment if format ID is 0x00
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + sizeof(TIM2_FILEHEADER));
	} else {
		// 128 byte alignment if format ID is 0x01
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pFileHdr + 0x80);
	}

	// replace all ClutType members of all pictures
	for(i=0; i<pFileHdr->Pictures; i++) {
		if(pPictHdr->ClutColors) {
			nCluts++;
		}

		// advance pointer to the next picture header
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	return(nCluts);
}

