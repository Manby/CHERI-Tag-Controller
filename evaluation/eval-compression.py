import argparse
from utils import *
from compression import doRun as doRunCompression
from stats import doRun as doRunStats

parser = argparse.ArgumentParser()
parser.add_argument('-l', dest='workload_list', type=str, required=True)
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-d', dest='divisions', type=str, required=False, default="10")
parser.add_argument('-s', dest='skip', type=str, required=False, default="0")

parsed_args = parser.parse_args()

workloads = getWorkloads(parsed_args.workload_list)

stats = {}
save_name = "saves/eval-compression-"+getTimestamp()+".json"
skip = int(parsed_args.skip)

schemes = ["baseline", "phoenix-8f", "phoenix-4t", "phoenix-4f"]

print("~~~~~ Configuring ChampSim ~~~~~\n")
champsimConfig(128, 4, None)        # 4-way 32KiB Cache
print("\n\n")

for workload in workloads:
    curr_workload_stats = {}
    initial_state = workload[1]+'/trace_initial_state.bin'
    llc_requests = workload[1]+'/trace_llc_requests.gz'

    print("******************  USING WORKLOAD: " + workload[0] + "  ******************")
    for scheme in schemes:
        if skip > 0:
            skip -= 1
            continue

        print("##########  EMULATING SCHEME: " + scheme + "  ##########")
        if scheme == "baseline":
            curr_workload_stats[scheme] = doRunCompression(scheme, initial_state, llc_requests,
                                                int(parsed_args.n),
                                                int(parsed_args.divisions))
            stats[workload[0]] = curr_workload_stats
            save(stats, save_name)

        else:
            curr_workload_stats[scheme] = doRunStats(scheme, initial_state, llc_requests,
                                                int(parsed_args.n), 0, True)

            stats[workload[0]] = curr_workload_stats
            save(stats, save_name)

print(stats)
