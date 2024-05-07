/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library NTGUI package (Release 3.0 version)
*/
/*
 *                      Emotion Engine Library
 *
 *      Copyright (C) 2004 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                            output.h
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *                    Aug,28,2003     wparks
 */
#ifndef __OUTPUT_H__
#define __OUTPUT_H__
#ifdef __cplusplus
extern "C" {
#endif

#define PRINTF(x) (scePrintf x)
#define VPRINTF(x) (sceVprintf x)
#define VSNPRINTF(x) (sceVsnprintf x)
#define SNPRINTF(x) (sceSnprintf x)

#ifdef __cplusplus
}
#endif
#endif /* !__OUTPUT_H__ */
