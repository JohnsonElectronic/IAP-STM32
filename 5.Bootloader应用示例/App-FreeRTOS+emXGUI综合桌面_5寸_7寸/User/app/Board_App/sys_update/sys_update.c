/**
  ******************************************************************************
  * @file    iap_arch.c
  * @author  long
  * @version V1.0
  * @date    2019-11-23
  * @brief   xmodem 对外接口文件
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 STM32 F429 开发板  
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
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

/* 图片资源名 */
//#define GUI_SETTINGS_BACKGROUNG_PIC      "settingsdesktop.jpg"        // 800*480

/* 宏定义 */
#define SYSSTEM_VERSION          L"当前系统版本是：V1.0.0"        // 系统版本号
#define SAVE_APP_OFFSET_ADDR     22020096                         // 保存 app 的偏移地址从第 21MB 开始的

static uint8_t update_flag = 1;

typedef struct
{
	uint8_t  update_flag;    // 更新标志
	uint32_t app_size;       // app 大小
} app_info_t;

uint32_t xmodem_actual_flash_address = SAVE_APP_OFFSET_ADDR + sizeof(app_info_t);

typedef enum 
{ 
  /****************** 按钮控件 ID 值 *******************/
  ID_SYS_UPDATE_EXIT = 0x1000,      // 退出按钮
  ID_SYS_UPDATE_UPDATE,             // 系统升级
  /***************** 文本控件 ID 值 *********************/
  ID_SYS_UPDATE_NUM,              // 下载文件数量
  ID_SYS_UPDATE_RES,          // 下载结果提示
  ID_SYS_UPDATE_TITLE,
	ID_SYS_UPDATE_NAME,
	ID_SYS_UPDATE_PROGRE,
	
}sys_update_id_t;

typedef struct{
	WCHAR *icon_name;    // 图标名
	RECT rc;             // 位置信息
	sys_update_id_t id;         // 按钮ID
}sys_update_t;

#define SYS_UPDATE_ICON_BTN_NUM     2     // 按钮数量

//图标管理数组
const sys_update_t sys_updat_icon[] = {

  /* 按钮 */
  {L"F",            { 18,  19,  60,  45},  ID_SYS_UPDATE_EXIT},      // 0. 退出按钮
  {L"下载固件",     {318, 390, 166,  70},  ID_SYS_UPDATE_UPDATE},    // 1. 升级按钮
  {L"已下载：0字节",{ 36, 269,  500, 28},  ID_SYS_UPDATE_NUM},       // 2. 下载文件数量
  {L"",             { 36, 302, 500,  28},  ID_SYS_UPDATE_RES},       // 3. 下载结果提示
	{L"正在下载：",   { 36, 236,  600, 28},  ID_SYS_UPDATE_NAME},      // 4. 文件名
  {L"系统升级",     {100,   0,  600, 80},  ID_SYS_UPDATE_TITLE},     // 5. 主题
	{L"下载进度",     {100, 351,  600, 16},  ID_SYS_UPDATE_PROGRE },   // 6. 下载进度条
};

TaskHandle_t h_download;                    // 下载线程
HWND update_hwnd;
HWND download_progbar = NULL;

/*
 * 系统软件复位
 */
void Soft_Reset(void)
{
  __set_FAULTMASK(1);   /* 关闭所有中断 */  
  NVIC_SystemReset();   /* 系统复位 */
}

/**
 * @brief   Ymodem 发送一个字符的接口.
 * @param   ch ：发送的数据
 * @return  返回发送状态
 */
int y_transmit_ch(uint8_t ch)
{
	Usart_SendByte(DEBUG_USART, ch);
	
	return 0;
}

/**
 * @brief   Ymodem 擦除要保存接收数据的扇区.
 * @param   address ：根据地址来擦除扇区
 * @return  返回当前扇区剩余的大小
 */
uint32_t y_receive_flash_erasure(uint32_t address)
{
  SPI_FLASH_SectorErase(address);    // 擦除当前地址所在扇区

  return 4096;     // 返回擦除扇区的大小
}

uint8_t buff_c[1100];    // 数据缓冲区

/**
  * @brief  Ymodem 将接受到的数据保存到flash.
  * @param  start_address ：要写入的起始地址
  * @param  *data : 需要保存的数据
	* @param  len ：长度
  * @return 写入状态
 */
int y_receive_flash_write(uint32_t start_address, const void *data, uint32_t len)
{
  SPI_FLASH_BufferWrite((uint8_t *)data, start_address, len);
	
	/* 读写入的数据 */
	SPI_FLASH_BufferRead((uint8_t *)buff_c, start_address, len);
	
	/* 校验 */
	for (uint32_t i=0; i<len; i++)
	{
		if (*((uint8_t *)data + i) != buff_c[i])
		{
			return -1;
		}
	}
	
  return 0;    // 写入成功
}

/**
 * @brief   文件名和大小接收完成回调.
 * @param   *ptr: 控制句柄.
 * @param   *file_name: 文件名字.
 * @param   file_size: 文件大小，若为0xFFFFFFFF，则说明大小无效.
 * @return  返回写入的结果，0：成功，-1：失败.
 */
int receive_nanme_size_callback(void *ptr, char *file_name, y_uint32_t file_size)
{
	WCHAR wbuf[64];
  WCHAR wbuf1[64];

  x_mbstowcs_cp936(wbuf, file_name, 128);
  x_wsprintf(wbuf1, L"正在下载：%s", wbuf);
  SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_NAME), wbuf1);
	
	SendMessage(download_progbar, PBM_SET_RANGLE, TRUE, file_size);
  
  /* 用户应该在外部实现这个函数 */
  return 0;
}

static uint32_t recv_flash = 0;    /* 接收标准. */

/**
 * @brief   文件数据接收完成回调.
 * @param   *ptr: 控制句柄.
 * @param   *file_name: 文件名字.
 * @param   file_size: 文件大小，若为0xFFFFFFFF，则说明大小无效.
 * @return  返回写入的结果，0：成功，-1：失败.
 */
int receive_file_data_callback(void *ptr, char *file_data, uint32_t w_size)
{
  static uint32_t sector_size = 0;    /* 扇区剩余大小. */
  static uint32_t recv_size = 0;      /* 已接收大小大小. */
  WCHAR wbuf[128];
   
  if (recv_flash == 0)
  {
    /* 第一次写数据 */
    sector_size = 0;
    recv_size = 0;
    xmodem_actual_flash_address = SAVE_APP_OFFSET_ADDR + sizeof(app_info_t);
    sector_size += y_receive_flash_erasure(SAVE_APP_OFFSET_ADDR);    // 擦除第一个扇区（必须是4096的整数倍）
    sector_size -= sizeof(app_info_t);
    recv_flash = 1;
  }
  
  /* 当前扇区不够了擦除下一个. */
  if (sector_size <= w_size)
  {
    sector_size += y_receive_flash_erasure(xmodem_actual_flash_address + sector_size);
    if (sector_size <= w_size)
    {
      return -1;
    }
  }
  
  if (y_receive_flash_write(xmodem_actual_flash_address, (uint8_t *)file_data, w_size) == 0)    // 写入数据（一次性写入的数据不能超过一个扇区）
  {
    xmodem_actual_flash_address += w_size;
    recv_size += w_size;
    sector_size -= w_size;
		
    x_wsprintf(wbuf, L"已下载：%d字节！", recv_size);
    SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_NUM), wbuf);
		SendMessage(download_progbar, PBM_SET_VALUE, TRUE, recv_size);
    return 0;
  }
  else 
  {
    return -1;
  }
}

/**
 * @brief   文件接收完成回调.
 * @param   *ptr: 控制句柄.
 * @return  返回写入的结果，0：成功，-1：失败.
 */
int receive_file_callback(void *ptr)
{
  
  return 0;
}

/**
  * @brief  app 文件下载线程
  * @param  hwnd：屏幕窗口的句柄
  * @retval 无
  * @notes  
  */
uint8_t download_thread = 0;
static void app_bin_download(HWND hwnd)
{
  download_thread = 1;
  int res = -1;
  recv_flash = 0;

	while(download_thread) //线程已创建了
	{
    res = ymodem_receive();
    if ((res >> 15) & 1)
    {
			SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_UPDATE), L"重新下载");
      SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_RES), L"下载失败，请重试！");
    }
    else
    {
      app_info_t app;
      
      app.app_size = xmodem_actual_flash_address - SAVE_APP_OFFSET_ADDR - sizeof(app_info_t);
      app.update_flag = 0;
      
      y_receive_flash_write(SAVE_APP_OFFSET_ADDR, (uint8_t *)&app, sizeof(app));
      
      update_flag = app.update_flag;
      SetWindowText(GetDlgItem(update_hwnd, ID_SYS_UPDATE_UPDATE), L"重启升级");
      SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_RES), L"下载成功，请重启开发板升级！");
    }
    
    download_thread = 0;
  }
  
  download_thread = 0;
  recv_flash = 0;
  
  GUI_Thread_Delete(GUI_GetCurThreadHandle()); 
}

/**
  * @brief  进度条重绘
  */
static void progbar_owner_draw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc, hdc_mem;
	RECT rc, m_rc[2], rc_tmp;
//	int range,val;
	WCHAR wbuf[128];
	PROGRESSBAR_CFG cfg;
	hwnd =ds->hwnd;
	hdc =ds->hDC;

	
   /* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

	hdc_mem = CreateMemoryDC(SURF_SCREEN, rc.w, rc.h);
	
  BitBlt(hdc_mem, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

   //设置进度条的背景颜色
	SetBrushColor(hdc,MapRGB(hdc,250,250,250));
   //填充进度条的背景
  EnableAntiAlias(hdc, TRUE);
	FillRoundRect(hdc,&ds->rc, MIN(rc.w,rc.h)/2);   
//   //设置画笔颜色
	SetPenColor(hdc,MapRGB(hdc,100,10,10));
//   //绘制进度条的背景边框
//   DrawRect(hdc,&rc);
   /*************第二步***************/	
   cfg.cbSize =sizeof(cfg);
	cfg.fMask =PB_CFG_ALL;
	SendMessage(hwnd,PBM_GET_CFG,0,(LPARAM)&cfg);
   //生成进度条矩形
	MakeProgressRect(m_rc,&rc,cfg.Rangle,cfg.Value,PB_ORG_LEFT);
   //设置进度条的颜色
	SetBrushColor(hdc_mem,MapRGB(hdc,210,10,10));
  EnableAntiAlias(hdc, FALSE);
   //填充进度条
  // InflateRect(&m_rc[0],-1,-1);
  EnableAntiAlias(hdc_mem, TRUE);
	FillRoundRect(hdc_mem, &rc, rc.h/2);
  EnableAntiAlias(hdc_mem, FALSE);
  BitBlt(hdc, m_rc[0].x, m_rc[0].y, m_rc[0].w, m_rc[0].h, hdc_mem, 0, 0, SRCCOPY);
    
   //绘制进度条的边框，采用圆角边框
	//DrawRoundRect(hdc,&m_rc[0],MIN(rc.w,rc.h)/2);
   /************显示进度值****************/
	DeleteDC(hdc_mem);
}

static void det_exit_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
  HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

	/* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  GetClientRect(hwnd, &rc);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

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
	GetWindowText(hwnd, wbuf, 128); //获得按钮控件的文字
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_LEFT);//绘制文字(居中对齐方式)
}

static void btn_owner_draw(DRAWITEM_HDR *ds) //绘制一个按钮外观
{
	HDC hdc;
	RECT rc, rc_tmp;
  WCHAR wbuf[128];
  HWND hwnd;
  
  hwnd = ds->hwnd;
	hdc = ds->hDC;   //button的绘图上下文句柄.
	rc = ds->rc;     //button的绘制矩形区.

  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

  if (ds->State & BST_PUSHED)
  { //按钮是按下状态
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_png[hdc_clock_btn_press], 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 200, 200, 200));
  }
  else
  { //按钮是弹起状态
    BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_png[hdc_clock_btn], 0, 0, SRCCOPY);
    SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
  }
  
  GetWindowText(ds->hwnd, wbuf, 128); //获得按钮控件的文字
  
  /* 显示文本 */
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//绘制文字(居中对齐方式)
}

/*
 * @brief  重绘标题透明文本
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Title_Textbox_OwnerDraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

  /* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

	SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	GetWindowText(hwnd, wbuf, 128);                        // 获得按钮控件的文字
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // 绘制文字(居中对齐方式)
}

/*
 * @brief  重绘透明文本
 * @param  ds:	自定义绘制结构体
 * @retval NONE
*/
static void Textbox_OwnerDraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
  RECT rc,rc_tmp;
	WCHAR wbuf[128];

  hwnd = ds->hwnd; //button的窗口句柄.
	hdc = ds->hDC;   //button的绘图上下文句柄.
  rc = ds->rc;

  /* 背景 */
  GetClientRect(hwnd, &rc_tmp);//得到控件的位置
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//坐标转换
  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, hdc_clock_bk, rc_tmp.x, rc_tmp.y, SRCCOPY);

	SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	GetWindowText(hwnd, wbuf, 128);                        // 获得按钮控件的文字
	DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_LEFT);    // 绘制文字(居中对齐方式)
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
//          /* 根据图片数据创建JPG_DEC句柄 */
//          dec = JPG_Open(jpeg_buf, jpeg_size);

//          /* 绘制至内存对象 */
//          JPG_Draw(hdc_am_bk, 0, 0, dec);

//          /* 关闭JPG_DEC句柄 */
//          JPG_Close(dec);
//        }
//        /* 释放图片内容空间 */
//        RES_Release_Content((char **)&jpeg_buf);

        for (uint8_t xC=0; xC<SYS_UPDATE_ICON_BTN_NUM; xC++)     //  按钮
        {
          /* 循环创建按钮 */
          CreateWindow(BUTTON, sys_updat_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        sys_updat_icon[xC].rc.x, sys_updat_icon[xC].rc.y,
                        sys_updat_icon[xC].rc.w, sys_updat_icon[xC].rc.h,
                        hwnd, sys_updat_icon[xC].id, NULL, NULL); 
        }
        
        if (update_flag)    // 已经成功下载一个 app
        {
          SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_UPDATE), L"重启升级");
        }
        
        for (uint8_t xC=SYS_UPDATE_ICON_BTN_NUM; xC<SYS_UPDATE_ICON_BTN_NUM+4; xC++)     //  文本
        {
          /* 循环创建文本框 */
          CreateWindow(TEXTBOX, sys_updat_icon[xC].icon_name,  WS_OWNERDRAW | WS_VISIBLE,
                        sys_updat_icon[xC].rc.x, sys_updat_icon[xC].rc.y,
                        sys_updat_icon[xC].rc.w, sys_updat_icon[xC].rc.h,
                        hwnd, sys_updat_icon[xC].id, NULL, NULL); 
        }
				
				PROGRESSBAR_CFG cfg;
				
				//PROGRESSBAR_CFG结构体的大小
				cfg.cbSize	 = sizeof(PROGRESSBAR_CFG);
				//开启所有的功能
				cfg.fMask    = PB_CFG_ALL;
				//文字格式水平，垂直居中
				cfg.TextFlag = DT_VCENTER|DT_CENTER;  

				download_progbar = CreateWindow(PROGRESSBAR, L"Loading",
																		PBS_TEXT|PBS_ALIGN_LEFT|WS_VISIBLE|WS_OWNERDRAW|WS_TRANSPARENT,
																		sys_updat_icon[6].rc.x, sys_updat_icon[6].rc.y,
                                    sys_updat_icon[6].rc.w, sys_updat_icon[6].rc.h,
                            				hwnd, ID_SYS_UPDATE_PROGRE, NULL, NULL);

				SendMessage(download_progbar, PBM_GET_CFG,TRUE, (LPARAM)&cfg);
				SendMessage(download_progbar, PBM_SET_CFG,TRUE, (LPARAM)&cfg);
				SendMessage(download_progbar, PBM_SET_RANGLE, TRUE, 1);
				SendMessage(download_progbar, PBM_SET_VALUE, TRUE, 0);

        break;
      }

      case WM_TIMER:
      {


      }  
			break;     
      
      case WM_NOTIFY:
      {
         u16 code,  id;
         id  =LOWORD(wParam);//获取消息的ID码
         code=HIWORD(wParam);//获取消息的类型   

         //发送单击
        if(code == BN_CLICKED)
        {
          if (id == ID_SYS_UPDATE_EXIT)
          {
            PostCloseMessage(hwnd);    // 发送关闭窗口的消息
          }
          else if (id == ID_SYS_UPDATE_UPDATE)
          {
            if (update_flag == 0)
            {
              Soft_Reset();    // 复位系统
            }
            else if (download_thread == 0)
            {
              SetWindowText(GetDlgItem(hwnd, ID_SYS_UPDATE_UPDATE), L"正在下载");
              xTaskCreate((TaskFunction_t )(void(*)(void*))app_bin_download,   /* 任务入口函数 */
                          (const char*    )"firmware download",                /* 任务名字 */
                          (uint16_t       )5*1024/4,                           /* 任务栈大小FreeRTOS的任务栈以字为单位 */
                          (void*          )hwnd,                               /* 任务入口函数参数 */
                          (UBaseType_t    )6,                                  /* 任务的优先级 */
                          (TaskHandle_t  )&h_download);                           /* 任务控制块指针 */
            }
          }
        }

        break;
      }

      //重绘制函数消息
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
         else if(ds->ID == ID_SYS_UPDATE_NUM || ds->ID == ID_SYS_UPDATE_RES || ds->ID == ID_SYS_UPDATE_NAME)
         {
            Textbox_OwnerDraw(ds);
            return TRUE;
         }
         else if (ds->ID == ID_SYS_UPDATE_TITLE)
         {
           Title_Textbox_OwnerDraw(ds);
           return TRUE;
         }
				 else if (ds->ID == ID_SYS_UPDATE_PROGRE)
				 {
					 progbar_owner_draw(ds);
					 return TRUE;
				 }
         
         return FALSE;
      }
      
      //绘制窗口界面消息
      case WM_PAINT:
      {
        PAINTSTRUCT ps;
        RECT rc = {18, 135, 782, 42};
        HDC hdc;

        //开始绘制
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
        rc.y = 155;
        rc.w = 600;
        rc.h = 28;
        SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
        DrawText(hdc, L"1.请使用串口连接到主机（文件发送端）；", -1, &rc, DT_VCENTER|DT_LEFT);//绘制文字(居中对齐方式)
        rc.y += rc.h + 5;
        DrawText(hdc, L"2.按下固件升级后，开始下载新固件；", -1, &rc, DT_VCENTER|DT_LEFT);//绘制文字(居中对齐方式)
        
        rc.x = 36;
        rc.y = 107;
        rc.w = 600;
        rc.h = 30;
        DrawText(hdc, SYSSTEM_VERSION, -1, &rc, DT_VCENTER|DT_LEFT);//绘制文字(居中对齐方式)

        return TRUE;
      }

      //关闭窗口消息处理case
      case WM_CLOSE:
      {   
        DestroyWindow(hwnd);
        return TRUE;	
      }
    
      //关闭窗口消息处理case
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
	wcex.lpfnWndProc = win_proc; //设置主窗口消息处理的回调函数.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//创建主窗口
	update_hwnd = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,//
                                    &wcex,
                                    L"GUI sys update DIALOG",
                                    WS_VISIBLE,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
                                    NULL, NULL, NULL, NULL);

	//显示主窗口
	ShowWindow(update_hwnd, SW_SHOW);

	//开始窗口消息循环(窗口关闭并销毁时,GetMessage将返回FALSE,退出本消息循环)。
	while (GetMessage(&msg, update_hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
