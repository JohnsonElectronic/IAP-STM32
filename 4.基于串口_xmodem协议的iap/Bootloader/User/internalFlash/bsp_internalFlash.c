/**
  ******************************************************************************
  * @file    bsp_internalFlash.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   �ڲ�FLASH��д���Է���
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "./internalFlash/bsp_internalFlash.h"   
#include <stdio.h>

/**
  * @brief  ���ݵ�ַ�ͳ��Ȳ�����������
  * @param  start_address ��Ҫд�����ʼ��ַ
	* @param  len ������
  * @retval ��
  */
int erasure_sector(uint32_t start_address, uint32_t len)
{
	sector_t sector_inof;         // ��¼��������Ϣ
	uint32_t sector_size  = 0;    // ��¼���������Ĵ�С
	uint32_t address = start_address;
	
	sector_inof = GetSector(start_address);    // ��ȡ��һ����������Ϣ

  /* �����û����� (�û�����ָ������û��ʹ�õĿռ䣬�����Զ���)**/
  /* �������FLASH�ı�־λ */  
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
	
	if ((sector_inof.start_addr + sector_inof.size) - (start_address + len) > 0)    // ��һ���������㹻��������
	{
		/* VoltageRange_3 �ԡ��֡��Ĵ�С���в��� */ 
    if (FLASH_EraseSector(sector_inof.number, VoltageRange_3) != FLASH_COMPLETE)    // ������Ӧ����
    { 
      /*�����������أ�ʵ��Ӧ���пɼ��봦�� */
			return -1;
    }
	}
	else     // ��һ�������������Է�����������
	{
		sector_size += (sector_inof.start_addr + sector_inof.size) - start_address;    // ��¼��һ�������Ĵ�С
		address += sector_size;                // ƫ������һ����������ʼ��ַ
		
		do
		{
			sector_inof = GetSector(address);    // ��ȡһ����������Ϣ
			
			/* �������� VoltageRange_3 �ԡ��֡��Ĵ�С���в��� */ 
			if (FLASH_EraseSector(sector_inof.number, VoltageRange_3) != FLASH_COMPLETE)    // ������Ӧ����
			{ 
				/*�����������أ�ʵ��Ӧ���пɼ��봦�� */
				return -1;
			}
			
			address += sector_inof.size;        // ƫ������һ����������ʼ��ַ
			sector_size += sector_inof.size;    // ��¼���������Ĵ�С
			
			
		}while (sector_size >= len);          // �������������Դ������������
	}
	
	return 0;
}

/**
  * @brief  ���ݵ�ַ�ͳ��Ȳ�����������
  * @param  start_address ��Ҫд�����ʼ��ַ
  * @param  *data : ��Ҫ���������
	* @param  len ������
  * @retval ��
  */
int flash_write_data(uint32_t start_address, const void *data, uint32_t len)
{
	uint8_t *data_8 = (uint8_t *)data;
	uint32_t address = start_address;
	
	/* �ԡ��֡��Ĵ�СΪ��λд������ ******************************** */
	address = start_address;
  while (len)
  {
    if (FLASH_ProgramByte(address, *data_8) == FLASH_COMPLETE)
    {
			address++;
			data_8++;
      len -= len > 1 ? 1 : len;
    }
    else
    {
      /*д��������أ�ʵ��Ӧ���пɼ��봦�� */
			return -1;
    }
  }
	
	return 0;
}

int save_data_flash(uint32_t start_address, const void *data, uint32_t len) 
{
	uint8_t *data_v = (uint8_t *)data;
	uint32_t address = start_address;
	uint32_t data_len = len;
	__IO uint8_t uw_data = 0;
	__IO uint8_t uwMemoryProgramStatus = 0;
	
	if (start_address % 4 != 0)    // ����ַ�ĺϷ���
	{
		return -1;    // ��ַ�Ƿ�������
	}
	
	/* FLASH ���� ********************************/
  /* ʹ�ܷ���FLASH���ƼĴ��� */
  FLASH_Unlock();
	
	if (erasure_sector(address, len))    // ������������
	{
		return -1;    // ������������
	}
	
	if (flash_write_data(start_address, data, len))    // ������������
	{
		return -1;    // д���ݳ�������
	}

	/* ��FLASH��������ֹ���ݱ��۸�*/
  FLASH_Lock(); 
	
	address = start_address;
	
	printf("��ʼ��ַ �� 0x%X ������ַ ��0x%X \r\n", address, start_address + data_len);
	
	/* ����У�� */
  while (address < start_address + data_len)
  {
    uw_data = *(__IO uint8_t*)address;
		
    if (uw_data != *data_v++)
    {
			printf("��ַ �� 0x%X ʵ�������� ��0x%X  ϣ���������� ��0x%X \r\n", address, uw_data, *data_v);
      uwMemoryProgramStatus++;  
    }

    address++;
  }  
	
  /* ����У�鲻��ȷ */
  if(uwMemoryProgramStatus)
  {    
		return -1;
  }
  else /*����У����ȷ*/
  { 
		return 0;   
  }
}

/**
  * @brief  ��������ĵ�ַ���������ڵ�sector
  *					���磺
						uwStartSector = GetSector(FLASH_USER_START_ADDR);
						uwEndSector = GetSector(FLASH_USER_END_ADDR);	
  * @param  Address����ַ
  * @retval ��ַ���ڵ�sector
  */
sector_t GetSector(uint32_t Address)
{
  sector_t sector;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_0;
    sector.number = FLASH_Sector_0; 
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_1;
    sector.number = FLASH_Sector_1;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_2;
    sector.number = FLASH_Sector_2;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_3;
    sector.number = FLASH_Sector_3;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_4;
    sector.number = FLASH_Sector_4;  
		sector.size = 64 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_5;
    sector.number = FLASH_Sector_5;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_6;
    sector.number = FLASH_Sector_6;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_7;
    sector.number = FLASH_Sector_7;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_8;
    sector.number = FLASH_Sector_8;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_9;
    sector.number = FLASH_Sector_9;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_10;
    sector.number = FLASH_Sector_10;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_11;
    sector.number = FLASH_Sector_11;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_12;
    sector.number = FLASH_Sector_12;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_13;
    sector.number = FLASH_Sector_13;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_14;
    sector.number = FLASH_Sector_14;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_15;
    sector.number = FLASH_Sector_15;  
		sector.size = 16 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_16;
    sector.number = FLASH_Sector_16;  
		sector.size = 64 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_17;
    sector.number = FLASH_Sector_17;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_18;
    sector.number = FLASH_Sector_18;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_19;
    sector.number = FLASH_Sector_19;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_20;
    sector.number = FLASH_Sector_20;  
		sector.size = 128 * KB;
  } 
  else if((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_21;
    sector.number = FLASH_Sector_21;  
		sector.size = 128 * KB;
  }
  else if((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
  {
		sector.start_addr = ADDR_FLASH_SECTOR_22;
    sector.number = FLASH_Sector_22;  
		sector.size = 128 * KB;
  }
  else/*if ((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23)) */
  {
		sector.start_addr = ADDR_FLASH_SECTOR_23;
    sector.number = FLASH_Sector_23;  
		sector.size = 16 * KB;
  }
  
  return sector;
}



