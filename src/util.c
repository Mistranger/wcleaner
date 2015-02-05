#include "wcleaner.h"

#include "util.h"

void PrintFunction(const char *func, const char *format, ...)
{
    fprintf(stdout, "%s: ", func);
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
	fflush(stdout);
}
/*
bool strncasecmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *us1 = (const unsigned char *)s1;
    const unsigned char *us2 = (const unsigned char *)s2;
    if (n != 0) {
        do {
            if (tolower(*us1) != tolower(*us2++))
                return (tolower(*us1) - tolower(*--us2));
            if (*us1++ == '\0')
                break;
        } while (--n != 0);
    }
    return false;
}
*/

inline int ipow(int num, int n)
{
	int s = 1;
	for (int i = 0; i < n; ++i) {
		s *= num;
	}
	return s;
}


inline uint16_t read2bytes(FILE *file)
{
	uint16_t res = 0;
	for (size_t i = 0; i < 2; ++i) {
		res += fgetc(file) << (0x08 * i);
	}
	return res;
}

inline uint16_t read2bytes_mem(void *mem)
{
	uint16_t res = 0;
	for (size_t i = 0; i < 2; ++i) {
		res += *((uint8_t*)mem + i) << (0x08 * i);
	}
	return res;
}

inline uint32_t read4bytes(FILE *file)
{
	uint32_t res = 0;
	for (size_t i = 0; i < 4; ++i) {
		res += fgetc(file) << (0x08 * i);
	}
	return res;
}

inline uint32_t read4bytes_mem(void *mem)
{
	uint32_t res = 0;
	for (size_t i = 0; i < 4; ++i) {
		res += *((uint8_t*)mem + i) << (0x08 * i);
	}
	return res;
}

inline void write2bytes_mem(void *mem, uint16_t data)
{
    uint8_t *p = (uint8_t*)(mem);
	for (size_t i = 0; i < 2; ++i, ++p) {
		*p = (data & (0xFF << (0x08 * i))) >> (0x08 * i);
	}
}

inline void write4bytes(FILE *file, uint32_t data)
{
	for (size_t i = 0; i < 4; ++i) {
		fputc((data & (0xFF << (0x08 * i))) >> (0x08 * i), file);
	}
}

inline void write4bytes_mem(void *mem, uint32_t data)
{
    uint8_t *p = (uint8_t*)(mem);
	for (size_t i = 0; i < 4; ++i, ++p) {
		*p = (data & (0xFF << (0x08 * i))) >> (0x08 * i);
	}
}

inline char* strclone(const char *str, uint32_t maxLen)
{
	const size_t nameLen = strlen(str) + 1;
	char *result = (char*)malloc(nameLen * sizeof(char));
	strncpy(result, str, maxLen > nameLen ? nameLen : maxLen);
	return result;
}
