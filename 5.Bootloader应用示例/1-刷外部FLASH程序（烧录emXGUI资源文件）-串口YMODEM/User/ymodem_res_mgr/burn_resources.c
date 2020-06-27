/**
  ******************************************************************************
  * @file    ymodem_app.c
  * @author  long
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   ymodemӦ��ʾ��
  ******************************************************************************
***/

#include <string.h>
#include "./ymodem/ymodem.h"
#include "./ymodem_res_mgr/burn_resources.h"
#include "./usart/bsp_debug_usart.h"

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

#if 0

#include "./FATFS/ff.h"

/* ȫ�ֱ��� */
static FIL fnew;												/* �ļ����� */
static FRESULT res_sd;                  /* �ļ�������� */
static UINT fnum;            					  /* �ļ��ɹ���д���� */

/**
 * @brief   �ļ����ʹ�С������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_nanme_size_callback(void *ptr, char *file_name, uint32_t file_size)
{
  char buff[50];
  FIL **f_ptr = ptr;    /* �ļ����� */
  
  ptr = &fnew;
  
  sprintf(buff, "0:/%s", file_name);
  res_sd = f_open(*f_ptr, buff, FA_OPEN_APPEND | FA_WRITE );
  if ( res_sd != FR_OK )
  {	
    /* Flash ����. */
    return -1;
  }
  
  return 0;
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
  FIL **f_ptr = ptr;    /* �ļ����� */
  
  res_sd=f_write(*f_ptr, (uint8_t *)file_data, (uint32_t)w_size, &fnum);
  if(res_sd != FR_OK)
  {
    /* ���ٶ�д���ر��ļ� */
    f_close(*f_ptr);
    /* Flash ����. */
    return -1;
  }
  
  return 0;
}

/**
 * @brief   һ���ļ�������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_callback(void *ptr)
{
  FIL **f_ptr = ptr;    /* �ļ����� */
  f_close(*f_ptr);
  
  return 0;
}

#else

#include "./flash/bsp_spi_flash.h"

/**
  * @brief  ��FLASH�е�Ŀ¼������Ӧ����Դλ��
  * @param  res_base Ŀ¼��FLASH�еĻ���ַ
  * @param  res_name[in] Ҫ���ҵ���Դ����
  * @retval -1��ʾ�Ҳ���������ֵ��ʾ��Դ��FLASH�еĻ���ַ
  */
int GetResOffset(const char *res_name)
{
  
	int i, len;
	CatalogTypeDef dir;

	len =strlen(res_name);
  for(i=0; i<CATALOG_SIZE; i+=sizeof(CatalogTypeDef))
	{
		SPI_FLASH_BufferRead((u8*)&dir, RESOURCE_BASE_ADDR + i, sizeof(CatalogTypeDef));
    
		if(strncasecmp(dir.name, res_name, len)==0)
		{
			return dir.offset + RESOURCE_BASE_ADDR;
		}
	}

	return -1;
}

static CatalogTypeDef w_data;

/**
 * @brief   ��¼��ԴĿ¼.
 * @param   *data: Ŀ¼����.
 * @param   *size: Ŀ¼����.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int burn_catalog_flash(uint8_t *data, uint32_t size)
{
  static uint32_t ctatlog = RESOURCE_BASE_ADDR;
  static CatalogTypeDef dir = 
  {
    .name[0] = 0,
    .offset = CATALOG_SIZE,
    .size = 0,
  };
  
  strcpy(dir.name, (char *)data);
  dir.size = size;
  
  w_data = dir;
  
  SPI_FLASH_BufferWrite((u8*)&dir, ctatlog, sizeof(CatalogTypeDef));
  
  dir.offset += size;
  ctatlog += sizeof(CatalogTypeDef);
  
  return 0;
}

/**
 * @brief   ��¼��Դ����.
 * @param   *data: ��Դ����.
 * @param   *size: ��Դ����.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int burn_res_flash(uint8_t *data, uint32_t size)
{
  SPI_FLASH_BufferWrite((u8*)data, w_data.offset + RESOURCE_BASE_ADDR, size);
  w_data.offset += size;
  
  return 0;
}

/**
 * @brief   �ļ����ʹ�С������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_nanme_size_callback(void *ptr, char *file_name, uint32_t file_size)
{
  burn_catalog_flash((uint8_t *)file_name, file_size);
  
  return 0;
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
  burn_res_flash((uint8_t *)file_data, w_size);
  
  return 0;
}

/**
 * @brief   һ���ļ�������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_callback(void *ptr)
{
  return 0;
}

#endif
