import struct

def create_wrestler_sheet(filename):
    # 6 Frames, 32x32 each.
    # Total Width: 192, Height: 32
    width = 192
    height = 32
    
    row_width_bytes = (width + 1) // 2
    padding = (4 - (row_width_bytes % 4)) % 4
    data_size = (row_width_bytes + padding) * height
    offset = 14 + 40 + (16 * 4)
    file_size = offset + data_size
    
    with open(filename, 'wb') as f:
        # File Header
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(b'\x00\x00\x00\x00')
        f.write(struct.pack('<I', offset))
        
        # Info Header
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<I', width))
        f.write(struct.pack('<I', height))
        f.write(struct.pack('<H', 1))
        f.write(struct.pack('<H', 4)) # 4-bit
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', data_size))
        f.write(struct.pack('<I', 2835))
        f.write(struct.pack('<I', 2835))
        f.write(struct.pack('<I', 16))
        f.write(struct.pack('<I', 0))
        
        # Palette
        # 0: Transparent (Black)
        f.write(b'\x00\x00\x00\x00')
        # 1: Skin (Peach-ish) -> let's use White for visibility
        f.write(b'\xFF\xFF\xFF\x00')
        # 2: Trunks (Green)
        f.write(b'\x00\xFF\x00\x00') 
        # 3: Boots (Red)
        f.write(b'\x00\x00\xFF\x00')
        # 4: Black (Outline/Eyes) -> Actually let's make it dark gray
        f.write(b'\x33\x33\x33\x00')
        
        for i in range(11):
            f.write(b'\x00\x00\x00\x00')
            
        # Pixel Data
        # BMP is stored Bottom-Up usually.
        
        # We will generate row by row.
        for y in range(height):
            row = bytearray()
            # We are iterating 0..191
            # Since we pack 2 pixels per byte, we step by 2.
            for x in range(0, width, 2):
                
                def get_color(px, py):
                    # Determine which frame we are in
                    frame = px // 32
                    local_x = px % 32
                    local_y = py # BMP is bottom up, so 0 is bottom
                    
                    # Invert y for drawing logic (0=top)
                    draw_y = 31 - local_y
                    
                    # Default transparent
                    col = 0
                    
                    # Shared Body Shape (Rect 8,4 to 24,28)
                    if 8 <= local_x <= 24 and 4 <= draw_y <= 28:
                        col = 1 # Skin
                    
                    # Trunks (10,16 to 22,22)
                    if 8 <= local_x <= 24 and 18 <= draw_y <= 22:
                        col = 2 # Green
                        
                    # Frame Specifics
                    if frame == 0: # IDLE
                        # Eyes
                        if draw_y == 8 and (local_x == 12 or local_x == 20):
                            col = 4
                            
                    elif frame == 1: # WALK 1
                        # Left Leg up
                        if draw_y > 22 and local_x < 16:
                             col = 0 # cut leg
                             
                    elif frame == 2: # WALK 2
                         # Right Leg up
                        if draw_y > 22 and local_x > 16:
                             col = 0 # cut leg

                    elif frame == 3: # GRAPPLE
                        # Arms out (Red)
                        if 4 <= draw_y <= 14 and (local_x < 8 or local_x > 24):
                            col = 1
                            
                    elif frame == 4: # THROWN
                        # Upside down roughly (just flip colors?)
                        # Let's draw an X
                        if local_x == draw_y or local_x == (31-draw_y):
                            col = 3
                        else:
                            col = 0

                    elif frame == 5: # GROUNDED
                        # Flat on ground (bottom 8 pixels)
                        if draw_y > 24:
                            col = 1
                        else:
                            col = 0
                            
                    return col

                c1 = get_color(x, y)
                c2 = get_color(x+1, y)
                
                byte = (c1 << 4) | c2
                row.append(byte)
                
            f.write(row)
            f.write(b'\x00' * padding)

create_wrestler_sheet('res/wrestler_sheet.bmp')
print("Created res/wrestler_sheet.bmp")



