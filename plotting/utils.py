import json
import numpy as np
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


def bars(ax, width, d, groups, schemes, type_, count):

    x = np.arange(len(groups))

    i = 0
    for scheme in schemes:
        vals = [d[group][scheme][type_][count] for group in groups]
        rects = ax.bar(x + width * i, vals, width, label=scheme)
        ax.bar_label(rects, padding=3)

        i += 1

    ax.set_xticks(x + width, groups)

def lines(ax, d, vars, schemes, type_, count):
    x = np.arange(len(vars))

    for scheme in schemes:
        y = [d[var][scheme][type_][count] for var in vars]

        ax.plot(x, y, label=scheme)

    ax.set_xticks(x, vars)
