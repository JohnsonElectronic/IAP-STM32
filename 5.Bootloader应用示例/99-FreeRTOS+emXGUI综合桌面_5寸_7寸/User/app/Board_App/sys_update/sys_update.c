/**
  ******************************************************************************
  * @file    iap_arch.c
  * @author  long
  * @version V1.0
  * @date    2019-11-23
  * @brief   xmodem ����ӿ��ļ�
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
 
#include "emXGUI.h"
#include "x_libc.h"
#include "string.h"
#include "ff.h"
#include "GUI_AppDef.h"
#include "emXGUI_JPEG.h"
#include "./sys_update/sys_update.h"   
#include <stdio.h>
#include "./usart/bsp_debug_usart.h"
#include "./lcd/bsp_lcd.h"

/* ͼƬ��Դ�� */
//#define GUI_SETTINGS_BACKGROUNG_PIC      "settingsdesktop.jpg"        // 800*480

typedef enum 
{ 
  /****************** ��ť�ؼ� ID ֵ *******************/
  ID_SYS_UPDATE_EXIT = 0x1000,      // �˳���ť
  ID_SYS_UPDATE_DET,                // ���ڿ�����
	ID_SYS_UPDATE_THEME,              // ����ѡ��
  ID_SETTINGS_UPDATE,             // ϵͳ����
  ID_DET_EXIT,                // ���ڿ�����
  /***************** �ı��ؼ� ID ֵ *********************/
  ID_SETTINGS_TITLE,              // ����
  ID_SETTINGS_THEMEINFO,          // ������ʾ
  /***************** �Ӵ��� ID ֵ *********************/
  ID_DET_WIN,
}set_id_t;

typedef struct{
	WCHAR *icon_name;    // ͼ����
	RECT rc;             // λ����Ϣ
	set_id_t id;         // ��ťID
}set_icon_t;

#define ICON_BTN_NUM     4     // ��ť����

//ͼ���������
const set_icon_t set_icon[] = {

  /* ��ť */
  {L"-",           {740,  22,  36,  36}, ID_SETTINGS_EXIT},      // 0. �˳���ť
  {L"���ڿ�����",  { 18,  93, 782,  36}, ID_SETTINGS_DET},       // 1. ���ڿ�����
  {L"1",           {725, 142,  65,  30}, ID_SETTINGS_THEME},     // 2. ����ѡ��
  {L"ϵͳ����",    {18, 178, 782,  36},   ID_SETTINGS_UPDATE},   // 3. 
  {L"����",        {100, 0,  600,  80},  ID_SETTINGS_TITLE},      // 4. 
  {L"����",        {18, 135, 100, 42},   ID_SETTINGS_THEMEINFO},   // 5. 
};

static uint32_t xmodem_actual_flash_address = FLASH_APP_ADDR;       /* д��ĵ�ַ. */

/**
 * @brief   Xmodem ����һ���ַ��Ľӿ�.
 * @param   ch �����͵�����
 * @return  ���ط���״̬
 */
int x_transmit_ch(uint8_t ch)
{
	Usart_SendByte(DEBUG_USART, ch);
	
	return 0;
}

/**
 * @brief   Xmodem ����Ҫ����������ݵ�����.
 * @param   address �����ݵ�ַ����������
 * @return  ���ص�ǰ����ʣ��Ĵ�С
 */
uint32_t x_receive_flash_erasure(uint32_t address)
{
  sector_t sector_info;
  if (erasure_sector(address, 1))    // ������ǰ��ַ��������
  {
    return 0;    // ����ʧ��
  }

  sector_info = GetSector(address);    // �õ���ǰ��������Ϣ

  return (sector_info.size + sector_info.start_addr - address);     // ���ص�ǰ����ʣ���С
}

/**
  * @brief   Xmodem �����ܵ������ݱ��浽flash.
  * @param  start_address ��Ҫд�����ʼ��ַ
  * @param  *data : ��Ҫ���������
	* @param  len ������
  * @return д��״̬
 */
int x_receive_flash_writea(uint32_t start_address, const void *data, uint32_t len)
{
  if (flash_write_data(start_address, (uint8_t *)data, len) == 0)    // ������ǰ��ַ��������
  {
    return 0;    // д��ɹ�
  }
  else
  {
    return -1;    // д��ʧ��
  }
}

/**
 * @brief   �ļ����ݽ�����ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_data_callback(void *ptr, char *file_data, uint32_t w_size)
{
  static uint32_t sector_size = 0;    /* ����ʣ���С. */
  static uint32_t recv_size = 0;    /* ����ʣ���С. */
  uint8_t buff[128];
  
  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= w_size)
  {
    sector_size += x_receive_flash_erasure(xmodem_actual_flash_address + sector_size);

    if (0 == sector_size)
    {
      return -1;
    }
  }
  
  if (flash_write_data(xmodem_actual_flash_address, (uint8_t *)file_data, w_size) == 0)    // д������
  {
    xmodem_actual_flash_address += w_size;
    recv_size += w_size;
    sprintf((char*)buff, "                 �ѽ��գ�%d�ֽڣ�", recv_size);
    LCD_DisplayStringLine_EN_CH(LINE(3), buff);
    return 0;
  }
  else 
  {
    return -1;
  }
}

/**
 * @brief   �ļ�������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_callback(void *ptr)
{
//  printf("��ʼ���� App��\r\n");
//	iap_jump_app(FLASH_APP_ADDR);
  LCD_DisplayStringLine_EN_CH(LINE(5), "                 Ӧ�ó��������ɣ�");
  
  return 0;
}

static LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{ 
   switch(msg){
      case WM_CREATE:
      {
//        u8 *jpeg_buf;
//        u32 jpeg_size;
//        JPG_DEC *dec;
//        BOOL res = NULL;

//        res = RES_Load_Content(GUI_SETTINGS_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
//        //res = FS_Load_Content(GUI_SETTINGS_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
//        hdc_am_bk = CreateMemoryDC(SURF_SCREEN, GUI_XSIZE, GUI_YSIZE);
//        if(res)
//        {
//          /* ����ͼƬ���ݴ���JPG_DEC��� */
//          dec = JPG_Open(jpeg_buf, jpeg_size);

//          /* �������ڴ���� */
//          JPG_Draw(hdc_am_bk, 0, 0, dec);

//          /* �ر�JPG_DEC��� */
//          JPG_Close(dec);
//        }
//        /* �ͷ�ͼƬ���ݿռ� */
//        RES_Release_Content((char **)&jpeg_buf);

        for (uint8_t xC=0; xC<ICON_BTN_NUM; xC++)     //  ��ť
        {
          /* ѭ��������ť */
          CreateWindow(BUTTON, set_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        set_icon[xC].rc.x, set_icon[xC].rc.y,
                        set_icon[xC].rc.w, set_icon[xC].rc.h,
                        hwnd, set_icon[xC].id, NULL, NULL); 
        }

        if (Theme_Flag == 0)
        {
          SetWindowText(GetDlgItem(hwnd, ID_SETTINGS_THEME), L"1");
        }
        else if  (Theme_Flag == 1)
        {
          SetWindowText(GetDlgItem(hwnd, ID_SETTINGS_THEME), L"2");
        }
        
        for (uint8_t xC=ICON_BTN_NUM; xC<ICON_BTN_NUM+2; xC++)     //  ��ť
        {
          /* ѭ�������ı��� */
          CreateWindow(TEXTBOX, set_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        set_icon[xC].rc.x, set_icon[xC].rc.y,
                        set_icon[xC].rc.w, set_icon[xC].rc.h,
                        hwnd, set_icon[xC].id, NULL, NULL); 
        }

        break;
      }

      case WM_TIMER:
      {


      }  
			break;     
      
      case WM_NOTIFY:
      {
         u16 code,  id;
         id  =LOWORD(wParam);//��ȡ��Ϣ��ID��
         code=HIWORD(wParam);//��ȡ��Ϣ������   

         //���͵���
        if(code == BN_CLICKED)
        {
          if (id == ID_SETTINGS_EXIT)
          {
            PostCloseMessage(hwnd);    // ���͹رմ��ڵ���Ϣ
          }
          else if (id == ID_SETTINGS_DET)
          {
            WNDCLASS wcex;
						RECT rc;

						wcex.Tag = WNDCLASS_TAG;
						wcex.Style = CS_HREDRAW | CS_VREDRAW;
						wcex.lpfnWndProc = (WNDPROC)DetWinProc;
						wcex.cbClsExtra = 0;
						wcex.cbWndExtra = 0;
						wcex.hInstance = NULL;
						wcex.hIcon = NULL;
						wcex.hCursor = NULL;

						rc.x = 0;
						rc.y = 0;
						rc.w = GUI_XSIZE;
						rc.h = GUI_YSIZE;

						CreateWindow(&wcex, L"---", WS_VISIBLE, 
                         rc.x, rc.y, rc.w, rc.h, hwnd, ID_DET_WIN, NULL, NULL);
          }
          else if (id == ID_SETTINGS_UPDATE)
          {
            WNDCLASS wcex;
						RECT rc;

						wcex.Tag = WNDCLASS_TAG;
						wcex.Style = CS_HREDRAW | CS_VREDRAW;
						wcex.lpfnWndProc = (WNDPROC)DetWinProc;
						wcex.cbClsExtra = 0;
						wcex.cbWndExtra = 0;
						wcex.hInstance = NULL;
						wcex.hIcon = NULL;
						wcex.hCursor = NULL;

						rc.x = 0;
						rc.y = 0;
						rc.w = GUI_XSIZE;
						rc.h = GUI_YSIZE;

						CreateWindow(&wcex, L"---", WS_VISIBLE, 
                         rc.x, rc.y, rc.w, rc.h, hwnd, ID_DET_WIN, NULL, NULL);
          }
          else if (id == ID_SETTINGS_THEME)
          {
            WCHAR wbuf[3];
            HWND  wnd = GetDlgItem(hwnd, ID_SETTINGS_THEME);

            GetWindowText(wnd, wbuf, 3);
            if (wbuf[0] == L'1')
            {
              Theme_Flag = 1;
              SendMessage(GetDlgItem(hwnd_home, 0x1000), MSG_SET_BGCOLOR, COLOR_DESKTOP_BACK_GROUND_HEX, NULL);    // 0x1000 ��home ���б�ID
              SetWindowText(wnd, L"2");
            }
            else
            {
              SendMessage(GetDlgItem(hwnd_home, 0x1000), MSG_SET_BGCOLOR, 1, NULL);           // ����Ϊ 1 ʱ ʹ��ͼƬ��Ϊ����  0x1000 ��home ���б�ID
              Theme_Flag = 0;
              SetWindowText(wnd, L"1");
            }
            
          }
        }

        break;
      }

      //�ػ��ƺ�����Ϣ
      case WM_DRAWITEM:
      {
         DRAWITEM_HDR *ds;
         ds = (DRAWITEM_HDR*)lParam;        
         if(ds->ID == ID_SETTINGS_EXIT)
         {
            exit_owner_draw(ds);
            return TRUE;
         }
         else if(ds->ID == ID_SETTINGS_DET || ds->ID == ID_SETTINGS_UPDATE)
         {
            det_button_OwnerDraw(ds);
            return TRUE;
         }
         else if(ds->ID == ID_SETTINGS_THEME)
         {
            theme_button_OwnerDraw(ds);
            return TRUE;
         }
         else if(ds->ID == ID_SETTINGS_TITLE || ds->ID == ID_SETTINGS_THEMEINFO)
         {
            text_OwnerDraw(ds);
            return TRUE;
         }
         

        return FALSE;
      }
      
      //���ƴ��ڽ�����Ϣ
      case WM_PAINT:
      {
        PAINTSTRUCT ps;
        RECT rc = {18, 135, 782, 42};
        HDC hdc;

        //��ʼ����
        hdc = BeginPaint(hwnd, &ps); 
        SetPenColor(hdc, MapRGB(hdc, 220, 220, 220));
        HLine(hdc, rc.x, rc.y+rc.h, rc.x+rc.w);
        
        EndPaint(hwnd, &ps);
        break;
      }
      
      case WM_ERASEBKGND:
      {
        HDC hdc =(HDC)wParam;
        RECT rc =*(RECT*)lParam;

        BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc.x, rc.y, SRCCOPY);

        return TRUE;
      }

      //�رմ�����Ϣ����case
      case WM_CLOSE:
      {   
        DestroyWindow(hwnd);
        return TRUE;	
      }
    
      //�رմ�����Ϣ����case
      case WM_DESTROY:
      {        

        return PostQuitMessage(hwnd);		
      }
      
      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
     
   return WM_NULL;
}

void gui_sys_update_dialog(void)
{ 	
	WNDCLASS	wcex;
  HWND hwnd;
	MSG msg;

	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = win_proc; //������������Ϣ����Ļص�����.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//����������
	hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,//
                                    &wcex,
                                    L"GUI sys update DIALOG",
                                    WS_VISIBLE,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
                                    NULL, NULL, NULL, NULL);

	//��ʾ������
	ShowWindow(hwnd, SW_SHOW);

	//��ʼ������Ϣѭ��(���ڹرղ�����ʱ,GetMessage������FALSE,�˳�����Ϣѭ��)��
	while (GetMessage(&msg, hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
