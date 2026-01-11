/*
 * PS2 Live Graph Studio - Asset System
 * assets.h - Public API for asset loading and caching
 *
 * Single-threaded, fixed-size cache with bump allocator.
 * Development backend loads from host:assets/<path>
 *
 * C99 only, ps2sdk APIs only, no per-frame I/O or malloc.
 */

#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>

/* ============================================================
 * Configuration Constants
 * ============================================================ */

/* Maximum number of cached assets */
#define ASSET_CACHE_MAX     64

/* Total heap bytes for asset data (1 MB - conservative for PS2) */
#define ASSET_HEAP_BYTES    (1 * 1024 * 1024)

/* Maximum single asset size guard (1 MB) */
#define ASSET_MAX_SIZE      (1 * 1024 * 1024)

/* Maximum path length including prefix */
#define ASSET_PATH_MAX      256

/* Maximum relative path length (name argument) */
#define ASSET_NAME_MAX      192

/* Read chunk size for chunked reading fallback */
#define ASSET_READ_CHUNK    (64 * 1024)

/* ============================================================
 * Error Codes
 * ============================================================ */

typedef enum {
    ASSETS_OK               =  0,   /* Success */
    ASSETS_ERR_NOT_INIT     = -1,   /* System not initialized */
    ASSETS_ERR_NULL_PTR     = -2,   /* NULL argument passed */
    ASSETS_ERR_BAD_PATH     = -3,   /* Invalid or empty path */
    ASSETS_ERR_PATH_TRAVERSAL = -4, /* Path contains ".." */
    ASSETS_ERR_NOT_FOUND    = -5,   /* File not found or open failed */
    ASSETS_ERR_READ_FAIL    = -6,   /* Read error */
    ASSETS_ERR_TOO_LARGE    = -7,   /* Asset exceeds size limit */
    ASSETS_ERR_CACHE_FULL   = -8,   /* No free cache slots */
    ASSETS_ERR_HEAP_FULL    = -9,   /* Out of heap space */
    ASSETS_ERR_ALREADY_INIT = -10   /* Already initialized */
} assets_error_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Initialize the asset system.
 * Allocates the internal heap and clears the cache.
 * Must be called once before any assets_get calls.
 *
 * @return ASSETS_OK on success, or negative error code.
 */
int assets_init(void);

/**
 * Retrieve an asset by relative path.
 *
 * On first call for a given path, loads from storage and caches.
 * Subsequent calls return the cached pointer without I/O.
 *
 * Examples:
 *   assets_get("fonts/font_8x16.png", &data, &size)
 *     -> loads from "host:assets/fonts/font_8x16.png"
 *   assets_get("graphs/default.gph", &data, &size)
 *     -> loads from "host:assets/graphs/default.gph"
 *
 * @param name  Relative path (e.g., "fonts/font_8x16.png")
 * @param data  Output: pointer to asset data (read-only)
 * @param size  Output: size of asset in bytes
 *
 * @return ASSETS_OK on success, or negative error code.
 */
int assets_get(const char* name, const void** data, size_t* size);

/**
 * Shutdown the asset system and release all resources.
 * Frees the heap and clears the cache.
 * After this call, assets_init must be called again before use.
 */
void assets_shutdown(void);

/**
 * Get human-readable error string for an assets error code.
 *
 * @param err  Error code from assets_* functions
 * @return     Static string describing the error
 */
const char* assets_strerror(int err);

/**
 * Get the number of currently cached assets.
 *
 * @return Number of cached assets, or 0 if not initialized.
 */
int assets_get_cached_count(void);

/**
 * Get current heap usage in bytes.
 *
 * @return Bytes used in the asset heap, or 0 if not initialized.
 */
size_t assets_get_heap_used(void);

/* ============================================================
 * Future Extension Hooks (disabled by default)
 * ============================================================ */

#ifndef ASSETS_USE_PACK
#define ASSETS_USE_PACK 0
#endif

#if ASSETS_USE_PACK
/**
 * Initialize asset system with a packed archive file.
 * When enabled, assets_get reads from the archive instead of host:.
 *
 * @param pack_path  Path to the packed archive file
 * @return ASSETS_OK on success, or negative error code.
 */
int assets_init_pack(const char* pack_path);
#endif

#endif /* ASSETS_H */
