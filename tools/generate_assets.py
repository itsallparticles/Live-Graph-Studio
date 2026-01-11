#!/usr/bin/env python3
"""
Generate UI assets for PS2 Live Graph Studio
Creates simple PNG images for cursor, ports, and node elements.
"""

import struct
import zlib
import os

def create_png(width, height, pixels):
    """Create a PNG file from RGBA pixel data."""
    def png_chunk(chunk_type, data):
        chunk_len = struct.pack('>I', len(data))
        chunk_crc = struct.pack('>I', zlib.crc32(chunk_type + data) & 0xffffffff)
        return chunk_len + chunk_type + data + chunk_crc

    # PNG signature
    signature = b'\x89PNG\r\n\x1a\n'

    # IHDR chunk
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr = png_chunk(b'IHDR', ihdr_data)

    # IDAT chunk (image data)
    raw_data = b''
    for y in range(height):
        raw_data += b'\x00'  # Filter byte (none)
        for x in range(width):
            idx = (y * width + x) * 4
            raw_data += bytes(pixels[idx:idx+4])

    compressed = zlib.compress(raw_data, 9)
    idat = png_chunk(b'IDAT', compressed)

    # IEND chunk
    iend = png_chunk(b'IEND', b'')

    return signature + ihdr + idat + iend


def create_cursor(size=32):
    """Create a crosshair cursor with a center dot."""
    pixels = [0] * (size * size * 4)
    center = size // 2
    
    # Colors
    white = [255, 255, 255, 255]
    cyan = [0, 255, 255, 255]
    outline = [0, 0, 0, 200]
    
    def set_pixel(x, y, color):
        if 0 <= x < size and 0 <= y < size:
            idx = (y * size + x) * 4
            pixels[idx:idx+4] = color
    
    # Draw crosshair with outline
    for i in range(size):
        # Horizontal line
        if abs(i - center) > 2:  # Gap in center
            set_pixel(i, center - 1, outline)
            set_pixel(i, center, cyan)
            set_pixel(i, center + 1, outline)
        # Vertical line
        if abs(i - center) > 2:
            set_pixel(center - 1, i, outline)
            set_pixel(center, i, cyan)
            set_pixel(center + 1, i, outline)
    
    # Center dot
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            dist = abs(dx) + abs(dy)
            if dist <= 2:
                set_pixel(center + dx, center + dy, white if dist <= 1 else cyan)
    
    return create_png(size, size, pixels)


def create_port_dot(size=16, color_rgb=(255, 128, 0)):
    """Create a circular port indicator."""
    pixels = [0] * (size * size * 4)
    center = size // 2
    radius = size // 2 - 2
    
    def set_pixel(x, y, color):
        if 0 <= x < size and 0 <= y < size:
            idx = (y * size + x) * 4
            # Clamp values to valid range
            pixels[idx] = max(0, min(255, int(color[0])))
            pixels[idx+1] = max(0, min(255, int(color[1])))
            pixels[idx+2] = max(0, min(255, int(color[2])))
            pixels[idx+3] = max(0, min(255, int(color[3])))
    
    for y in range(size):
        for x in range(size):
            dx = x - center + 0.5
            dy = y - center + 0.5
            dist = (dx*dx + dy*dy) ** 0.5
            
            if dist <= radius - 1:
                # Inner fill with gradient
                brightness = 1.0 - (dist / radius) * 0.3
                r = min(255, int(color_rgb[0] * brightness))
                g = min(255, int(color_rgb[1] * brightness))
                b = min(255, int(color_rgb[2] * brightness))
                set_pixel(x, y, [r, g, b, 255])
            elif dist <= radius:
                # Edge (anti-aliased)
                alpha = int((radius - dist + 1) * 255)
                set_pixel(x, y, [color_rgb[0], color_rgb[1], color_rgb[2], alpha])
            elif dist <= radius + 1:
                # Outer glow
                alpha = int((radius + 1 - dist) * 100)
                set_pixel(x, y, [255, 255, 255, alpha])
    
    return create_png(size, size, pixels)


def create_node_background(width=128, height=64):
    """Create a node background with rounded corners and gradient."""
    pixels = [0] * (width * height * 4)
    corner_radius = 8
    
    # Colors
    bg_top = [80, 70, 60]
    bg_bottom = [50, 45, 40]
    border = [160, 150, 140]
    highlight = [100, 90, 80]
    
    def set_pixel(x, y, color):
        if 0 <= x < width and 0 <= y < height:
            idx = (y * width + x) * 4
            pixels[idx:idx+4] = color
    
    def in_rounded_rect(x, y, w, h, r):
        # Check if point is inside rounded rectangle
        if x < r:
            if y < r:
                return (x - r)**2 + (y - r)**2 <= r**2
            elif y >= h - r:
                return (x - r)**2 + (y - (h - r - 1))**2 <= r**2
        elif x >= w - r:
            if y < r:
                return (x - (w - r - 1))**2 + (y - r)**2 <= r**2
            elif y >= h - r:
                return (x - (w - r - 1))**2 + (y - (h - r - 1))**2 <= r**2
        return 0 <= x < w and 0 <= y < h
    
    for y in range(height):
        for x in range(width):
            if not in_rounded_rect(x, y, width, height, corner_radius):
                continue
            
            # Gradient from top to bottom
            t = y / height
            r = int(bg_top[0] * (1 - t) + bg_bottom[0] * t)
            g = int(bg_top[1] * (1 - t) + bg_bottom[1] * t)
            b = int(bg_top[2] * (1 - t) + bg_bottom[2] * t)
            
            # Top highlight
            if y < 3:
                r = min(255, r + 30)
                g = min(255, g + 30)
                b = min(255, b + 30)
            
            # Border
            is_border = (x < 2 or x >= width - 2 or y < 2 or y >= height - 2)
            if is_border:
                set_pixel(x, y, [border[0], border[1], border[2], 255])
            else:
                set_pixel(x, y, [r, g, b, 220])
    
    return create_png(width, height, pixels)


def create_node_icon(icon_type, size=32):
    """Create node type icons."""
    pixels = [0] * (size * size * 4)
    center = size // 2
    
    def set_pixel(x, y, color):
        if 0 <= x < size and 0 <= y < size:
            idx = (y * size + x) * 4
            pixels[idx:idx+4] = color
    
    def draw_circle(cx, cy, r, color):
        for y in range(size):
            for x in range(size):
                dist = ((x - cx)**2 + (y - cy)**2) ** 0.5
                if dist <= r:
                    set_pixel(x, y, color)
    
    def draw_line(x1, y1, x2, y2, color, thickness=2):
        steps = max(abs(x2 - x1), abs(y2 - y1), 1)
        for i in range(steps + 1):
            t = i / steps
            x = int(x1 + (x2 - x1) * t)
            y = int(y1 + (y2 - y1) * t)
            for dy in range(-thickness//2, thickness//2 + 1):
                for dx in range(-thickness//2, thickness//2 + 1):
                    set_pixel(x + dx, y + dy, color)
    
    def draw_rect(x1, y1, x2, y2, color):
        for y in range(y1, y2):
            for x in range(x1, x2):
                set_pixel(x, y, color)
    
    def draw_text(char, cx, cy, color, scale=1):
        """Simple pixel font for single characters."""
        patterns = {
            '+': [(1,0),(0,1),(1,1),(2,1),(1,2)],
            '-': [(0,1),(1,1),(2,1)],
            '*': [(0,0),(2,0),(1,1),(0,2),(2,2)],
            '/': [(2,0),(1,1),(0,2)],
            '%': [(0,0),(2,1),(0,2),(2,0),(2,2)],
            '~': [(0,1),(1,0),(2,1),(3,0)],
            'S': [(1,0),(2,0),(0,1),(1,2),(2,2),(0,3),(1,3)],
            'C': [(1,0),(2,0),(0,1),(0,2),(1,3),(2,3)],
            'T': [(0,0),(1,0),(2,0),(1,1),(1,2),(1,3)],
            'A': [(1,0),(0,1),(2,1),(0,2),(1,2),(2,2),(0,3),(2,3)],
            'N': [(0,0),(0,1),(0,2),(0,3),(1,1),(2,2),(2,0),(2,1),(2,3)],
            'L': [(0,0),(0,1),(0,2),(0,3),(1,3),(2,3)],
            'M': [(0,0),(0,1),(0,2),(0,3),(1,1),(2,0),(2,1),(2,2),(2,3)],
            'D': [(0,0),(0,1),(0,2),(0,3),(1,0),(1,3),(2,1),(2,2)],
            'H': [(0,0),(0,1),(0,2),(0,3),(1,2),(2,0),(2,1),(2,2),(2,3)],
            'G': [(1,0),(2,0),(0,1),(0,2),(1,3),(2,3),(2,2),(1,2)],
            'R': [(0,0),(0,1),(0,2),(0,3),(1,0),(2,1),(1,2),(2,3)],
            'X': [(0,0),(2,0),(1,1),(1,2),(0,3),(2,3)],
            'Y': [(0,0),(2,0),(1,1),(1,2),(1,3)],
            '?': [(1,0),(0,1),(2,1),(2,2),(1,2),(1,4)],
        }
        if char in patterns:
            for px, py in patterns[char]:
                for sy in range(scale):
                    for sx in range(scale):
                        set_pixel(cx + px*scale + sx, cy + py*scale + sy, color)
    
    white = [255, 255, 255, 255]
    yellow = [255, 220, 100, 255]
    blue = [100, 150, 255, 255]
    green = [100, 255, 150, 255]
    red = [255, 100, 100, 255]
    purple = [200, 100, 255, 255]
    orange = [255, 150, 50, 255]
    cyan = [50, 255, 255, 255]
    pink = [255, 150, 200, 255]
    gray = [150, 150, 150, 255]
    
    if icon_type == 'time':
        # Clock icon
        draw_circle(center, center, 12, white)
        draw_circle(center, center, 10, [40, 40, 50, 255])
        draw_line(center, center, center, center - 7, yellow, 2)
        draw_line(center, center, center + 5, center + 3, yellow, 2)
        
    elif icon_type == 'sin':
        # Sine wave
        import math
        for x in range(4, size - 4):
            y = int(center + 8 * math.sin((x - 4) * math.pi * 2 / (size - 8)))
            for dy in range(-1, 2):
                set_pixel(x, y + dy, green)
                
    elif icon_type == 'cos':
        # Cosine wave (starts at peak)
        import math
        for x in range(4, size - 4):
            y = int(center + 8 * math.cos((x - 4) * math.pi * 2 / (size - 8)))
            for dy in range(-1, 2):
                set_pixel(x, y + dy, cyan)
                
    elif icon_type == 'tan':
        # Tangent wave (vertical asymptotes)
        import math
        for x in range(4, size - 4):
            t = (x - 4) / (size - 8) * math.pi - math.pi/2
            if abs(t) < math.pi/2 - 0.3:
                y = int(center + 6 * math.tan(t * 0.8))
                if 4 < y < size - 4:
                    for dy in range(-1, 2):
                        set_pixel(x, y + dy, orange)
        # Draw asymptote lines
        draw_line(4, 4, 4, size-4, gray, 1)
        draw_line(size-4, 4, size-4, size-4, gray, 1)
                
    elif icon_type == 'atan2':
        # Arrow showing angle
        draw_circle(center, center, 10, [40, 40, 60, 255])
        draw_line(center, center, center + 8, center - 5, yellow, 2)
        draw_line(center + 8, center - 5, center + 5, center - 5, yellow, 1)
        draw_line(center + 8, center - 5, center + 8, center - 2, yellow, 1)
        # Angle arc
        import math
        for i in range(8):
            angle = -0.5 + i * 0.08
            x = int(center + 6 * math.cos(angle))
            y = int(center + 6 * math.sin(angle))
            set_pixel(x, y, purple)
                
    elif icon_type == 'add':
        # Plus sign
        draw_line(center, 6, center, size - 6, blue, 3)
        draw_line(6, center, size - 6, center, blue, 3)
        
    elif icon_type == 'sub':
        # Minus sign
        draw_line(6, center, size - 6, center, blue, 3)
        
    elif icon_type == 'mul':
        # X sign
        draw_line(8, 8, size - 8, size - 8, purple, 3)
        draw_line(size - 8, 8, 8, size - 8, purple, 3)
        
    elif icon_type == 'div':
        # Division sign
        draw_line(6, center, size - 6, center, purple, 2)
        draw_circle(center, center - 6, 2, purple)
        draw_circle(center, center + 6, 2, purple)
        
    elif icon_type == 'mod':
        # Percent sign
        draw_circle(10, 10, 4, orange)
        draw_circle(22, 22, 4, orange)
        draw_line(22, 8, 10, 24, orange, 2)
        
    elif icon_type == 'abs':
        # |x| bars
        draw_line(8, 6, 8, size - 6, yellow, 2)
        draw_line(size - 8, 6, size - 8, size - 6, yellow, 2)
        draw_text('X', center - 4, center - 5, yellow, 2)
        
    elif icon_type == 'neg':
        # Negative sign with arrow
        draw_line(6, center, size - 6, center, red, 3)
        draw_line(size - 10, center - 4, size - 6, center, red, 2)
        draw_line(size - 10, center + 4, size - 6, center, red, 2)
        
    elif icon_type == 'min':
        # Down arrow / min
        draw_line(center, 6, center, size - 8, green, 2)
        draw_line(center - 6, size - 14, center, size - 8, green, 2)
        draw_line(center + 6, size - 14, center, size - 8, green, 2)
        draw_line(6, size - 6, size - 6, size - 6, green, 2)
        
    elif icon_type == 'max':
        # Up arrow / max
        draw_line(center, size - 6, center, 8, green, 2)
        draw_line(center - 6, 14, center, 8, green, 2)
        draw_line(center + 6, 14, center, 8, green, 2)
        draw_line(6, 6, size - 6, 6, green, 2)
        
    elif icon_type == 'clamp':
        # Bars with limited range
        draw_rect(6, 10, 10, size - 10, gray)
        draw_rect(size - 10, 10, size - 6, size - 10, gray)
        draw_line(10, center, size - 10, center, cyan, 2)
        draw_circle(center, center, 3, cyan)
        
    elif icon_type == 'map':
        # Two ranges with arrow
        draw_line(6, 10, 14, 10, blue, 2)
        draw_line(6, 14, 14, 14, blue, 2)
        draw_line(14, center, size - 14, center, white, 1)
        draw_line(size - 18, center - 3, size - 14, center, white, 1)
        draw_line(size - 18, center + 3, size - 14, center, white, 1)
        draw_line(size - 14, size - 14, size - 6, size - 14, green, 2)
        draw_line(size - 14, size - 10, size - 6, size - 10, green, 2)
        
    elif icon_type == 'const':
        # Number "1"
        draw_line(center - 4, 8, center, 6, yellow, 2)
        draw_line(center, 6, center, size - 8, yellow, 3)
        draw_line(center - 6, size - 8, center + 6, size - 8, yellow, 2)
        
    elif icon_type == 'noise':
        # Random dots
        import random
        random.seed(42)  # Deterministic
        for _ in range(30):
            x = random.randint(6, size - 6)
            y = random.randint(6, size - 6)
            c = random.randint(100, 255)
            draw_circle(x, y, 1, [c, c, c, 255])
        
    elif icon_type == 'lfo':
        # Triangle wave (oscillator)
        for x in range(4, size - 4):
            t = (x - 4) / (size - 8)
            if t < 0.25:
                y = int(center - 8 * (t * 4))
            elif t < 0.75:
                y = int(center - 8 + 16 * ((t - 0.25) * 2))
            else:
                y = int(center + 8 - 8 * ((t - 0.75) * 4))
            for dy in range(-1, 2):
                set_pixel(x, y + dy, purple)
                
    elif icon_type == 'lerp':
        # Gradient bar
        for x in range(6, size - 6):
            t = (x - 6) / (size - 12)
            for y in range(center - 4, center + 4):
                r = int(100 + 155 * t)
                g = int(100 + 100 * (1 - t))
                set_pixel(x, y, [r, g, 150, 255])
        draw_circle(8, center, 3, blue)
        draw_circle(size - 8, center, 3, red)
        
    elif icon_type == 'smooth':
        # S-curve
        import math
        for x in range(4, size - 4):
            t = (x - 4) / (size - 8)
            s = t * t * (3 - 2 * t)  # Smoothstep
            y = int(center + 10 - 20 * s)
            for dy in range(-1, 2):
                set_pixel(x, y + dy, cyan)
                
    elif icon_type == 'step':
        # Step function
        draw_line(4, size - 8, center, size - 8, green, 2)
        draw_line(center, size - 8, center, 8, green, 2)
        draw_line(center, 8, size - 4, 8, green, 2)
        
    elif icon_type == 'pulse':
        # Pulse spike
        draw_line(4, size - 8, center - 4, size - 8, orange, 2)
        draw_line(center - 4, size - 8, center - 4, 8, orange, 2)
        draw_line(center - 4, 8, center + 4, 8, orange, 2)
        draw_line(center + 4, 8, center + 4, size - 8, orange, 2)
        draw_line(center + 4, size - 8, size - 4, size - 8, orange, 2)
        
    elif icon_type == 'hold':
        # Sample and hold staircase
        draw_line(4, 8, 10, 8, cyan, 2)
        draw_line(10, 8, 10, 14, cyan, 2)
        draw_line(10, 14, 16, 14, cyan, 2)
        draw_line(16, 14, 16, size - 14, cyan, 2)
        draw_line(16, size - 14, 22, size - 14, cyan, 2)
        draw_line(22, size - 14, 22, size - 8, cyan, 2)
        draw_line(22, size - 8, size - 4, size - 8, cyan, 2)
        
    elif icon_type == 'delay':
        # Clock with delay arrow
        draw_circle(center, center, 10, gray)
        draw_circle(center, center, 8, [40, 40, 50, 255])
        draw_line(center, center, center + 5, center - 3, white, 2)
        # Curved arrow
        import math
        for i in range(12):
            angle = 0.5 + i * 0.15
            x = int(center + 12 * math.cos(angle))
            y = int(center + 12 * math.sin(angle))
            set_pixel(x, y, yellow)
        
    elif icon_type == 'compare':
        # < > = signs
        draw_line(8, 10, 14, center, white, 2)
        draw_line(8, size - 10, 14, center, white, 2)
        draw_line(size - 8, 10, size - 14, center, white, 2)
        draw_line(size - 8, size - 10, size - 14, center, white, 2)
        
    elif icon_type == 'select':
        # Switch/mux symbol
        draw_line(6, 10, 14, 10, blue, 2)
        draw_line(6, size - 10, 14, size - 10, green, 2)
        draw_line(14, center, size - 6, center, white, 2)
        draw_circle(14, center, 3, yellow)
        
    elif icon_type == 'gate':
        # Gate symbol
        draw_rect(8, 8, size - 8, size - 8, [40, 40, 60, 255])
        draw_line(8, 8, 8, size - 8, gray, 2)
        draw_line(size - 8, 8, size - 8, size - 8, gray, 2)
        draw_line(8, 8, size - 8, 8, gray, 2)
        draw_line(8, size - 8, size - 8, size - 8, gray, 2)
        # Opening
        draw_line(center - 3, size - 8, center + 3, size - 8, [40, 40, 60, 255], 3)
        draw_line(center, size - 8, center, size - 4, green, 2)
        
    elif icon_type == 'split':
        # Split arrow
        draw_line(6, center, center, center, white, 2)
        draw_line(center, center, size - 8, 10, blue, 2)
        draw_line(center, center, size - 8, size - 10, green, 2)
        draw_circle(center, center, 3, white)
        
    elif icon_type == 'combine':
        # Combine arrows
        draw_line(8, 10, center, center, blue, 2)
        draw_line(8, size - 10, center, center, green, 2)
        draw_line(center, center, size - 6, center, white, 2)
        draw_circle(center, center, 3, white)
        
    elif icon_type == 'colorize':
        # RGB circles
        draw_circle(center - 5, center - 3, 6, [255, 80, 80, 200])
        draw_circle(center + 5, center - 3, 6, [80, 255, 80, 200])
        draw_circle(center, center + 5, 6, [80, 80, 255, 200])
        
    elif icon_type == 'hsv':
        # Color wheel
        import math
        for y in range(6, size - 6):
            for x in range(6, size - 6):
                dx = x - center
                dy = y - center
                dist = (dx*dx + dy*dy) ** 0.5
                if dist < 11 and dist > 4:
                    angle = math.atan2(dy, dx)
                    h = (angle + math.pi) / (2 * math.pi)
                    # Simple HSV to RGB
                    i = int(h * 6)
                    f = h * 6 - i
                    if i == 0: r, g, b = 1, f, 0
                    elif i == 1: r, g, b = 1-f, 1, 0
                    elif i == 2: r, g, b = 0, 1, f
                    elif i == 3: r, g, b = 0, 1-f, 1
                    elif i == 4: r, g, b = f, 0, 1
                    else: r, g, b = 1, 0, 1-f
                    set_pixel(x, y, [int(r*255), int(g*255), int(b*255), 255])
                    
    elif icon_type == 'gradient':
        # Multi-color gradient bar
        colors = [red, yellow, green, cyan, blue, purple]
        for x in range(6, size - 6):
            t = (x - 6) / (size - 12) * (len(colors) - 1)
            i = int(t)
            f = t - i
            if i >= len(colors) - 1:
                i = len(colors) - 2
                f = 1.0
            c1, c2 = colors[i], colors[i+1]
            r = int(c1[0] * (1-f) + c2[0] * f)
            g = int(c1[1] * (1-f) + c2[1] * f)
            b = int(c1[2] * (1-f) + c2[2] * f)
            for y in range(center - 5, center + 5):
                set_pixel(x, y, [r, g, b, 255])
                
    elif icon_type == 'transform2d':
        # Rotation/transform arrows
        draw_rect(10, 10, size - 10, size - 10, [40, 40, 60, 200])
        import math
        for i in range(16):
            angle = i * math.pi / 8
            x = int(center + 10 * math.cos(angle))
            y = int(center + 10 * math.sin(angle))
            set_pixel(x, y, cyan)
        draw_line(center, center, center + 6, center - 6, yellow, 2)
        
    elif icon_type == 'render':
        # Screen/display icon
        for y in range(6, size - 10):
            for x in range(6, size - 6):
                set_pixel(x, y, [60, 60, 80, 255])
        for x in range(8, size - 8):
            set_pixel(x, 8, white)
        # Stand
        draw_line(center - 4, size - 10, center + 4, size - 10, gray, 2)
        draw_line(center, size - 10, center, size - 6, gray, 2)
        
    elif icon_type == 'render_circle':
        # Filled circle
        draw_circle(center, center, 10, [60, 60, 80, 255])
        draw_circle(center, center, 8, cyan)
        
    elif icon_type == 'render_line':
        # Diagonal line
        draw_rect(6, 6, size - 6, size - 6, [40, 40, 50, 200])
        draw_line(8, size - 8, size - 8, 8, yellow, 3)
        
    elif icon_type == 'pad':
        # Controller icon
        draw_circle(center - 6, center, 5, [80, 80, 100, 255])
        draw_circle(center + 6, center, 5, [80, 80, 100, 255])
        for x in range(center - 6, center + 7):
            for y in range(center - 3, center + 4):
                set_pixel(x, y, [80, 80, 100, 255])
        # Buttons
        set_pixel(center + 5, center - 2, red)
        set_pixel(center + 7, center, green)
        set_pixel(center + 5, center + 2, blue)
        set_pixel(center + 3, center, yellow)
        
    elif icon_type == 'debug':
        # Bug icon
        draw_circle(center, center, 8, [80, 60, 40, 255])
        draw_circle(center - 3, center - 3, 2, white)
        draw_circle(center + 3, center - 3, 2, white)
        # Antennae
        draw_line(center - 4, center - 8, center - 2, center - 5, gray, 1)
        draw_line(center + 4, center - 8, center + 2, center - 5, gray, 1)
        # Legs
        draw_line(center - 10, center - 2, center - 6, center, gray, 1)
        draw_line(center + 10, center - 2, center + 6, center, gray, 1)
        draw_line(center - 10, center + 4, center - 6, center + 2, gray, 1)
        draw_line(center + 10, center + 4, center + 6, center + 2, gray, 1)
        
    else:
        # Default: question mark
        draw_text('?', center - 4, center - 8, white, 3)
    
    return create_png(size, size, pixels)


def main():
    # Ensure output directories exist
    os.makedirs('../assets/ui', exist_ok=True)
    os.makedirs('../assets/icons', exist_ok=True)
    
    print("Generating UI assets...")
    
    # Cursor
    with open('../assets/ui/cursor.png', 'wb') as f:
        f.write(create_cursor(32))
    print("  Created cursor.png (32x32)")
    
    # Port dots - input (blue) and output (orange)
    with open('../assets/ui/port_in.png', 'wb') as f:
        f.write(create_port_dot(16, (100, 150, 255)))
    print("  Created port_in.png (16x16)")
    
    with open('../assets/ui/port_out.png', 'wb') as f:
        f.write(create_port_dot(16, (255, 150, 50)))
    print("  Created port_out.png (16x16)")
    
    with open('../assets/ui/port_dot.png', 'wb') as f:
        f.write(create_port_dot(16, (200, 200, 200)))
    print("  Created port_dot.png (16x16)")
    
    # Node background
    with open('../assets/ui/node_bg.png', 'wb') as f:
        f.write(create_node_background(128, 64))
    print("  Created node_bg.png (128x64)")
    
    # Node icons - ALL node types
    icon_types = [
        # Sources
        'const', 'time', 'pad', 'noise', 'lfo',
        # Math
        'add', 'sub', 'mul', 'div', 'mod', 'abs', 'neg', 'min', 'max', 'clamp', 'map',
        # Trigonometry
        'sin', 'cos', 'tan', 'atan2',
        # Filters
        'lerp', 'smooth', 'step', 'pulse', 'hold', 'delay',
        # Logic/Comparison
        'compare', 'select', 'gate',
        # Vector/Color
        'split', 'combine', 'colorize', 'hsv', 'gradient',
        # Transform
        'transform2d',
        # Sinks/Render
        'render', 'render_circle', 'render_line',
        # Utility
        'debug'
    ]
    
    for icon_type in icon_types:
        with open(f'../assets/icons/node_{icon_type}.png', 'wb') as f:
            f.write(create_node_icon(icon_type, 32))
        print(f"  Created node_{icon_type}.png (32x32)")
    
    print(f"\nDone! Created {len(icon_types)} node icons plus UI assets.")
    print("Assets in: assets/ui/ and assets/icons/")


if __name__ == '__main__':
    main()
