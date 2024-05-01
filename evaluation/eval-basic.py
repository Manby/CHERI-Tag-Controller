import argparse
from utils import *
from stats import doRun

parser = argparse.ArgumentParser()
parser.add_argument('-l', dest='workload_list', type=str, required=True)
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-w', dest='warmup', type=str, required=False, default="0.1")
parser.add_argument('-m', dest='morello', type=str, required=False, default="no")

parsed_args = parser.parse_args()

workloads = getWorkloads(parsed_args.workload_list)

stats = {}

if parsed_args.morello == "yes":
    schemes = ["morello-t", "morello-z"]

    print("~~~~~ Configuring ChampSim ~~~~~\n")
    champsimConfig(128, 4, None)        # 4-way 32KiB Cache
    print("\n\n")

else:
    schemes = ["baseline", "etm", "phoenix-8t"]

    print("~~~~~ Configuring ChampSim ~~~~~\n")
    champsimConfig(128, 4, None)        # 4-way 32KiB Cache
    print("\n\n")

for workload in workloads:
    curr_workload_stats = {}
    initial_state = workload[1]+'/trace_initial_state.bin'
    llc_requests = workload[1]+'/trace_llc_requests.gz'

    print("******************  USING WORKLOAD: " + workload[0] + "  ******************")
    for scheme in schemes:
        if scheme == "morello-z":
            # update the cache parameters
            print("~~~~~ Reconfiguring ChampSim ~~~~~\n")
            champsimConfig(128, 4, None)        # 4-way 32KiB Cache
            print("\n\n")

        print("##########  EMULATING SCHEME: " + scheme + "  ##########")
        curr_workload_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                            int(parsed_args.n),
                                            float(parsed_args.warmup))

    stats[workload[0]] = curr_workload_stats

print(stats)

save(stats, "saves/stats-"+getTimestamp()+".json")
