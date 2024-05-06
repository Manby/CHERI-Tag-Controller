import argparse
from utils import *
from stats import doRun

parser = argparse.ArgumentParser()
parser.add_argument('-l', dest='workload_list', type=str, required=True)
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-w', dest='warmup', type=str, required=False, default="0.1")
parser.add_argument('-m', dest='morello', type=str, required=False, default="no")
parser.add_argument('-s', dest='skip', type=str, required=False, default="0")

parsed_args = parser.parse_args()

workloads = getWorkloads(parsed_args.workload_list)

stats = {}
save_name = "saves/eval-ways-"+getTimestamp()+".json"
skip = int(parsed_args.skip)

if parsed_args.morello == "yes":
    schemes = ["morello-t", "morello-z"]

    cache_params_t = [
            (256, 1, None),
            (128, 2, None),
            (32, 8, None)
            ]
    cache_params_z = [
            (256, 1, None),
            (128, 2, None),
            (32, 8, None)
            ]

elif parsed_args.morello == "p":
    schemes = ["phoenixs-8t", "phoenix-8t"]

    cache_params = [
            (512, 1, None),
            (256, 2, None),
            (64, 8, None)
            ]

else:
    schemes = ["baseline", "etm", "phoenixs-8t"]

    cache_params = [
            (512, 1, None),
            (256, 2, None),
            (64, 8, None)
            ]

for workload in workloads:
    curr_workload_stats = {}
    initial_state = workload[1]+'/trace_initial_state.bin'
    llc_requests = workload[1]+'/trace_llc_requests.gz'

    print("******************  USING WORKLOAD: " + workload[0] + "  ******************")


    if parsed_args.morello == "yes":
        for params_t, params_z in zip(cache_params_t, cache_params_z):
            curr_params_stats = {}

            if skip > 0:
                skip -= 1
            else:
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

                curr_workload_stats[str((params_t, params_z))] = curr_params_stats
                stats[workload[0]] = curr_workload_stats
                save(stats, save_name)

            if skip > 0:
                skip -= 1
            else:
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
                save(stats, save_name)

    else:
        for params in cache_params:
            curr_params_stats = {}
            # update the cache parameters
            print("~~~~~ Reconfiguring ChampSim ~~~~~\n")
            champsimConfig(*params)
            print("\n\n")

            for scheme in schemes:
                if skip > 0:
                    skip -= 1
                    continue

                print("##########  EMULATING SCHEME: " + scheme + "  ##########")
                curr_params_stats[scheme] = doRun(scheme, initial_state, llc_requests,
                                                    int(parsed_args.n),
                                                    float(parsed_args.warmup))

                curr_workload_stats[str(params)] = curr_params_stats
                stats[workload[0]] = curr_workload_stats
                save(stats, save_name)

print(stats)
