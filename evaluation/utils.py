import threading
import subprocess
import copy
import json
from datetime import datetime

PATH = "../.."

def runControllerSimulator(*args):
    outputList = []
    controller_simulator = threading.Thread(target=runControllerSimulatorInner,
                                            args=(outputList, *args))
    controller_simulator.start()
    controller_simulator.join()

    print(outputList[0])
    return outputList[0]


def runChampsim(*args):
    outputList = []
    champsim_simulator = threading.Thread(target=runChampsimInner,
                                            args=(outputList, *args))
    champsim_simulator.start()
    champsim_simulator.join()

    print(outputList[0])
    return outputList[0]

def runControllerSimulatorInner(outputList, *args):
    out = subprocess.check_output([PATH+'/CHERI-Tag-Controller/simulator/bin/cheri-tag-controller',
                    *args]).decode("utf-8")
    outputList.append(out)

def runChampsimInner(outputList, *args):
    out = subprocess.check_output(['bin/champsim',
                             *args]).decode("utf-8")
    outputList.append(out)


def getAccessLogpoints(controllerSimOut):
    access_logpoints = []
    for line in controllerSimOut.split('\n'):
        if "DUMP: " in line:
            access_logpoints.append(line[6:])

    return access_logpoints

def getNumAccesses(controllerSimOut):
    for line in controllerSimOut.split('\n'):
        if "Performed" in line:
            num_accesses = line[line.find("[")+1:line.find("]")]

    return num_accesses

def getControllerStats(controllerSimOut):
    stats = {}
    for line in controllerSimOut.split('\n'):
        if "REPORT" in line:
            if "DRAM" in line: continue

            parts = [x for x in line.split(" ") if x != ""]
            stats[parts[1].lower()] = int(parts[3])

    return stats

def getChampsimStats(champsimOut):
    stats = {}
    for line in champsimOut.split('\n'):
        if "cpu0_L1D" in line:
            if "AVERAGE" in line: continue

            parts = [x for x in line.split(" ") if x != ""]
            sub = {}
            i = 2
            while i < len(parts):
                name = parts[i][:-1].lower()
                i += 1
                count = int(parts[i])
                i += 1
                sub[name] = count

            stats[parts[1].lower()] = sub

    return stats

templateConfig = {
    "block_size": 64,

    "ooo_cpu": [
        {
            "ifetch_buffer_size":1,
            "decode_buffer_size":1,
            "dispatch_buffer_size":1,
            "rob_size": 1,
            "lq_size": 1,
            "sq_size": 1,
            "fetch_width": 1,
            "decode_width": 1,
            "dispatch_width": 1,
            "execute_width": 1,
            "lq_width": 1,
            "sq_width": 1,
            "retire_width": 1,
            "scheduler_size": 1,
            "L2C":{"name": "l2c_cache"},
            "PTW":{"lower_level": "l2c_cache"}
        }
    ],

    "L1I": {
        "sets": 64,
        "ways": 8
    },
    "L1D": {
        "sets": 64,
        "ways": 12,
        "rq_size": 1,
        "wq_size": 1,
        "pq_size": 1,
        "mshr_size": 1
    }
}

def champsimConfig(sets, ways, prefetcher):
    config = copy.deepcopy(templateConfig)

    config["L1D"]["sets"] = sets
    config["L1D"]["ways"] = ways

    if prefetcher is not None:
        config["L1D"]["prefetch_activate"] = "LOAD,PREFETCH"
        config["L1D"]["prefetcher"] = prefetcher

    with open("out/champsim-config.json", "w") as f:
        f.write(json.dumps(config))

    subprocess.run([PATH+"/ChampSim-instrumented/ChampSim/config.sh", "out/champsim-config.json"])

    subprocess.run(["make", "-C", PATH+"/ChampSim-instrumented/ChampSim/"])

def getTimestamp():
    fstring = "%Y-%m-%d-%H_%M_%S"
    now = datetime.now()
    return now.strftime(fstring)

def save(obj, path=None):
    if path == None:
        path = "saves/"+getTimestamp()+".json"

    with open(path, "w") as f:
        f.write(json.dumps(obj))
