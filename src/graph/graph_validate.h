#ifndef GRAPH_VALIDATE_H
#define GRAPH_VALIDATE_H

#include "graph_types.h"

/* ============================================================
 * Graph Validation and Evaluation Plan Building
 * ============================================================
 * Validates the graph structure and builds a topologically sorted
 * evaluation plan. Rejects cycles and locates the primary sink node.
 * ============================================================ */

/* Build evaluation plan from graph
 * - Validates all node references
 * - Detects cycles (returns STATUS_ERR_CYCLE_DETECTED)
 * - Locates RENDER2D sink (returns STATUS_ERR_NO_SINK if missing)
 * - Produces stable topological ordering in plan->order[]
 */
Status graph_build_eval_plan(const Graph *g, EvalPlan *plan);

/* Validate a single connection reference */
Status graph_validate_connection(const Graph *g, const Connection *conn);

/* Check if graph has at least one sink node */
int graph_has_sink(const Graph *g);
uint16_t graph_count_sinks(const Graph *g);

#endif /* GRAPH_VALIDATE_H */
