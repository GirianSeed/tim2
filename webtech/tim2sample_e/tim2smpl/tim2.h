#ifndef _TIM2_H_INCLUDED
#define _TIM2_H_INCLUDED


// portable typde definition
typedef unsigned char  TIM2_UCHAR8;
typedef unsigned int   TIM2_UINT32;
typedef unsigned short TIM2_UINT16;

#ifdef R5900
typedef unsigned long  TIM2_UINT64;		// PS2
#else	// R5900
#ifdef WIN32
typedef unsigned __int64 TIM2_UINT64;	// Win32
#else	// WIN32
typedef unsigned long long TIM2_UINT64;	// GNU-C
#endif	// WIN32
#endif	// R5900


// constant definition for ClutType, ImageType in picture header
enum TIM2_gattr_type {
	TIM2_NONE = 0,			// no CLUT (for ClutType)
	TIM2_RGB16,				// 16 bit color (for both of ClutType, ImageType)
	TIM2_RGB24,				// 24 bit color (for ImageType)
	TIM2_RGB32,				// 32 bit color (for ClutType, ImageType)
	TIM2_IDTEX4,			// 16 color texture (for ImageType)
	TIM2_IDTEX8				// 256 color texture (for ImageType)
};

// TIM2 file header
typedef struct {
	TIM2_UCHAR8 FileId[4];				// file ID ('T','I','M','2' or 'C','L','T','2')
	TIM2_UCHAR8 FormatVersion;			// version of file format
	TIM2_UCHAR8 FormatId;				// ID of format
	TIM2_UINT16 Pictures;				// number of picture data
	TIM2_UCHAR8 pad[8];					// for alignment
} TIM2_FILEHEADER;


// TIM2 picture header
typedef struct {
	TIM2_UINT32 TotalSize;				// total size of the picture data in bytes
	TIM2_UINT32 ClutSize;				// CLUT data size in bytes
	TIM2_UINT32 ImageSize;				// image data size in bytes
	TIM2_UINT16 HeaderSize;				// amount of headers
	TIM2_UINT16 ClutColors;				// colors in CLUT
	TIM2_UCHAR8 PictFormat;				// picture format
	TIM2_UCHAR8 MipMapTextures;			// number of MIPMAP texture
	TIM2_UCHAR8 ClutType;				// CLUT type
	TIM2_UCHAR8 ImageType;				// image type
	TIM2_UINT16 ImageWidth;				// width of image (not in bits)
	TIM2_UINT16 ImageHeight;			// height of image (not in bits)

	TIM2_UINT64 GsTex0;					// TEX0
	TIM2_UINT64 GsTex1;					// TEX1
	TIM2_UINT32 GsTexaFbaPabe;			// bitfield of TEXA, FBA and PABE
	TIM2_UINT32 GsTexClut;				// TEXCLUT (lower 32 bits)
} TIM2_PICTUREHEADER;


// TIM2 MIPMAP header
typedef struct {
	TIM2_UINT64 GsMiptbp1;				// MIPTBP1 (actual 64 bit image)
	TIM2_UINT64 GsMiptbp2;				// MIPTBP2 (actual 64 bit image)
	TIM2_UINT32 MMImageSize[0];			// image size of N-th MIPMAP texture in bytes
} TIM2_MIPMAPHEADER;


// TIM2 extended header
typedef struct {
	TIM2_UCHAR8 ExHeaderId[4];			// extended header ID ('e','X','t','\x00')
	TIM2_UINT32 UserSpaceSize;			// size of user space
	TIM2_UINT32 UserDataSize;			// size of user data
	TIM2_UINT32 Reserved;				// reserved
} TIM2_EXHEADER;


// prototypes for TIM2 file loader
int   Tim2CheckFileHeaer(void *p);								// check if the file is TIM2
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *p, int imgno);	// get imgno-th picture header
int   Tim2IsClut2(TIM2_PICTUREHEADER *ph);						// check if it is CLUT2

int Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph,
					int mipmap, int *pWidth, int *pHeight);		// get picture size of mipmap-level texture

TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize);
																// get top address and size of MIPMAP header

void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize);		// get top address and size of user space
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize);		// get top address and size of user data
char *Tim2GetComment(TIM2_PICTUREHEADER *ph);					// get top address of comment string
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap);			// get top address of image data of mipma-level texture
void *Tim2GetClut(TIM2_PICTUREHEADER *ph);						// get top address of CLUT data

unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph,
					int clut, int no);							// get color data of assumed CLUT set, assumed index
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph,
					int clut, int no, unsigned int newcolor);	// set color data of assumed CLUT set, assumed index

unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph,
					int mipmap, int x, int y);					// get texel data of assumed MIPMAP level, assumed texel coordinates
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph,
					int mipmap, int x, int y, unsigned int newtexel);	// set texel data of assumed level, assumed coordinates

unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph,
					int mipmap, int clut, int x, int y);		// get texel color of assumed MIPMAP level and coordinates, using assumed CLUT


#ifdef R5900
unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph,
					unsigned int tbp, unsigned int cbp);		// transfer image data and CLUT data to GS local memory
#endif	// R5900

#endif /* _TIM2_H_INCLUDED */
