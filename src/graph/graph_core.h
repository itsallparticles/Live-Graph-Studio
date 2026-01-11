#ifndef GRAPH_CORE_H
#define GRAPH_CORE_H

#include "graph_types.h"

/* ============================================================
 * Graph Initialization
 * ============================================================ */
void graph_init(Graph *g);

/* ============================================================
 * Node Allocation
 * ============================================================ */
Status graph_alloc_node(Graph *g, NodeType type, NodeId *out_id);
Status graph_free_node(Graph *g, NodeId id);

/* ============================================================
 * Node Connections
 * ============================================================ */
Status graph_connect(Graph *g, NodeId src_node, uint8_t src_port,
                     NodeId dst_node, uint8_t dst_port);
Status graph_disconnect(Graph *g, NodeId dst_node, uint8_t dst_port);

/* ============================================================
 * Node Parameters
 * ============================================================ */
Status graph_set_param(Graph *g, NodeId id, uint8_t param_idx, float value);
Status graph_get_param(const Graph *g, NodeId id, uint8_t param_idx, float *out_value);

/* ============================================================
 * Node Queries
 * ============================================================ */
int graph_node_is_valid(const Graph *g, NodeId id);
NodeType graph_node_get_type(const Graph *g, NodeId id);

/* ============================================================
 * Graph Copy
 * ============================================================ */
void graph_copy(Graph *dst, const Graph *src);

#endif /* GRAPH_CORE_H */
