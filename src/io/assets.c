/*
 * PS2 Live Graph Studio - Asset System
 * assets.c - Implementation of asset loading and caching
 *
 * Development backend: loads from host:assets/<path>
 * Uses POSIX file I/O (open/read/close/lseek) via ps2sdk newlib.
 * Fixed-size cache with bump allocator for asset data.
 *
 * Define ASSETS_USE_EMBEDDED=1 to use embedded assets (no file I/O).
 *
 * C99 only, ps2sdk APIs only, no per-frame I/O or malloc.
 */

#include "assets.h"
#include <string.h>
#include <stdlib.h>

/* Use embedded assets by default for standalone ELF */
#ifndef ASSETS_USE_EMBEDDED
#define ASSETS_USE_EMBEDDED 1
#endif

#if ASSETS_USE_EMBEDDED
#include "assets_embedded.h"
#else
#include <unistd.h>
#include <fcntl.h>
#endif

/* ============================================================
 * Error Strings
 * ============================================================ */
static const char* const s_error_strings[] = {
    "Success",                          /* ASSETS_OK = 0 */
    "System not initialized",           /* ASSETS_ERR_NOT_INIT = -1 */
    "NULL pointer argument",            /* ASSETS_ERR_NULL_PTR = -2 */
    "Invalid or empty path",            /* ASSETS_ERR_BAD_PATH = -3 */
    "Path traversal rejected",          /* ASSETS_ERR_PATH_TRAVERSAL = -4 */
    "File not found",                   /* ASSETS_ERR_NOT_FOUND = -5 */
    "Read error",                       /* ASSETS_ERR_READ_FAIL = -6 */
    "Asset too large",                  /* ASSETS_ERR_TOO_LARGE = -7 */
    "Cache full",                       /* ASSETS_ERR_CACHE_FULL = -8 */
    "Heap full",                        /* ASSETS_ERR_HEAP_FULL = -9 */
    "Already initialized"               /* ASSETS_ERR_ALREADY_INIT = -10 */
};

#define ERROR_STRING_COUNT 11

/* ============================================================
 * Cache Entry Structure
 * ============================================================ */
typedef struct {
    char    name[ASSET_NAME_MAX];   /* Relative path used as key */
    void*   data;                   /* Pointer into heap */
    size_t  size;                   /* Size in bytes */
    int     valid;                  /* 1 if entry is valid */
} asset_cache_entry_t;

/* ============================================================
 * Static State
 * ============================================================ */

/* Cache table */
static asset_cache_entry_t s_cache[ASSET_CACHE_MAX];
static int s_cache_count = 0;

/* Bump allocator heap */
static unsigned char* s_heap = NULL;
static size_t s_heap_used = 0;

/* Initialization flag */
static int s_initialized = 0;

/* ============================================================
 * Internal: Path Validation
 * ============================================================ */

/**
 * Check if path contains ".." (directory traversal)
 * @return 1 if path is safe, 0 if it contains traversal
 */
static int path_is_safe(const char* path)
{
    const char* p;

    if (!path || path[0] == '\0') {
        return 0;
    }

    /* Check for ".." anywhere in path */
    p = path;
    while (*p) {
        if (p[0] == '.' && p[1] == '.') {
            /* Found ".." - reject */
            return 0;
        }
        p++;
    }

    /* Reject absolute paths or paths starting with / */
    if (path[0] == '/' || path[0] == '\\') {
        return 0;
    }

    /* Reject paths with : (device prefix) */
    p = path;
    while (*p) {
        if (*p == ':') {
            return 0;
        }
        p++;
    }

    return 1;
}

#if !ASSETS_USE_EMBEDDED
/**
 * Build full path: "host:assets/<name>"
 * Ensures no double slashes.
 * @return 0 on success, -1 if buffer too small
 */
static int build_full_path(const char* name, char* out, size_t out_size)
{
    static const char prefix[] = "host:assets/";
    size_t prefix_len;
    size_t name_len;
    const char* name_start;

    prefix_len = sizeof(prefix) - 1; /* strlen without null */
    
    /* Skip leading slash in name if present */
    name_start = name;
    if (name_start[0] == '/') {
        name_start++;
    }

    name_len = strlen(name_start);

    if (prefix_len + name_len + 1 > out_size) {
        return -1;
    }

    memcpy(out, prefix, prefix_len);
    memcpy(out + prefix_len, name_start, name_len);
    out[prefix_len + name_len] = '\0';

    return 0;
}

#endif /* !ASSETS_USE_EMBEDDED */

/* ============================================================
 * Internal: Cache Lookup
 * ============================================================ */

/**
 * Find cache entry by name
 * @return index if found, -1 if not found
 */
static int cache_find(const char* name)
{
    int i;

    for (i = 0; i < s_cache_count; i++) {
        if (s_cache[i].valid && strcmp(s_cache[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * Allocate from bump heap
 * @return pointer to allocated memory, or NULL if out of space
 */
static void* heap_alloc(size_t size)
{
    void* ptr;

    if (s_heap_used + size > ASSET_HEAP_BYTES) {
        return NULL;
    }

    ptr = s_heap + s_heap_used;
    s_heap_used += size;

    /* Align to 16 bytes for DMA compatibility */
    s_heap_used = (s_heap_used + 15) & ~((size_t)15);

    return ptr;
}

/* ============================================================
 * Internal: File I/O (ps2sdk)
 * ============================================================ */

#if ASSETS_USE_EMBEDDED

/**
 * Load asset from embedded data.
 * Copies the embedded data into our heap for consistent ownership.
 *
 * @param name      Asset name (e.g., "ui/cursor.png")
 * @param out_data  Output: pointer to loaded data (copied to heap)
 * @param out_size  Output: size of loaded data
 * @return ASSETS_OK or error code
 */
static int load_file_embedded(const char* name, void** out_data, size_t* out_size)
{
    const EmbeddedAsset* asset;
    void* buffer;

    /* Look up in embedded table */
    asset = embedded_asset_find(name);
    if (!asset) {
        return ASSETS_ERR_NOT_FOUND;
    }

    /* Check size */
    if (asset->size > ASSET_MAX_SIZE) {
        return ASSETS_ERR_TOO_LARGE;
    }

    /* Allocate from heap */
    buffer = heap_alloc(asset->size > 0 ? asset->size : 16);
    if (!buffer) {
        return ASSETS_ERR_HEAP_FULL;
    }

    /* Copy data to heap */
    if (asset->size > 0) {
        memcpy(buffer, asset->data, asset->size);
    }

    *out_data = buffer;
    *out_size = asset->size;
    return ASSETS_OK;
}

#else /* !ASSETS_USE_EMBEDDED */

/**
 * Load file from host: filesystem using ps2sdk fio calls.
 * Uses seek to determine file size, then reads entire file.
 * Falls back to chunked reading if seek fails.
 *
 * @param path      Full path (e.g., "host:assets/fonts/font.png")
 * @param out_data  Output: pointer to loaded data (allocated from heap)
 * @param out_size  Output: size of loaded data
 * @return ASSETS_OK or error code
 */
static int load_file_host(const char* path, void** out_data, size_t* out_size)
{
    int fd;
    off_t file_size;
    ssize_t bytes_read;
    size_t total_read;
    void* buffer;
    unsigned char* read_ptr;

    /* Open file */
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return ASSETS_ERR_NOT_FOUND;
    }

    /* Try to get file size via seek */
    file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        /* Seek failed - use chunked reading with size guard */
        /* Reset to beginning first */
        lseek(fd, 0, SEEK_SET);

        /* Allocate maximum allowed size temporarily */
        buffer = heap_alloc(ASSET_MAX_SIZE);
        if (!buffer) {
            close(fd);
            return ASSETS_ERR_HEAP_FULL;
        }

        /* Read in chunks until EOF */
        total_read = 0;
        read_ptr = (unsigned char*)buffer;

        while (total_read < ASSET_MAX_SIZE) {
            size_t chunk_size = ASSET_READ_CHUNK;
            if (total_read + chunk_size > ASSET_MAX_SIZE) {
                chunk_size = ASSET_MAX_SIZE - total_read;
            }

            bytes_read = read(fd, read_ptr, chunk_size);
            if (bytes_read < 0) {
                close(fd);
                /* Note: heap space is "leaked" but this is a fatal error anyway */
                return ASSETS_ERR_READ_FAIL;
            }
            if (bytes_read == 0) {
                /* EOF reached */
                break;
            }

            total_read += (size_t)bytes_read;
            read_ptr += bytes_read;
        }

        close(fd);

        if (total_read >= ASSET_MAX_SIZE) {
            return ASSETS_ERR_TOO_LARGE;
        }

        *out_data = buffer;
        *out_size = total_read;
        return ASSETS_OK;
    }

    /* Got file size via seek */
    if (file_size == 0) {
        close(fd);
        /* Empty file - allocate minimal buffer */
        buffer = heap_alloc(16);
        if (!buffer) {
            return ASSETS_ERR_HEAP_FULL;
        }
        *out_data = buffer;
        *out_size = 0;
        return ASSETS_OK;
    }

    if ((size_t)file_size > ASSET_MAX_SIZE) {
        close(fd);
        return ASSETS_ERR_TOO_LARGE;
    }

    /* Seek back to beginning */
    if (lseek(fd, 0, SEEK_SET) < 0) {
        close(fd);
        return ASSETS_ERR_READ_FAIL;
    }

    /* Allocate from heap */
    buffer = heap_alloc((size_t)file_size);
    if (!buffer) {
        close(fd);
        return ASSETS_ERR_HEAP_FULL;
    }

    /* Read entire file */
    total_read = 0;
    read_ptr = (unsigned char*)buffer;

    while (total_read < (size_t)file_size) {
        bytes_read = read(fd, read_ptr, (size_t)file_size - total_read);
        if (bytes_read < 0) {
            close(fd);
            return ASSETS_ERR_READ_FAIL;
        }
        if (bytes_read == 0) {
            /* Unexpected EOF */
            close(fd);
            return ASSETS_ERR_READ_FAIL;
        }
        total_read += (size_t)bytes_read;
        read_ptr += bytes_read;
    }

    close(fd);

    *out_data = buffer;
    *out_size = (size_t)file_size;
    return ASSETS_OK;
}

#endif /* !ASSETS_USE_EMBEDDED */

/* ============================================================
 * Future: Packed Archive Backend
 * ============================================================ */

#if ASSETS_USE_PACK && !ASSETS_USE_EMBEDDED

/* Placeholder for packed archive support */
/* When enabled, this would read from a single archive file
 * instead of individual host: files */

static int s_pack_fd = -1;

int assets_init_pack(const char* pack_path)
{
    /* TODO: Implement packed archive loading */
    /* 1. Open archive file */
    /* 2. Read and validate header */
    /* 3. Build index table */
    (void)pack_path;
    return ASSETS_ERR_NOT_FOUND;
}

static int load_file_pack(const char* name, void** out_data, size_t* out_size)
{
    /* TODO: Implement packed asset extraction */
    /* 1. Look up name in index table */
    /* 2. Seek to offset in archive */
    /* 3. Read asset data */
    (void)name;
    (void)out_data;
    (void)out_size;
    return ASSETS_ERR_NOT_FOUND;
}

#endif /* ASSETS_USE_PACK */

/* ============================================================
 * Public API Implementation
 * ============================================================ */

int assets_init(void)
{
    int i;

    if (s_initialized) {
        return ASSETS_ERR_ALREADY_INIT;
    }

    /* Allocate heap */
    s_heap = (unsigned char*)malloc(ASSET_HEAP_BYTES);
    if (!s_heap) {
        return ASSETS_ERR_HEAP_FULL;
    }

    /* Clear heap tracking */
    s_heap_used = 0;

    /* Clear cache */
    for (i = 0; i < ASSET_CACHE_MAX; i++) {
        s_cache[i].name[0] = '\0';
        s_cache[i].data = NULL;
        s_cache[i].size = 0;
        s_cache[i].valid = 0;
    }
    s_cache_count = 0;

    s_initialized = 1;

    return ASSETS_OK;
}

void assets_shutdown(void)
{
    int i;

    if (!s_initialized) {
        return;
    }

    /* Clear cache entries */
    for (i = 0; i < ASSET_CACHE_MAX; i++) {
        s_cache[i].name[0] = '\0';
        s_cache[i].data = NULL;
        s_cache[i].size = 0;
        s_cache[i].valid = 0;
    }
    s_cache_count = 0;

    /* Free heap */
    if (s_heap) {
        free(s_heap);
        s_heap = NULL;
    }
    s_heap_used = 0;

    s_initialized = 0;

#if ASSETS_USE_PACK
    if (s_pack_fd >= 0) {
        close(s_pack_fd);
        s_pack_fd = -1;
    }
#endif
}

int assets_get(const char* name, const void** data, size_t* size)
{
    int index;
    int result;
#if !ASSETS_USE_EMBEDDED && !ASSETS_USE_PACK
    char full_path[ASSET_PATH_MAX];
#endif
    void* loaded_data;
    size_t loaded_size;

    /* Validate state */
    if (!s_initialized) {
        return ASSETS_ERR_NOT_INIT;
    }

    /* Validate inputs */
    if (!name || !data || !size) {
        return ASSETS_ERR_NULL_PTR;
    }

    if (name[0] == '\0') {
        return ASSETS_ERR_BAD_PATH;
    }

    /* Check path length */
    if (strlen(name) >= ASSET_NAME_MAX) {
        return ASSETS_ERR_BAD_PATH;
    }

    /* Validate path safety */
    if (!path_is_safe(name)) {
        return ASSETS_ERR_PATH_TRAVERSAL;
    }

    /* Check cache first */
    index = cache_find(name);
    if (index >= 0) {
        /* Cache hit - return cached data */
        *data = s_cache[index].data;
        *size = s_cache[index].size;
        return ASSETS_OK;
    }

    /* Cache miss - need to load */

    /* Check for free cache slot */
    if (s_cache_count >= ASSET_CACHE_MAX) {
        return ASSETS_ERR_CACHE_FULL;
    }

    /* Load file */
#if ASSETS_USE_EMBEDDED
    /* Load from embedded data */
    result = load_file_embedded(name, &loaded_data, &loaded_size);
#elif ASSETS_USE_PACK
    result = load_file_pack(name, &loaded_data, &loaded_size);
#else
    /* Build full path for host: filesystem */
    if (build_full_path(name, full_path, sizeof(full_path)) < 0) {
        return ASSETS_ERR_BAD_PATH;
    }
    result = load_file_host(full_path, &loaded_data, &loaded_size);
#endif

    if (result != ASSETS_OK) {
        return result;
    }

    /* Add to cache */
    index = s_cache_count;
    strncpy(s_cache[index].name, name, ASSET_NAME_MAX - 1);
    s_cache[index].name[ASSET_NAME_MAX - 1] = '\0';
    s_cache[index].data = loaded_data;
    s_cache[index].size = loaded_size;
    s_cache[index].valid = 1;
    s_cache_count++;

    /* Return loaded data */
    *data = loaded_data;
    *size = loaded_size;

    return ASSETS_OK;
}

const char* assets_strerror(int err)
{
    int index;

    if (err > 0) {
        return "Unknown error";
    }

    index = -err;
    if (index >= ERROR_STRING_COUNT) {
        return "Unknown error";
    }

    return s_error_strings[index];
}

int assets_get_cached_count(void)
{
    if (!s_initialized) {
        return 0;
    }
    return s_cache_count;
}

size_t assets_get_heap_used(void)
{
    if (!s_initialized) {
        return 0;
    }
    return s_heap_used;
}
