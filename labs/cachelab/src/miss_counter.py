import argparse
from typing import Optional


def addr_to_index(
    base: int, rows: int, cols: int, addr: int
) -> Optional[tuple[int, int]]:
    offset = addr - base
    if offset % 4 != 0:
        return None
    index_1d = offset // 4
    y = index_1d // cols
    x = index_1d % cols
    if y >= rows or y < 0:
        return None
    return (y, x)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("filename")
    parser.add_argument("m")
    parser.add_argument("n")
    parser.add_argument("a_base")
    parser.add_argument("b_base")
    args = parser.parse_args()

    filename = args.filename
    m = int(args.m)
    n = int(args.n)
    a_base = int(args.a_base, 16)
    b_base = int(args.b_base, 16)

    miss_result_a = []
    miss_result_b = []

    for _ in range(n // 8):
        miss_result_a.append([0] * (m // 8))
    for _ in range(m // 8):
        miss_result_b.append([0] * (n // 8))

    with open(filename) as f:
        for line in f:
            if line[0] not in "LSM":
                continue

            op, addr_size, *verdict = line.split()
            addr = int(addr_size.split(",")[0], 16)
            if "miss" not in verdict:
                continue

            if op == "L":
                idx = addr_to_index(a_base, n, m, addr)
            else:
                idx = addr_to_index(b_base, m, n, addr)

            if idx is None:
                continue
            y, x = idx

            if op == "L":
                miss_result_a[y // 8][x // 8] += 1
            else:
                miss_result_b[y // 8][x // 8] += 1

    print("Matrix A:")
    for line in miss_result_a:
        print(*line)
    print("\nMatrix B:")
    for line in miss_result_b:
        print(*line)
