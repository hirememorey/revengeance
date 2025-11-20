import struct

def create_ring_bg(filename):
    # We need a 320x224 background.
    # However, Genesis backgrounds are tilemaps (grids of 8x8 tiles).
    # SGDK can import a large image and convert it to tiles + map.
    
    width = 320
    height = 224
    
    # BMP Header
    row_width_bytes = (width + 1) // 2
    padding = (4 - (row_width_bytes % 4)) % 4
    data_size = (row_width_bytes + padding) * height
    offset = 14 + 40 + (16 * 4)
    file_size = offset + data_size
    
    with open(filename, 'wb') as f:
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(b'\x00\x00\x00\x00')
        f.write(struct.pack('<I', offset))
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<I', width))
        f.write(struct.pack('<I', height))
        f.write(struct.pack('<H', 1))
        f.write(struct.pack('<H', 4)) 
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', data_size))
        f.write(struct.pack('<I', 2835))
        f.write(struct.pack('<I', 2835))
        f.write(struct.pack('<I', 16))
        f.write(struct.pack('<I', 0))
        
        # Palette (16 colors)
        # 0: Transparent/Black
        f.write(b'\x00\x00\x00\x00')
        # 1: Mat Blue
        f.write(b'\xCC\x00\x00\x00') # Blue
        # 2: Rope White
        f.write(b'\xFF\xFF\xFF\x00')
        # 3: Post Red
        f.write(b'\x00\x00\xFF\x00')
        # 4: Crowd Dark
        f.write(b'\x20\x20\x20\x00')
        # 5: Crowd Light
        f.write(b'\x50\x50\x50\x00')
        
        for i in range(10):
            f.write(b'\x00\x00\x00\x00')
            
        # Pixel Data (Bottom Up)
        for y in range(height):
            row = bytearray()
            draw_y = height - 1 - y
            
            for x in range(0, width, 2):
                def get_col(px, py):
                    # Top Area: Crowd (0-40)
                    if py < 40:
                        # Noise pattern
                        if (px + py) % 5 == 0: return 5
                        return 4
                        
                    # Ring Posts (Left/Right)
                    if (px < 20 or px > 300) and py > 40:
                         if (px < 10 or px > 310): return 0 # Void
                         return 3 # Post Red
                         
                    # Ropes (Horizontal lines)
                    if py == 60 or py == 90 or py == 120:
                        return 2 # White
                        
                    # Mat (The rest)
                    if py > 150:
                        # Canvas texture
                        if (px + py) % 8 == 0: return 0 # darker
                        return 1 # Blue
                        
                    # Apron/Floor
                    return 0 
                    
                c1 = get_col(x, draw_y)
                c2 = get_col(x+1, draw_y)
                row.append((c1 << 4) | c2)
                
            f.write(row)
            f.write(b'\x00' * padding)

create_ring_bg('res/ring_bg.bmp')



