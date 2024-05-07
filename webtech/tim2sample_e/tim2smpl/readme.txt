TIM2 - PlayStation2 2D Graphics - File Format Specification
                [PRELIMINARY, CONFIDENTIAL]

                                                                     99/11/18
                                                         web technology Corp.

Sample program for TIM2 format

This program is a sample program for TIM2 format as a reference.

TIM2 format is specified by web technology Corp., as a 2D image format for PS2.
We wish this TIM2 format is well utilized as a standard 2D image format for PS2
by Developers and Tool vendors of PS2 software.

No limitation in utilizing TIM2 format by authorized Developers and Tool vendors
of PS2. No royalty fee requested.

However, TIM2 format includes technical information of PS2, so disclosure
of TIM2 format and related programs to other than authorized Developers and
Tool vendors is prohibited.

TIM2 format is still preliminary. Please be aware of further enhancements
in case of developing programs according to this document.

Implimentation of this sample program is only for reference. If it conflicts
with a documentation of TIM2 format specifications, specifications takes precedence
over this implimentation.

This specification is 'AS-IS', no warranty in any case by web technology Corp.

             PlayStation, PlayStation2, EmotionEngine, GraphicsSynthesizer are
               all registered trademark of Sony Computer Entertainment Inc..


<Description of Sample Program>

tim2smpl program loads TIM2 file from HDD of host machine, and displays it on the screen.
This program is for PS2 environment.


tim2.h:       Header file defines the structure of file header, picture header etc.
tim2.c:       TIM2 data handler
main.c:       Body of the program
makefile:     makefile for ee-gcc

<About support of TIM2 format>

If there is some question about this specification, please contact to
TIM2 support section of web technology Corp.

TIM2 support section of web technology Corp.:
    http://www.webtech.co.jp/eng/istudio/ps2/tim2.html

This specification can be modified for enhancements. Please be aware of that
when using this information to create TIM2 loader, saver or such application program.

[End of TEXT]
