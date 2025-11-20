import struct

def create_16color_bmp(filename, width, height):
    # File Header (14 bytes)
    # BM (2), Size (4), Reserved (4), Offset (4)
    # Info Header (40 bytes)
    # Size (4), Width (4), Height (4), Planes (2), BitCount (2)
    # Compression (4), SizeImage (4), XRes (4), YRes (4), ColorsUsed (4), ColorsImportant (4)
    
    # Palette (16 * 4 bytes)
    # BGRA
    
    # Pixel Data (width/2 * height bytes for 4-bit)
    # Rows are padded to 4-byte boundary
    
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
        f.write(struct.pack('<I', 40)) # Header Size
        f.write(struct.pack('<I', width))
        f.write(struct.pack('<I', height))
        f.write(struct.pack('<H', 1)) # Planes
        f.write(struct.pack('<H', 4)) # BitCount (4-bit)
        f.write(struct.pack('<I', 0)) # Compression (BI_RGB)
        f.write(struct.pack('<I', data_size))
        f.write(struct.pack('<I', 2835)) # XPelsPerMeter
        f.write(struct.pack('<I', 2835)) # YPelsPerMeter
        f.write(struct.pack('<I', 16)) # Colors Used
        f.write(struct.pack('<I', 0)) # Important Colors
        
        # Palette (16 colors: Blue, Green, Red, Alpha)
        # Index 0: Transparent (usually magenta or black)
        f.write(b'\x00\x00\x00\x00') # 0: Black
        f.write(b'\x00\x00\xFF\x00') # 1: Red
        f.write(b'\x00\xFF\x00\x00') # 2: Green
        f.write(b'\xFF\x00\x00\x00') # 3: Blue
        f.write(b'\xFF\xFF\xFF\x00') # 4: White
        for i in range(11):
            f.write(b'\x00\x00\x00\x00') # Rest black
            
        # Pixel Data (Bottom-up)
        # Draw a box
        for y in range(height):
            row = bytearray()
            for x in range(0, width, 2):
                # Combine two pixels into one byte
                # Pixel 1
                c1 = 1 # Red
                if x > 2 and x < width-3 and y > 2 and y < height-3:
                    c1 = 2 # Green body
                
                # Pixel 2
                c2 = 1 # Red
                if (x+1) > 2 and (x+1) < width-3 and y > 2 and y < height-3:
                    c2 = 2 # Green body
                
                # Pack: High nibble is first pixel, Low nibble is second
                byte = (c1 << 4) | c2
                row.append(byte)
            
            f.write(row)
            f.write(b'\x00' * padding)

create_16color_bmp('res/wrestler.bmp', 32, 32)
print("Created res/wrestler.bmp")



