//-------------------------------------------------------------------------
/*
 Copyright (C) 2013 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFDuke3D

 Duke Nukem 3D is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
//-------------------------------------------------------------------------

#include "compat.h"
#include "baselayer.h"

#include "scriptfile.h"
#include "cache1d.h"
#include "crc32.h"

#include "build.h"
#include "grpscan.h"

struct grpfile grpfiles[] = {
    { "Registered Version 1.3d",    0xBBC9CE44, 26524524, GAMEDUKE, "duke3d13.grp",       NULL, NULL },
    { "Registered Version 1.4",     0xF514A6AC, 44348015, GAMEDUKE, "duke3d14.grp",       NULL, NULL },
    { "Registered Version 1.5",     0xFD3DCFF1, 44356548, GAMEDUKE, "duke3d.grp",         NULL, NULL },
    { "20th Anniversary",           0x982AFE4A, 44356548, GAMEDUKE, "duke3d20th.grp",     NULL, NULL },
    { "Shareware Version",          0x983AD923, 11035779, GAMEDUKE, "duke3dshare.grp",    NULL, NULL },
    { "Mac Shareware Version",      0xC5F71561, 10444391, GAMEDUKE, "duke3dmacshare.grp", NULL, NULL },
    // { "Mac Registered Version",     0x00000000, 0,        GAMEDUKE, "duke3dmac.grp",      NULL, NULL },
    { "NAM",                        0x75C1F07B, 43448927, GAMENAM,  "nam.grp",            NULL, NULL },
    { NULL, 0, 0, 0, NULL, NULL, NULL },
};
struct grpfile *foundgrps = NULL;

#define GRPCACHEFILE "grpfiles.cache"
static struct grpcache {
    struct grpcache *next;
    char name[BMAX_PATH+1];
    int crcval;
    int size;
    int mtime;
} *grpcache = NULL, *usedgrpcache = NULL;

static int LoadGroupsCache(void)
{
    struct grpcache *fg;

    int fsize, fmtime, fcrcval;
    char *fname;

    scriptfile *script;

    script = scriptfile_fromfile(GRPCACHEFILE);
    if (!script) return -1;

    while (!scriptfile_eof(script)) {
        if (scriptfile_getstring(script, &fname)) break;    // filename
        if (scriptfile_getnumber(script, &fsize)) break;    // filesize
        if (scriptfile_getnumber(script, &fmtime)) break;   // modification time
        if (scriptfile_getnumber(script, &fcrcval)) break;  // crc checksum

        fg = calloc(1, sizeof(struct grpcache));
        fg->next = grpcache;
        grpcache = fg;

        strncpy(fg->name, fname, BMAX_PATH);
        fg->crcval = fcrcval;
        fg->size = fsize;
        fg->mtime = fmtime;
    }

    scriptfile_close(script);
    return 0;
}

static void FreeGroupsCache(void)
{
    struct grpcache *fg;

    while (grpcache) {
        fg = grpcache->next;
        free(grpcache);
        grpcache = fg;
    }
}

// Compute the CRC-32 checksum for the contents of an open file.
static unsigned int ChecksumFile(int fh, struct importgroupsmeta *cbs)
{
    int b;
    unsigned int crc;
    unsigned char buf[16384];

    crc32init(&crc);
    lseek(fh, 0, SEEK_SET);
    do {
        if (cbs && cbs->cancelled(cbs->data)) return 0;
        b = read(fh, buf, sizeof(buf));
        if (b > 0) crc32block(&crc, buf, b);
    } while (b == sizeof(buf));
    crc32finish(&crc);

    return crc;
}

// Scan the cache1d path stack for identifiable GRP files, checking the
// cache to avoid checksumming previously identified files, and identify
// any new ones found.
int ScanGroups(void)
{
    CACHE1D_FIND_REC *srch, *sidx;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;
    char *fn;
    struct Bstat st;
    int i;

    buildprintf("Scanning for GRP files...\n");

    LoadGroupsCache();

    srch = klistpath("/", "*.grp", CACHE1D_FIND_FILE);

    for (sidx = srch; sidx; sidx = sidx->next) {
        for (fg = grpcache; fg; fg = fg->next) {
            if (!Bstrcmp(fg->name, sidx->name)) break;
        }

        if (fg) {
            if (findfrompath(sidx->name, &fn)) continue;    // failed to resolve the filename
            if (Bstat(fn, &st)) { free(fn); continue; } // failed to stat the file
            free(fn);
            if (fg->size == st.st_size && fg->mtime == st.st_mtime) {
                grp = (struct grpfile *)calloc(1, sizeof(struct grpfile));
                grp->name = strdup(sidx->name);
                grp->crcval = fg->crcval;
                grp->size = fg->size;
                grp->ref = NULL;
                grp->next = foundgrps;
                foundgrps = grp;

                // Determine which grpfiles[] entry, if any, matches.
                for (i = 0; grpfiles[i].name; i++) {
                    if (grpfiles[i].crcval == grp->crcval && grpfiles[i].size == grp->size) {
                        grp->ref = &grpfiles[i];
                        grp->game = grp->ref->game;
                        break;
                    }
                }

                fgg = (struct grpcache *)calloc(1, sizeof(struct grpcache));
                strcpy(fgg->name, fg->name);
                fgg->crcval = fg->crcval;
                fgg->size = fg->size;
                fgg->mtime = fg->mtime;
                fgg->next = usedgrpcache;
                usedgrpcache = fgg;
                continue;
            }
        }

        {
            int b, fh;
            unsigned int crcval;

            fh = openfrompath(sidx->name, BO_RDONLY|BO_BINARY, BS_IREAD);
            if (fh < 0) continue;
            if (fstat(fh, &st)) continue;

            buildprintf(" Checksumming %s\n", sidx->name);
            crcval = ChecksumFile(fh, NULL);
            close(fh);

            grp = (struct grpfile *)calloc(1, sizeof(struct grpfile));
            grp->name = strdup(sidx->name);
            grp->crcval = crcval;
            grp->size = st.st_size;
            grp->next = foundgrps;
            foundgrps = grp;

            // Determine which grpfiles[] entry, if any, matches.
            for (i = 0; grpfiles[i].name; i++) {
                if (grpfiles[i].crcval == grp->crcval && grpfiles[i].size == grp->size) {
                    grp->ref = &grpfiles[i];
                    grp->game = grp->ref->game;
                    break;
                }
            }

            fgg = (struct grpcache *)calloc(1, sizeof(struct grpcache));
            strncpy(fgg->name, sidx->name, BMAX_PATH);
            fgg->crcval = crcval;
            fgg->size = st.st_size;
            fgg->mtime = st.st_mtime;
            fgg->next = usedgrpcache;
            usedgrpcache = fgg;
        }
    }

    klistfree(srch);
    FreeGroupsCache();

    if (usedgrpcache) {
        FILE *fp;
        fp = fopen(GRPCACHEFILE, "wt");
        if (fp) {
            for (fg = usedgrpcache; fg; fg=fgg) {
                fgg = fg->next;
                fprintf(fp, "\"%s\" %d %d %d\n", fg->name, fg->size, fg->mtime, fg->crcval);
                free(fg);
            }
            fclose(fp);
        }
    }

    return 0;
}

void FreeGroups(void)
{
    struct grpfile *fg;

    while (foundgrps) {
        fg = foundgrps->next;
        free((char*)foundgrps->name);
        free(foundgrps);
        foundgrps = fg;
    }
}

enum {
    COPYFILE_OK = 0,
    COPYFILE_ERR_EXISTS = -1,
    COPYFILE_ERR_OPEN = -2,
    COPYFILE_ERR_RW = -3,
    COPYFILE_ERR_CANCELLED = -4,
};

// Copy the contents of 'fh' to file 'fname', but only if 'fname' doesn't already exist.
static int CopyFile(int fh, int size, const char *fname, struct importgroupsmeta *cbs)
{
    int ofh, b, rv = COPYFILE_OK;
    char buf[16384];

    ofh = open(fname, O_WRONLY|O_BINARY|O_CREAT|O_EXCL, BS_IREAD|BS_IWRITE);
    if (ofh < 0) {
        if (errno == EEXIST) return COPYFILE_ERR_EXISTS;
        return COPYFILE_ERR_OPEN;
    }
    lseek(fh, 0, SEEK_SET);
    do {
        if (cbs->cancelled(cbs->data)) {
            rv = COPYFILE_ERR_CANCELLED;
        } else if ((b = read(fh, buf, sizeof(buf))) < 0) {
            rv = COPYFILE_ERR_RW;
        } else if (b > 0 && write(ofh, buf, b) != b) {
            rv = COPYFILE_ERR_RW;
        }
    } while (b == sizeof(buf) && rv == COPYFILE_OK);
    close(ofh);
    if (size != lseek(fh, 0, SEEK_CUR)) rv = COPYFILE_ERR_RW;  // File not the expected length.
    if (rv != COPYFILE_OK) remove(fname);
    return rv;
}

static int ImportGroupFromFile(const char *path, int size, struct importgroupsmeta *cbs)
{
    char buf[12];
    int i, fh, rv;
    unsigned int crcval;
    struct grpfile *grp;

    fh = open(path, O_RDONLY|O_BINARY, S_IREAD);
    if (fh < 0) return IMPORTGROUP_OK;  // Maybe no permission. Whatever.

    lseek(fh, 0, SEEK_SET);
    if (read(fh, buf, 12) != 12 || memcmp(buf, "KenSilverman", 12)) {
        close(fh);
        return IMPORTGROUP_OK;
    }

    crcval = ChecksumFile(fh, cbs);

    for (i = 0; grpfiles[i].name; i++) {
        if (grpfiles[i].crcval == crcval && grpfiles[i].size == size)
            break;
    }
    if (!grpfiles[i].name) {
        close(fh);
        return IMPORTGROUP_OK;
    }

    switch (CopyFile(fh, grpfiles[i].size, grpfiles[i].importname, cbs)) {
        case COPYFILE_ERR_CANCELLED:
            rv = IMPORTGROUP_OK;
            break;
        case COPYFILE_ERR_EXISTS:
            rv = IMPORTGROUP_SKIPPED;
            break;
        case COPYFILE_OK: // Copied. Add to the identified files list.
            grp = (struct grpfile *)calloc(1, sizeof(struct grpfile));
            grp->name = strdup(grpfiles[i].importname);
            grp->crcval = crcval;
            grp->size = grpfiles[i].size;
            grp->next = foundgrps;
            grp->ref = &grpfiles[i];
            grp->game = grpfiles[i].game;
            foundgrps = grp;
            rv = IMPORTGROUP_COPIED;
            break;
        default:
            rv = IMPORTGROUP_ERROR;
            break;
    }
    close(fh);
    return rv;
}

static int ImportGroupsFromDir(const char *path, struct importgroupsmeta *cbs)
{
    BDIR *dir;
    struct Bdirent *dirent;
    int subpathlen, found = 0, errors = 0;
    char *subpath;

    cbs->progress(cbs->data, path);

    dir = Bopendir(path);
    if (!dir) return IMPORTGROUP_OK;    // Maybe no permission.

    while ((dirent = Breaddir(dir))) {
        if (cbs->cancelled(cbs->data)) break;

        if (dirent->name[0] == '.' &&
            (dirent->name[1] == 0 || (dirent->name[1] == '.' && dirent->name[2] == 0)))
                continue;
        if (!(dirent->mode & (BS_IFDIR|BS_IFREG))) continue; // Ignore non-directories and non-regular files.
        if ((dirent->mode & BS_IFREG) && !Bwildmatch(dirent->name, "*.grp")) continue;  // Ignore non-.grp files.

        subpathlen = strlen(path) + 1 + dirent->namlen + 1;
        subpath = malloc(subpathlen);
        if (!subpath) { Bclosedir(dir); break; }
        snprintf(subpath, subpathlen, "%s/%s", path, dirent->name);

        if (dirent->mode & BS_IFDIR) {
            switch (ImportGroupsFromDir(subpath, cbs)) {
                case IMPORTGROUP_COPIED: found = 1; break;
                case IMPORTGROUP_ERROR: errors = 1; break;
            }
        }
        else {
            switch (ImportGroupFromFile(subpath, dirent->size, cbs)) {
                case IMPORTGROUP_SKIPPED: buildprintf("Skipped %s\n", subpath); break;
                case IMPORTGROUP_COPIED: buildprintf("Imported %s\n", subpath); found = 1; break;
                case IMPORTGROUP_ERROR: buildprintf("Error importing %s\n", subpath); errors = 1; break;
            }
        }
        free(subpath);
    }
    Bclosedir(dir);
    if (found) return IMPORTGROUP_COPIED;   // Finding anything is considered fine.
    else if (errors) return IMPORTGROUP_ERROR; // Finding nothing but errors reports back errors.
    return IMPORTGROUP_OK;
}

int ImportGroupsFromPath(const char *path, struct importgroupsmeta *cbs)
{
    struct stat st;
    int found = 0, errors = 0;

    if (stat(path, &st) < 0) return IMPORTGROUP_OK;

    if (st.st_mode & S_IFDIR) {
        switch (ImportGroupsFromDir(path, cbs)) {
            case IMPORTGROUP_COPIED: found = 1; break;
            case IMPORTGROUP_ERROR: errors = 1; break;
        }
    } else if (st.st_mode & S_IFREG) {
        switch (ImportGroupFromFile(path, st.st_size, cbs)) {
            case IMPORTGROUP_SKIPPED: buildprintf("Skipped %s\n", path); break;
            case IMPORTGROUP_COPIED: buildprintf("Imported %s\n", path); found = 1; break;
            case IMPORTGROUP_ERROR: buildprintf("Error importing %s\n", path); errors = 1; break;
        }
    }

    if (found) return IMPORTGROUP_COPIED;         // Finding anything is considered fine.
    else if (errors) return IMPORTGROUP_ERROR; // Finding nothing but errors reports back errors.
    return IMPORTGROUP_OK;
}
