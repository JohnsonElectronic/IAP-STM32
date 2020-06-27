
#ifndef	__EMXGUI_PNG_H__
#define	__EMXGUI_PNG_H__

#ifdef	__cplusplus
extern	"C"{
#endif

#include "emXGUI.h"

/*===================================================================================*/

typedef struct __PNG_DEC PNG_DEC;
  
////PNG�����λͼ����(BITMAP).
PNG_DEC*    PNG_DecodeOpen(const char *png_dat);
BOOL		PNG_DecodeGetBitmap(PNG_DEC *dec,BITMAP *bm);
void		PNG_DecodeClose(PNG_DEC *dec);

////���PNGλͼ��Ϣ.
BOOL	PNG_GetInfo(BITMAPINFO *info,const char *png_dat);

////PNGֱ����ʾ,��PNGΪGray/Alpha��ʽʱ,ExColorָ����ɫֵ.
BOOL	PNG_DrawEx(HDC hdc,int x,int y,const char *png_dat,COLOR_RGB32 ExColor);


#define	PNG_Open		PNG_DecodeOpen
#define	PNG_GetBitmap	PNG_DecodeGetBitmap
#define	PNG_Close		PNG_DecodeClose

#define	PNG_Draw(hdc,x,y,png_dat)	PNG_DrawEx(hdc,x,y,png_dat,XRGB8888(0,0,0))
/*===================================================================================*/

#ifdef	__cplusplus
}
#endif
#endif
