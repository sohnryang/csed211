import argparse


def matrix_offset(cols: int, y: int, x: int) -> int:
    return cols * y + x


def cache_set_id(addr: int) -> int:
    return (addr >> 5) & 0b11111


def collect_sets(a_base: int, b_base: int, m: int, n: int) -> list[list[int]]:
    overlap_grid = []
    for y in range(0, n, 8):
        line = []
        for x in range(0, m, 8):
            a_sets = [
                cache_set_id(a_base + 4 * matrix_offset(m, y + i, x)) for i in range(8)
            ]
            b_sets = [
                cache_set_id(b_base + 4 * matrix_offset(n, x + i, y)) for i in range(8)
            ]
            line.append((a_sets, b_sets))
        overlap_grid.append(line)
    return overlap_grid


def collect_sets_single(
    a_base: int, b_base: int, m: int, n: int
) -> tuple[list[list[int]], list[list[int]]]:
    a_cachesets = []
    b_cachesets = []
    for y in range(n):
        a_cachesets.append(
            [cache_set_id(a_base + 4 * matrix_offset(m, y, x)) for x in range(m)]
        )
    for x in range(m):
        b_cachesets.append(
            [cache_set_id(b_base + 4 * matrix_offset(n, x, y)) for y in range(n)]
        )
    return a_cachesets, b_cachesets


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("m", type=int)
    parser.add_argument("n", type=int)
    parser.add_argument("a_base")
    parser.add_argument("b_base")

    args = parser.parse_args()
    m = args.m
    n = args.n
    a_base = int(args.a_base, 16)
    b_base = int(args.b_base, 16)

    a_cachesets, b_cachesets = collect_sets_single(a_base, b_base, m, n)
    print("Matrix A:")
    for line in a_cachesets:
        print(*[f"{x:2d}" for x in line])

    print("\nMatrix B:")
    for line in b_cachesets:
        print(*[f"{x:2d}" for x in line])
