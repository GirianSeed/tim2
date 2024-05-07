/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library NTGUI package (Release 3.0 version)
 */
/*
 *            Network Configuration GUI2 Library Sample
 *
 *                         Version 0.10
 *                           Shift-JIS
 *
 *      Copyright (C) 2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.10
 */
#include <stdio.h>

// TIM2�t�@�C���w�b�_�̍\����ݒ�

#ifdef R5900

// PS2���̂Ƃ�
#include <eekernel.h>
#include <sifdev.h>
#include <libgraph.h>
#include "tim2.h"
#include "output.h"

// �v���g�^�C�v�錾
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int sx, int sy, u_long128 *pImage);
static int  Tim2CalcBufWidth(int psm, int w);
static int  Tim2CalcBufSize(int psm, int w, int h);
static int  Tim2GetLog2(int n);

#else	// R5900

// ��PS2���̂Ƃ�

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4200)
#endif	// WIN32
#include "tim2.h"
#ifdef WIN32
#pragma warning(pop)
#endif	// WIN32

#endif	// R5900



// TIM2�t�@�C���̃t�@�C���w�b�_���`�F�b�N����
// ����
// pTim2    TIM2�`���̃f�[�^�̐擪�A�h���X
// �Ԃ�l
//          0�̂Ƃ��G���[
//          1�̂Ƃ�����I��(TIM2)
//          2�̂Ƃ�����I��(CLUT2)
int Tim2CheckFileHeaer(void *pTim2)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	int i;

	// TIM2�V�O�l�`�����`�F�b�N
	if(pFileHdr->FileId[0]=='T' || pFileHdr->FileId[1]=='I' || pFileHdr->FileId[2]=='M' || pFileHdr->FileId[3]=='2') {
		// TIM2�������Ƃ�
		i = 1;
	} else if(pFileHdr->FileId[0]=='C' || pFileHdr->FileId[1]=='L' || pFileHdr->FileId[2]=='T' || pFileHdr->FileId[3]=='2') {
		// CLUT2�������Ƃ�
		i = 2;
	} else {
		// �C���[�K���Ȏ��ʕ����������Ƃ�
		PRINTF(("Tim2CheckFileHeaer: TIM2 is broken %02X,%02X,%02X,%02X\n",
					pFileHdr->FileId[0], pFileHdr->FileId[1], pFileHdr->FileId[2], pFileHdr->FileId[3]));
		return(0);
	}

	// TIM2�t�@�C���t�H�[�}�b�g�o�[�W����,�t�H�[�}�b�gID���`�F�b�N
	if(!(pFileHdr->FormatVersion==0x03 ||
	    (pFileHdr->FormatVersion==0x04 && (pFileHdr->FormatId==0x00 || pFileHdr->FormatId==0x01)))) {
		PRINTF(("Tim2CheckFileHeaer: TIM2 is broken (2)\n"));
		return(0);
	}
	return(i);
}



// �w�肳�ꂽ�ԍ��̃s�N�`���w�b�_�𓾂�
// ����
// pTim2    TIM2�`���̃f�[�^�̐擪�A�h���X
// imgno    ���Ԗڂ̃s�N�`���w�b�_���Q�Ƃ��邩�w��
// �Ԃ�l
//          �s�N�`���w�b�_�ւ̃|�C���^
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *pTim2, int imgno)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	TIM2_PICTUREHEADER *pPictHdr;
	int i;

	// �s�N�`���ԍ����`�F�b�N
	if(imgno>=pFileHdr->Pictures) {
		PRINTF(("Tim2GetPictureHeader: Illegal image no.(%d)\n", imgno));
		return(NULL);
	}

	if(pFileHdr->FormatId==0x00) {
		// �t�H�[�}�b�gID��0x00�̂Ƃ��A16�o�C�g�A���C�������g
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + sizeof(TIM2_FILEHEADER));
	} else {
		// �t�H�[�}�b�gID��0x01�̂Ƃ��A128�o�C�g�A���C�������g
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + 0x80);
	}

	// �w�肳�ꂽ�s�N�`���ԍ��܂ŃX�L�b�v
	for(i=0; i<imgno; i++) {
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	return(pPictHdr);
}


// �s�N�`���f�[�^��CLUT2�f�[�^�ł��邩�ǂ�������
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// �߂�l
//          0�̂Ƃ�TIM2
//          1�̂Ƃ�CLUT2
int Tim2IsClut2(TIM2_PICTUREHEADER *ph)
{
	// �s�N�`���w�b�_��MipMapTextures�����o�𔻕�
	if(ph->MipMapTextures==0) {
		// �e�N�X�`��������0�̂Ƃ��ACLUT2�f�[�^
		return(1);
	} else {
		// �e�N�X�`��������1���ȏ�̂Ƃ��ATIM2�f�[�^
		return(0);
	}
}


// MIPMAP���x�����Ƃ̃e�N�X�`���T�C�Y�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// mipmap:  MIPMAP�e�N�X�`�����x��
// pWidth:  X�T�C�Y���󂯎�邽�߂�int�^�ϐ��փ|�C���^
// pHeight: Y�T�C�Y���󂯎�邽�߂�int�^�ϐ��փ|�C���^
// �߂�l
//          MIPMAP�e�N�X�`���̃T�C�Y
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

	// MIPMAP�e�N�X�`���T�C�Y�̓t�@�C���w�b�_��FormatId�̎w��ɂ�����炸�A
	// ���16�o�C�g�A���C�������g���E�Ő��񂳂��
	n = (n + 15) & ~15;
	return(n);
}


// MIPMAP�w�b�_�̃A�h���X,�T�C�Y�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// pSize:   MIPMAP�w�b�_�̃T�C�Y���󂯎�邽�߂�int�^�ϐ��փ|�C���^
//          �T�C�Y���K�v�̂Ȃ��Ƃ���NULL��
// �߂�l
//          NULL�̂Ƃ�MIPMAP�w�b�_�Ȃ�
//          NULL�łȂ��Ƃ��AMIPMAP�w�b�_�̐擪�A�h���X
TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize)
{
	TIM2_MIPMAPHEADER *pMmHdr;

	static const char mmsize[] = {
		 0,  // �e�N�X�`��0��(CLUT2�f�[�^�̂Ƃ�)
		 0,  // LV0�e�N�X�`���̂�(MIPMAP�w�b�_�Ȃ�)
		32,  // LV1 MipMap�܂�
		32,  // LV2 MipMap�܂�
		32,  // LV3 MipMap�܂�
		48,  // LV4 MipMap�܂�
		48,  // LV5 MipMap�܂�
		48   // LV6 MipMap�܂�
	};

	if(ph->MipMapTextures>1) {
		pMmHdr = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	} else {
		pMmHdr = NULL;
	}

	if(pSize) {
		// �g���w�b�_���Ȃ������ꍇ�A
		*pSize = mmsize[ph->MipMapTextures];
	}
	return(pMmHdr);
}


// ���[�U�[�X�y�[�X�̃A�h���X,�T�C�Y�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// pSize:   ���[�U�[�X�y�[�X�̃T�C�Y���󂯎�邽�߂�int�^�ϐ��փ|�C���^
//          �T�C�Y���K�v�̂Ȃ��Ƃ���NULL��
// �߂�l
//          NULL�̂Ƃ����[�U�[�X�y�[�X�Ȃ�
//          NULL�łȂ��Ƃ��A���[�U�[�X�y�[�X�̐擪�A�h���X
void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;

	static const char mmsize[] = {
		sizeof(TIM2_PICTUREHEADER)     ,	// �e�N�X�`��0��(CLUT2�f�[�^�̂Ƃ�)
		sizeof(TIM2_PICTUREHEADER)     ,	// LV0�e�N�X�`���̂�(MIPMAP�w�b�_�Ȃ�)
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV1 MipMap�܂�
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV2 MipMap�܂�
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV3 MipMap�܂�
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV4 MipMap�܂�
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV5 MipMap�܂�
		sizeof(TIM2_PICTUREHEADER) + 48 	// LV6 MipMap�܂�
	};

	// �w�b�_�T�C�Y�𒲂ׂ�
	if(ph->HeaderSize==mmsize[ph->MipMapTextures]) {
		// ���[�U�[�X�y�[�X�����݂��Ȃ��Ƃ�
		if(pSize) *pSize = 0;
		return(NULL);
	}

	// ���[�U�[�X�y�[�X�����݂���Ƃ�
	pUserSpace = (void *)((char *)ph + mmsize[ph->MipMapTextures]);
	if(pSize) {
		// ���[�U�[�X�y�[�X�̃T�C�Y���v�Z
		// �g���w�b�_������ꍇ�́A�����炩��g�[�^���T�C�Y���擾
		TIM2_EXHEADER *pExHdr;

		pExHdr = (TIM2_EXHEADER *)pUserSpace;
		if(pExHdr->ExHeaderId[0]!='e' ||
			pExHdr->ExHeaderId[1]!='X' ||
			pExHdr->ExHeaderId[2]!='t' ||
			pExHdr->ExHeaderId[3]!=0x00) {

			// �g���w�b�_���Ȃ������ꍇ�A
			*pSize = (ph->HeaderSize - mmsize[ph->MipMapTextures]);
		} else {
			// �g���w�b�_���������ꍇ
			*pSize = pExHdr->UserSpaceSize;
		}
	}
	return(pUserSpace);
}


// ���[�U�[�f�[�^�̃A�h���X,�T�C�Y�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// pSize:   ���[�U�[�f�[�^�̃T�C�Y���󂯎�邽�߂�int�^�ϐ��փ|�C���^
//          �T�C�Y���K�v�̂Ȃ��Ƃ���NULL��
// �߂�l
//          NULL�̂Ƃ����[�U�[�f�[�^�Ȃ�
//          NULL�łȂ��Ƃ��A���[�U�[�f�[�^�̐擪�A�h���X
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, pSize);
	if(pUserSpace==NULL) {
		// ���[�U�[�X�y�[�X�����݂��Ȃ������Ƃ�
		return(NULL);
	}

	// ���[�U�[�X�y�[�X�Ɋg���w�b�_�����邩�ǂ����`�F�b�N
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// �g���w�b�_��������Ȃ������ꍇ
		return(pUserSpace);
	}

	// �g���w�b�_�����������ꍇ
	if(pSize) {
		// ���[�U�[�f�[�^�����̃T�C�Y��Ԃ�
		*pSize = pExHdr->UserDataSize;
	}
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER));
}


// ���[�U�[�X�y�[�X�Ɋi�[���ꂽ�R�����g������̐擪�A�h���X�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// �߂�l
//          NULL�̂Ƃ��R�����g������Ȃ�
//          NULL�łȂ��Ƃ��A�R�����g������(ASCIZ)�̐擪�A�h���X
char *Tim2GetComment(TIM2_PICTUREHEADER *ph)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, NULL);
	if(pUserSpace==NULL) {
		// ���[�U�[�X�y�[�X�����݂��Ȃ������Ƃ�
		return(NULL);
	}

	// ���[�U�[�X�y�[�X�Ɋg���w�b�_�����邩�ǂ����`�F�b�N
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// �g���w�b�_��������Ȃ������ꍇ
		return(NULL);
	}

	// �g���w�b�_���݂��Ă����Ƃ�
	if(pExHdr->UserSpaceSize==((sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize))) {
		// ���[�U�[�X�y�[�X�̗L�ӂȃT�C�Y���A�g���w�b�_�ƃ��[�U�[�f�[�^�̍��v�T�C�Y�ɓ����������Ƃ�
		// �R�����g������͑��݂��Ȃ��Ɣ��f�ł���
		return(NULL);
	}

	// �R�����g�����������ꍇ
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize);
}



// �w�肵��MIPMAP���x���̃C���[�W�f�[�^�̐擪�A�h���X�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// mipmap:  MIPMAP�e�N�X�`�����x��
// �߂�l
//          NULL�̂Ƃ��A�Y������C���[�W�f�[�^�Ȃ�
//          NULL�łȂ��Ƃ��A�C���[�W�f�[�^�̐擪�A�h���X
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap)
{
	void *pImage;
	TIM2_MIPMAPHEADER *pm;
	int i;

	if(mipmap>=ph->MipMapTextures) {
		// �w�肳�ꂽ���x����MIPMAP�e�N�X�`���͑��݂��Ȃ�
		return(NULL);
	}

	// �C���[�W�f�[�^�̐擪�A�h���X���v�Z
	pImage = (void *)((char *)ph + ph->HeaderSize);
	if(ph->MipMapTextures==1) {
		// LV0�e�N�X�`���݂̂̏ꍇ
		return(pImage);
	}

	// MIPMAP�e�N�X�`�������݂��Ă���ꍇ
	pm = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	for(i=0; i<mipmap; i++) {
		pImage = (void *)((char *)pImage + pm->MMImageSize[i]);
	}
	return(pImage);
}


// CLUT�f�[�^�̐擪�A�h���X�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// �߂�l
//          NULL�̂Ƃ��A�Y������CLUT�f�[�^�Ȃ�
//          NULL�łȂ��Ƃ��ACLUT�f�[�^�̐擪�A�h���X
void *Tim2GetClut(TIM2_PICTUREHEADER *ph)
{
	void *pClut;
	if(ph->ClutColors==0) {
		// CLUT�f�[�^�����\������F����0�̂Ƃ�
		pClut = NULL;
	} else {
		// CLUT�f�[�^�����݂���Ƃ�
		pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);
	}
	return(pClut);
}


// CLUT�J���[�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// clut:    CLUT�Z�b�g�̎w��
// no:      ���Ԗڂ̃C���f�N�X���擾���邩�w��
// �߂�l
//          RGBA32�t�H�[�}�b�g�ŐF��Ԃ�
//          clut,no���̎w��ɃG���[������Ƃ��A0x00000000(��)��Ԃ�
unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no)
{
	unsigned char *pClut;
	int n;
	unsigned char r, g, b, a;

	pClut = Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUT�f�[�^���Ȃ������Ƃ�
		return(0);
	}

	// �܂��A���Ԗڂ̐F�f�[�^���v�Z
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // �s���ȃs�N�Z���J���[�̂Ƃ�
	}
	if(n>ph->ClutColors) {
		// �w�肳�ꂽCLUT�ԍ�,�C���f�N�X�̐F�f�[�^�����݂��Ȃ������Ƃ�
		return(0);
	}

	// CLUT���̃t�H�[�}�b�g�ɂ���ẮA�z�u�����ёւ����Ă���\��������
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,16�r�b�g,���ёւ�����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,24�r�b�g,���ёւ�����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,32�r�b�g,���ёւ�����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256�F,CSM1,16�r�b�g,���ёւ�����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256�F,CSM1,24�r�b�g,���ёւ�����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256�F,CSM1,32�r�b�g,���ёւ�����
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8�`15��+16�`23��
				} else if((n & 31)<24) {
					n -= 8;    // +16�`23��+8�`15��
				}
			}
			break;
	}

	// CLUT���̃s�N�Z���t�H�[�}�b�g�Ɋ�Â��āA�F�f�[�^�𓾂�
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bit�J���[�̂Ƃ�
			r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
			g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
			b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
			a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);
			break;

		case TIM2_RGB24:
			// 24bit�J���[�̂Ƃ�
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			break;

		case TIM2_RGB32:
			// 32bit�J���[�̂Ƃ�
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			break;

		default:
			// �s���ȃs�N�Z���t�H�[�}�b�g�̏ꍇ
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// CLUT�J���[��ݒ肷��
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// clut:    CLUT�Z�b�g�̎w��
// no:      ���Ԗڂ̃C���f�N�X��ݒ肷�邩�w��
// color:   �ݒ肷��F(RGB32�t�H�[�}�b�g)
// �߂�l
//          RGBA32�t�H�[�}�b�g�ŌÂ��F��Ԃ�
//          clut,no���̎w��ɃG���[������Ƃ��A0x00000000(��)��Ԃ�
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no, unsigned int newcolor)
{
	unsigned char *pClut;
	unsigned char r, g, b, a;
	int n;

	pClut = Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUT�f�[�^���Ȃ������Ƃ�
		return(0);
	}

	// �܂��A���Ԗڂ̐F�f�[�^���v�Z
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // �s���ȃs�N�Z���J���[�̂Ƃ�
	}
	if(n>ph->ClutColors) {
		// �w�肳�ꂽCLUT�ԍ�,�C���f�N�X�̐F�f�[�^�����݂��Ȃ������Ƃ�
		return(0);
	}

	// CLUT���̃t�H�[�}�b�g�ɂ���ẮA�z�u�����ёւ����Ă���\��������
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,16�r�b�g,���ёւ�����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,24�r�b�g,���ёւ�����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,32�r�b�g,���ёւ�����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256�F,CSM1,16�r�b�g,���ёւ�����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256�F,CSM1,24�r�b�g,���ёւ�����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256�F,CSM1,32�r�b�g,���ёւ�����
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8�`15��+16�`23��
				} else if((n & 31)<24) {
					n -= 8;    // +16�`23��+8�`15��
				}
			}
			break;
	}

	// CLUT���̃s�N�Z���t�H�[�}�b�g�Ɋ�Â��āA�F�f�[�^�𓾂�
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bit�J���[�̂Ƃ�
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
			// 24bit�J���[�̂Ƃ�
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			pClut[n*3]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*3 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*3 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			break;

		case TIM2_RGB32:
			// 32bit�J���[�̂Ƃ�
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
			// �s���ȃs�N�Z���t�H�[�}�b�g�̏ꍇ
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// �e�N�Z��(�F���Ƃ͌���Ȃ�)�f�[�^�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// mipmap:  MIPMAP�e�N�X�`�����x��
// x:       �e�N�Z���f�[�^�𓾂�e�N�Z��X���W
// y:       �e�N�Z���f�[�^�𓾂�e�N�Z��Y���W
// �߂�l
//          �F���(4bit�܂���8bit�̃C���f�N�X�ԍ��A�܂���16bit,24bit,32bit�̃_�C���N�g�J���[)
unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y)
{
	unsigned char *pImage;
	int t;
	int w, h;

	pImage = Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// �w�背�x���̃e�N�X�`���f�[�^���Ȃ������ꍇ
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// �e�N�Z�����W���s���ȂƂ�
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

	// �s���ȃs�N�Z���t�H�[�}�b�g�������ꍇ
	return(0);
}



// �e�N�Z��(�F���Ƃ͌���Ȃ�)�f�[�^��ݒ肷��
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// mipmap:  MIPMAP�e�N�X�`�����x��
// x:       �e�N�Z���f�[�^�𓾂�e�N�Z��X���W
// y:       �e�N�Z���f�[�^�𓾂�e�N�Z��Y���W
// �߂�l
//          �F���(4bit�܂���8bit�̃C���f�N�X�ԍ��A�܂���16bit,24bit,32bit�̃_�C���N�g�J���[)
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y, unsigned int newtexel)
{
	unsigned char *pImage;
	int t;
	int w, h;
	unsigned int oldtexel;

	pImage = Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// �w�背�x���̃e�N�X�`���f�[�^���Ȃ������ꍇ
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// �e�N�Z�����W���s���ȂƂ�
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

	// �s���ȃs�N�Z���t�H�[�}�b�g�������ꍇ
	return(0);
}


// �e�N�X�`���J���[�𓾂�
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// mipmap:  MIPMAP�e�N�X�`�����x��
// clut:    �C���f�N�X�J���[�̕ϊ��ɗp����CLUT�Z�b�g�ԍ�
// x:       �e�N�Z���f�[�^�𓾂�e�N�Z��X���W
// y:       �e�N�Z���f�[�^�𓾂�e�N�Z��Y���W
// �߂�l
//          RGBA32�t�H�[�}�b�g�ŐF��Ԃ�
//          clut�̎w��ɃG���[������Ƃ��A0x00000000(��)��Ԃ�
//          x,y�̎w��ɃG���[���������Ƃ��̓���͕ۏ؂��Ȃ�
unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph, int mipmap, int clut, int x, int y)
{
	unsigned int t;
	if(Tim2GetImage(ph, mipmap)==NULL) {
		// �w�背�x���̃e�N�X�`���f�[�^���Ȃ������ꍇ
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






// ����ȍ~�̊֐��́APS2��ee-gcc�ł̂ݎg�p�ł���֐�
#ifdef R5900

// TIM2�s�N�`���f�[�^��GS���[�J���������ɓǂݍ���
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// tbp:     �]����e�N�X�`���o�b�t�@�̃y�[�W�x�[�X�A�h���X(-1�̂Ƃ��w�b�_���̒l���g�p)
// cbp:     �]����CLUT�o�b�t�@�̃y�[�W�x�[�X�A�h���X(-1�̂Ƃ��w�b�_���̒l���g�p)
// �߂�l
//          NULL�̂Ƃ�	�G���[
//          NULL�łȂ��Ƃ��A���̃o�b�t�@�A�h���X
// ����
//          CLUT�i�[���[�h�Ƃ���CSM2���w�肳��Ă����ꍇ���A�����I��CSM1�ɕϊ�����GS�ɑ��M�����
unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph, unsigned int tbp, unsigned int cbp)
{
	// CLUT�f�[�^��]���]��
	tbp = Tim2LoadImage(ph, tbp);
	Tim2LoadClut(ph, cbp);
	return(tbp);
}


// TIM2�s�N�`���̃C���[�W�f�[�^����GS���[�J���������ɓǂݍ���
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// tbp:     �]����e�N�X�`���o�b�t�@�̃y�[�W�x�[�X�A�h���X(-1�̂Ƃ��w�b�_���̒l���g�p)
// �߂�l
//          NULL�̂Ƃ�	�G���[
//          NULL�łȂ��Ƃ��A���̃e�N�X�`���o�b�t�@�A�h���X
// ����
//          CLUT�i�[���[�h�Ƃ���CSM2���w�肳��Ă����ꍇ���A�����I��CSM1�ɕϊ�����GS�ɑ��M�����
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp)
{
	// �C���[�W�f�[�^�𑗐M
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

		psm = psmtbl[ph->ImageType - 1]; // �s�N�Z���t�H�[�}�b�g�𓾂�
		((sceGsTex0 *)&ph->GsTex0)->PSM  = psm;

		w = ph->ImageWidth;  // �C���[�WX�T�C�Y
		h = ph->ImageHeight; // �C���[�WY�T�C�Y

		// �C���[�W�f�[�^�̐擪�A�h���X���v�Z
		pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
		if(tbp==-1) {
			// tbp�̎w�肪-1�̂Ƃ��A�s�N�`���w�b�_�Ɏw�肳�ꂽGsTex0����tbp,tbw�𓾂�
			tbp = ((sceGsTex0 *)&ph->GsTex0)->TBP0;
			tbw = ((sceGsTex0 *)&ph->GsTex0)->TBW;

			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // �e�N�X�`���f�[�^��GS�ɓ]��
		} else {
			// tbp�̎w�肪�w�肳�ꂽ�Ƃ��A�s�N�`���w�b�_��GsTex0�����o��tbp,tbw���I�[�o�[���C�h
			tbw = Tim2CalcBufWidth(psm, w);
			// GS��TEX0���W�X�^�ɐݒ肷��l���X�V
			((sceGsTex0 *)&ph->GsTex0)->TBP0 = tbp;
			((sceGsTex0 *)&ph->GsTex0)->TBW  = tbw;
			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // �e�N�X�`���f�[�^��GS�ɓ]��
			tbp += Tim2CalcBufSize(psm, w, h);            // tbp�̒l���X�V
		}

		if(ph->MipMapTextures>1) {
			// MIPMAP�e�N�X�`��������ꍇ
			TIM2_MIPMAPHEADER *pm;

			pm = (TIM2_MIPMAPHEADER *)(ph + 1); // �s�N�`���w�b�_�̒����MIPMAP�w�b�_

			// LV0�̃e�N�X�`���T�C�Y�𓾂�
			// tbp�������Ŏw�肳�ꂽ�Ƃ��A�s�N�`���w�b�_�ɂ���miptbp�𖳎����Ď����v�Z
			if(tbp!=-1) {
				pm->GsMiptbp1 = 0;
				pm->GsMiptbp2 = 0;
			}

			pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
			// �eMIPMAP���x���̃C���[�W��]��
			for(i=1; i<ph->MipMapTextures; i++) {
				// MIPMAP���x����������ƁA�e�N�X�`���T�C�Y��1/2�ɂȂ�
				w = w / 2;
				h = h / 2;

				pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
				if(tbp==-1) {
					// �e�N�X�`���y�[�W�̎w�肪-1�̂Ƃ��AMIPMAP�w�b�_�Ɏw�肳�ꂽtbp,tbw���g�p����
					int miptbp;
					if(i<4) {
						// MIPMAP���x��1,2,3�̂Ƃ�
						miptbp = (pm->GsMiptbp1>>((i-1)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp1>>((i-1)*20 + 14)) & 0x3F;
					} else {
						// MIPMAP���x��4,5,6�̂Ƃ�
						miptbp = (pm->GsMiptbp2>>((i-4)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp2>>((i-4)*20 + 14)) & 0x3F;
					}
					Tim2LoadTexture(psm, miptbp, tbw, w, h, pImage);
				} else {
					// �e�N�X�`���y�[�W���w�肳��Ă���Ƃ��AMIPMAP�w�b�_���Đݒ�
					tbw = Tim2CalcBufWidth(psm, w);    // �e�N�X�`�������v�Z
					if(i<4) {
						// MIPMAP���x��1,2,3�̂Ƃ�
						pm->GsMiptbp1 |= ((u_long)tbp)<<((i-1)*20);
						pm->GsMiptbp1 |= ((u_long)tbw)<<((i-1)*20 + 14);
					} else {
						// MIPMAP���x��4,5,6�̂Ƃ�
						pm->GsMiptbp2 |= ((u_long)tbp)<<((i-4)*20);
						pm->GsMiptbp2 |= ((u_long)tbw)<<((i-4)*20 + 14);
					}
					Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);
					tbp += Tim2CalcBufSize(psm, w, h); // tbp�̒l���X�V
				}
			}
		}
	}
	return(tbp);
}



// TIM2�s�N�`����CLUT�f�[�^����GS���[�J���������ɓ]��
// ����
// ph:      TIM2�s�N�`���w�b�_�̐擪�A�h���X
// cbp:     �]����CLUT�o�b�t�@�̃y�[�W�x�[�X�A�h���X(-1�̂Ƃ��w�b�_���̒l���g�p)
// �߂�l
//          0�̂Ƃ��G���[
//          ��0�̂Ƃ�����
// ����
//          CLUT�i�[���[�h�Ƃ���CSM2���w�肳��Ă����ꍇ���A�����I��CSM1�ɕϊ�����GS�ɑ��M�����
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp)
{
	int i;
	sceGsLoadImage li;
	u_long128 *pClut;
	int	cpsm;

	// CLUT�s�N�Z���t�H�[�}�b�g�𓾂�
	if(ph->ClutType==TIM2_NONE) {
		// CLUT�f�[�^�����݂��Ȃ��Ƃ�
		return(1);
	} else if((ph->ClutType & 0x3F)==TIM2_RGB16) {
		cpsm = SCE_GS_PSMCT16;
	} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
		cpsm = SCE_GS_PSMCT24;
	} else {
		cpsm = SCE_GS_PSMCT32;
	}
	((sceGsTex0 *)&ph->GsTex0)->CPSM = cpsm; // CLUT���s�N�Z���t�H�[�}�b�g�ݒ�
	((sceGsTex0 *)&ph->GsTex0)->CSM  = 0;    // CLUT�i�[���[�h(���CSM1)
	((sceGsTex0 *)&ph->GsTex0)->CSA  = 0;    // CLUT�G���g���I�t�Z�b�g(���0)
	((sceGsTex0 *)&ph->GsTex0)->CLD  = 1;    // CLUT�o�b�t�@�̃��[�h����(��Ƀ��[�h)

	if(cbp==-1) {
		// cbp�̎w�肪�Ȃ��Ƃ��A�s�N�`���w�b�_��GsTex0�����o����l���擾
		cbp = ((sceGsTex0 *)&ph->GsTex0)->CBP;
	} else {
		// cbp���w�肳�ꂽ�Ƃ��A�s�N�`���w�b�_��GsTex0�����o�̒l���I�[�o�[���C�h
		((sceGsTex0 *)&ph->GsTex0)->CBP = cbp;
	}

	// CLUT�f�[�^�̐擪�A�h���X���v�Z
	pClut = (u_long128 *)((char *)ph + ph->HeaderSize + ph->ImageSize);

	// CLUT�f�[�^��GS���[�J���������ɑ��M
	// CLUT�`���ƃe�N�X�`���`���ɂ����CLUT�f�[�^�̃t�H�[�}�b�g�Ȃǂ����܂�
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,16�r�b�g,���ёւ�����
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,24�r�b�g,���ёւ�����
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16�F,CSM1,32�r�b�g,���ёւ�����
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256�F,CSM1,16�r�b�g,���ёւ�����
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256�F,CSM1,24�r�b�g,���ёւ�����
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256�F,CSM1,32�r�b�g,���ёւ�����
			// 256�FCLUT���ACLUT�i�[���[�h��CSM1�̂Ƃ�
			// 16�FCLUT���ACLUT�i�[���[�h��CSM1�œ���ւ��ς݃t���O��ON�̂Ƃ�
			// ���łɃs�N�Z��������ւ����Ĕz�u����Ă���̂ł��̂܂ܓ]���\���[
			Tim2LoadTexture(cpsm, cbp, 1, 16, (ph->ClutColors / 16), pClut);
			break;

		case (( TIM2_RGB16        <<8) | TIM2_IDTEX4): // 16�F,CSM1,16�r�b�g,���j�A�z�u
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX4): // 16�F,CSM1,24�r�b�g,���j�A�z�u
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX4): // 16�F,CSM1,32�r�b�g,���j�A�z�u
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX4): // 16�F,CSM2,16�r�b�g,���j�A�z�u
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX4): // 16�F,CSM2,24�r�b�g,���j�A�z�u
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX4): // 16�F,CSM2,32�r�b�g,���j�A�z�u
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX8): // 256�F,CSM2,16�r�b�g,���j�A�z�u
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX8): // 256�F,CSM2,24�r�b�g,���j�A�z�u
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX8): // 256�F,CSM2,32�r�b�g,���j�A�z�u
			// 16�FCLUT���ACLUT�i�[���[�h��CSM1�œ���ւ��ς݃t���O��OFF�̂Ƃ�
			// 16�FCLUT���ACLUT�i�[���[�h��CSM2�̂Ƃ�
			// 256�FCLUT���ACLUT�i�[���[�h��CSM2�̂Ƃ�

			// CSM2�̓p�t�H�[�}���X�������̂ŁACSM1�Ƃ��ē���ւ��Ȃ���]��
			for(i=0; i<(ph->ClutColors/16); i++) {
				sceGsSetDefLoadImage(&li, cbp, 1, cpsm, (i & 1)*8, (i>>1)*2, 8, 2);
				FlushCache(WRITEBACK_DCACHE);   // D�L���b�V���̑|���o��
				sceGsExecLoadImage(&li, pClut); // CLUT�f�[�^��GS�ɓ]��
				sceGsSyncPath(0, 0);            // �f�[�^�]���I���܂őҋ@

				// ����16�F�ցA�A�h���X�X�V
				if((ph->ClutType & 0x3F)==TIM2_RGB16) {
					pClut = (u_long128 *)((char *)pClut + 2*16); // 16bit�F�̂Ƃ�
				} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
					pClut = (u_long128 *)((char *)pClut + 3*16); // 24bit�F�̂Ƃ�
				} else {
					pClut = (u_long128 *)((char *)pClut + 4*16); // 32bit�F�̂Ƃ�
				}
			}
			break;

		default:
			PRINTF(("Illegal clut and texture combination. ($%02X,$%02X)\n", ph->ClutType, ph->ImageType));
			return(0);
	}
	return(1);
}


// TIM2�t�@�C���ŃX�i�b�v�V���b�g�������o��
// ����
// d0:      �������X�^�̃t���[���o�b�t�@
// d1:      ����X�^�̃t���[���o�b�t�@(NULL�̂Ƃ��m���C���^���[�X)
// pszFname:TIM2�t�@�C����
// �Ԃ�l
//          0�̂Ƃ��G���[
//          ��0�̂Ƃ�����
int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1, char *pszFname)
{
	int i;

	int h;               // �t�@�C���n���h��
	int nWidth, nHeight; // �C���[�W�̐��@
	int nImageType;      // �C���[�W�����
	int psm;             // �s�N�Z���t�H�[�}�b�g
	int nBytes;          // 1���X�^���\������o�C�g��

	// �摜�T�C�Y,�s�N�Z���t�H�[�}�b�g�𓾂�
	nWidth  = d0->display.DW / (d0->display.MAGH + 1);
	nHeight = d0->display.DH + 1;
	psm     = d0->dispfb.PSM;

	// �s�N�Z���t�H�[�}�b�g����A1���X�^������̃o�C�g��,TIM2�s�N�Z����ʂ����߂�
	switch(psm) {
		case SCE_GS_PSMCT32 :	nBytes = nWidth*4;	nImageType = TIM2_RGB32;	break;
		case SCE_GS_PSMCT24 :	nBytes = nWidth*3;	nImageType = TIM2_RGB24;	break;
		case SCE_GS_PSMCT16 :	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		case SCE_GS_PSMCT16S:	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		default:
			// �s���ȃs�N�Z���t�H�[�}�b�g�̂Ƃ��A�G���[�I��
			// GS_PSGPU24�t�H�[�}�b�g�͔�T�|�[�g
			PRINTF(("Illegal pixel format.\n"));
			return(0);
	}


	// TIM2�t�@�C�����I�[�v��
	h = sceOpen(pszFname, SCE_WRONLY | SCE_CREAT);
	if(h==-1) {
		// �t�@�C���I�[�v�����s
		PRINTF(("file create failure.\n"));
		return(0);
	}

	// �t�@�C���w�b�_�������o��
	{
		TIM2_FILEHEADER fhdr;

		fhdr.FileId[0] = 'T';   // �t�@�C��ID��ݒ�
		fhdr.FileId[1] = 'I';
		fhdr.FileId[2] = 'M';
		fhdr.FileId[3] = '2';
		fhdr.FormatVersion = 3; // �t�H�[�}�b�g�o�[�W����4
		fhdr.FormatId  = 0;     // 16�o�C�g�A���C�������g���[�h
		fhdr.Pictures  = 1;     // �s�N�`��������1��
		for(i=0; i<8; i++) {
			fhdr.pad[i] = 0x00; // �p�f�B���O�����o��0x00�ŃN���A
		}

		sceWrite(h, &fhdr, sizeof(TIM2_FILEHEADER)); // �t�@�C���w�b�_�������o��
	}

	// �s�N�`���w�b�_�������o��
	{
		TIM2_PICTUREHEADER phdr;
		int nImageSize;

		nImageSize = nBytes * nHeight;
		phdr.TotalSize   = sizeof(TIM2_PICTUREHEADER) + nImageSize; // �g�[�^���T�C�Y
		phdr.ClutSize    = 0;                           // CLUT���Ȃ�
		phdr.ImageSize   = nImageSize;                  // �C���[�W���T�C�Y
		phdr.HeaderSize  = sizeof(TIM2_PICTUREHEADER);  // �w�b�_���T�C�Y
		phdr.ClutColors  = 0;                           // CLUT�F��
		phdr.PictFormat  = 0;                           // �s�N�`���`��
		phdr.MipMapTextures = 1;                        // MIPMAP�e�N�X�`������
		phdr.ClutType    = TIM2_NONE;                   // CLUT���Ȃ�
		phdr.ImageType   = nImageType;                  // �C���[�W���
		phdr.ImageWidth  = nWidth;                      // �C���[�W����
		phdr.ImageHeight = nHeight;                     // �C���[�W����

		// GS���W�X�^�̐ݒ�͑S��0�ɂ��Ă���
		phdr.GsTex0        = 0;
		((sceGsTex0 *)&phdr.GsTex0)->TBW = Tim2CalcBufWidth(psm, nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->PSM = psm;
		((sceGsTex0 *)&phdr.GsTex0)->TW  = Tim2GetLog2(nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->TH  = Tim2GetLog2(nHeight);
		phdr.GsTex1        = 0;
		phdr.GsTexaFbaPabe = 0;
		phdr.GsTexClut     = 0;

		sceWrite(h, &phdr, sizeof(TIM2_PICTUREHEADER)); // �s�N�`���w�b�_�������o��
	}


	// �C���[�W�f�[�^�̏����o��
	for(i=0; i<nHeight; i++) {
		u_char buf[4096];   // ���X�^�o�b�t�@��4KB�m��
		sceGsStoreImage si;

		if(d1) {
			// �C���^���[�X�̂Ƃ�
			if(!(i & 1)) {
				// �C���^���[�X�\���̋������X�^�̂Ƃ�
				sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
				                      d0->dispfb.DBX, (d0->dispfb.DBY + i/2),
				                      nWidth, 1);
			} else {
				// �C���^���[�X�\���̊���X�^�̂Ƃ�
				sceGsSetDefStoreImage(&si, d1->dispfb.FBP*32, d1->dispfb.FBW, psm,
				                      d1->dispfb.DBX, (d1->dispfb.DBY + i/2),
				                      nWidth, 1);
			}
		} else {
			// �m���C���^���[�X�̂Ƃ�
			sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
			                      d0->dispfb.DBX, (d0->dispfb.DBY + i),
			                      nWidth, 1);
		}
		FlushCache(WRITEBACK_DCACHE); // D�L���b�V���̑|���o��

		sceGsExecStoreImage(&si, (u_long128 *)buf); // VRAM�ւ̓]�����N��
		sceGsSyncPath(0, 0);                        // �f�[�^�]���I���܂őҋ@

		sceWrite(h, buf, nBytes);  // 1���X�^���̃f�[�^�������o��
	}

	// �t�@�C���̏����o������
	sceClose(h);  // �t�@�C�����N���[�Y
	return(1);
}






// �e�N�X�`���f�[�^��]��
// ����
// psm:     �e�N�X�`���s�N�Z���t�H�[�}�b�g
// tbp:     �e�N�X�`���o�b�t�@�̃x�[�X�|�C���g
// tbw:     �e�N�X�`���o�b�t�@�̕�
// w:       �]���̈�̉���
// h:       �]���̈�̃��C����
// pImage:  �e�N�X�`���C���[�W���i�[����Ă���A�h���X
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

	// DMA�̍ő�]���ʂ�512KB�𒴂��Ȃ��悤�ɕ������Ȃ��瑗�M
	l = 32764 * 16 / n;
	for(i=0; i<h; i+=l) {
		p = (u_long128 *)((char *)pImage + n*i);
		if((i+l)>h) {
			l = h - i;
		}

		sceGsSetDefLoadImage(&li, tbp, tbw, psm, 0, i, w, l);
		FlushCache(WRITEBACK_DCACHE); // D�L���b�V���̑|���o��
		sceGsExecLoadImage(&li, p);   // GS���[�J���������ւ̓]�����N��
		sceGsSyncPath(0, 0);          // �f�[�^�]���I���܂őҋ@
	}
	return;
}



// �w�肳�ꂽ�s�N�Z���t�H�[�}�b�g�ƃe�N�X�`���T�C�Y����A�o�b�t�@�T�C�Y���v�Z
// ����
// psm:     �e�N�X�`���s�N�Z���t�H�[�}�b�g
// w:       ����
// �Ԃ�l
//          1���C���ŏ����o�b�t�@�T�C�Y
//          �P�ʂ�256�o�C�g(64word)
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



// �w�肳�ꂽ�s�N�Z���t�H�[�}�b�g�ƃe�N�X�`���T�C�Y����A�o�b�t�@�T�C�Y���v�Z
// ����
// psm:     �e�N�X�`���s�N�Z���t�H�[�}�b�g
// w:       ����
// h:       ���C����
// �Ԃ�l
//          1���C���ŏ����o�b�t�@�T�C�Y
//          �P�ʂ�256�o�C�g(64word)
// ����
//          �����e�N�X�`�����قȂ�s�N�Z���t�H�[�}�b�g���i�[����ꍇ�́A
//          2KB�y�[�W�o�E���_���܂�tbp���A���C�������g�𒲐�����K�v������B
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
			// 1�s�N�Z��������A1���[�h����
			return(((w+63)/64) * ((h+31)/32));

		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			// 1�s�N�Z��������A1/2���[�h����
			return(((w+63)/64) * ((h+63)/64));

		case SCE_GS_PSMT8:
			// 1�s�N�Z��������A1/4���[�h����
			return(((w+127)/128) * ((h+63)/64));

		case SCE_GS_PSMT4:
			// 1�s�N�Z��������A1/8���[�h����
			return(((w+127)/128) * ((h+127)/128));
	}
	// �G���[?
	return(0);
*/
}



// �r�b�g���𓾂�
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


#endif	// R5900
