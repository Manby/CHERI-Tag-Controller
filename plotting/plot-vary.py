from utils import *
import matplotlib.pyplot as plt
import numpy as np

dc1 = load('data/wli1.txt')
dc2 = load('data/wli2.txt')
dc1 = rename(dc1, 'phoenix-8t', 'phoenix')
dc2 = collapseMorello(dc2)

d1 = load('data/wli3.txt')
d2 = load('data/wli4.txt')

d1 = rename(d1, 'phoenix-8t', 'phoenix')
d1 = rename(d1, '(64, 4, None)', '16')
d1 = rename(d1, '(256, 4, None)', '64')
d1 = rename(d1, '(512, 4, None)', '128')
d1 = integrateControls(d1, dc1, '32')

d2 = collapseMorello(d2)
d2 = rename(d2, '((32, 4, None), (32, 4, None))', '16')
d2 = rename(d2, '((128, 4, None), (128, 4, None))', '64')
d2 = rename(d2, '((256, 4, None), (256, 4, None))', '128')
d2 = integrateControls(d2, dc2, '32')

dCapacity = mergeDicts(d1, d2)

d1 = load('data/wli5.txt')
d2 = load('data/wli6.txt')

d1 = rename(d1, 'phoenix-8t', 'phoenix')
d1 = rename(d1, '(128, 1, None)', '1')
d1 = rename(d1, '(128, 2, None)', '2')
d1 = rename(d1, '(128, 8, None)', '8')
d1 = integrateControls(d1, dc1, '4')

d2 = collapseMorello(d2)
d2 = rename(d2, '((64, 1, None), (64, 1, None))', '1')
d2 = rename(d2, '((64, 2, None), (64, 2, None))', '2')
d2 = rename(d2, '((64, 8, None), (64, 8, None))', '8')
d2 = integrateControls(d2, dc2, '4')

dWays = mergeDicts(d1, d2)

d1 = load('data/wli7.txt')
d2 = load('data/wli8.txt')

d1 = rename(d1, 'phoenix-8t', 'phoenix')
d1 = rename(d1, '(128, 4, \'ip_stride\')', 'stride')
d1 = rename(d1, '(128, 4, \'next_line\')', 'next line')
d1 = integrateControls(d1, dc1, 'none')

d2 = collapseMorello(d2)
d2 = rename(d2, '((64, 4, \'ip_stride\'), (64, 4, \'ip_stride\'))', 'stride')
d2 = rename(d2, '((64, 4, \'next_line\'), (64, 4, \'next_line\'))', 'next line')
d2 = integrateControls(d2, dc2, 'none')

dPrefetcher = mergeDicts(d1, d2)

for d, vars, xlabel in [
        (dCapacity, ["16", "32", "64", "128"], 'Cache size (KiB)'),
        (dWays, ["1", "2", "4", "8"], 'Number of ways'),
        ]:
    workloads = list(d.keys())
    for workload in workloads:
        for schemes in [['baseline', 'morello', 'etm'], ['etm', 'phoenix']]:
            for count in ['miss']:

                fig, ax = plt.subplots(layout='constrained')

                lines(ax, d[workload], vars, schemes, 'total', 'miss')

                ax.set_xlabel(xlabel)
                ax.set_ylabel('Cache '+count+'es')
                ax.set_title('Total number of cache '+count+'es for workload ' + workload)
                ax.legend()

                plt.show()

for d, vars, xlabel in [
        (dPrefetcher, ['none', 'stride', 'next line'], 'Prefetcher')
        ]:
    workloads = list(d.keys())
    for workload in workloads:
        for schemes in [['baseline', 'morello', 'etm'], ['etm', 'phoenix']]:
            for count in ['miss']:

                fig, ax = plt.subplots(layout='constrained')

                bars(ax, 0.15, d[workload], vars, schemes, 'total', count)

                ax.set_xlabel(xlabel)
                ax.set_ylabel('Cache '+count+'es')
                ax.set_title('Total number of cache '+count+'es for workload ' + workload)
                ax.legend()

                plt.show()
