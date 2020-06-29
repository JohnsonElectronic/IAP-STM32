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
#include "./pic_load/gui_pic_load.h"
#include "./ymodem/ymodem.h"

/* ͼƬ��Դ�� */
//#define GUI_SETTINGS_BACKGROUNG_PIC      "settingsdesktop.jpg"        // 800*480

/* �궨�� */
#define SYSSTEM_VERSION          L"��ǰϵͳ�汾�ǣ�V1.0.0"        // ϵͳ�汾��
#define SAVE_APP_OFFSET_ADDR     22020096                         // ���� app ��ƫ�Ƶ�ַ�ӵ� 21MB ��ʼ��

static uint8_t update_flag = 1;

typedef struct
{
	uint8_t  update_flag;    // ���±�־
	uint32_t app_size;       // app ��С
} app_info_t;

uint32_t xmodem_actual_flash_address = SAVE_APP_OFFSET_ADDR + sizeof(app_info_t);

typedef enum 
{ 
  /****************** ��ť�ؼ� ID ֵ *******************/
  ID_SYS_UPDATE_EXIT = 0x1000,      // �˳���ť
  ID_SYS_UPDATE_UPDATE,             // ϵͳ����
  /***************** �ı��ؼ� ID ֵ *********************/
  ID_SYS_UPDATE_NUM,              // �����ļ�����
  ID_SYS_UPDATE_RES,          // ���ؽ����ʾ
  ID_SYS_UPDATE_TITLE,
}sys_update_id_t;

typedef struct{
	WCHAR *icon_name;    // ͼ����
	RECT rc;             // λ����Ϣ
	sys_update_id_t id;         // ��ťID
}sys_update_t;

#define SYS_UPDATE_ICON_BTN_NUM     2     // ��ť����

//ͼ���������
const sys_update_t sys_updat_icon[] = {

  /* ��ť */
  {L"F",            { 18,  19,  60,  45},  ID_SYS_UPDATE_EXIT},      // 0. �˳���ť
  {L"���ع̼�",     {318, 390, 166,  70},  ID_SYS_UPDATE_UPDATE},    // 1. ������ť
  {L"�����أ�0�ֽ�",{ 36, 276,  500, 30},  ID_SYS_UPDATE_NUM},       // 2. �����ļ�����
  {L"",             { 36, 311, 500,  30},  ID_SYS_UPDATE_RES},       // 3. ���ؽ����ʾ
  {L"ϵͳ����",     {100,   0,  600, 80},  ID_SYS_UPDATE_TITLE},     // 5. ����
};

TaskHandle_t h_download;                    // �����߳�
HWND update_hwnd;

/*
 * ϵͳ�����λ
 */
void Soft_Reset(void)
{
  __set_FAULTMASK(1);   /* �ر������ж� */  
  NVIC_SystemReset();   /* ϵͳ��λ */
}

/**
 * @brief   Ymodem ����һ���ַ��Ľӿ�.
 * @param   ch �����͵�����
 * @return  ���ط���״̬
 */
int y_transmit_ch(uint8_t ch)
{
	Usart_SendByte(DEBUG_USART, ch);
	
	return 0;
}

/**
 * @brief   Ymodem ����Ҫ����������ݵ�����.
 * @param   address �����ݵ�ַ����������
 * @return  ���ص�ǰ����ʣ��Ĵ�С
 */
uint32_t x_receive_flash_erasure(uint32_t address)
{
  SPI_FLASH_SectorErase(address);    // ������ǰ��ַ��������

  return 4096;     // ���ز��������Ĵ�С
}

/**
  * @brief  Ymodem �����ܵ������ݱ��浽flash.
  * @param  start_address ��Ҫд�����ʼ��ַ
  * @param  *data : ��Ҫ���������
	* @param  len ������
  * @return д��״̬
 */
int x_receive_flash_write(uint32_t start_address, const void *data, uint32_t len)
{
  SPI_FLASH_BufferWrite((uint8_t *)data, start_address, len);

  return 0;    // д��ɹ�
}

/**
 * @brief   �ļ����ʹ�С������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_nanme_size_callback(void *ptr, char *file_name, y_uint32_t file_size)
{
  Y_UNUSED(ptr);
  Y_UNUSED(file_name);
  Y_UNUSED(file_size);
  
  /* �û�Ӧ�����ⲿʵ��������� */
  return 0;
}

static uint32_t recv_flash = 0;    /* ����ʣ���С. */

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
  WCHAR wbuf[128];
  
  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= w_size)
  {
    
    if (recv_flash == 0)
    {
      sector_size = 0;
      recv_size = 0;
      xmodem_actual_flash_address = SAVE_APP_OFFSET_ADDR + sizeof(app_info_t);
      sector_size += x_receive_flash_erasure(SAVE_APP_OFFSET_ADDR);
      recv_flash = 1;
    }
    else
    {
      sector_size += x_receive_flash_erasure(xmodem_actual_flash_address + sector_size);
    }

    if (sector_size <= w_size)
    {
      return -1;
    }
  }
  
  if (x_receive_flash_write(xmodem_actual_flash_address, (uint8_t *)file_data, w_size) == 0)    // д������
  {
    xmodem_actual_flash_address += w_size;
    recv_size += w_size;
    x_wsprintf(wbuf, L"�����أ�%d�ֽڣ�", recv_size);
    SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_NUM), wbuf);
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
  app_info_t app;
  recv_flash = 0;
  
  app.app_size = xmodem_actual_flash_address - SAVE_APP_OFFSET_ADDR - sizeof(app_info_t);
  app.update_flag = 0;
  
  x_receive_flash_write(SAVE_APP_OFFSET_ADDR, (uint8_t *)&app, sizeof(app));
  
  update_flag = app.update_flag;
  SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_UPDATE), L"��������");
  return 0;
}

/**
  * @brief  ���������б����
  * @param  hwnd����Ļ���ڵľ��
  * @retval ��
  * @notes  
  */
uint8_t download_thread = 0;
static void app_bin_download(HWND hwnd)
{
  download_thread = 1;
  int res = -1;
  recv_flash = 0;

	while(download_thread) //�߳��Ѵ�����
	{
    res = ymodem_receive();
    if ((res >> 16) & 1)
    {
      SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_RES), L"����ʧ�ܣ������ԣ�");
    }
    else
    {
      SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_RES), L"���سɹ�������������������");
    }
    
    download_thread = 0;
  }
  
  download_thread = 0;
  recv_flash = 0;
  
  GUI_Thread_Delete(GUI_GetCurThreadHandle()); 
}

static void det_exit_owner_draw(DRAWITEM_HDR *ds) //����һ����ť���
{
  HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button�Ĵ��ھ��.
	hdc = ds->hDC;   //button�Ļ�ͼ�����ľ��.
  rc = ds->rc;

	/* ���� */
  GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
  GetClientRect(hwnd, &rc);//�õ��ؼ���λ��
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
  {
    SetTextColor(hdc, MapRGB(hdc, 200, 200, 200));
  }
  else
  {
    SetTextColor(hdc, MapRGB(hdc, 250, 250, 250));
  }
	
  SetFont(hdc, controlFont_48);
	GetWindowText(hwnd, wbuf, 128); //��ð�ť�ؼ�������
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_LEFT);//��������(���ж��뷽ʽ)
}

static void btn_owner_draw(DRAWITEM_HDR *ds) //����һ����ť���
{
	HDC hdc;
	RECT rc, rc_tmp;
  WCHAR wbuf[128];
  HWND hwnd;
  
  hwnd = ds->hwnd;
	hdc = ds->hDC;   //button�Ļ�ͼ�����ľ��.
	rc = ds->rc;     //button�Ļ��ƾ�����.

  GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
  { //��ť�ǰ���״̬
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_png[hdc_clock_btn_press], 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 200, 200, 200));
  }
  else
  { //��ť�ǵ���״̬
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_png[hdc_clock_btn], 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
  }
  
  GetWindowText(ds->hwnd, wbuf, 128); //��ð�ť�ؼ�������
  
  /* ��ʾ�ı� */
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//��������(���ж��뷽ʽ)
}

/*
 * @brief  �ػ����͸���ı�
 * @param  ds:	�Զ�����ƽṹ��
 * @retval NONE
*/
static void Title_Textbox_OwnerDraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button�Ĵ��ھ��.
	hdc = ds->hDC;   //button�Ļ�ͼ�����ľ��.
  rc = ds->rc;

  /* ���� */
  GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

	SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	GetWindowText(hwnd, wbuf, 128);                        // ��ð�ť�ؼ�������
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // ��������(���ж��뷽ʽ)
}

/*
 * @brief  �ػ�͸���ı�
 * @param  ds:	�Զ�����ƽṹ��
 * @retval NONE
*/
static void Textbox_OwnerDraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button�Ĵ��ھ��.
	hdc = ds->hDC;   //button�Ļ�ͼ�����ľ��.
  rc = ds->rc;

  /* ���� */
  GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

	SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	GetWindowText(hwnd, wbuf, 128);                        // ��ð�ť�ؼ�������
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_LEFT);    // ��������(���ж��뷽ʽ)
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

        for (uint8_t xC=0; xC<SYS_UPDATE_ICON_BTN_NUM; xC++)     //  ��ť
        {
          /* ѭ��������ť */
          CreateWindow(BUTTON, sys_updat_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        sys_updat_icon[xC].rc.x, sys_updat_icon[xC].rc.y,
                        sys_updat_icon[xC].rc.w, sys_updat_icon[xC].rc.h,
                        hwnd, sys_updat_icon[xC].id, NULL, NULL); 
        }
        
        for (uint8_t xC=SYS_UPDATE_ICON_BTN_NUM; xC<SYS_UPDATE_ICON_BTN_NUM+3; xC++)     //  �ı�
        {
          /* ѭ�������ı��� */
          CreateWindow(TEXTBOX, sys_updat_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        sys_updat_icon[xC].rc.x, sys_updat_icon[xC].rc.y,
                        sys_updat_icon[xC].rc.w, sys_updat_icon[xC].rc.h,
                        hwnd, sys_updat_icon[xC].id, NULL, NULL); 
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
          if (id == ID_SYS_UPDATE_EXIT)
          {
            PostCloseMessage(hwnd);    // ���͹رմ��ڵ���Ϣ
          }
          else if (id == ID_SYS_UPDATE_UPDATE)
          {
            if (update_flag == 0)
            {
              Soft_Reset();    // ��λϵͳ
            }
            else if (download_thread)
            {
              SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_UPDATE), L"���ع̼�");
              download_thread = 0;
            }
            else
            {
              SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_UPDATE), L"��������");
              xTaskCreate((TaskFunction_t )(void(*)(void*))app_bin_download,   /* ������ں��� */
                          (const char*    )"firmware download",                /* �������� */
                          (uint16_t       )5*1024/4,                           /* ����ջ��СFreeRTOS������ջ����Ϊ��λ */
                          (void*          )hwnd,                               /* ������ں������� */
                          (UBaseType_t    )6,                                  /* ��������ȼ� */
                          (TaskHandle_t  )&h_download);                           /* ������ƿ�ָ�� */
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
         if(ds->ID == ID_SYS_UPDATE_EXIT)
         {
            det_exit_owner_draw(ds);
            return TRUE;
         }
         else if(ds->ID == ID_SYS_UPDATE_UPDATE)
         {
            btn_owner_draw(ds);
            return TRUE;
         }
         else if(ds->ID == ID_SYS_UPDATE_NUM || ds->ID == ID_SYS_UPDATE_RES)
         {
            Textbox_OwnerDraw(ds);
            return TRUE;
         }
         else if (ds->ID == ID_SYS_UPDATE_TITLE)
         {
           Title_Textbox_OwnerDraw(ds);
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

        EndPaint(hwnd, &ps);
        break;
      }
      
      case WM_ERASEBKGND:
      {
        HDC hdc =(HDC)wParam;
        RECT rc =*(RECT*)lParam;

        BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc.x, rc.y, SRCCOPY);
        
        rc.x = 36;
        rc.y = 175;
        rc.w = 600;
        rc.h = 30;
        SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
        DrawText(hdc, L"1.��ʹ�ô������ӵ��������ļ����Ͷˣ���", -1, &rc, DT_VCENTER|DT_LEFT);//��������(���ж��뷽ʽ)
        rc.y += rc.h;
        DrawText(hdc, L"2.���¹̼������󣬿�ʼ�����¹̼���", -1, &rc, DT_VCENTER|DT_LEFT);//��������(���ж��뷽ʽ)
        
        rc.x = 36;
        rc.y = 107;
        rc.w = 600;
        rc.h = 30;
        DrawText(hdc, SYSSTEM_VERSION, -1, &rc, DT_VCENTER|DT_LEFT);//��������(���ж��뷽ʽ)

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
	update_hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,//
                                    &wcex,
                                    L"GUI sys update DIALOG",
                                    WS_VISIBLE,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
                                    NULL, NULL, NULL, NULL);

	//��ʾ������
	ShowWindow(update_hwnd, SW_SHOW);

	//��ʼ������Ϣѭ��(���ڹرղ�����ʱ,GetMessage������FALSE,�˳�����Ϣѭ��)��
	while (GetMessage(&msg, update_hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
