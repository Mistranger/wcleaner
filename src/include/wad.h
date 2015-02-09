#ifndef __I_WAD_
#define __I_WAD_

typedef struct wEntry_s {
	char name[8];           /// Holds lump's name
	struct wEntry_s *prev;  /// Link to previous lump in list
	struct wEntry_s *next;  /// Link to next lump in list
	uint32_t hash;          /// CRC32 hash code
	uint32_t size;          /// Lump's size
	uint32_t offset;        /// Original lump's offset from input wad
	void *data;             /// Pointer to data array
} wEntry_t;

// Structure containing info about wad file
typedef struct wFile_s {
	FILE *file;
	char *name;
	size_t lumpCount;
	size_t size;
	struct wEntry_s *eBegin;
	struct wEntry_s *eEnd;
} wFile_t;

wFile_t *Wad_Open(const char *filename);
void Wad_NewEntry(wFile_t *wFile, wEntry_t *entry, wEntry_t *before);
wEntry_t *Wad_FindEntry(const wFile_t *wFile, const char *name, const wEntry_t *after, bool rev);
void Wad_DeleteEntry(wFile_t *wFile, wEntry_t *toDel);
void Wad_Output(const wFile_t *wFile, const char *outFile);
bool Wad_Close(wFile_t *wFile);
#endif
