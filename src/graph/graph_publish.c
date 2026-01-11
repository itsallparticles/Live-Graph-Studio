#include "graph_publish.h"
#include "graph_core.h"
#include "graph_validate.h"
#include <string.h>

/* ============================================================
 * Result Strings
 * ============================================================ */
static const char *s_publish_result_strings[] = {
    "OK",
    "Error: NULL pointer",
    "Error: Cycle detected",
    "Error: No sink node",
    "Error: Validation failed"
};

/* ============================================================
 * Validation Helper
 * ============================================================ */
static PublishResult validate_graph_internal(const Graph *g, EvalPlan *plan)
{
    if (!g) {
        return PUBLISH_ERR_NULL_PTR;
    }

    /* Build eval plan (performs cycle detection and sink check) */
    Status status = graph_build_eval_plan(g, plan);

    switch (status) {
        case STATUS_OK:
            return PUBLISH_OK;
        case STATUS_ERR_CYCLE_DETECTED:
            return PUBLISH_ERR_CYCLE;
        case STATUS_ERR_NO_SINK:
            return PUBLISH_ERR_NO_SINK;
        default:
            return PUBLISH_ERR_VALIDATION;
    }
}

/* ============================================================
 * Publish API Implementation
 * ============================================================ */
PublishResult graph_publish(const Graph *edit_graph, Graph *active_graph, EvalPlan *out_plan)
{
    EvalPlan temp_plan;
    EvalPlan *plan_ptr;
    PublishResult result;

    if (!edit_graph || !active_graph) {
        return PUBLISH_ERR_NULL_PTR;
    }

    /* Use provided plan or temp storage */
    plan_ptr = out_plan ? out_plan : &temp_plan;

    /* Validate before commit */
    result = validate_graph_internal(edit_graph, plan_ptr);
    if (result != PUBLISH_OK) {
        return result;
    }

    /* Atomic copy: memcpy entire graph structure */
    memcpy(active_graph->nodes, edit_graph->nodes, sizeof(active_graph->nodes));
    active_graph->node_count = edit_graph->node_count;

    /* Increment version on active graph */
    active_graph->version++;

    return PUBLISH_OK;
}

PublishResult graph_publish_validate(const Graph *edit_graph, EvalPlan *out_plan)
{
    EvalPlan temp_plan;
    EvalPlan *plan_ptr;

    if (!edit_graph) {
        return PUBLISH_ERR_NULL_PTR;
    }

    plan_ptr = out_plan ? out_plan : &temp_plan;
    return validate_graph_internal(edit_graph, plan_ptr);
}

const char *graph_publish_result_str(PublishResult result)
{
    if (result < 0 || result > PUBLISH_ERR_VALIDATION) {
        return "Unknown error";
    }
    return s_publish_result_strings[result];
}

int graph_publish_in_sync(const Graph *edit_graph, const Graph *active_graph)
{
    if (!edit_graph || !active_graph) {
        return 0;
    }
    return edit_graph->version == active_graph->version;
}

uint16_t graph_get_version(const Graph *g)
{
    if (!g) {
        return 0;
    }
    return g->version;
}

void graph_publish_revert(Graph *edit_graph, const Graph *active_graph)
{
    if (!edit_graph || !active_graph) {
        return;
    }

    /* Copy active graph back to edit graph */
    graph_copy(edit_graph, active_graph);
}
