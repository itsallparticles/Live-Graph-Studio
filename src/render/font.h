#ifndef FONT_H
#define FONT_H

#include <stdint.h>

/* ============================================================
 * Font Module
 * ============================================================
 * Minimal bitmap font renderer for ASCII 32..126.
 * Uses an embedded 8x8 monospace bitmap font.
 * ============================================================ */

/* Font dimensions */
#define FONT_CHAR_WIDTH   8
#define FONT_CHAR_HEIGHT  8
#define FONT_FIRST_CHAR   32
#define FONT_LAST_CHAR    126
#define FONT_CHAR_COUNT   (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)

/* ============================================================
 * Font API
 * ============================================================ */

/* Initialize font subsystem.
 * Must be called after render_init().
 * Returns 0 on success, -1 on failure. */
int font_init(void);

/* Shutdown font subsystem. */
void font_shutdown(void);

/* Draw a single character at screen coordinates.
 * c: ASCII character (32-126)
 * x, y: top-left position in screen coordinates
 * color: RGBA color
 * scale: integer scale factor (1 = 8x8, 2 = 16x16, etc.) */
void font_draw_char_screen(char c, int x, int y, uint64_t color, int scale);

/* Draw a string at screen coordinates.
 * str: null-terminated ASCII string
 * x, y: top-left position in screen coordinates
 * color: RGBA color
 * scale: integer scale factor */
void font_draw_string_screen(const char *str, int x, int y, uint64_t color, int scale);

/* Draw a single character at normalized coordinates.
 * c: ASCII character (32-126)
 * x, y: top-left position (0.0-1.0)
 * color: RGBA color
 * scale: integer scale factor */
void font_draw_char(char c, float x, float y, uint64_t color, int scale);

/* Draw a string at normalized coordinates.
 * str: null-terminated ASCII string
 * x, y: top-left position (0.0-1.0)
 * color: RGBA color
 * scale: integer scale factor */
void font_draw_string(const char *str, float x, float y, uint64_t color, int scale);

/* Draw formatted text at screen coordinates (printf-style).
 * x, y: top-left position in screen coordinates
 * color: RGBA color
 * scale: integer scale factor
 * fmt: printf format string
 * Returns number of characters printed. */
int font_printf_screen(int x, int y, uint64_t color, int scale, const char *fmt, ...);

/* Draw formatted text at normalized coordinates (printf-style).
 * x, y: top-left position (0.0-1.0)
 * color: RGBA color
 * scale: integer scale factor
 * fmt: printf format string
 * Returns number of characters printed. */
int font_printf(float x, float y, uint64_t color, int scale, const char *fmt, ...);

/* Get string width in pixels at given scale. */
int font_string_width(const char *str, int scale);

/* Get string height in pixels at given scale. */
int font_string_height(int scale);

/* Check if font subsystem is initialized. */
int font_is_initialized(void);

#endif /* FONT_H */
