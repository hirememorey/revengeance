import png

# Create a 32x32 image (4x4 tiles)
# Green box with a red outline
width = 32
height = 32
palette = [(0,0,0), (255,0,0), (0,255,0), (0,0,255)] # Transparent, Red, Green, Blue
rows = []

for y in range(height):
    row = []
    for x in range(width):
        if x == 0 or x == width-1 or y == 0 or y == height-1:
            row.append(1) # Red border
        else:
            row.append(2) # Green body
    rows.append(row)

f = open('res/wrestler_placeholder.png', 'wb')
w = png.Writer(width, height, palette=palette, bitdepth=2)
w.write(f, rows)
f.close()



