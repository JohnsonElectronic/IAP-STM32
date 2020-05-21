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
#define X_MAX_ERRORS ((uint8_t)3u)

/* ��ͷ���ձ�־ */
#define X_IS_PACKET     ((uint8_t)1u)
#define X_NO_PACKET     ((uint8_t)0u)

/* ���ݰ���С. */
#define X_PACKET_128_SIZE   ((uint16_t)128u)
#define X_PACKET_1024_SIZE  ((uint16_t)1024u)
#define X_PACKET_CRC_SIZE   ((uint16_t)2u)

/* �������λ��(������ͷ). */
#define X_PACKET_NUMBER_INDEX             ((uint16_t)1u)
#define X_PACKET_NUMBER_COMPLEMENT_INDEX  ((uint16_t)2u)
#define X_PACKET_DATA_INDEX               ((uint16_t)3u)

/* Э�鶨����ֽ�. */
#define X_SOH ((uint8_t)0x01u)  /**< ��ͷ (128 bytes). */
#define X_STX ((uint8_t)0x02u)  /**< ��ͷ (1024 bytes). */
#define X_EOT ((uint8_t)0x04u)  /**< �������. */
#define X_ACK ((uint8_t)0x06u)  /**< Ӧ��. */
#define X_NAK ((uint8_t)0x15u)  /**< ��Ӧ��. */
#define X_CAN ((uint8_t)0x18u)  /**< ȡ��. */
#define X_C   ((uint8_t)0x43u)  /**< ASCII��C����Ҫ֪ͨ��λ��������Ҫ��CRC16. */

/* ���ܵ�״̬����. */
typedef enum {
  X_OK            = 0x00u, /**< ����ɹ�. */
  X_ERROR_CRC     = 0x01u, /**< CRC У�����. */
  X_ERROR_NUMBER  = 0x02u, /**< ��������ƥ�����. */
  X_ERROR_UART    = 0x04u, /**< �������. */
  X_ERROR_FLASH   = 0x06u, /**< Flash ����. */
  X_ERROR         = 0xFFu  /**< ��������. */
} xmodem_status;

void xmodem_receive(void);

#endif /* XMODEM_H_ */
