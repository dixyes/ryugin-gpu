
import math
import struct as st

f = open("/dev/fb0", "wb")
for y in range(600):
    for x in range(800):
        r = int(128 * math.cos((y * 800 + x) / math.pi / 250 )) + 127
        b = int(128 * math.sin((y * 800 + x) / math.pi / 250 )) + 127
        f.write(st.pack("BBBB", b, 255, r, 0))

