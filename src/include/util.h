#ifndef __I_UTIL_
#define __I_UTIL_

extern void PrintFunction(const char *func, const char *format, ...);
//extern bool strncasecmp(const char *s1, const char *s2, size_t n);
/// Print debug information with function name.
#define DebugPrint(args) \
	do { if (!Settings.SupressDebug) PrintFunction(__func__, args); } while (0)
#define Assert(cond) \
	do { if (!(cond)) { AbortAt(__FILE__, __LINE__, __func__, #cond); }} while (0)

extern void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr);

/// Integer power function
int ipow(int num, int n);

/// Read 2 bytes in little-endian format from file stream
extern uint16_t read2bytes(FILE *file);
/// Read 2 bytes in little-endian format from memory
extern uint16_t read2bytes_mem(void *mem);
/// Read 4 bytes in little-endian format from file stream
extern uint32_t read4bytes(FILE *file);
/// Read 2 bytes in little-endian format from memory
extern uint32_t read4bytes_mem(void *mem);
/// Write 2 bytes in little-endian format into memory
extern void write2bytes_mem(void *mem, uint16_t data);
/// Write 4 bytes in little-endian format into file
extern void write4bytes(FILE *file, uint32_t data);
/// Write 4 bytes in little-endian format into memory
extern void write4bytes_mem(void *mem, uint32_t data);
/// Copy string and return a pointer to it in memory
extern char* strclone(const char *str, uint32_t maxLen);

#endif
