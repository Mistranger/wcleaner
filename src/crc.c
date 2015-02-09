#include "wcleaner.h"

#include "crc.h"

uint32_t crc_table[256];

void CRC_Init()
{
	for (uint32_t i = 0; i < 256; ++i) {
		uint32_t c = i;
		for (int j = 0; j < 8; ++j) {
			c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
		}
		crc_table[i] = c;
	}
}

uint32_t CRC_Compute(const uint8_t *buf, const size_t len)
{
	uint32_t c = 0xFFFFFFFF;
	for (size_t i = 0; i < len; ++i) {
		c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
	}
	return c ^ 0xFFFFFFFF;
}
