#ifndef __I_TEXOPT_
#define __I_TEXOPT_

#include "wad.h"

extern int tex_compare(const void *lop, const void *rop);
extern int pat_compare(const void *lop, const void *rop);

typedef struct tPnPatch_s {
	uint16_t patchnum;
	char name[8];
	bool toDelete;
} tPnPatch_t;

typedef struct tPatch_s {
	uint16_t originx, originy;
	uint16_t patchnum;
	uint16_t newpatchnum; // offset in new optimized PNAMES
	uint16_t stepdir, colormap;
} tPatch_t;

typedef struct tTexture_s {
	char name[8];
	uint32_t masked;
	uint16_t width;
	uint16_t height;
	uint32_t columndir;
	uint16_t patchcount;
	struct tPatch_s *patches;
	struct tTexture_s *prev, *next;
	bool animated;
	bool isswitch;
} tTexture_t;

typedef struct tFlat_s {
	char name[8];
	bool animated;
} tFlat_t;

extern bool TexOpt_Optimize(wFile_t *wFile);

#endif //__I_TEXOPT_
