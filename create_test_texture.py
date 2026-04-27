#!/usr/bin/env python3
"""
Basit bir test texture oluşturur (checkerboard pattern)
"""

try:
    from PIL import Image
except ImportError:
    print("PIL/Pillow yüklü değil. Yükleniyor...")
    import subprocess
    subprocess.check_call(["pip", "install", "pillow"])
    from PIL import Image

# 256x256 checkerboard texture
size = 256
img = Image.new('RGB', (size, size))
pixels = img.load()

# Checkerboard pattern (8x8 kareler)
square_size = 32
for y in range(size):
    for x in range(size):
        # Hangi karede olduğumuzu hesapla
        square_x = x // square_size
        square_y = y // square_size
        
        # Satranç tahtası deseni
        if (square_x + square_y) % 2 == 0:
            pixels[x, y] = (255, 100, 50)  # Turuncu
        else:
            pixels[x, y] = (50, 150, 255)  # Mavi

img.save('assets/textures/test_checkerboard.png')
print("✓ Test texture oluşturuldu: assets/textures/test_checkerboard.png")
