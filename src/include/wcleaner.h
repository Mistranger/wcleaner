#ifndef __I_WCLEANER_
#define __I_WCLEANER_

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <mem.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define _C_  ,    /// Debug , to simulate vararg macros

// Program settings
typedef struct wSettings_s {
	char *ConfigFile;
	bool SupressDebug;
} wSettings_t;

extern wSettings_t Settings;
extern const char DefaultSettingsFileName[];

extern void WC_Cleanup();
extern void WC_Exit(int code);

#endif
