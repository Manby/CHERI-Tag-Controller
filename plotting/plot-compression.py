import argparse
from utils import *
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('-s', dest='show', type=str, required=False, default="yes")
parsed_args = parser.parse_args()

dc = load('data/wli1.txt')
#d = load('data/wli9.txt')
d = load('data/wli9.txt')
d = integrateControlsCompression(d, dc, 'phoenixs-8t')

d = rename(d, 'phoenix-8t', 'phoenixb-8t')
d = rename(d, 'phoenix-8f', 'phoenixb-8f')
d = rename(d, 'phoenix-4t', 'phoenixb-ft')
d = rename(d, 'phoenix-4f', 'phoenixb-4f')
d = rename(d, 'phoenixs-8t', 'phoenix TWS=8, True LRU')
d = rename(d, 'phoenixs-8f', 'phoenix TWS=8, False LRU')
d = rename(d, 'phoenixs-4t', 'phoenix TWS=4, True LRU')
d = rename(d, 'phoenixs-4f', 'phoenix TWS=4, False LRU')


S = 16777216    # tag table size in bytes
P = 4096        # number of pages

#d = rename(d, 'phoenix-8t', 'phoenix')

workloads = list(d.keys())

for schemes, name in zip([['baseline']], ['curr']):
    fig, ax = plt.subplots(layout='constrained', figsize=(10,5))

    boxesBaseline(ax, 0.8, d, workloads, schemes)

    plt.grid(color='gray', linewidth=0.5, axis='y')
    plt.gca().set_axisbelow(True)

    ax2 = ax.twinx()
    ymin, ymax = ax.get_ylim()
    bottom = ymin/S * 100
    top = ymax/S * 100
    ax2.plot([0,bottom],[0,top], linestyle='None')
    ax2.set_ylabel('% of original size')
    ax2.set_ylim(ymin=bottom, ymax=top)
    ax2.set_yticks([y/S*100 for y in list(ax.get_yticks())[1:-1]])

    ax.set_ylabel('Compressed size (bytes)')
    ax.set_title('Compressed sizes of tag tables of different workloads')

    savePlot(plt, 'compression-'+name)
    if parsed_args.show == "yes": plt.show()


#for schemes, name in zip([['phoenix-8t']], ['new']):
for schemes, name in zip([['phoenix TWS=8, True LRU', 'phoenix TWS=8, False LRU', 'phoenix TWS=4, True LRU', 'phoenix TWS=4, False LRU']], ['new']):
    fig, ax = plt.subplots(layout='constrained', figsize=(10,5))

    boxesPhoenix(ax, 0.8, d, workloads, schemes)

    plt.grid(color='gray', linewidth=0.5, axis='y')
    plt.gca().set_axisbelow(True)

    ax2 = ax.twinx()
    ymin, ymax = ax.get_ylim()
    bottom = ymin/P * 100
    top = ymax/P * 100
    ax2.plot([0,bottom],[0,top], linestyle='None')
    ax2.set_ylabel('% of all 4096 pages')
    ax2.set_ylim(ymin=bottom, ymax=top)
    ax2.set_yticks([y/P*100 for y in list(ax.get_yticks())[1:-1]])

    ax.set_ylabel('Number of allocated pages')
    ax.set_title('Number of allocated tag table pages required for different workloads')

    savePlot(plt, 'compression-'+name)
    if parsed_args.show == "yes": plt.show()
