import json

FBOBJ_TIME_START = 974271648
SAMPLING_FACTOR = 600

def load_file(filename):
    """ returns list of json objects, assumes file can fit in memory """
    with open(filename) as f:
        data = f.read()
        new_data = data.replace('}\n{', '},{')
        json_data = json.loads(rf'[{new_data}]')
    return json_data

def get_accesses(json_obj):
    if "global" not in json_obj:
        if "stores_requested" not in json_obj["log"]:
            return json_obj["sets"]["hits"] + json_obj["sets"]["misses"]
        return json_obj["log"]["hits"] + json_obj["log"]["misses"]
    if "accessesAfterFlush" in json_obj["global"]:
        return json_obj["global"]["accessesAfterFlush"]
    if "totalAccesses" not in json_obj["global"]:
        return 0
    return json_obj["global"]["totalAccesses"]

def get_hits(json_obj):
    return json_obj["global"]["hits"]

def get_misses(json_obj):
    if "global" not in json_obj:
        if 'set' in json_obj:
            return json_obj['sets']['misses']
        return json_obj["log"]["misses"]
    return json_obj["global"]["misses"]

def get_unique_bytes(json_obj):
    return json_obj["global"]["uniqueBytes"]

def get_admitted_all(json_obj):
    ret = 0
    for key in json_obj.keys():
        if "Admission" in key and "numAdmits" in json_obj[key]:
            ret += json_obj[key]["numAdmits"]
    return ret

def get_poss_admitted_all(json_obj):
    ret = 0
    for key in json_obj.keys():
        if "Admission" in key and "numPossibleAdmits" in json_obj[key]:
            ret += json_obj[key]["numPossibleAdmits"]
    return ret

def get_admitted_perc_pre_log(json_obj):
    num_admits = 0
    possible_admits = 0
    for key in json_obj.keys():
        if "preLogAdmission" in key and "numAdmits" in json_obj[key]:
            num_admits += json_obj[key]["numAdmits"]
            possible_admits += json_obj[key]["numPossibleAdmits"]
    if not possible_admits:
        return 1 # effectively admits everything
    return num_admits / possible_admits

def get_admitted_perc_pre_set(json_obj):
    num_admits = 0
    possible_admits = 0
    for key in json_obj.keys():
        if "preSetAdmission" in key and "numAdmits" in json_obj[key]:
            num_admits += json_obj[key]["numAdmits"]
            possible_admits += json_obj[key]["numPossibleAdmits"]
    if not possible_admits:
        return 1 # effectively admits everything
    return num_admits / possible_admits

def get_poss_admitted_all(json_obj):
    ret = 0
    for key in json_obj.keys():
        if "Admission" in key and "numPossibleAdmits" in json_obj[key]:
            ret += json_obj[key]["numPossibleAdmits"]
    return ret

def find_first_flush(json_list):
    if 'global' not in json_list:
        return 0, json_list[0]
    for i, json_obj in enumerate(json_list):
        if 'numStatFlushes' in json_obj['global']:
            return i, json_obj
    return 0, json_list[0]

def get_bytes_written(json_obj):
    bytes = 0
    for value in json_obj.values():
        if "bytes_written" in value:
            bytes += value["bytes_written"]
    return bytes

def get_bytes_written_log(json_obj):
    if 'log' in json_obj:
        return json_obj['log']['bytes_written']
    return 0

def get_bytes_written_log_readmit(json_obj):
    if 'log' in json_obj and 'bytes_readmitted' in json_obj['log']:
        return json_obj['log']['bytes_readmitted']
    return 0

def get_bytes_written_sets(json_obj):
    if 'sets' in json_obj:
        return json_obj['sets']['bytes_written']
    return 0

def get_bytes_requested(json_obj):
    bytes = 0
    for value in json_obj.values():
        if "stores_requested_bytes" in value:
            bytes += value["stores_requested_bytes"]
    return bytes

def get_bytes_requested_flash(json_obj, only_written = False):
    """Chooses log admission over set admission"""
    preLogAdmission = [key for key in json_obj.keys() if "preLog" in key]
    if preLogAdmission and not only_written:
        return json_obj[preLogAdmission[0]]["sizePossibleAdmits"]
    if "log" in json_obj and json_obj["log"]["stores_requested_bytes"]:
        return json_obj["log"]["stores_requested_bytes"]
    preSetAdmission = [key for key in json_obj.keys() if "preSet" in key and json_obj[key]]
    if preSetAdmission and not only_written:
        return json_obj[preSetAdmission[0]]["sizePossibleAdmits"]
    else:
        return json_obj["sets"]["stores_requested_bytes"]

def get_timestamp(json_obj):
    if 'global' not in json_obj:
        return (21 * 3600 * 24 * (get_accesses(json_obj) / 173691960))
    if 'timestamp' not in json_obj['global']:
        return FBOBJ_TIME_START
    return json_obj["global"]["timestamp"]

def get_miss_rate(json_obj):
    return get_misses(json_obj) / get_accesses(json_obj)

def get_misses_dram(json_obj):
    if "memCache" not in json_obj:
        return 0
    if 'misses' not in json_obj["memCache"]:
        return 0
    return json_obj["memCache"]["misses"]

def get_miss_rate_flash(json_obj):
    all_misses = get_misses(json_obj)
    dram_misses = get_misses_dram(json_obj)
    flash_hits = dram_misses - all_misses
    if not dram_misses:
        return get_miss_rate(json_obj)
    return 1 - (flash_hits / dram_misses)

def get_miss_rate_sets(json_obj):
    set_misses = json_obj["sets"]["misses"]
    set_hits = json_obj["sets"]["hits"]
    return set_misses / (set_hits + set_misses)

def get_miss_rate_in_between_outputs(new_json_obj, old_json_obj):
    miss_diff = get_misses(new_json_obj) - get_misses(old_json_obj)
    accesses_diff = get_accesses(new_json_obj) - get_accesses(old_json_obj)
    return miss_diff/accesses_diff

def get_miss_rate_in_between_outputs_flash(new_json_obj, old_json_obj):
    all_miss_diff = get_misses(new_json_obj) - get_misses(old_json_obj)
    dram_misses_diff = get_misses_dram(new_json_obj) - get_misses_dram(old_json_obj)
    flash_hits_diff = dram_misses_diff - all_miss_diff
    if not dram_misses_diff:
        return get_miss_rate_in_between_outputs(new_json_obj, old_json_obj)
    return 1 - (flash_hits_diff / dram_misses_diff)

def get_admit_per_in_between_outputs(new_json_obj, old_json_obj):
    admitted_diff = get_admitted_all(new_json_obj) - get_admitted_all(old_json_obj)
    pos_admitted_diff = get_poss_admitted_all(new_json_obj) - get_poss_admitted_all(old_json_obj)
    return admitted_diff / pos_admitted_diff

def get_hits_per_structure(json_obj):
    """ returns (totoal, mem, log, sets) """
    total = json_obj['global']['hits']
    memory = 0
    sets = 0
    if 'memCache' in json_obj:
        memory = json_obj['memCache']['hits']
    if 'sets' in json_obj:
        sets = json_obj['sets']['hits']
    return (total, memory, total - memory - sets, sets)

def get_total_size(json_obj):
    c = 0
    total = 0
    if 'memCache' in json_obj and 'current_size' in json_obj['memCache']:
        c += json_obj['memCache']['current_size']
        total += json_obj['memCache']['lruCacheCapacity']
    if 'sets' in json_obj and 'current_size' in json_obj['sets']:
        c += json_obj['sets']['current_size']
        total += json_obj['sets']['numSets'] * json_obj['sets']['setCapacity']
    if 'log' in json_obj and 'current_size' in json_obj['log']:
        c += json_obj['log']['current_size']
        total += json_obj['log']['logCapacity']
    return c, total

def get_num_evictions(json_obj):
    c = 0
    total = 0
    if 'sets' in json_obj and 'numEvictions' in json_obj['sets']:
        return json_obj['sets']['numEvictions']
    if 'log' in json_obj and 'numEvictions' in json_obj['log']:
        return json_obj['log']['numEvictions']
    return 0

def get_mbs(json_obj, warmup_end_obj):
    bytes = get_bytes_written(json_obj)
    mb = bytes * SAMPLING_FACTOR / (1024**2)

    warmup_start_time = get_timestamp(warmup_end_obj)
    end_time = get_timestamp(json_obj)
    total_time = end_time - warmup_start_time

    return mb / total_time

def get_mbs_mb(json_obj):
    return get_bytes_written(json_obj) / (1024 * 1024 * 7 * 24 * 60 * 60)
