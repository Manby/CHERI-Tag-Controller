from utils import *
import matplotlib.pyplot as plt
import numpy as np

d1 = load('data/wli1.txt')
d2 = load('data/wli2.txt')

d = mergeDicts(d1, d2)
d = collapseMorello(d)
d = rename(d, 'phoenix-8t', 'phoenix')

for schemes in [['baseline', 'morello', 'etm'], ['etm', 'phoenix']]:
    for count in ['access', 'miss']:
        workloads = list(d.keys())

        fig, ax = plt.subplots(layout='constrained')

        bars(ax, 0.15, d, workloads, schemes, 'total', count)

        ax.set_ylabel('Cache '+count+'es')
        ax.set_title('Total number of cache '+count+'es')
        ax.legend()

        plt.show()
