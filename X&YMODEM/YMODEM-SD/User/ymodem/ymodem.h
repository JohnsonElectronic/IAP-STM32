#ifndef XMODEM_H_
#define XMODEM_H_

#include "stm32f4xx.h"
#include <stdio.h>
//#include "uart.h"
//#include "flash.h"
//#include "stdbool.h"

/* Xmodem (128 �ֽ�) ����ʽ
 * Byte  0:       Header
 * Byte  1:       Packet number
 * Byte  2:       Packet number complement
 * Bytes 3-130:   Data
 * Bytes 131-132: CRC
 */

/* Xmodem (1024 bytes) packet format
 * Byte  0:         Header
 * Byte  1:         Packet number
 * Byte  2:         Packet number complement
 * Bytes 3-1026:    Data
 * Bytes 1027-1028: CRC
 */

/* ����������(�û�����). */
#define Y_MAY_ERRORS ((uint8_t)3u)

/* ��ͷ���ձ�־ */
#define Y_IS_PACKET     ((uint8_t)1u)
#define Y_NO_PACKET     ((uint8_t)0u)

/* ���ݰ���С. */
#define Y_PACKET_128_SIZE   ((uint16_t)128u)
#define Y_PACKET_1024_SIZE  ((uint16_t)1024u)
#define Y_PACKET_CRC_SIZE   ((uint16_t)2u)

/* �������λ��(������ͷ). */
#define Y_PACKET_NUMBER_INDEX             ((uint16_t)1u)
#define Y_PACKET_NUMBER_COMPLEMENT_INDEX  ((uint16_t)2u)
#define Y_PACKET_DATA_INDEX               ((uint16_t)3u)

/* Э�鶨����ֽ�. */
#define Y_SOH ((uint8_t)0x01u)  /**< ��ͷ (128 bytes). */
#define Y_STX ((uint8_t)0x02u)  /**< ��ͷ (1024 bytes). */
#define Y_EOT ((uint8_t)0x04u)  /**< �������. */
#define Y_ACK ((uint8_t)0x06u)  /**< Ӧ��. */
#define Y_NAK ((uint8_t)0x15u)  /**< ��Ӧ��. */
#define Y_CAN ((uint8_t)0x18u)  /**< ȡ��. */
#define Y_C   ((uint8_t)0x43u)  /**< ASCII��C����Ҫ֪ͨ��λ��������Ҫ��CRC16. */

/* ���ܵ�״̬����. */
typedef enum {
  Y_OK            = 0x00u, /**< ����ɹ�. */
  Y_ERROR_CRC     = 0x01u, /**< CRC У�����. */
  Y_ERROR_NUMBER  = 0x02u, /**< ��������ƥ�����. */
  Y_ERROR_UART    = 0x04u, /**< �������. */
  Y_ERROR_FLASH   = 0x06u, /**< Flash ����. */
  Y_ERROR         = 0xFFu  /**< ��������. */
} ymodem_status;

void ymodem_receive(uint16_t rec_num);

#endif /* XMODEM_H_ */
