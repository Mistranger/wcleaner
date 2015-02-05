#include "wcleaner.h"

#include "crc.h"
#include "texopt.h"
#include "textures.h"
#include "util.h"
#include "wad.h"

bool IsTexture2;           /// True, if TEXTURE2 was found

// TexOpt - textures
tTexture_t *TextureList, *LastTexture;
uint32_t TextureCount;
uint8_t *UsedTextures;
uint32_t UsedTexturesCount;

// TexOpt - patches
tPnPatch_t *Pnames;
uint32_t PatchCount;
uint16_t *UsedPatches;
uint32_t UsedPatchesCount;
uint32_t OptimizedPatchesCount;

// TexOpt - flats
tFlat_t *Flats;
uint32_t FlatCount;
uint8_t *UsedFlats;
uint32_t UsedFlatsCount;

inline int tex_compare(const void *lop, const void *rop)
{
	return strncmp(lop, rop, 8);
}

inline int pat_compare(const void *lop, const void *rop)
{
	return ((*(uint16_t*)lop > *(uint16_t*)rop) ? 1 : ((*(uint16_t*)lop < *(uint16_t*)rop) ? -1 : 0));
}

static bool tex_search(const void *key, const void *data, const size_t count, const size_t size,
    int (*compar)(const void *lop, const void *rop))
{
    const unsigned char *p_key = (unsigned char*)key;
    const unsigned char *p_begin = (unsigned char*)data;
    unsigned char *p_data = (unsigned char*)data;
    const size_t length =  count * size;

    for (; p_data < p_begin + length; p_data += size) {
        if (!compar(p_key, p_data)) {
            return true;
        }
    }
    return false;
}

static int TexOpt_FillTextureList(const wFile_t *wFile, wEntry_t *texture)
{
	FILE *input = wFile->file;
	fseek(input, texture->offset, SEEK_SET);
	uint8_t *texLump = (uint8_t*)malloc(texture->size * sizeof(uint8_t)); // Our lump's data
	fread(texLump, 1, texture->size, input);
	uint32_t numTextures = read4bytes_mem(texLump);
	if (numTextures) {
		uint32_t doff; // data offset
		for (size_t i = 0; i < numTextures; ++i) {
            ++TextureCount;
			tTexture_t *tex = (tTexture_t*)malloc(sizeof(tTexture_t));
			doff = read4bytes_mem((void*)(texLump + 4 + 4 * i));
			memcpy((void*)(tex->name), (void*)(texLump + doff), 8);
			tex->masked = read4bytes_mem((void*)(texLump + doff + 0x08));
			tex->width = read2bytes_mem((void*)(texLump + doff + 0x0C));
			tex->height = read2bytes_mem((void*)(texLump + doff + 0x0E));
			tex->columndir = read4bytes_mem((void*)(texLump + doff + 0x10));
			tex->animated = false;
			tex->isswitch = false;
			tex->patchcount = read2bytes_mem((void*)(texLump + doff + 0x14));
			tex->patches = (tPatch_t*)malloc(tex->patchcount * sizeof(tPatch_t));
			for (int j = 0; j < tex->patchcount; ++j) {
				tex->patches[j].originx = read2bytes_mem((void*)(texLump + doff + 0x16 + j * 0x0A));
				tex->patches[j].originy = read2bytes_mem((void*)(texLump + doff + 0x18 + j * 0x0A));
				tex->patches[j].patchnum = tex->patches[j].newpatchnum
                    = read2bytes_mem((void*)(texLump + doff + 0x1A + j * 0x0A));
				tex->patches[j].stepdir = read2bytes_mem((void*)(texLump + doff + 0x1C + j * 0x0A));
				tex->patches[j].colormap = read2bytes_mem((void*)(texLump + doff + 0x1E + j * 0x0A));
			}
			if (TextureList == NULL) {
				TextureList = tex;
				tex->prev = tex->next = NULL;
			} else {
				tex->next = NULL;
				tex->prev = LastTexture;
				tex->prev->next = tex;
			}
			LastTexture = tex;
		}
	}
	free(texLump);

	return numTextures;
}

static void TexOpt_FillAnimatedTextures(wFile_t *wFile)
{
    FILE *input = wFile->file;
    if (!TextureCount && !FlatCount) {
        DebugPrint("WARNING: no textures and flats loaded\n");
        return;
    }

    wEntry_t *animated;
    char *animFlatList = NULL;
    size_t animFlatListSize = 0;
    char *animTexList = NULL;
    size_t animTexListSize = 0;
    if ((animated = Wad_FindEntry(wFile, "ANIMATED", NULL, true)) != NULL) {
        uint8_t *buf = (uint8_t*)malloc(animated->size);
        char firstTex[9];
        char lastTex[9];

        fseek(input, animated->offset, SEEK_SET);
        fread(buf, 1, animated->size, input);
        uint8_t *p = buf;
        // first count textures and flats
        for (uint8_t *p = buf; *p != 0xFF; p += 0x17) { // through ANIMATED lumps
            if (*p == 0) {
                ++animFlatListSize;
            } else {
                ++animTexListSize;
            }
        }
        animFlatListSize *= 0x10;
        animTexListSize *= 0x10;
        animFlatList = (char*)malloc(animFlatListSize);
        animTexList = (char*)malloc(animTexListSize);
        char *afp = animFlatList;
        char *atp = animTexList;
        for (size_t i = *p++; i != 0xFF; i = *p, ++p) {          // end of file
            strcpy(lastTex, (char*)p);  p += 0x09;   // last texture
            strcpy(firstTex, (char*)p); p += 0x09;   // first texture
            // copy to array
            if (i == 0) {
                strncpy(afp, firstTex, 0x08);  afp += 0x08;
                strncpy(afp, lastTex, 0x08);   afp += 0x08;
            } else {
                strncpy(atp, firstTex, 0x08);  atp += 0x08;
                strncpy(atp, lastTex, 0x08);   atp += 0x08;
            }
            p += 0x04;                  // skip speed
        }
        free(buf);
    } else {
        DebugPrint("ANIMATED not found, will try to use standard animated textures\n");
    }

    // Now let's fill information about texture animation
	bool isAnimated = false;
	const char *texEnd = NULL;
	for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        if (isAnimated) {
            tex->animated = true;
            if (!strncmp(tex->name, texEnd, 8)) {
                // end of animation
                isAnimated = false;
            }
            continue;
        }
	    for (size_t i = 0; i < (animTexList ? animTexListSize : StAnimTexSize); i += 0x10) {
            if (!strncmp(tex->name, (animTexList ? animTexList : StAnimTex) + i, 8)) {
                isAnimated = true;
                texEnd = (animTexList ? animTexList : StAnimTex) + i + 0x08; // get animation end texture name
                tex->animated = true;
                break;
            }
	    }
	}
	isAnimated = false, texEnd = NULL;
	for (size_t i = 0; i < FlatCount; ++i) {
        if (isAnimated) {
            Flats[i].animated = true;
            if (!strncmp(Flats[i].name, texEnd, 8)) {
                // end of animation
                isAnimated = false;
            }
            continue;
        }
	    for (size_t j = 0; j < (animFlatList ? animFlatListSize : StAnimFlatSize); j += 0x10) {
            if (!strncmp(Flats[i].name, (animFlatList ? animFlatList : StAnimFlat) + j, 8)) {
                isAnimated = true;
                texEnd = (animFlatList ? animFlatList : StAnimFlat) + j + 0x08; // get animation end texture name
                Flats[i].animated = true;
                break;
            }
	    }
	}
	free(animFlatList);
	free(animTexList);
}

static void TexOpt_FillSwitches(wFile_t *wFile)
{
    FILE *input = wFile->file;
    if (!TextureCount) {
        DebugPrint("WARNING: no textures loaded\n");
        return;
    }
    wEntry_t *switches;
    char *switchList = NULL;
    size_t switchListSize = 0;
    if ((switches = Wad_FindEntry(wFile, "SWITCHES", NULL, true)) != NULL) {
        uint8_t *buf = (uint8_t*)malloc(switches->size);

        fseek(input, switches->offset, SEEK_SET);
        fread(buf, 1, switches->size, input);

        switchListSize = (switches->size / 0x14 - 1) * 0x10; // SWITCHES text and lump sizes
        switchList = (char*)malloc(switchListSize);
        char *sp = switchList;
        uint8_t *p = buf;
        while (read2bytes_mem(p + 0x12)) {
            strncpy(sp, (char*)p, 8);  p += 0x09; sp += 0x08;   // off switch
            strncpy(sp, (char*)p, 8);  p += 0x09; sp += 0x08;   // on switch
            p += 0x02;
        }
        free(buf);
    } else {
        DebugPrint("SWITCHES not found, will try to use standard switches\n");
    }

    // fill information about switches
    for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        for (size_t i = 0; i < (switchList ? switchListSize : StSwitchesSize); i += 0x08) {
            if (!strncmp(tex->name, (switchList ? switchList : StSwitches) + i, 8)) {
                tex->isswitch = true;
                break;
            }
        }
    }
    free(switchList);
}

static void TexOpt_DeleteTexture(tTexture_t *toDel)
{
	if (toDel->prev) {
		toDel->prev->next = toDel->next;
	}
	if (toDel->next) {
		toDel->next->prev = toDel->prev;
	}
	if (TextureList == toDel) {
		TextureList = toDel->next;
	}
	if (LastTexture == toDel) {
		LastTexture = toDel->prev;
	}

	free(toDel);
}

static void TexOpt_DeletePatch(wFile_t *wFile, tPnPatch_t *toDel)
{
    char patname[9];
    memset(patname, 0, sizeof(patname));
    memcpy(patname, toDel->name, 8);
    wEntry_t *entry = Wad_FindEntry(wFile, patname, NULL, false);
    // reduce patchnum in all textures
    for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        for (int j = 0; j < tex->patchcount; ++j) {
            if (tex->patches[j].patchnum > toDel->patchnum) {
                --tex->patches[j].newpatchnum;
            }
        }
    }
    --OptimizedPatchesCount;
	toDel->toDelete = true;
    Wad_DeleteEntry(wFile, entry);
}

static void TexOpt_FillUsedTextures(wFile_t *wFile)
{
	FILE *input = wFile->file;
	const size_t SIDEDEFS_ELEM = 30;
	wEntry_t *entry = NULL;
	UsedTexturesCount = 0;
	while ((entry = Wad_FindEntry(wFile, "SIDEDEFS", entry, false)) != NULL) {
		UsedTexturesCount += 3 * entry->size / SIDEDEFS_ELEM;
	}

	char *textures = (char*)malloc(UsedTexturesCount * 8);
	entry = NULL;
	size_t c = 0;
	while ((entry = Wad_FindEntry(wFile, "SIDEDEFS", entry, false)) != NULL) {
		uint8_t *sidedefs = (uint8_t*)malloc(entry->size);
		fseek(input, entry->offset, SEEK_SET);
		fread(sidedefs, 1, entry->size, input);
		for (size_t i = 0; i < entry->size; i += SIDEDEFS_ELEM, ++c) {
			memcpy(&textures[c * 8], sidedefs + i + 0x04, 8);
			memcpy(&textures[++c * 8], sidedefs + i + 0x0C, 8);
			memcpy(&textures[++c * 8], sidedefs + i + 0x14, 8);
		}
		free(sidedefs);
	}

	// sorting for cleanup
	qsort(textures, c, 8, tex_compare);

	// Clean up duplicates
	char *last = textures;
	uint32_t *offsets = (uint32_t*)malloc(c * sizeof(uint32_t));
	size_t p = 1; // 0 index already unique, so we have 1 unique texture by now
	offsets[0] = 0;

	// start from 1 because 0 is unique
	for (size_t i = 1; i < c; ++i) {
		if (strncmp(&textures[i * 8], last, 8)) {
			offsets[p++] = i * 8;
			last = &textures[i * 8];
		}
	}

	UsedTextures = (uint8_t*)malloc(p * 8);
	UsedTexturesCount = p;
	for (size_t i = 0; i < p; ++i) {
		memcpy(&UsedTextures[i * 8], &textures[offsets[i]], 8);
	}
	FILE *sss = fopen("1sd.txt", "wb");
	for (size_t i = 0; i < p; ++i) {
		fwrite(&UsedTextures[i*8], 8, 1, sss);
	}
	fclose(sss);
	free(offsets);
	free(textures);
}

static void TexOpt_FillUsedPatches(wFile_t *wFile)
{
    if (!UsedTexturesCount) { // load used textures now
        TexOpt_FillUsedTextures(wFile);
        if (!UsedTexturesCount) { // nothing to optimize
            DebugPrint("No used patches!");
            return;
        }
    }

    size_t count = 0;
    int c = 0;

    // count number of used patches first
    for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        count += tex->patchcount;
    }

    uint16_t *patches = (uint16_t*)malloc(count * sizeof(uint16_t));
    for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        for (size_t i = 0; i < tex->patchcount; ++i) {
            patches[c++] = tex->patches[i].patchnum;
        }
    }

    qsort(patches, count, sizeof(uint16_t), pat_compare);
    /*printf("\n");
    for (size_t i = 0; i < count; ++i) {
        printf("%i ", patches[i]);
    }*/
    // Clean out duplicates
    uint16_t last = patches[0];
    uint32_t *offsets = (uint32_t*)malloc(count * sizeof(uint32_t));
    size_t p = 1;  // 0 index already unique
    offsets[0] = 0;

    for (size_t i = 0; i < count; ++i) {
        if (patches[i] != last) {
            offsets[p++] = i;
            last = patches[i];
        }
    }

    UsedPatchesCount = p;
    UsedPatches = (uint16_t*)malloc(p * sizeof(uint16_t));
    for (size_t i = 0; i < p; ++i) {
        UsedPatches[i] = patches[offsets[i]];
    }
    free(offsets);
    free(patches);
}

static void TexOpt_FillUsedFlats(wFile_t *wFile)
{
	FILE *input = wFile->file;
	const size_t SECTORS_ELEM = 26;
	wEntry_t *entry = NULL;
	UsedFlatsCount = 0;
	while ((entry = Wad_FindEntry(wFile, "SECTORS", entry, false)) != NULL) {
		UsedFlatsCount += 2 * entry->size / SECTORS_ELEM;
	}

	char *flats = (char*)malloc(UsedFlatsCount * 8);
	entry = NULL;
	size_t c = 0;
	while ((entry = Wad_FindEntry(wFile, "SECTORS", entry, false)) != NULL) {
		uint8_t *sectors = (uint8_t*)malloc(entry->size);
		fseek(input, entry->offset, SEEK_SET);
		fread(sectors, 1, entry->size, input);
		for (size_t i = 0; i < entry->size; i += SECTORS_ELEM, ++c) {
			memcpy(&flats[c * 8], sectors + i + 0x04, 8);
			memcpy(&flats[++c * 8], sectors + i + 0x0C, 8);
		}
		free(sectors);
	}

	qsort(flats, c, 8, tex_compare);
	// Clean out duplicates
	char *last = flats;
	uint32_t *offsets = (uint32_t*)malloc(c * sizeof(uint32_t));
	size_t p = 1; // 0 index already unique
	offsets[0] = 0;

	for (size_t i = 0; i < c; ++i) {
		if (memcmp(flats + i * 8, last, 8)) {
			offsets[p++] = i * 8;
			last = flats + i * 8;
		}
	}

	UsedFlats = (uint8_t*)malloc(p * 8);
	UsedFlatsCount = p;
	for (size_t i = 0; i < p; ++i) {
		memcpy(&UsedFlats[i * 8], &flats[offsets[i]], 8);
	}/*
	for (size_t i = 0; i < UsedFlatsCount; ++i) {
        fwrite(&UsedFlats[i * 8], 1, 8, stdout);
    }*/
	free(offsets);
	free(flats);
}

static bool TexOpt_OptimizeTextures(wFile_t *wFile)
{
    size_t totalUnused = 0;
    if (!UsedTexturesCount) { // load used textures now
        TexOpt_FillUsedTextures(wFile);
        if (!UsedTexturesCount) { // nothing to optimize
            DebugPrint("Nothing to optimize!");
            return false;
        }
    }

    for (tTexture_t *tex = TextureList; tex != NULL;) {
        char texname[9];
// TODO (cybermind#1#): binary search

        if (tex->animated || tex->isswitch) {
            tex = tex->next;
            continue;
        }
        // look in standard textures
        if (tex_search(tex->name, StTex, StTexSize / 8, 8, tex_compare)
            || tex_search(tex->name, UsedTextures, UsedTexturesCount, 8, tex_compare)) {
                tex = tex->next;
                continue;
        }
        /*
        if (b_search(tex->name, StTex, StTexSize / 8, 8, tex_compare)
            || b_search(tex->name, UsedTextures, UsedTexturesCount, 8, tex_compare)) {
                // texture is used
                found = true;
        }*/
        ++totalUnused;
        memset(texname, 0, sizeof(texname));
        memcpy(texname, tex->name, 8);
        DebugPrint("Found unused texture %s\n" _C_ texname);

        // delete it
        tTexture_t *old = tex;
        tex = tex->next;
        TexOpt_DeleteTexture(old);
    }
    if (TextureCount) {
        DebugPrint("Total %d unused textures (%i%% unused)\n" _C_ totalUnused _C_ totalUnused * 100 / TextureCount);
    }
    return true;
}

static bool TexOpt_OptimizePatches(wFile_t *wFile)
{
    size_t totalUnused = 0;
    OptimizedPatchesCount = PatchCount;
    if (!UsedPatchesCount) { // load used patches now
        TexOpt_FillUsedPatches(wFile);
        if (!UsedPatchesCount) { // nothing to optimize
            DebugPrint("Nothing to optimize!");
            return false;
        }
    }

    for (size_t i = 0; i < PatchCount; ++i) {
        tPnPatch_t *patch = &Pnames[i];
        char patname[9];
        memset(patname, 0, sizeof(patname));
        memcpy(patname, patch->name, 8);


        // look in standard patches
        if (tex_search(patch->name, StPat, StPatSize / 8, 8, tex_compare)
            || tex_search(&patch->patchnum, UsedPatches, UsedPatchesCount, 2, pat_compare)) {
                continue;
        }
        ++totalUnused;
        if (!Wad_FindEntry(wFile, patname, NULL, false)) {
            DebugPrint("Found invalid patch %s\n" _C_ patname);
        } else {
            DebugPrint("Found unused patch %s\n" _C_ patname);
        }
        // delete it
        TexOpt_DeletePatch(wFile, patch);
    }
    if (PatchCount) {
        DebugPrint("Total %d unused patches (%i%% unused)\n" _C_ totalUnused _C_ totalUnused * 100 / PatchCount);
    }
    return true;
}

static bool TexOpt_OptimizeFlats(wFile_t *wFile)
{
    size_t totalUnused = 0;
    if (!UsedFlatsCount) { // load used patches now
        TexOpt_FillUsedFlats(wFile);
        if (!UsedFlatsCount) { // nothing to optimize
            DebugPrint("Nothing to optimize!");
            return false;
        }
    }

    for (size_t i = 0; i < FlatCount; ++i) {
        const tFlat_t *flat = &Flats[i];
        char flatname[9];

        if (flat->animated) {
            continue;
        }
        if (tex_search(flat->name, UsedFlats, UsedFlatsCount, 8, tex_compare)) {
            continue;
        }
        ++totalUnused;
        memset(flatname, 0, sizeof(flatname));
        memcpy(flatname, flat->name, 8);
        DebugPrint("Found unused flat %s\n" _C_ flatname);

        // delete it
        Wad_DeleteEntry(wFile, Wad_FindEntry(wFile, flatname, NULL, false));
    }
    if (FlatCount) {
        DebugPrint("Total %d unused flats (%i%% unused)\n" _C_ totalUnused _C_ totalUnused * 100 / FlatCount);
    }
    return true;
}

static void TexOpt_LoadPNames(const wFile_t *wFile)
{
    FILE *input = wFile->file;
    // Extract PNAMES lump
	wEntry_t *pnames = Wad_FindEntry(wFile, "PNAMES", NULL, false);
	if (!pnames) {
		fprintf(stderr, "PNAMES not found - no new textures in PWAD\n");
		return;
	}
	fseek(input, pnames->offset, SEEK_SET);
	PatchCount = read4bytes(input);
	Pnames = (tPnPatch_t*)malloc(PatchCount * sizeof(tPnPatch_t));
	for (size_t i = 0; i < PatchCount; ++i) {
		fread(Pnames[i].name, 1, 8, input);
		Pnames[i].patchnum = i;
		Pnames[i].toDelete = false;
	}
	DebugPrint("Loaded %i patches\n" _C_ PatchCount);
}

static void TexOpt_LoadTextures(const wFile_t *wFile)
{
    size_t texCount = 0;
	wEntry_t *texture = Wad_FindEntry(wFile, "TEXTURE1", NULL, false);
	if (!texture) {
		DebugPrint("ERROR: TEXTURE1 not found - no new textures in PWAD\n");
		return;
	}
	texCount = TexOpt_FillTextureList(wFile, texture);
	texture = Wad_FindEntry(wFile, "TEXTURE2", NULL, false);
	if (!texture) {
		DebugPrint("TEXTURE2 not found\n");
	} else {
		texCount += TexOpt_FillTextureList(wFile, texture);
		IsTexture2 = true;
	}
	DebugPrint("Loaded %i textures\n" _C_ texCount);
}

static void TexOpt_LoadFlats(const wFile_t *wFile)
{
    size_t flatCount = 0;
	wEntry_t *flatStart = Wad_FindEntry(wFile, "F_START", NULL, false) ;
	if (!flatStart) {
        flatStart = Wad_FindEntry(wFile, "FF_START", NULL, false) ;
	}
	wEntry_t *flatEnd = Wad_FindEntry(wFile, "F_END", NULL, false);
	if (!flatEnd) {
        flatEnd = Wad_FindEntry(wFile, "FF_END", NULL, false) ;
	}
	if (!flatStart || !flatEnd) {
		DebugPrint("No flats in PWAD or incorrect flat markers\n");
		return;
	}
	// count flats
	for (wEntry_t *flat = flatStart->next; flat && flat != flatEnd; ++flatCount, flat = flat->next) {
	}

	Flats = (tFlat_t*)malloc(flatCount * sizeof(tFlat_t));
	// load them
	size_t i = 0;
	for (wEntry_t *flat = flatStart->next; flat && flat != flatEnd; ++i, flat = flat->next) {
        memcpy(&Flats[i].name, flat->name, 8);
        Flats[i].animated = false;
	}
    FlatCount = flatCount;
	DebugPrint("Loaded %i flats\n" _C_ flatCount);
}

static void TexOpt_Output(wFile_t *wFile)
{
    size_t textureCount = 0;
    size_t textureSize = 0;

    // calculate new size of TEXTURE1
    for (tTexture_t *tex = TextureList; tex != NULL; tex = tex->next) {
        ++textureCount;
        textureSize += 0x16 + 0x0A * tex->patchcount; // texture entry size
    }
    textureSize += 4 * textureCount; // pointer to textures
    textureSize += 4; // numtextures

    // create new TEXTURE and fill it
    wEntry_t *texture = Wad_FindEntry(wFile, "TEXTURE1", NULL, false);
    wEntry_t *newTexture = (wEntry_t*)malloc(sizeof(wEntry_t));
    newTexture->data = NULL;
    newTexture->size = textureSize;
    strncpy(newTexture->name, "TEXTURE1", 8);
    uint8_t *textureData = (uint8_t*)malloc(textureSize);
    uint8_t *p = textureData; // pointer
    write4bytes_mem(textureData, textureCount);

    // Create offsets structure
    uint32_t *offsets = (uint32_t*)malloc(textureCount * sizeof(uint32_t));
    p += 0x04 + textureCount * 0x04; // shift to texture data
    size_t count = 0;
    for (tTexture_t *tex = TextureList; tex != NULL;  tex = tex->next) {
        offsets[count++] = p - textureData;
        memcpy(p, tex->name, 8);             p += 0x08;  /// data offsets
        write4bytes_mem(p, tex->masked);     p += 0x04;
        write2bytes_mem(p, tex->width);      p += 0x02;
        write2bytes_mem(p, tex->height);     p += 0x02;
        write4bytes_mem(p, tex->columndir);  p += 0x04;
        write2bytes_mem(p, tex->patchcount); p += 0x02;
        for (size_t i = 0; i < tex->patchcount; ++i) {
            write2bytes_mem(p, tex->patches[i].originx);     p += 0x02;
            write2bytes_mem(p, tex->patches[i].originy);     p += 0x02;
            write2bytes_mem(p, tex->patches[i].newpatchnum); p += 0x02;
            write2bytes_mem(p, tex->patches[i].stepdir);     p += 0x02;
            write2bytes_mem(p, tex->patches[i].colormap);    p += 0x02;
        }
    }
    // now write offsets
    p = textureData + 0x04;
    for (size_t i = 0; i < textureCount; ++i, p += 0x04) {
        write4bytes_mem(p, offsets[i]);
    }
    free(offsets);
    newTexture->data = textureData;
    Wad_NewEntry(wFile, texture, newTexture);
    // delete old TEXTURE1
    Wad_DeleteEntry(wFile, texture);

    // Create PNAMES
    uint8_t *pnames = (uint8_t*)malloc(OptimizedPatchesCount * 0x08 + 0x04);
    p = pnames;
    write4bytes_mem(p, OptimizedPatchesCount); p += 0x04;
    size_t i, pcount = 0;
    for (i = 0; i < PatchCount; ++i) {
        if (!Pnames[i].toDelete) {
            memcpy(p, Pnames[i].name, 8);
            p += 0x08;
            ++pcount;
        }
    }
    if (pcount != OptimizedPatchesCount) {
        DebugPrint("ERROR: incorrect number of patches");
        WC_Exit(1);
    }
    wEntry_t *newPNames = (wEntry_t*)malloc(sizeof(wEntry_t));
    newPNames->data = pnames;
    newPNames->size = OptimizedPatchesCount * 0x08 + 0x04;
    strncpy(newPNames->name, "PNAMES", 8);

    wEntry_t *oldPnames = Wad_FindEntry(wFile, "PNAMES", NULL, false);
    Wad_NewEntry(wFile, oldPnames, newPNames);
    // delete old PNAMES
    Wad_DeleteEntry(wFile, oldPnames);
}

bool TexOpt_Optimize(wFile_t *wFile)
{
	if (!wFile->eBegin) {
		DebugPrint("Fill data first\n");
		return false;
	}
	TexOpt_LoadPNames(wFile);
	TexOpt_LoadTextures(wFile);
	TexOpt_LoadFlats(wFile);
	if (TextureCount || FlatCount || PatchCount) {
	    if (IsTexture2) {
            // No need in TEXTURE2 furthermore
            Wad_DeleteEntry(wFile, Wad_FindEntry(wFile, "TEXTURE2", NULL, false));
        }
        TexOpt_FillAnimatedTextures(wFile);
        TexOpt_FillSwitches(wFile);
        if (TextureCount) {
            TexOpt_FillUsedTextures(wFile);
            TexOpt_OptimizeTextures(wFile);
        }
        if (PatchCount) {
            TexOpt_FillUsedPatches(wFile);
            TexOpt_OptimizePatches(wFile);
        }
        if (FlatCount) {
            TexOpt_FillUsedFlats(wFile);
            TexOpt_OptimizeFlats(wFile);
        }
        TexOpt_Output(wFile);
	}

    // clean memory
    if (Pnames) {
        free(Pnames);
    }
    if (Flats) {
        free(Flats);
    }
	for (tTexture_t *tex = TextureList; tex != NULL;) {
        free(tex->patches);
	    tTexture_t *old = tex;
        tex = tex->next;
		free(old);
	}
	if (UsedFlats) {
        free(UsedFlats);
	}
	if (UsedTextures) {
        free(UsedTextures);
	}
	if (UsedPatches) {
        free(UsedPatches);
	}
	if (!TextureCount && !FlatCount && !PatchCount) {
        return false;
	}

	return true;
}
