#include <stdio.h>
#include <string.h>
#include "../src/common.h"
#include "../src/graph/graph_core.h"
#include "../src/io/graph_io.h"

static void set_ui_positions(UiMetaBank *ui, NodeId time_id, NodeId sin_id,
                             NodeId color_id, NodeId render_id)
{
    if (!ui) {
        return;
    }
    memset(ui, 0, sizeof(*ui));

    if (time_id < MAX_NODES) {
        ui->meta[time_id].x = -220.0f;
        ui->meta[time_id].y = -40.0f;
    }
    if (sin_id < MAX_NODES) {
        ui->meta[sin_id].x = -40.0f;
        ui->meta[sin_id].y = -40.0f;
    }
    if (color_id < MAX_NODES) {
        ui->meta[color_id].x = 140.0f;
        ui->meta[color_id].y = -40.0f;
    }
    if (render_id < MAX_NODES) {
        ui->meta[render_id].x = 320.0f;
        ui->meta[render_id].y = -40.0f;
    }
}

int main(void)
{
    Graph g;
    UiMetaBank ui;
    NodeId time_id = INVALID_NODE_ID;
    NodeId sin_id = INVALID_NODE_ID;
    NodeId color_id = INVALID_NODE_ID;
    NodeId render_id = INVALID_NODE_ID;
    GraphIoResult result;

    graph_init(&g);

    if (graph_alloc_node(&g, NODE_TYPE_TIME, &time_id) != STATUS_OK) {
        fprintf(stderr, "Failed to alloc TIME\n");
        return 1;
    }
    if (graph_alloc_node(&g, NODE_TYPE_SIN, &sin_id) != STATUS_OK) {
        fprintf(stderr, "Failed to alloc SIN\n");
        return 1;
    }
    if (graph_alloc_node(&g, NODE_TYPE_COLORIZE, &color_id) != STATUS_OK) {
        fprintf(stderr, "Failed to alloc COLORIZE\n");
        return 1;
    }
    if (graph_alloc_node(&g, NODE_TYPE_RENDER2D, &render_id) != STATUS_OK) {
        fprintf(stderr, "Failed to alloc RENDER2D\n");
        return 1;
    }

    graph_connect(&g, time_id, 0, sin_id, 0);
    graph_set_param(&g, sin_id, 0, 1.0f);
    graph_set_param(&g, sin_id, 1, 1.0f);
    graph_set_param(&g, sin_id, 2, 0.0f);

    graph_connect(&g, sin_id, 0, color_id, 0);

    graph_connect(&g, color_id, 0, render_id, 0);
    graph_connect(&g, color_id, 1, render_id, 1);
    graph_connect(&g, color_id, 2, render_id, 2);

    graph_set_param(&g, render_id, 0, 0.3f);
    graph_set_param(&g, render_id, 1, 0.3f);
    graph_set_param(&g, render_id, 2, 0.4f);
    graph_set_param(&g, render_id, 3, 0.4f);

    set_ui_positions(&ui, time_id, sin_id, color_id, render_id);

    result = graph_io_save("assets/graphs/default.gph", &g, &ui);
    if (result != GRAPH_IO_OK) {
        fprintf(stderr, "Failed to save default.gph: %s\n", graph_io_result_str(result));
        return 1;
    }

    printf("Wrote assets/graphs/default.gph (nodes=%u)\n", g.node_count);
    return 0;
}
