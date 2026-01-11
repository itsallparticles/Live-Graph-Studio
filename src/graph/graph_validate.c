#include "graph_validate.h"
#include "graph_core.h"

/* ============================================================
 * Internal State for Topological Sort (Kahn's Algorithm)
 * ============================================================ */
typedef struct {
    uint8_t  in_degree[MAX_NODES];    /* Incoming edge count per node */
    uint8_t  visited[MAX_NODES];      /* Visited flag for processing */
    NodeId   queue[MAX_NODES];        /* Processing queue */
    uint16_t queue_head;
    uint16_t queue_tail;
} TopoState;

/* ============================================================
 * Helper: Count incoming edges for each node
 * ============================================================ */
static void compute_in_degrees(const Graph *g, TopoState *state)
{
    uint16_t i, j;
    NodeId src;

    /* Initialize all to zero */
    for (i = 0; i < MAX_NODES; i++) {
        state->in_degree[i] = 0;
        state->visited[i] = 0;
    }

    /* Count incoming edges */
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_NONE) {
            continue;
        }
        for (j = 0; j < MAX_IN_PORTS; j++) {
            src = g->nodes[i].inputs[j].src_node;
            if (src != INVALID_NODE_ID && src < MAX_NODES) {
                if (g->nodes[src].type != NODE_TYPE_NONE) {
                    state->in_degree[i]++;
                }
            }
        }
    }
}

/* ============================================================
 * Helper: Queue operations
 * ============================================================ */
static void queue_init(TopoState *state)
{
    state->queue_head = 0;
    state->queue_tail = 0;
}

static void queue_push(TopoState *state, NodeId id)
{
    if (state->queue_tail < MAX_NODES) {
        state->queue[state->queue_tail++] = id;
    }
}

static int queue_empty(const TopoState *state)
{
    return state->queue_head >= state->queue_tail;
}

static NodeId queue_pop(TopoState *state)
{
    if (queue_empty(state)) {
        return INVALID_NODE_ID;
    }
    return state->queue[state->queue_head++];
}

/* ============================================================
 * Validate a single connection reference
 * ============================================================ */
Status graph_validate_connection(const Graph *g, const Connection *conn)
{
    if (g == NULL || conn == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Disconnected is valid */
    if (conn->src_node == INVALID_NODE_ID) {
        return STATUS_OK;
    }

    /* Check bounds */
    if (conn->src_node >= MAX_NODES) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Check source node exists */
    if (g->nodes[conn->src_node].type == NODE_TYPE_NONE) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Check port bounds */
    if (conn->src_port >= MAX_OUT_PORTS) {
        return STATUS_ERR_INVALID_PORT;
    }

    return STATUS_OK;
}

/* ============================================================
 * Check if graph has at least one sink node
 * ============================================================ */
int graph_has_sink(const Graph *g)
{
    uint16_t i;

    if (g == NULL) {
        return 0;
    }

    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_RENDER2D) {
            return 1;
        }
    }

    return 0;
}

/* ============================================================
 * Count sink nodes in graph
 * ============================================================ */
uint16_t graph_count_sinks(const Graph *g)
{
    uint16_t i, count = 0;

    if (g == NULL) {
        return 0;
    }

    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_RENDER2D) {
            count++;
        }
    }

    return count;
}

/* ============================================================
 * Find primary sink node (first RENDER2D)
 * ============================================================ */
static NodeId find_sink(const Graph *g)
{
    uint16_t i;

    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_RENDER2D) {
            return (NodeId)i;
        }
    }

    return INVALID_NODE_ID;
}

/* ============================================================
 * Build Evaluation Plan (Kahn's Algorithm for Topological Sort)
 * ============================================================ */
Status graph_build_eval_plan(const Graph *g, EvalPlan *plan)
{
    TopoState state;
    uint16_t i, j;
    uint16_t active_count = 0;
    NodeId current;
    NodeId src;

    if (g == NULL || plan == NULL) {
        return STATUS_ERR_INVALID_NODE;
    }

    /* Initialize plan */
    plan->count = 0;
    plan->sink_id = INVALID_NODE_ID;

    /* Find sink node */
    plan->sink_id = find_sink(g);
    if (plan->sink_id == INVALID_NODE_ID) {
        return STATUS_ERR_NO_SINK;
    }

    /* Validate all connections first */
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_NONE) {
            continue;
        }
        active_count++;
        for (j = 0; j < MAX_IN_PORTS; j++) {
            Status s = graph_validate_connection(g, &g->nodes[i].inputs[j]);
            if (s != STATUS_OK) {
                return STATUS_ERR_VALIDATION_FAIL;
            }
        }
    }

    /* No nodes? Return early (but we have a sink, so this shouldn't happen) */
    if (active_count == 0) {
        return STATUS_OK;
    }

    /* Compute in-degrees */
    compute_in_degrees(g, &state);

    /* Initialize queue with nodes that have no incoming edges */
    queue_init(&state);
    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type != NODE_TYPE_NONE && state.in_degree[i] == 0) {
            queue_push(&state, (NodeId)i);
            state.visited[i] = 1;
        }
    }

    /* Process nodes in topological order */
    while (!queue_empty(&state)) {
        current = queue_pop(&state);

        if (current == INVALID_NODE_ID) {
            break;
        }

        /* Add to evaluation order */
        if (plan->count < MAX_NODES) {
            plan->order[plan->count++] = current;
        }

        /* Reduce in-degree of nodes that depend on this one */
        for (i = 0; i < MAX_NODES; i++) {
            if (g->nodes[i].type == NODE_TYPE_NONE) {
                continue;
            }
            if (state.visited[i]) {
                continue;
            }

            for (j = 0; j < MAX_IN_PORTS; j++) {
                src = g->nodes[i].inputs[j].src_node;
                if (src == current) {
                    if (state.in_degree[i] > 0) {
                        state.in_degree[i]--;
                    }
                }
            }

            /* If all dependencies satisfied, add to queue */
            if (state.in_degree[i] == 0 && !state.visited[i]) {
                queue_push(&state, (NodeId)i);
                state.visited[i] = 1;
            }
        }
    }

    /* Check for cycles: if we didn't process all nodes, there's a cycle */
    if (plan->count != active_count) {
        plan->count = 0;
        plan->sink_id = INVALID_NODE_ID;
        return STATUS_ERR_CYCLE_DETECTED;
    }

    return STATUS_OK;
}
