import json
import numpy as np
import matplotlib.patches as patches
import matplotlib.pyplot as plt

def readFile(filename):
    with open(filename) as f:
        return json.loads(f.read())

def mergeDicts(a, b):
    if type(a) != dict:
        return a

    for k in a:
        if k in b:
            a[k] = mergeDicts(a[k], b[k])

    for k in b:
        if not k in a:
            a[k] = b[k]

    return a

def rename(d, old, new):
    if type(d) != dict:
        return d

    e = {}
    for k in d:
        if k == old:
            e[new] = d[k]
        else:
            e[k] = rename(d[k], old, new)

    return e

def load(filename):
    data = readFile(filename)
    if type(data) == dict:
        return data

    d1 = {}
    for d2 in data:
        d1 = mergeDicts(d1, d2)

    return d1

def collapseMorello(d):
    if type(d) != dict:
        return d

    mz = None
    mt = None

    for k in d:
        if k == "morello-z":
            mz = d[k]
        elif k == "morello-t":
            mt = d[k]

    if mz is not None and mt is not None:
        m = collapseMorelloInner(mz, mt)
        del d["morello-z"]
        del d["morello-t"]
        d["morello"] = m
        return d

    else:
        for k in d:
            d[k] = collapseMorello(d[k])

        return d

def collapseMorelloInner(mz, mt):
    m = {}
    for k in mz:
        if k == "entries":
            m["entries"] = mz[k]

        else:
            dd = {}
            for kk in mz[k]:
                dd[kk] = mz[k][kk] + mt[k][kk]
            m[k] = dd

    return m

def integrateControls(d, dc, name):
    for workload in d:
        d[workload][name] = dc[workload]
    return d

def integrateControlsCompression(d, dc, target):
    for workload in d:
        d[workload][target] = dc[workload][target]
    return d

def printDict(d, level=0):
    print('│   '*level+'{')
    level += 1

    for item in d:
        if not type(d[item]) in [dict, list]:
            try:
                print('│   '*level+ str(item)+': '+str(d[item]))
            except UnicodeEncodeError:
                print('│   '*level+ str(item)+': -')

        elif type(d[item]) == dict:
            print('│   '*level+ str(item)+':')
            printDict(d[item], level)

        elif type(d[item]) == list:
            print('│   '*level+ str(item)+':')
            printList(d[item], level)

    level -= 1
    print('│   '*level+'}')


def printList(l, level=0):
    print('│   '*level+'[')
    level += 1

    for item in l:
        if not type(item) in [dict, list]:
            try:
                print('│   '*level+str(item))
            except UnicodeEncodeError:
                print('│   '*level+'-')

        elif type(item) == dict:
            printDict(item, level)

        elif type(item) == list:
            printList(item, level)

    level -= 1
    print('│   '*level+']')

def printData(data):
    global level
    level = -1

    if type(data) == list:
        printList(data, level)
    elif type(data) == dict:
        printDict(data, level)

colours = {
        'baseline': '#0046D1',
        'morello':  '#09B300',
        'etm':      '#8B00D0',
        'phoenix':  '#FF7B00',
        'morello-z': '#09B300',
        'morello-t': '#0903B0',
}
coloursHigh = {
        'etm':      '#3B0080',
        'phoenix':  '#9F4B00',
}
coloursLow = {
        'etm':      '#9B00E0',
        'phoenix':  '#FF8B00',
}

def barsMorello(ax, width, d, groups, schemes, type_, count):
    x = np.arange(len(groups))

    i = 0
    for scheme in schemes:
        if count == 'miss':
            vals = [d[group][scheme][type_][count] /
                    d[group][scheme][type_]['access']  for group in groups]
            print(vals)
        else:
            vals = [d[group][scheme][type_][count] for group in groups]

        rects = ax.bar(x + width * i, vals, width, label=scheme,
                       color=colours[scheme])
        #ax.bar_label(rects, padding=3)

        i += 1

    ax.set_xticks(x + 0.5*width, groups)

def bars(ax, width, d, groups, schemes, type_, count):
    x = np.arange(len(groups))

    i = 0
    for scheme in schemes:
        vals = [d[group][scheme][type_][count] for group in groups]

        '''
        if scheme == 'morello':
            vals = [d[group][scheme][type_][count] * 10 for group in groups]
        else:
            vals = [d[group][scheme][type_][count] for group in groups]
        '''

        rects = ax.bar(x + width * i, vals, width, label=scheme,
                       color=colours[scheme])
        #ax.bar_label(rects, padding=3)

        i += 1

    ax.set_xticks(x + 0.5*width, groups)

def barsBounds(ax, width, d, groups, schemes, type_, count):
    x = np.arange(len(groups))

    i = 0
    for scheme in schemes:
        valsHigh = [d[group][scheme][type_][count] for group in groups]
        if scheme == 'etm':
            valsHigh = [(0.1 + (2-1)) * (v/50000000) * 100 for v in valsHigh]
        elif scheme == 'phoenix':
            valsHigh = [(0.1 + (3-1)) * (v/50000000) * 100 for v in valsHigh]

        rects = ax.bar(x + width * i, valsHigh, width, label=scheme+' worst case',
                       color=coloursHigh[scheme], zorder=2)
        #ax.bar_label(rects, padding=3)

        i += 1

    i=0
    for scheme in schemes:
        valsLow = [d[group][scheme][type_][count] for group in groups]
        valsLow = [(0.1 * v/50000000) * 100 for v in valsLow]

        rects = ax.bar(x + width * i, valsLow, width, label=scheme+' best case',
                       color=coloursLow[scheme], zorder=3)
        #ax.bar_label(rects, padding=3)

        i += 1

    ax.set_xticks(x + 0.5*width, groups)

def lines(ax, d, vars, schemes, type_, count):
    x = np.arange(len(vars))

    for scheme in schemes:
        y = [d[var][scheme][type_][count] for var in vars]

        ax.plot(x, y, label=scheme, color=colours[scheme])

    ax.set_xticks(x, vars)

def boxesBaseline(ax, width, d, groups, schemes):
    x = np.arange(len(groups))

    i = 0

    data = []

    scheme = schemes[0]
    algs = ['gzip', 'bzip2', 'zip']
    algColours = ['#990a00', '#006609', '#000f63']

    for group in groups:
        l = []

        algs = d[group][scheme][1]
        for alg in algs:
            vals = [d[group][scheme][1][alg][i] for i in range(len(d[group][scheme][1][alg]))]
            l.append(vals)

        data.append(l)
        i += 1

    for i, group in enumerate(groups):
        bplot = ax.boxplot(data[i], positions = [1 + (1*i) * (len(algs) + 2) + j for j in
                                         range(len(algs))], widths=width,
                           patch_artist=True, notch=True, showfliers=False,
                           showcaps=False)

        for part in ['boxes', 'whiskers', 'fliers', 'medians', 'caps']:
            for piece, colour in zip(bplot[part], algColours):
                plt.setp(piece, color=colour)
                if part == 'boxes':
                    plt.setp(piece, facecolor=colour)
                elif part == 'fliers':
                    plt.setp(piece, markeredgecolor=colour)

    ax.set_xticks([avg([1 + (1*i) * (len(algs) + 2) + j for j in range(len(algs))]) for i, group in enumerate(groups)])
    ax.set_xticklabels(groups)

    all_patches = [patches.Patch(color=c, label=alg) for alg, c in
                   zip(algs, algColours)]
    plt.legend(handles=all_patches)

def boxesPhoenix(ax, width, d, groups, schemes):
    x = np.arange(len(groups))

    i = 0

    # [group1, group2, ...] group1=[[a1, a2, ...], [b1, b2, ...]]
    data = []

    schemeColours = ['#ffcc00', '#927800', '#ee0000', '#920000']

    for group in groups:
        l = []
        for scheme in schemes:
            if scheme == 'baseline':
                vals = [avg([d[group][scheme][1][alg][i] for alg in d[group][scheme][1]]) for i in range(len(d[group][scheme][1]['gzip']))]
                #rects = ax.boxplot(x + width * i, vals, widths=width)
                #ax.bar_label(rects, padding=3)


            else:
                vals = [d[group][scheme]['blocks']['avg'],]

            l.append(vals)

        data.append(l)
        i += 1

    for i, group in enumerate(groups):
        bplot = ax.boxplot(data[i], positions = [1 + i * (len(schemes) + 1) + j for j in
                                         range(len(schemes))], widths=width,
                   patch_artist=True, notch=True, showfliers=False,
                   showcaps=False)

        for part in ['boxes', 'whiskers', 'fliers', 'medians', 'caps']:
            for piece, colour in zip(bplot[part], schemeColours):
                plt.setp(piece, color=colour)
                if part == 'boxes':
                    plt.setp(piece, facecolor=colour)
                elif part == 'fliers':
                    plt.setp(piece, markeredgecolor=colour)

    ax.set_xticks([avg([1 + i * (len(schemes) + 1) + j for j in range(len(schemes))]) for i, group in enumerate(groups)])
    ax.set_xticklabels(groups)

    all_patches = [patches.Patch(color=c, label=scheme) for scheme, c in
                   zip(schemes, schemeColours)]
    plt.legend(handles=all_patches)

def avg(l):
    return sum(l) / len(l)


def savePlot(plt, name):
    plt.savefig("plots/"+name+".pdf", format="pdf", bbox_inches="tight")
