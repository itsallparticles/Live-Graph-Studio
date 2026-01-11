#include "graph_eval.h"
#include "graph_core.h"
#include "../nodes/node_registry.h"
#include <string.h>

/* ============================================================
 * Graph Evaluation
 * ============================================================
 * Memory usage:
 *   OutputBank: ~4KB (MAX_NODES * MAX_OUT_PORTS * sizeof(float))
 *              = 256 * 4 * 4 = 4096 bytes
 * ============================================================ */

/* ============================================================
 * Initialize Output Bank
 * ============================================================ */
void graph_eval_init_outputs(OutputBank *bank)
{
    if (!bank) {
        return;
    }
    memset(bank->out, 0, sizeof(bank->out));
}

/* ============================================================
 * Get Output Value
 * ============================================================ */
float graph_eval_get_output(const OutputBank *bank,
                            NodeId node_id,
                            uint8_t port)
{
    if (!bank) {
        return 0.0f;
    }
    if (node_id == INVALID_NODE_ID || node_id >= MAX_NODES) {
        return 0.0f;
    }
    if (port >= MAX_OUT_PORTS) {
        return 0.0f;
    }
    return bank->out[node_id][port];
}

/* ============================================================
 * Gather Inputs for a Node
 * ============================================================
 * Reads connected outputs from the bank into inputs array.
 * Unconnected ports get 0.0f.
 * ============================================================ */
static void gather_inputs(const Graph *graph,
                          const OutputBank *bank,
                          NodeId node_id,
                          float inputs[MAX_IN_PORTS])
{
    const Node *node;
    int i;

    /* Zero inputs first */
    for (i = 0; i < MAX_IN_PORTS; i++) {
        inputs[i] = 0.0f;
    }

    if (node_id == INVALID_NODE_ID || node_id >= MAX_NODES) {
        return;
    }

    node = &graph->nodes[node_id];
    if (node->type == NODE_TYPE_NONE) {
        return;
    }

    /* Gather from connections (only from active source nodes) */
    for (i = 0; i < MAX_IN_PORTS; i++) {
        const Connection *conn = &node->inputs[i];
        if (conn->src_node != INVALID_NODE_ID &&
            conn->src_node < MAX_NODES &&
            conn->src_port < MAX_OUT_PORTS &&
            graph->nodes[conn->src_node].type != NODE_TYPE_NONE) {
            inputs[i] = bank->out[conn->src_node][conn->src_port];
        }
    }
}

/* ============================================================
 * Evaluate Single Node
 * ============================================================ */
static void eval_node(const Graph *graph,
                      const OutputBank *bank,
                      NodeId node_id,
                      float outputs[MAX_OUT_PORTS],
                      const RuntimeContext *ctx)
{
    const Node *node;
    NodeEvalFunc eval_func;
    float inputs[MAX_IN_PORTS];
    int i;

    /* Zero outputs first */
    for (i = 0; i < MAX_OUT_PORTS; i++) {
        outputs[i] = 0.0f;
    }

    if (node_id == INVALID_NODE_ID || node_id >= MAX_NODES) {
        return;
    }

    node = &graph->nodes[node_id];
    if (node->type == NODE_TYPE_NONE) {
        return;
    }

    /* Gather inputs from connected nodes */
    gather_inputs(graph, bank, node_id, inputs);

    /* Get and call eval function */
    eval_func = node_registry_get_eval(node->type);
    if (eval_func) {
        eval_func(node, inputs, outputs, ctx);
    }
}

/* ============================================================
 * Evaluate Graph
 * ============================================================
 * Iterates through nodes in topological order (from EvalPlan).
 * Each node's outputs are computed and stored in OutputBank.
 * ============================================================ */
void graph_eval(const Graph *graph,
                const EvalPlan *plan,
                OutputBank *bank,
                const RuntimeContext *ctx)
{
    uint16_t i;
    uint16_t eval_count;
    NodeId node_id;
    float outputs[MAX_OUT_PORTS];
    int j;

    if (!graph || !plan || !bank || !ctx) {
        return;
    }

    /* Clamp count to MAX_NODES defensively */
    eval_count = (plan->count <= MAX_NODES) ? plan->count : MAX_NODES;

    /* Evaluate nodes in topological order */
    for (i = 0; i < eval_count; i++) {
        node_id = plan->order[i];

        /* Skip invalid entries */
        if (node_id == INVALID_NODE_ID || node_id >= MAX_NODES) {
            continue;
        }

        /* Skip inactive nodes */
        if (graph->nodes[node_id].type == NODE_TYPE_NONE) {
            continue;
        }

        /* Evaluate this node */
        eval_node(graph, bank, node_id, outputs, ctx);

        /* Store outputs in bank */
        for (j = 0; j < MAX_OUT_PORTS; j++) {
            bank->out[node_id][j] = outputs[j];
        }
    }
}

/* ============================================================
 * Get Sink Output (convenience wrapper)
 * ============================================================ */
float graph_eval_get_sink_output(const OutputBank *bank,
                                 const EvalPlan *plan,
                                 uint8_t port)
{
    if (!bank || !plan) {
        return 0.0f;
    }
    return graph_eval_get_output(bank, plan->sink_id, port);
}
