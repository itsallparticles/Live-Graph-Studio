#include "graph_io.h"
#include "../graph/graph_core.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Result Strings
 * ============================================================ */
static const char * const s_io_result_strings[] = {
    "OK",
    "Error: NULL pointer",
    "Error: Failed to open file",
    "Error: Failed to read file",
    "Error: Failed to write file",
    "Error: Invalid magic number",
    "Error: Unsupported version",
    "Error: Checksum mismatch",
    "Error: File truncated",
    "Error: Buffer too small"
};

/* ============================================================
 * Checksum Helper
 * ============================================================ */
static uint32_t compute_checksum(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    size_t i;

    for (i = 0; i < size; i++) {
        sum = (sum << 1) | (sum >> 31);  /* Rotate left */
        sum ^= bytes[i];
        sum += bytes[i];
    }

    return sum;
}

/* ============================================================
 * Sanitization
 * ============================================================ */
int graph_io_sanitize(Graph *g)
{
    int sanitized = 0;
    NodeId i;
    uint8_t p;

    if (!g) {
        return 0;
    }

    for (i = 0; i < MAX_NODES; i++) {
        Node *node = &g->nodes[i];

        if (node->type == NODE_TYPE_NONE) {
            continue;
        }

        /* Validate node type */
        if (node->type >= NODE_TYPE_COUNT) {
            node->type = NODE_TYPE_NONE;
            sanitized++;
            continue;
        }

        /* Validate connections */
        for (p = 0; p < MAX_IN_PORTS; p++) {
            Connection *conn = &node->inputs[p];

            if (conn->src_node == INVALID_NODE_ID) {
                continue;
            }

            /* Check source node bounds */
            if (conn->src_node >= MAX_NODES) {
                conn->src_node = INVALID_NODE_ID;
                conn->src_port = 0;
                sanitized++;
                continue;
            }

            /* Check source node exists */
            if (g->nodes[conn->src_node].type == NODE_TYPE_NONE) {
                conn->src_node = INVALID_NODE_ID;
                conn->src_port = 0;
                sanitized++;
                continue;
            }

            /* Check port bounds */
            if (conn->src_port >= MAX_OUT_PORTS) {
                conn->src_node = INVALID_NODE_ID;
                conn->src_port = 0;
                sanitized++;
            }
        }
    }

    /* Recount nodes */
    g->node_count = 0;
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type != NODE_TYPE_NONE) {
            g->node_count++;
        }
    }

    return sanitized;
}

/* ============================================================
 * Serialization Size
 * ============================================================ */
size_t graph_io_get_serialized_size(const Graph *g, int include_ui_meta)
{
    size_t size;

    (void)g;  /* Graph doesn't affect size in current format */

    size = sizeof(GraphFileHeader);
    size += sizeof(Node) * MAX_NODES;

    if (include_ui_meta) {
        size += sizeof(UiMeta) * MAX_NODES;
    }

    return size;
}

/* ============================================================
 * Serialize to Buffer
 * ============================================================ */
GraphIoResult graph_io_serialize(void *buffer, size_t buffer_size, size_t *out_size,
                                  const Graph *g, const UiMetaBank *ui_meta)
{
    uint8_t *ptr;
    size_t required_size;
    GraphFileHeader header;

    if (!buffer || !g) {
        return GRAPH_IO_ERR_NULL_PTR;
    }

    required_size = graph_io_get_serialized_size(g, ui_meta != NULL);

    if (buffer_size < required_size) {
        return GRAPH_IO_ERR_BUFFER_TOO_SMALL;
    }

    ptr = (uint8_t *)buffer;

    /* Build header */
    header.magic = GRAPH_IO_MAGIC;
    header.version = GRAPH_IO_VERSION;
    header.node_count = g->node_count;
    header.graph_version = g->version;
    header.flags = (ui_meta != NULL) ? 1 : 0;  /* Flag bit 0: has UI meta */
    header.checksum = 0;  /* Computed after data */

    /* Skip header for now */
    ptr += sizeof(GraphFileHeader);

    /* Write nodes */
    memcpy(ptr, g->nodes, sizeof(Node) * MAX_NODES);
    ptr += sizeof(Node) * MAX_NODES;

    /* Write UI metadata if provided */
    if (ui_meta) {
        memcpy(ptr, ui_meta->meta, sizeof(UiMeta) * MAX_NODES);
        ptr += sizeof(UiMeta) * MAX_NODES;  /* ptr not used after this */
    }

    /* Compute checksum over data (excluding header) */
    header.checksum = compute_checksum((uint8_t *)buffer + sizeof(GraphFileHeader),
                                        required_size - sizeof(GraphFileHeader));

    /* Write header at start */
    memcpy(buffer, &header, sizeof(GraphFileHeader));

    if (out_size) {
        *out_size = required_size;
    }

    return GRAPH_IO_OK;
}

/* ============================================================
 * Deserialize from Buffer
 * ============================================================ */
GraphIoResult graph_io_deserialize(const void *buffer, size_t buffer_size,
                                    Graph *g, UiMetaBank *ui_meta)
{
    const uint8_t *ptr;
    GraphFileHeader header;
    size_t expected_size;
    uint32_t computed_checksum;
    int has_ui_meta;

    if (!buffer || !g) {
        return GRAPH_IO_ERR_NULL_PTR;
    }

    if (buffer_size < sizeof(GraphFileHeader)) {
        return GRAPH_IO_ERR_TRUNCATED;
    }

    ptr = (const uint8_t *)buffer;

    /* Read header */
    memcpy(&header, ptr, sizeof(GraphFileHeader));
    ptr += sizeof(GraphFileHeader);

    /* Validate magic */
    if (header.magic != GRAPH_IO_MAGIC) {
        return GRAPH_IO_ERR_BAD_MAGIC;
    }

    /* Validate version */
    if (header.version > GRAPH_IO_VERSION) {
        return GRAPH_IO_ERR_BAD_VERSION;
    }

    has_ui_meta = (header.flags & 1) != 0;
    expected_size = sizeof(GraphFileHeader) + sizeof(Node) * MAX_NODES;
    if (has_ui_meta) {
        expected_size += sizeof(UiMeta) * MAX_NODES;
    }

    if (buffer_size < expected_size) {
        return GRAPH_IO_ERR_TRUNCATED;
    }

    /* Validate checksum */
    computed_checksum = compute_checksum((const uint8_t *)buffer + sizeof(GraphFileHeader),
                                          expected_size - sizeof(GraphFileHeader));
    if (computed_checksum != header.checksum) {
        return GRAPH_IO_ERR_BAD_CHECKSUM;
    }

    /* Initialize graph */
    graph_init(g);

    /* Read nodes */
    memcpy(g->nodes, ptr, sizeof(Node) * MAX_NODES);
    ptr += sizeof(Node) * MAX_NODES;

    g->node_count = header.node_count;
    g->version = header.graph_version;

    /* Read UI metadata if present and requested */
    if (has_ui_meta && ui_meta) {
        memcpy(ui_meta->meta, ptr, sizeof(UiMeta) * MAX_NODES);
    } else if (ui_meta) {
        /* No UI meta in file, zero it out */
        memset(ui_meta->meta, 0, sizeof(UiMeta) * MAX_NODES);
    }

    /* Sanitize loaded data (also recounts nodes) */
    graph_io_sanitize(g);

    return GRAPH_IO_OK;
}

/* ============================================================
 * File I/O
 * ============================================================ */

/* Static buffer for file I/O to avoid malloc.
 * WARNING: Not reentrant - do not call save/load concurrently. */
static uint8_t s_io_buffer[sizeof(GraphFileHeader) + sizeof(Node) * MAX_NODES + sizeof(UiMeta) * MAX_NODES];

GraphIoResult graph_io_save(const char *path, const Graph *g, const UiMetaBank *ui_meta)
{
    FILE *f;
    size_t serialized_size;
    GraphIoResult result;
    size_t written;

    if (!path || !g) {
        return GRAPH_IO_ERR_NULL_PTR;
    }

    /* Serialize to buffer */
    result = graph_io_serialize(s_io_buffer, sizeof(s_io_buffer), &serialized_size, g, ui_meta);
    if (result != GRAPH_IO_OK) {
        return result;
    }

    /* Open file for writing */
    f = fopen(path, "wb");
    if (!f) {
        return GRAPH_IO_ERR_OPEN_FAIL;
    }

    /* Write buffer to file */
    written = fwrite(s_io_buffer, 1, serialized_size, f);
    fclose(f);

    if (written != serialized_size) {
        return GRAPH_IO_ERR_WRITE_FAIL;
    }

    return GRAPH_IO_OK;
}

GraphIoResult graph_io_load(const char *path, Graph *g, UiMetaBank *ui_meta)
{
    FILE *f;
    size_t read_size;

    if (!path || !g) {
        return GRAPH_IO_ERR_NULL_PTR;
    }

    /* Open file for reading */
    f = fopen(path, "rb");
    if (!f) {
        return GRAPH_IO_ERR_OPEN_FAIL;
    }

    /* Read entire file into buffer */
    read_size = fread(s_io_buffer, 1, sizeof(s_io_buffer), f);
    fclose(f);

    if (read_size < sizeof(GraphFileHeader)) {
        return GRAPH_IO_ERR_TRUNCATED;
    }

    /* Deserialize from buffer */
    return graph_io_deserialize(s_io_buffer, read_size, g, ui_meta);
}

/* ============================================================
 * Result String
 * ============================================================ */
const char *graph_io_result_str(GraphIoResult result)
{
    if (result > GRAPH_IO_ERR_BUFFER_TOO_SMALL) {
        return "Unknown error";
    }
    return s_io_result_strings[result];
}
