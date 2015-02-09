#ifndef __I_CONFIG_
#define __I_CONFIG_

#define MAX_FILE_NAME_LENGTH 260

typedef struct cFile_s {
	unsigned char *GameName;
	unsigned char IWADPath[MAX_FILE_NAME_LENGTH];

} cFile_t;

extern cFile_t *C_LoadConfig(const char *name);

extern int C_InitSettings(const char *name);

#endif
