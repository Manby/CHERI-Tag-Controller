import argparse
from utils import *
from stats import doRun

parser = argparse.ArgumentParser()
parser.add_argument('-l', dest='workload_list', type=str, required=True)
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-w', dest='warmup', type=str, required=False, default="0.1")

parsed_args = parser.parse_args()

workloads = getWorkloads(parsed_args.workload_list)

print("~~~~~ Configuring ChampSim ~~~~~\n")
champsimConfig(, None)
print("\n\n")

stats = {}
schemes = ["baseline", "etm", "morello-t", "morello-z", "phoenix-8t"]

for workload in workloads:
    curr_workload_stats = {}
    initial_state = workload[1]+'/trace_initial_state.bin'
    llc_requests = workload[1]+'/trace_llc_requests.gz'

    print("******************  USING WORKLOAD: " + workload[0] + "  ******************")
    for scheme in schemes:
        print("##########  EMULATING SCHEME: " + scheme + "  ##########")
        curr_workload_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                            int(parsed_args.n),
                                            float(parsed_args.warmup))

    stats[workload[0]] = curr_workload_stats

print(stats)

save(stats, "saves/stats-"+getTimestamp()+".json")
