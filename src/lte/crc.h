#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

uint16_t lte_crc16_pack(const uint8_t *crc, int len);
uint16_t lte_crc16_gen(const uint8_t *data, int len);

int lte_crc16_chk_unpacked(const uint8_t *data, int dlen,
			   const uint8_t *crc, int clen);
int lte_crc24a_chk(const uint8_t *data, int dlen,
		   const uint8_t *crc, int clen);
int lte_crc24b_chk(const uint8_t *data, int dlen,
		   const uint8_t *crc, int clen);

#endif /* _CRC_H_ */
