import argparse
import os
from utils import *

def gzipAll(paths):
    compressed_sizes = []
    for p in paths:
        subprocess.run(["gzip", "-fk", p])
        compressed_sizes.append(os.path.getsize(p+".gz"))

    return compressed_sizes

def bzip2All(paths):
    compressed_sizes = []
    for p in paths:
        subprocess.run(["bzip2", "-fk", p])
        compressed_sizes.append(os.path.getsize(p+".bz2"))

    return compressed_sizes

def zipAll(paths):
    compressed_sizes = []
    for p in paths:
        subprocess.run(["zip", p+".zip", p], stdout=subprocess.DEVNULL)
        compressed_sizes.append(os.path.getsize(p+".zip"))

    return compressed_sizes

def chopUpTableLogs(divisions):
    size_all = os.path.getsize("out/controller_log")
    size_one = size_all // divisions

    with open("out/controller_log", "rb") as all:
        paths = []

        for i in range(divisions):
            curr = "out/controller_log_"+str(i)
            paths.append(curr)
            with open(curr, "wb") as one:
                raw = all.read(size_one)
                one.write(raw)

    return size_one, paths

def doRun(scheme, initial_state, llc_requests, n, divisions):
    logpoints = [str(int(i/divisions * n) - 1) for i in range(1,divisions+1)]

    print("\n===== Running tag controller simulator =====\n")
    controllerSimOut = runControllerSimulator(scheme, initial_state,
                                              llc_requests, "output_trace.gz",
                                              str(n),
                                              "out/controller_log",
                                              *logpoints)
    print('\n')

    original_size, paths = chopUpTableLogs(divisions)

    compressed_sizes = {}
    print("Performing gzip...")
    compressed_sizes["gzip"] = gzipAll(paths)
    print("Performing bzip2...")
    compressed_sizes["bzip2"] = bzip2All(paths)
    print("Performing zip...")
    compressed_sizes["zip"] = zipAll(paths)

    return original_size, compressed_sizes

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run a tag controller simulation and see how compressible the tag table is.')
    parser.add_argument('-i', dest='initial_state', type=str, required=True)
    parser.add_argument('-l', dest='llc_requests', type=str, required=True)
    parser.add_argument('-n', dest='n', type=str, required=True)
    parser.add_argument('-d', dest='divisions', type=str, required=False, default="10")

    parsed_args = parser.parse_args()


    sizes = {}
    for scheme in ["baseline"]:#, "etm", "morello-t", "morello-z"]:
        print("##########  EMULATING SCHEME: " + scheme + "  ##########")
        sizes[scheme] = doRun(scheme, parsed_args.initial_state, parsed_args.llc_requests,
              int(parsed_args.n), int(parsed_args.divisions))

    print(sizes)

    save(sizes, "saves/sizes-"+getTimestamp()+".json")
