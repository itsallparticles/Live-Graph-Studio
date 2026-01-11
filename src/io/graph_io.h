#ifndef GRAPH_IO_H
#define GRAPH_IO_H

#include "../common.h"
#include "../graph/graph_types.h"

/* ============================================================
 * Graph I/O Module
 * ============================================================
 * Binary serialization for graphs with validation.
 * Format: Header + Node data + UI metadata
 * Sanitizes bad connections on load.
 * ============================================================ */

/* ============================================================
 * File Format Constants
 * ============================================================ */
#define GRAPH_IO_MAGIC       0x4C475348  /* "LGSH" - Live Graph Studio Header */
#define GRAPH_IO_VERSION     1

/* ============================================================
 * File Header Structure
 * ============================================================ */
typedef struct {
    uint32_t magic;           /* GRAPH_IO_MAGIC */
    uint16_t version;         /* File format version */
    uint16_t node_count;      /* Number of active nodes */
    uint16_t graph_version;   /* Graph version number */
    uint16_t flags;           /* Reserved flags */
    uint32_t checksum;        /* Simple checksum for validation */
} GraphFileHeader;

/* ============================================================
 * I/O Result Codes
 * ============================================================ */
typedef enum {
    GRAPH_IO_OK = 0,
    GRAPH_IO_ERR_NULL_PTR,
    GRAPH_IO_ERR_OPEN_FAIL,
    GRAPH_IO_ERR_READ_FAIL,
    GRAPH_IO_ERR_WRITE_FAIL,
    GRAPH_IO_ERR_BAD_MAGIC,
    GRAPH_IO_ERR_BAD_VERSION,
    GRAPH_IO_ERR_BAD_CHECKSUM,
    GRAPH_IO_ERR_TRUNCATED,
    GRAPH_IO_ERR_BUFFER_TOO_SMALL
} GraphIoResult;

/* ============================================================
 * Graph I/O API
 * ============================================================ */

/* Save graph to file.
 * path: file path (host: prefix for ps2client)
 * g: graph to save
 * ui_meta: optional UI metadata (may be NULL)
 * Returns: GRAPH_IO_OK on success */
GraphIoResult graph_io_save(const char *path, const Graph *g, const UiMetaBank *ui_meta);

/* Load graph from file.
 * path: file path (host: prefix for ps2client)
 * g: destination graph (will be initialized)
 * ui_meta: optional UI metadata destination (may be NULL)
 * Returns: GRAPH_IO_OK on success */
GraphIoResult graph_io_load(const char *path, Graph *g, UiMetaBank *ui_meta);

/* Serialize graph to memory buffer.
 * buffer: destination buffer
 * buffer_size: size of buffer in bytes
 * out_size: actual bytes written (output)
 * g: graph to serialize
 * ui_meta: optional UI metadata (may be NULL)
 * Returns: GRAPH_IO_OK on success */
GraphIoResult graph_io_serialize(void *buffer, size_t buffer_size, size_t *out_size,
                                  const Graph *g, const UiMetaBank *ui_meta);

/* Deserialize graph from memory buffer.
 * buffer: source buffer
 * buffer_size: size of buffer in bytes
 * g: destination graph
 * ui_meta: optional UI metadata destination (may be NULL)
 * Returns: GRAPH_IO_OK on success */
GraphIoResult graph_io_deserialize(const void *buffer, size_t buffer_size,
                                    Graph *g, UiMetaBank *ui_meta);

/* Get required buffer size for serialization */
size_t graph_io_get_serialized_size(const Graph *g, int include_ui_meta);

/* Sanitize graph connections (remove invalid references).
 * Called automatically on load, but available for manual use.
 * Returns: number of connections sanitized */
int graph_io_sanitize(Graph *g);

/* Get human-readable string for I/O result */
const char *graph_io_result_str(GraphIoResult result);

#endif /* GRAPH_IO_H */
