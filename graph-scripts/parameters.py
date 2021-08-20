fb_params = {
    "SAMPLE_FACTOR": 100,
    "LAYERS": 3,
    "MAX_FLASH_CAP": 6666,
    "WRITE_RATE_LIMIT": 62.5,
    "AVG_OBJ_SIZE": 291,
    "DRAM_CUTOFF_GB": 16,
}

DRAM_OVERHEAD_BITS = 30
DWPD = 3
GRAPH_SIZE = (5, 4.5)

DEV_WA = [
    (0, 0),
    (.25, 1.143322),
    (.50, 1.238678),
    (.66, 1.593991),
    (.80, 2.087966),
    (.90, 3.382906),
    (.99, 9.551039),
    (float('inf'), 9.551039),
]

colors = {
    'Kangaroo': 'tab:orange',
    'SA': (0.0, 0.0, 0.5, 1.0),
    'LS': (0.0, 0.8333333333333334, 1.0, 1.0),
}

def get_color(label):
    return colors[label]

def get_dev_wr(wr, flash_per):
    ind = next(i - 1 for i, val in enumerate(DEV_WA) if flash_per < val[0])
    if (ind < 0): ind = 0
    if ind + 1 >= len(DEV_WA):
        slope = 0
    else:
        slope = (DEV_WA[ind + 1][1] - DEV_WA[ind][1]) / (DEV_WA[ind + 1][0] - DEV_WA[ind][0])
    wa =  DEV_WA[ind][1] + (slope * (flash_per - DEV_WA[ind][0]))
    return wa * wr
