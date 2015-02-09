#include "wcleaner.h"

#include "crc.h"
#include "util.h"
#include "wad.h"

// Pointer to currently opened wad file
wFile_t *CurWadFile;

wFile_t *Wad_Open(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	int32_t lumpCount;
	if (!file) {
		DebugPrint("ERROR: Cannot open file : %s\n" _C_ filename);
		return NULL;
	} else { // Perform some checks
		char verify[4];
		fread(verify, 1, 4, file);
		if (strncmp(verify, "PWAD", 4)) {
			DebugPrint("ERROR: Not a PWAD : %s\n" _C_ filename);
			fclose(file);
			return NULL;
		}
		lumpCount = read4bytes(file);
		if (!lumpCount) {
			DebugPrint("ERROR: Empty PWAD : %s\n" _C_ filename);
			fclose(file);
			return NULL;
		}
	}

	// Create wad file structure
	wFile_t *wadFile = (wFile_t *)malloc(sizeof(wFile_t));
	memset(wadFile, 0, sizeof(wFile_t));
	wadFile->name = strclone(filename, INT_MAX);
	wadFile->file = file;
	uint32_t dirOffset = read4bytes(file);
	fseek(file, dirOffset, SEEK_SET);

	// Read all enrties
	for (int i = 0; i < lumpCount; ++i) {
		wEntry_t *entry = (wEntry_t *)malloc(sizeof(wEntry_t));
		entry->data = NULL;
		entry->offset = read4bytes(file);
		entry->size = read4bytes(file);
		fread(entry->name, 1, 8, file);
		Wad_NewEntry(wadFile, NULL, entry);
	}
	DebugPrint("Loaded %s with %d entries\n" _C_ wadFile->name _C_ lumpCount);
	wEntry_t *entry = wadFile->eBegin;

	// Calc lump's hash code
	do {
		fseek(file, entry->offset, SEEK_SET);
		uint8_t *data = malloc(entry->size);
		fread(data, 1, entry->size, file);
		entry->hash = CRC_Compute(data, entry->size);
		free(data);
		entry = entry->next;
	} while (entry != NULL);

	return wadFile;
}

void Wad_NewEntry(wFile_t *wFile, wEntry_t *before, wEntry_t *entry)
{
	if (!wFile->eBegin) { // first entry
		wFile->eEnd = wFile->eBegin = entry;
		entry->next = entry->prev = NULL;
	} else {
		// If before is NULL, will add into entrylist end
		wEntry_t *bef = before != NULL ? before : wFile->eEnd;

		if (before == NULL) {
			entry->prev = bef;
			entry->next = NULL;
			wFile->eEnd->next = entry;
			wFile->eEnd = entry;
		} else {
			entry->next = bef;
			entry->prev = bef->prev;
			if (bef == wFile->eBegin) {
				wFile->eBegin = entry;
			} else {
				bef->prev->next = entry;
			}
			bef->prev = entry;
		}
	}
	++wFile->lumpCount;
	wFile->size += entry->size;
}

wEntry_t *Wad_FindEntry(const wFile_t *wFile, const char *name, const wEntry_t *after, bool rev)
{
	for (wEntry_t *entry = rev ? wFile->eEnd : (after ? after->next : wFile->eBegin);
		 entry != NULL;
		 rev ? (entry = entry->prev) : (entry = entry->next)) {
		if (!strncmp(entry->name, name, 8)) {
			return entry;
		}
	}
	return NULL;
}

void Wad_DeleteEntry(wFile_t *wFile, wEntry_t *toDel)
{
	if (toDel == NULL) {
		DebugPrint("ERROR: entry doesn't exist\n");
		return;
	}
	wFile->size -= toDel->size;
	--wFile->lumpCount;
	if (toDel->prev) {
		toDel->prev->next = toDel->next;
	}
	if (toDel->next) {
		toDel->next->prev = toDel->prev;
	}
	if (wFile->eBegin == toDel) {
		wFile->eBegin = toDel->next;
	}
	if (wFile->eEnd == toDel) {
		wFile->eEnd = toDel->prev;
	}

	free(toDel);
}

void Wad_Output(const wFile_t *wFile, const char *outFile)
{
	bool overwrite = false;
	char outputWad[260];
	if (!outFile) {
		overwrite = true;
		strcpy(outputWad, wFile->name);
		strcat(outputWad, ".temp");
	} else {
		strcpy(outputWad, outFile);
	}
	FILE *input = wFile->file;
	FILE *output = fopen(outputWad, "wb");
	if (!output) {
		// TO-DO : handle error
	}

	// Write header info
	const char verify[4] = {'P', 'W', 'A', 'D'};
	fwrite(verify, 1, 4, output);
	write4bytes(output, wFile->lumpCount);
	write4bytes(output, 0); // dir will be filled later
	uint32_t dirOffset = 0x0C; // data starts from offset 0x0C
	int32_t *offsets = (int32_t *)malloc(wFile->lumpCount * sizeof(int32_t));
	int i = 0;

	// Write all entries
	for (wEntry_t *entry = wFile->eBegin; entry != NULL; entry = entry->next) {

		offsets[i++] = dirOffset;
		dirOffset += entry->size;
		if (entry->data != NULL) {
			fwrite(entry->data, 1, entry->size, output);
			free(entry->data);
		} else {
			uint8_t *buf = (uint8_t *)malloc(entry->size * sizeof(uint8_t));
			fseek(input, entry->offset, SEEK_SET);
			fread(buf, 1, entry->size, input);
			fwrite(buf, 1, entry->size, output);
			free(buf);
		}
	}

	// Write directory
	i = 0;
	for (wEntry_t *entry = wFile->eBegin; entry != NULL; entry = entry->next) {
		write4bytes(output, offsets[i++]);
		write4bytes(output, entry->size);
		fwrite(entry->name, 1, 8, output);
	}

	// Now write dir offset
	fseek(output, 0x08, SEEK_SET);
	write4bytes(output, dirOffset);
	fclose(output);
	free(offsets);

	// Erase original PWAD
	if (overwrite) {
		fclose(wFile->file);
		remove(wFile->name);

		rename(outputWad, wFile->name);
	}
}

bool Wad_Close(wFile_t *wFile)
{
	for (wEntry_t *entry = wFile->eBegin; entry != NULL;) {
		wEntry_t *old = entry;
		entry = entry->next;
		free(old);
	}
	fclose(wFile->file);
	return true;
}
