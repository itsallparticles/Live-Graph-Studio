#ifndef GRAPH_EVAL_H
#define GRAPH_EVAL_H

#include <stdint.h>
#include "graph_types.h"
#include "../runtime/runtime.h"

/* ============================================================
 * Graph Evaluation API
 * ============================================================
 * Evaluates the graph in topological order using the EvalPlan.
 * Node outputs are stored in the OutputBank for downstream use.
 * ============================================================ */

/* Initialize output bank (zero all outputs).
 * Must be called before graph_eval(). */
void graph_eval_init_outputs(OutputBank *bank);

/* Evaluate entire graph using precomputed EvalPlan.
 * - graph: the graph to evaluate (ActiveGraph)
 * - plan: precomputed topological order from graph_build_eval_plan
 * - bank: output storage for all nodes (must be initialized first)
 * - ctx: runtime context (time, dt, pad state)
 */
void graph_eval(const Graph *graph,
                const EvalPlan *plan,
                OutputBank *bank,
                const RuntimeContext *ctx);

/* Get output value from a specific node/port.
 * Returns 0.0f if node_id is invalid. */
float graph_eval_get_output(const OutputBank *bank,
                            NodeId node_id,
                            uint8_t port);

/* Get output value from the sink node (convenience wrapper).
 * Returns 0.0f if plan has no valid sink. */
float graph_eval_get_sink_output(const OutputBank *bank,
                                 const EvalPlan *plan,
                                 uint8_t port);

#endif /* GRAPH_EVAL_H */
