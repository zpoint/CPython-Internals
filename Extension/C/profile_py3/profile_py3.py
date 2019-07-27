import line_profiler
import atexit
profile = line_profiler.LineProfiler()
atexit.register(profile.print_stats)

meaningless_dict = {chr(i) if i < 256 else i: i for i in range(500)}


@profile
def my_cpu_bound_task(x, y):
    """
    meaningless code
    """
    r = bytearray(1000)
    for i in range(1000):
        if i in meaningless_dict.keys():
            y += 1
        x += i
        y = (x % 256) ^ (y % 256)
        r[i] = y
    return bytes(r)


@profile
def run():
    for i in range(9999):
        my_cpu_bound_task(i, i+1)


if __name__ == "__main__":
    run()
