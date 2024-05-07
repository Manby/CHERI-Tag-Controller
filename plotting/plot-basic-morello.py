import argparse
from utils import *
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('-s', dest='show', type=str, required=False, default="yes")
parsed_args = parser.parse_args()

d1 = load('data/wli1.txt')
#d2 = load('data/wli2.txt')
d2 = load('data/wli2m.txt')

d = mergeDicts(d1, d2)
#d = collapseMorello(d)
d = rename(d, 'phoenixs-8t', 'phoenix')

N = 50000000

for schemes, name in zip([['morello-z', 'morello-t'], ['baseline', 'morello'], ['etm', 'phoenix'],
                          ['baseline', 'morello', 'etm', 'phoenix']],
                         ['a', 'curr', 'new', 'all']):
    for count in ['access', 'miss']:
        workloads = list(d.keys())

        fig, ax = plt.subplots(layout='constrained', figsize=(10,5))

        barsMorello(ax, 0.25, d, workloads, schemes, 'total', count)

        ax.set_ylabel('Cache '+count+'es')
        ax.set_title('Total number of cache '+count+'es')
        ax.legend()

        plt.grid(color='gray', linewidth=0.5, axis='y')
        plt.gca().set_axisbelow(True)

        if count == "miss":
            ax2 = ax.twinx()
            ymin, ymax = ax.get_ylim()
            top = ymax/N * 100
            ax2.plot([0,0],[0,top], linestyle='None')
            ax2.set_ylabel('% Additional DRAM accesses')
            ax2.set_ylim(ymin=0, ymax=top)

        savePlot(plt, 'basic-'+name+'-'+count)
        if parsed_args.show == "yes": plt.show()

for schemes, name in zip([['etm', 'phoenix']], ['bounds']):
    for count in ['miss']:
        workloads = list(d.keys())

        fig, ax = plt.subplots(layout='constrained', figsize=(10,5))

        barsBounds(ax, 0.25, d, workloads, schemes, 'total', count)

        ax.set_ylabel('% Additional DRAM access latency')
        ax.set_title('Best and worst case additional DRAM access latency')
        ax.legend()

        plt.grid(color='gray', linewidth=0.5, axis='y')
        plt.gca().set_axisbelow(True)

        savePlot(plt, 'basic-'+name+'-'+count)
        if parsed_args.show == "yes": plt.show()
