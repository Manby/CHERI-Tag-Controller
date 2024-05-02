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

    cache_params_t = [
            (64, 4, 'ip_stride'),
            (64, 4, 'next_line'),
            #(64, 4, 'va_ampm_lite')
            ]
    cache_params_z = [
            (64, 4, 'ip_stride'),
            (64, 4, 'next_line'),
            #(64, 4, 'va_ampm_lite')
            ]

else:
    schemes = ["baseline", "etm", "phoenix-8t"]

    cache_params = [
            (128, 4, 'ip_stride'),
            (128, 4, 'next_line'),
            #(128, 4, 'va_ampm_lite')
            ]

for workload in workloads:
    curr_workload_stats = {}
    initial_state = workload[1]+'/trace_initial_state.bin'
    llc_requests = workload[1]+'/trace_llc_requests.gz'

    print("******************  USING WORKLOAD: " + workload[0] + "  ******************")


    if parsed_args.morello == "yes":
        curr_params_stats = {}
        for params_t, params_z in zip(cache_params_t, cache_params_z):
            # first do tag cache
            # update the cache parameters
            print("~~~~~ Reconfiguring ChampSim ~~~~~\n")
            champsimConfig(*params_t)
            print("\n\n")

            scheme = "morello-t"
            print("##########  EMULATING SCHEME: " + scheme + "  ##########")
            curr_params_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                                int(parsed_args.n),
                                                float(parsed_args.warmup))

            # next do zero cache
            # update the cache parameters
            print("~~~~~ Reconfiguring ChampSim ~~~~~\n")
            champsimConfig(*params_z)
            print("\n\n")
            scheme = "morello-z"
            print("##########  EMULATING SCHEME: " + scheme + "  ##########")
            curr_params_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                                int(parsed_args.n),
                                                float(parsed_args.warmup))

            curr_workload_stats[str((params_t, params_z))] = curr_params_stats

        stats[workload[0]] = curr_workload_stats

    else:
        curr_params_stats = {}
        for params in cache_params:
            # update the cache parameters
            print("~~~~~ Reconfiguring ChampSim ~~~~~\n")
            champsimConfig(*params)
            print("\n\n")

            for scheme in schemes:
                print("##########  EMULATING SCHEME: " + scheme + "  ##########")
                curr_params_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                                    int(parsed_args.n),
                                                    float(parsed_args.warmup))

            curr_workload_stats[str(params)] = curr_params_stats

        stats[workload[0]] = curr_workload_stats

print(stats)

save(stats, "saves/eval-prefetcher-"+getTimestamp()+".json")
