/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 3.0
 */
/*
 *                      ATOK Library Sample
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

#ifndef _TIM2_H_INCLUDED
#define _TIM2_H_INCLUDED


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
// �T�C�Y����̂��߂Ɍ^��`
typedef unsigned char  TIM2_UCHAR8;
typedef unsigned int   TIM2_UINT32;
typedef unsigned short TIM2_UINT16;

#ifdef R5900
typedef unsigned long  TIM2_UINT64;     // PS2
#else  // R5900
#ifdef WIN32
typedef unsigned __int64 TIM2_UINT64;   // Win32
#else  // WIN32
typedef unsigned long long TIM2_UINT64; // GNU-C
#endif // WIN32
#endif // R5900


// �s�N�`���w�b�_��ClutType,ImageType�Ŏg�p�����萔�̒�`
enum TIM2_gattr_type {
	TIM2_NONE = 0,  // CLUT�f�[�^�Ȃ��̂Ƃ���ClutType�Ŏg�p
	TIM2_RGB16,     // 16bit�J���[(ClutType,ImageType�����Ŏg�p)
	TIM2_RGB24,     // 24bit�J���[(ImageType�ł̂ݎg�p)
	TIM2_RGB32,     // 32bit�J���[(ClutType,ImageType�����Ŏg�p)
	TIM2_IDTEX4,    // 16�F�e�N�X�`��(ImageType�ł̂ݎg�p)
	TIM2_IDTEX8     // 16�F�e�N�X�`��(ImageType�ł̂ݎg�p)
};

// TIM2�t�@�C���w�b�_
typedef struct {
	TIM2_UCHAR8 FileId[4];      // �t�@�C��ID('T','I','M','2'�܂���'C','L','T','2')
	TIM2_UCHAR8 FormatVersion;  // �t�@�C���t�H�[�}�b�g�o�[�W����
	TIM2_UCHAR8 FormatId;       // �t�H�[�}�b�gID
	TIM2_UINT16 Pictures;       // �s�N�`���f�[�^��
	TIM2_UCHAR8 pad[8];         // 16�o�C�g�A���C�������g
} TIM2_FILEHEADER;


// TIM2�s�N�`���w�b�_
typedef struct {
	TIM2_UINT32 TotalSize;      // �s�N�`���f�[�^�S�̂̃o�C�g�T�C�Y
	TIM2_UINT32 ClutSize;       // CLUT�f�[�^���̃o�C�g�T�C�Y
	TIM2_UINT32 ImageSize;      // �C���[�W�f�[�^���̃o�C�g�T�C�Y
	TIM2_UINT16 HeaderSize;     // �摜�w�b�_�T�C�Y
	TIM2_UINT16 ClutColors;     // �N���b�g�T�C�Y�i�N���b�g���̐F���j
	TIM2_UCHAR8 PictFormat;     // �摜ID
	TIM2_UCHAR8 MipMapTextures; // MIPMAP����
	TIM2_UCHAR8 ClutType;       // �N���b�g�����
	TIM2_UCHAR8 ImageType;      // �C���[�W�����
	TIM2_UINT16 ImageWidth;     // �摜���T�C�Y(�r�b�g���ł͂���܂���)
	TIM2_UINT16 ImageHeight;    // �摜�c�T�C�Y(�r�b�g���ł͂���܂���)

	TIM2_UINT64 GsTex0;         // TEX0
	TIM2_UINT64 GsTex1;         // TEX1
	TIM2_UINT32 GsTexaFbaPabe;  // TEXA, FBA, PABE�̍����r�b�g
	TIM2_UINT32 GsTexClut;      // TEXCLUT(����32bit���̂܂�)
} TIM2_PICTUREHEADER;


// TIM2�~�b�v�}�b�v�w�b�_
typedef struct {
	TIM2_UINT64 GsMiptbp1;      // MIPTBP1(64�r�b�g���̂܂�)
	TIM2_UINT64 GsMiptbp2;      // MIPTBP2(64�r�b�g���̂܂�)
	TIM2_UINT32 MMImageSize[0]; // MIPMAP[?]���ڂ̉摜�o�C�g�T�C�Y
} TIM2_MIPMAPHEADER;


// TIM2�g���w�b�_
typedef struct {
	TIM2_UCHAR8 ExHeaderId[4];  // �g���R�����g�w�b�_ID('e','X','t','\x00')
	TIM2_UINT32 UserSpaceSize;  // ���[�U�[�X�y�[�X�T�C�Y
	TIM2_UINT32 UserDataSize;   // ���[�U�[�f�[�^�̃T�C�Y
	TIM2_UINT32 Reserved;       // ���U�[�u
} TIM2_EXHEADER;


// TIM2�t�@�C�����[�_�̃v���g�^�C�v�錾
int   Tim2CheckFileHeaer(void *p);  // TIM2�t�@�C�����ǂ����`�F�b�N
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *p, int imgno); // �w�薇���ڂ̃s�N�`���w�b�_���擾
int   Tim2IsClut2(TIM2_PICTUREHEADER *ph);                    // TIM2,CLUT2�̔���

int   Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph,
                   int mipmap, int *pWidth, int *pHeight);  // MIPMAP���x�����Ƃ̃s�N�`���T�C�Y���擾

TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize);
                                                            // MIPMAP�w�b�_�̊J�n�A�h���X,�T�C�Y���擾

void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize); // ���[�U�[�X�y�[�X�̊J�n�A�h���X,�T�C�Y���擾
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize);  // ���[�U�[�f�[�^�̊J�n�A�h���X,�T�C�Y���擾
char *Tim2GetComment(TIM2_PICTUREHEADER *ph);               // �R�����g������̊J�n�A�h���X���擾
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap);     // �w�背�x���̃C���[�W�f�[�^�̊J�n�A�h���X���擾
void *Tim2GetClut(TIM2_PICTUREHEADER *ph);                  // CLUT�f�[�^�̊J�n�A�h���X���擾

unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph,
                    int clut, int no);                        // �w��CLUT�Z�b�g,�C���f�N�X��CLUT�J���[�f�[�^���擾
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph,
                    int clut, int no, unsigned int newcolor); // �w��CLUT�Z�b�g,�C���f�N�X��CLUT�J���[�f�[�^��ݒ�

unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph,
                    int mipmap, int x, int y);        // �w��MIPMAP���x���̎w��e�N�Z�����W����e�N�Z���f�[�^���擾
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph,
                    int mipmap, int x, int y, unsigned int newtexel); // �w�背�x���̎w����W�փe�N�Z���f�[�^��ݒ�

unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph,
                    int mipmap, int clut, int x, int y); // �w��MIPMAP���x������A�w��CLUT���g���ăe�N�Z���J���[���擾


#ifdef R5900

unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph,
                    unsigned int tbp, unsigned int cbp);   // �C���[�W�f�[�^,CLUT�f�[�^��GS���[�J���������֓]��
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp); // �C���[�W�f�[�^��GS���[�J���������֓]��
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp);  // CLUT�f�[�^��GS���[�J���������֓]��

int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1,
                    char *pszFname);    // �X�i�b�v�V���b�g�摜��TIM2�t�@�C���Ńz�X�gHDD�ɏ����o��

#endif // R5900

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* _TIM2_H_INCLUDED */
