#ifndef GRAPH_PUBLISH_H
#define GRAPH_PUBLISH_H

#include "graph_types.h"

/* ============================================================
 * Graph Publish Module
 * ============================================================
 * Manages the commit process from EditGraph to ActiveGraph.
 * Validates before commit, performs atomic copy, tracks versions.
 * ============================================================ */

/* ============================================================
 * Publish Result (detailed commit status)
 * ============================================================ */
typedef enum {
    PUBLISH_OK = 0,
    PUBLISH_ERR_NULL_PTR,
    PUBLISH_ERR_CYCLE,
    PUBLISH_ERR_NO_SINK,
    PUBLISH_ERR_VALIDATION
} PublishResult;

/* ============================================================
 * Publish API
 * ============================================================ */

/* Commit edit graph to active graph.
 * - Validates edit_graph (cycle detection, sink check)
 * - On success: copies edit_graph to active_graph, increments version
 * - On failure: active_graph unchanged
 * - out_plan: if non-NULL and success, receives the new evaluation plan
 * Returns: PublishResult indicating success or failure reason */
PublishResult graph_publish(const Graph *edit_graph, Graph *active_graph, EvalPlan *out_plan);

/* Validate edit graph without committing.
 * Useful for preview/dry-run before actual commit.
 * Returns: PublishResult indicating if commit would succeed */
PublishResult graph_publish_validate(const Graph *edit_graph, EvalPlan *out_plan);

/* Get human-readable string for publish result */
const char *graph_publish_result_str(PublishResult result);

/* Check if graphs are in sync (same version) */
int graph_publish_in_sync(const Graph *edit_graph, const Graph *active_graph);

/* Get current version of a graph */
uint16_t graph_get_version(const Graph *g);

/* Sync edit graph from active graph (revert uncommitted changes) */
void graph_publish_revert(Graph *edit_graph, const Graph *active_graph);

#endif /* GRAPH_PUBLISH_H */
