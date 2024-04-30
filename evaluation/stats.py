import argparse
from utils import *

parser = argparse.ArgumentParser(description='Run a simulation and gather the metrics.')
parser.add_argument('-i', dest='initial_state', type=str, required=True)
parser.add_argument('-l', dest='llc_requests', type=str, required=True)
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-w', dest='warmup', type=str, required=False, default="0.1")

parsed_args = parser.parse_args()


def doRun(scheme, initial_state, llc_requests, n, warmup):
    print("\n===== Running tag controller simulator =====\n")
    controllerSimOut = runControllerSimulator(scheme, initial_state,
                                              llc_requests, "out/output_trace.gz",
                                              str(n))
    print('\n')


    controllerStats = getControllerStats(controllerSimOut)
    if (controllerStats): controllerStats = {'blocks': controllerStats}

    num_accesses = getNumAccesses(controllerSimOut)

    num_warmup = round(warmup * int(num_accesses))
    num_simulation = int(num_accesses) - num_warmup

    print("\n===== Running ChampSim =====\n")
    champsimOut = runChampsim("--warmup-instructions", str(num_warmup),
                          "--simulation-instructions", str(num_simulation),
                          "out/output_trace.gz")
    print('\n')

    champsimStats = getChampsimStats(champsimOut)

    controllerStats.update(champsimStats)

    return controllerStats

print("~~~~~ Configuring ChampSim ~~~~~\n")
champsimConfig(64, 12, None)
print()

stats = {}
#schemes = ["baseline", "etm", "morello-t", "morello-z"]
schemes = ["baseline", "etm", "phoenix"]
for scheme in schemes:
    print("##########  EMULATING SCHEME: " + scheme + "  ##########")
    stats[scheme] = doRun(scheme, parsed_args.initial_state, parsed_args.llc_requests,
          int(parsed_args.n), float(parsed_args.warmup))

print(stats)

save(stats, "saves/stats-"+getTimestamp()+".json")
