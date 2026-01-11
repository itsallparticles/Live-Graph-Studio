#include "graph_core.h"
#include "../nodes/node_registry.h"
#include <string.h>

/* ============================================================
 * Graph Initialization
 * ============================================================ */
void graph_init(Graph *g)
{
    uint16_t i, j;

    if (g == NULL) {
        return;
    }

    memset(g, 0, sizeof(Graph));

    /* Initialize all nodes as unused */
    for (i = 0; i < MAX_NODES; i++) {
        g->nodes[i].type = NODE_TYPE_NONE;
        for (j = 0; j < MAX_IN_PORTS; j++) {
            g->nodes[i].inputs[j].src_node = INVALID_NODE_ID;
            g->nodes[i].inputs[j].src_port = 0;
        }
    }

    g->node_count = 0;
    g->version = 0;
}

/* ============================================================
 * Node Allocation
 * ============================================================ */
Status graph_alloc_node(Graph *g, NodeType type, NodeId *out_id)
{
    uint16_t i, j;
    const NodeMeta *meta;

    if (g == NULL || out_id == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (type == NODE_TYPE_NONE || type >= NODE_TYPE_COUNT) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Get metadata for default params */
    meta = node_registry_get_meta(type);

    /* Find first free slot */
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_NONE) {
            /* Initialize node */
            g->nodes[i].type = type;
            for (j = 0; j < MAX_IN_PORTS; j++) {
                g->nodes[i].inputs[j].src_node = INVALID_NODE_ID;
                g->nodes[i].inputs[j].src_port = 0;
            }
            /* Apply param defaults from registry */
            for (j = 0; j < MAX_PARAMS; j++) {
                if (meta != NULL && j < meta->num_params) {
                    g->nodes[i].params[j] = meta->param_defaults[j];
                } else {
                    g->nodes[i].params[j] = 0.0f;
                }
            }
            for (j = 0; j < MAX_NODE_STATE; j++) {
                g->nodes[i].state_u32[j] = 0;
            }

            g->node_count++;
            *out_id = (NodeId)i;
            return STATUS_OK;
        }
    }

    return STATUS_ERR_GRAPH_FULL;
}

Status graph_free_node(Graph *g, NodeId id)
{
    uint16_t i, j;

    if (g == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (id >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (g->nodes[id].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Disconnect any nodes that reference this node */
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type != NODE_TYPE_NONE) {
            for (j = 0; j < MAX_IN_PORTS; j++) {
                if (g->nodes[i].inputs[j].src_node == id) {
                    g->nodes[i].inputs[j].src_node = INVALID_NODE_ID;
                    g->nodes[i].inputs[j].src_port = 0;
                }
            }
        }
    }

    /* Clear the node */
    g->nodes[id].type = NODE_TYPE_NONE;
    for (j = 0; j < MAX_IN_PORTS; j++) {
        g->nodes[id].inputs[j].src_node = INVALID_NODE_ID;
        g->nodes[id].inputs[j].src_port = 0;
    }

    if (g->node_count > 0) {
        g->node_count--;
    }

    return STATUS_OK;
}

/* ============================================================
 * Node Connections
 * ============================================================ */
Status graph_connect(Graph *g, NodeId src_node, uint8_t src_port,
                     NodeId dst_node, uint8_t dst_port)
{
    if (g == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Validate source node */
    if (src_node >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }
    if (g->nodes[src_node].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Validate destination node */
    if (dst_node >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }
    if (g->nodes[dst_node].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Validate ports */
    if (src_port >= MAX_OUT_PORTS) {
        return STATUS_ERR_INVALID_PORT;
    }
    if (dst_port >= MAX_IN_PORTS) {
        return STATUS_ERR_INVALID_PORT;
    }

    /* Prevent self-connection */
    if (src_node == dst_node) {
        return STATUS_ERR_CYCLE_DETECTED;
    }

    /* Make connection */
    g->nodes[dst_node].inputs[dst_port].src_node = src_node;
    g->nodes[dst_node].inputs[dst_port].src_port = src_port;

    return STATUS_OK;
}

Status graph_disconnect(Graph *g, NodeId dst_node, uint8_t dst_port)
{
    if (g == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (dst_node >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (g->nodes[dst_node].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (dst_port >= MAX_IN_PORTS) {
        return STATUS_ERR_INVALID_PORT;
    }

    g->nodes[dst_node].inputs[dst_port].src_node = INVALID_NODE_ID;
    g->nodes[dst_node].inputs[dst_port].src_port = 0;

    return STATUS_OK;
}

/* ============================================================
 * Node Parameters
 * ============================================================ */
Status graph_set_param(Graph *g, NodeId id, uint8_t param_idx, float value)
{
    if (g == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (id >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (g->nodes[id].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (param_idx >= MAX_PARAMS) {
        return STATUS_ERR_INVALID_PORT;
    }

    g->nodes[id].params[param_idx] = value;
    return STATUS_OK;
}

Status graph_get_param(const Graph *g, NodeId id, uint8_t param_idx, float *out_value)
{
    if (g == NULL || out_value == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (id >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (g->nodes[id].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    if (param_idx >= MAX_PARAMS) {
        return STATUS_ERR_INVALID_PORT;
    }

    *out_value = g->nodes[id].params[param_idx];
    return STATUS_OK;
}

/* ============================================================
 * Node Queries
 * ============================================================ */
int graph_node_is_valid(const Graph *g, NodeId id)
{
    if (g == NULL) {
        return 0;
    }

    if (id >= MAX_NODES) {
        return 0;
    }

    return (g->nodes[id].type != NODE_TYPE_NONE) ? 1 : 0;
}

NodeType graph_node_get_type(const Graph *g, NodeId id)
{
    if (g == NULL) {
        return NODE_TYPE_NONE;
    }

    if (id >= MAX_NODES) {
        return NODE_TYPE_NONE;
    }

    return g->nodes[id].type;
}

void graph_copy(Graph *dst, const Graph *src)
{
    if (dst == NULL || src == NULL) {
        return;
    }
    memcpy(dst, src, sizeof(Graph));
}
