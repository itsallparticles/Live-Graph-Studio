#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

/* ============================================================
 * Render Module
 * ============================================================
 * gsKit-based rendering with normalized coordinate system.
 * Coordinates: (0,0) = top-left, (1,1) = bottom-right
 * Colors: RGBA with 0-255 per component
 * ============================================================ */

/* Screen dimensions (NTSC 640x480) */
#define RENDER_SCREEN_WIDTH   640
#define RENDER_SCREEN_HEIGHT  480

/* Color helper macros
 * Format: bits 0-7 = R, 8-15 = G, 16-23 = B, 24-31 = A */
#define RENDER_COLOR(r, g, b, a) \
    (((uint64_t)(a) << 24) | ((uint64_t)(b) << 16) | ((uint64_t)(g) << 8) | (uint64_t)(r))
#define RENDER_RGBA(r, g, b, a) RENDER_COLOR(r, g, b, a)

/* Color extraction macros */
#define RENDER_COLOR_R(c) ((uint8_t)((c) & 0xFF))
#define RENDER_COLOR_G(c) ((uint8_t)(((c) >> 8) & 0xFF))
#define RENDER_COLOR_B(c) ((uint8_t)(((c) >> 16) & 0xFF))
#define RENDER_COLOR_A(c) ((uint8_t)(((c) >> 24) & 0xFF))

/* Predefined colors */
#define RENDER_COLOR_WHITE   RENDER_COLOR(255, 255, 255, 128)
#define RENDER_COLOR_BLACK   RENDER_COLOR(0, 0, 0, 128)
#define RENDER_COLOR_RED     RENDER_COLOR(255, 0, 0, 128)
#define RENDER_COLOR_GREEN   RENDER_COLOR(0, 255, 0, 128)
#define RENDER_COLOR_BLUE    RENDER_COLOR(0, 0, 255, 128)
#define RENDER_COLOR_YELLOW  RENDER_COLOR(255, 255, 0, 128)
#define RENDER_COLOR_CYAN    RENDER_COLOR(0, 255, 255, 128)
#define RENDER_COLOR_MAGENTA RENDER_COLOR(255, 0, 255, 128)
#define RENDER_COLOR_GRAY    RENDER_COLOR(128, 128, 128, 128)

/* ============================================================
 * Render API
 * ============================================================ */

/* Initialize rendering subsystem.
 * Returns 0 on success, -1 on failure. */
int render_init(void);

/* Shutdown rendering subsystem. */
void render_shutdown(void);

/* Begin frame rendering. Call at start of each frame. */
void render_begin_frame(void);

/* End frame rendering and flip buffers. */
void render_end_frame(void);

/* Clear screen with specified color. */
void render_clear(uint64_t color);

/* ============================================================
 * Drawing Primitives (normalized coordinates 0.0-1.0)
 * ============================================================ */

/* Draw filled rectangle.
 * x, y: top-left corner (normalized)
 * w, h: width and height (normalized)
 * color: RGBA color */
void render_rect(float x, float y, float w, float h, uint64_t color);

/* Draw filled rectangle with screen coordinates. */
void render_rect_screen(int x, int y, int w, int h, uint64_t color);

/* Draw line.
 * x1, y1: start point (normalized)
 * x2, y2: end point (normalized)
 * color: RGBA color */
void render_line(float x1, float y1, float x2, float y2, uint64_t color);

/* Draw line with screen coordinates. */
void render_line_screen(int x1, int y1, int x2, int y2, uint64_t color);

/* Draw rectangle outline.
 * x, y: top-left corner (normalized)
 * w, h: width and height (normalized)
 * color: RGBA color */
void render_rect_outline(float x, float y, float w, float h, uint64_t color);

/* Draw rectangle outline with screen coordinates. */
void render_rect_outline_screen(int x, int y, int w, int h, uint64_t color);

/* Draw circle (approximated with line segments).
 * cx, cy: center (normalized)
 * r: radius (normalized, relative to screen width)
 * color: RGBA color
 * segments: number of line segments (8-32 typical) */
void render_circle(float cx, float cy, float r, uint64_t color, int segments);

/* Draw filled circle. */
void render_circle_filled(float cx, float cy, float r, uint64_t color, int segments);

/* ============================================================
 * Coordinate Conversion
 * ============================================================ */

/* Convert normalized X (0.0-1.0) to screen X. */
int render_norm_to_screen_x(float nx);

/* Convert normalized Y (0.0-1.0) to screen Y. */
int render_norm_to_screen_y(float ny);

/* Convert screen X to normalized X. */
float render_screen_to_norm_x(int sx);

/* Convert screen Y to normalized Y. */
float render_screen_to_norm_y(int sy);

/* Get screen dimensions. */
int render_get_width(void);
int render_get_height(void);

/* Check if render subsystem is initialized. */
int render_is_initialized(void);

/* Get gsKit context (for font/texture modules).
 * Returns NULL if not initialized. */
struct gsGlobal *render_get_gs_context(void);

#endif /* RENDER_H */
