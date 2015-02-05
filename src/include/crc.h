#ifndef __I_CRC_
#define __I_CRC_

extern void CRC_Init();
extern uint32_t CRC_Compute(const uint8_t *buf, const size_t len);
#endif

