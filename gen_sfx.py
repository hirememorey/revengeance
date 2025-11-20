import math
import struct
import random

def create_impact_wav(filename):
    # Standard WAV header (44 byte) + PCM Data
    # We will generate a "White Noise" burst with exponential decay for an impact sound.
    
    sample_rate = 16000 # Genesis PCM is often low rate
    duration_secs = 0.25
    num_samples = int(sample_rate * duration_secs)
    
    data = bytearray()
    
    for i in range(num_samples):
        # White noise: random -1.0 to 1.0
        val = random.uniform(-1.0, 1.0)
        
        # Decay: Linear or Exponential
        decay = 1.0 - (i / num_samples)
        val *= decay * decay # quadratic decay for sharp hit
        
        # Convert to 8-bit unsigned (0-255) which SGDK often likes,
        # OR 8-bit signed (-128 to 127). Standard WAV 8-bit is Unsigned.
        # 128 is silence.
        
        byte_val = int((val * 127) + 128)
        if byte_val < 0: byte_val = 0
        if byte_val > 255: byte_val = 255
        
        data.append(byte_val)
        
    # Write WAV File
    with open(filename, 'wb') as f:
        # RIFF Header
        f.write(b'RIFF')
        file_size = 36 + len(data)
        f.write(struct.pack('<I', file_size))
        f.write(b'WAVE')
        
        # fmt chunk
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16)) # Chunk size
        f.write(struct.pack('<H', 1)) # PCM
        f.write(struct.pack('<H', 1)) # Mono
        f.write(struct.pack('<I', sample_rate))
        f.write(struct.pack('<I', sample_rate)) # Byte Rate (SampleRate * NumChannels * BitsPerSample/8)
        f.write(struct.pack('<H', 1)) # Block Align
        f.write(struct.pack('<H', 8)) # Bits Per Sample
        
        # data chunk
        f.write(b'data')
        f.write(struct.pack('<I', len(data)))
        f.write(data)

create_impact_wav('res/sfx_hit.wav')
print("Created res/sfx_hit.wav")



