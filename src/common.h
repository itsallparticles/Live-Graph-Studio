#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * Graph Limits (from GRAPH_MODEL.md)
 * ============================================================ */
#define MAX_NODES       256
#define MAX_IN_PORTS    4
#define MAX_OUT_PORTS   4
#define MAX_PARAMS      8
#define MAX_NODE_STATE  4

/* ============================================================
 * NodeId Type and Invalid Sentinel
 * ============================================================ */
typedef uint16_t NodeId;
#define INVALID_NODE_ID ((NodeId)0xFFFF)

/* ============================================================
 * Utility Macros
 * ============================================================ */
#define LGS_MIN(a, b)   (((a) < (b)) ? (a) : (b))
#define LGS_MAX(a, b)   (((a) > (b)) ? (a) : (b))
#define LGS_CLAMP(x, lo, hi) (LGS_MIN(LGS_MAX((x), (lo)), (hi)))

/* ============================================================
 * Debug Assert
 * ============================================================ */
#ifdef DEBUG
    #include <stdio.h>
    #define DEBUG_ASSERT(cond) \
        do { \
            if (!(cond)) { \
                printf("ASSERT FAILED: %s @ %s:%d\n", #cond, __FILE__, __LINE__); \
                while (1) {} \
            } \
        } while (0)
#else
    #define DEBUG_ASSERT(cond) ((void)0)
#endif

/* ============================================================
 * Status Codes
 * ============================================================ */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_INVALID_NODE,
    STATUS_ERR_INVALID_PORT,
    STATUS_ERR_GRAPH_FULL,
    STATUS_ERR_CYCLE_DETECTED,
    STATUS_ERR_NO_SINK,
    STATUS_ERR_IO_FAIL,
    STATUS_ERR_VALIDATION_FAIL
} Status;

#endif /* COMMON_H */
