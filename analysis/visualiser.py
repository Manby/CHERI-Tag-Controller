import argparse
import threading
import subprocess
import struct
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description='Visualise the state of the table and cache at given points in the execution of a workload.')
parser.add_argument('logpoints', metavar='P', type=str, nargs='+', help='the integer index of the instructions after executing which a visualisation of the cache will be made')
parser.add_argument('-i', dest='initial_state', type=str, required=True)
parser.add_argument('-l', dest='llc_requests', type=str, required=True)
parser.add_argument('-o', dest='output', type=str, required=False,
                    default='out')
parser.add_argument('-n', dest='n', type=str, required=True)
parser.add_argument('-s', dest='scheme', type=str, required=False,
                    default="baseline")

parsed_args = parser.parse_args()

PATH = "../.."

def run_controller_simulator(outputList, *args):
    out = subprocess.check_output([PATH+'/CHERI-Tag-Controller/simulator/bin/cheri-tag-controller',
                    *args]).decode("utf-8")
    outputList.append(out)
    print(out)

def run_champsim(*args):
    subprocess.run([PATH+'/ChampSim-instrumented/ChampSim/bin/champsim', *args])


outputList = []
controller_simulator = threading.Thread(target=run_controller_simulator,
                                        args=(outputList,
                                              parsed_args.scheme,
                                              parsed_args.initial_state,
                                              parsed_args.llc_requests,
                                              "output_trace",
                                              parsed_args.n,
                                              "controller_log",
                                              *parsed_args.logpoints))

print("Running tag controller simulator")
controller_simulator.start()
controller_simulator.join()
print('\n')

access_logpoints = []
for line in outputList[0].split('\n'):
    if "DUMP: " in line:
        access_logpoints.append(line[6:])
    elif "Performed" in line:
        num_accesses = line[line.find("[")+1:line.find("]")]

print("Num accesses:", num_accesses)
print("Access logpoints:", access_logpoints)
print()

champsim = threading.Thread(target=run_champsim,
                                        args=("--warmup-instructions",
                                              "0",
                                              "--simulation-instructions",
                                              num_accesses,
                                              "output_trace",
                                              "--logpoints",
                                              *access_logpoints,
                                              "--logfile",
                                              "champsim_log"))

print("Running ChampSim")
champsim.start()
champsim.join()
print('\n')


if parsed_args.scheme in ["morello-t", "morello-z", "baseline"]:
    #L = 2**24   # size in bytes of the tag leaf table
    L = 2**24   # size in bytes of the tag leaf table
    N = 768*8   # size in bytes of the tag cache

    LX = 4096*8
    LY = (L*8)//LX

    with open("controller_log", "rb") as dram_f, open("champsim_log", "rb") as cache_f:
        for ip, p in enumerate(parsed_args.logpoints):
            dram_raw = dram_f.read(L)

            cache_raw = cache_f.read(N)
            cache = set(map(lambda x: x[0], struct.iter_unpack('Q', cache_raw)))
            if (1 << 31) in cache:
                cache.remove(1 << 31)
                cache.add(0)

            if 0xffffffff80000000 in cache:
                print("Removed extraneous 0xffffffff80000000")
                cache.remove(0xffffffff80000000)

            img = Image.new(mode="RGB", size=(LX, LY), color="white")
            pixels = img.load()

            print("Drawing table")
            for base in cache:
                for i in range(64*8):
                    pixels[(8*base+i)%LX, (8*base+i)//LX] = (128, 255, 128)

            for b in range(L):
                byte = dram_raw[b]
                if byte == 0:
                    continue

                in_cache = (b//64)*64 in cache
                for i in range(8):
                    bit = byte & (1<<(7-i))

                    if bit and in_cache:
                        colour = (0, 128, 255)
                    elif not bit and in_cache:
                        colour = (128, 255, 128)
                    elif bit and not in_cache:
                        colour = (0, 0, 255)
                    elif not bit and not in_cache:
                        colour = (255, 255, 255)

                    pixels[(8*b+i)%LX, (8*b+i)//LX] = colour

            img.save(parsed_args.output+"-"+str(p)+".png")
            print("Saved", parsed_args.output+"-"+str(p)+".png")

elif parsed_args.scheme == "etm":
    #L = 2**24   # size in bytes of the tag leaf table
    L = 2**24   # size in bytes of the tag leaf table
    R = L//512  # size in bytes of the tag root table
    #N = 1024*8
    N = 768*8   # size in bytes of the tag cache

    RX = 64*8   # pixels (bits) in x direction of root table
    RY = (R*8)//RX
    LX = 4096*8
    LY = (L*8)//LX

    with open("controller_log", "rb") as dram_f, open("champsim_log", "rb") as cache_f:
        for ip, p in enumerate(parsed_args.logpoints):
            dram_raw = dram_f.read(R+L)

            cache_raw = cache_f.read(N)
            cache = set(map(lambda x: x[0], struct.iter_unpack('Q', cache_raw)))

            if (1 << 31) in cache:
                cache.remove(1 << 31)
                cache.add(0)

            if 0xffffffff80000000 in cache:
                print("Removed extraneous 0xffffffff80000000")
                cache.remove(0xffffffff80000000)

            img = Image.new(mode="RGB", size=(LX, RY+LY), color="white")

            print("Drawing table")
            draw = ImageDraw.Draw(img)
            draw.rectangle(((RX, 0), (LX-1, RY-1)), fill="black")
            pixels = img.load()

            for base in cache:
                if base < R:
                    for i in range(64*8):
                        pixels[(8*base+i)%RX, (8*base+i)//RX] = (128, 255, 128)
                else:
                    for i in range(64*8):
                        pixels[(8*(base-R)+i)%LX, (8*(base-R)+i)//LX + RY] = (128, 255, 128)

            for b in range(R):
                byte = dram_raw[b]

                in_cache = (b//64)*64 in cache
                for i in range(8):
                    bit = byte & (1<<(7-i))

                    if bit and in_cache:
                        colour = (255, 128, 0)
                    elif not bit and in_cache:
                        colour = (128, 255, 128)

                        leaf_base = (8*b+i)*64
                        if not (R+leaf_base) in cache:
                            for l in range(512):
                                pixels[(leaf_base*8+l)%LX, RY+((leaf_base*8+l)//LX)] = (225, 225, 128)

                    elif bit and not in_cache:
                        colour = (255, 0, 0)
                    elif not bit and not in_cache:
                        colour = (255, 255, 255)

                    pixels[(8*b+i)%RX, (8*b+i)//RX] = colour

            for b in range(L):
                byte = dram_raw[b+R]
                if byte == 0:
                    continue

                #in_cache_direct = R + (b//64)*64 in cache
                #in_cache_indirect = (b//(64*512))*64 in cache
                in_cache_direct = pixels[(8*b)%LX, (8*b)//LX+RY] == (128, 255, 128)
                in_cache_indirect = pixels[(8*b)%LX, (8*b)//LX+RY] == (225, 225, 128)
                in_cache = in_cache_direct or in_cache_indirect
                for i in range(8):
                    bit = byte & (1<<(7-i))

                    if bit and in_cache_direct:
                        colour = (0, 128, 255)
                    elif bit and in_cache_indirect:
                        colour = (128, 128, 255)
                    elif bit and not in_cache:
                        colour = (0, 0, 255)
                    elif not bit and in_cache_direct:
                        colour = (128, 255, 128)
                    elif not bit and in_cache_indirect:
                        colour = (225, 225, 128)
                    elif not bit and not in_cache:
                        colour = (255, 255, 255)

                    pixels[(8*b+i)%LX, (8*b+i)//LX + RY] = colour

            img.save(parsed_args.output+"-"+str(p)+".png")
            print("Saved", parsed_args.output+"-"+str(p)+".png")
