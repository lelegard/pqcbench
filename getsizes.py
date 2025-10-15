#!/usr/bin/env python
#----------------------------------------------------------------------------
# pqcbench - Copyright (c) 2025, Thierry Lelegard
# BSD 2-Clause License, see LICENSE file.
# A Python module to get the wrapped key and signature size of algorithms.
#----------------------------------------------------------------------------

import os, sys, glob

rootdir = os.path.dirname(os.path.abspath(__file__))
sizes = dict()

for filename in glob.glob(rootdir + '/results/*.txt'):
    with open(filename, 'r') as input:
        algo = None
        for line in input:
            line = [field.strip() for field in line.split(':')]
            if len(line) >= 2:
                if line[0] == 'algo':
                    algo = line[1]
                elif algo is not None and (line[0] == 'wrapped-size' or line[0] == 'signature-size'):
                    size = int(line[1])
                    if algo not in sizes:
                        sizes[algo] = size
                    elif sizes[algo] != size:
                        print('error: incompatible sizes for %s: %d and %d' % (algo, sizes[algo], size), file=sys.stderr)

if len(sizes) == 0:
    print('error: no size found')
    exit(1)

for algo in sizes:
    sizes[algo] = '{:,}'.format(int(sizes[algo]))

witdth1 = max([len(algo) for algo in sizes])
witdth2 = max([len(s) for s in sizes.values()])

with open(rootdir + '/SIZES.txt', 'w') as output:
    print('SIZE IN BYTES OF WRAPPED KEYS AND SIGNATURES', file=output)
    print('', file=output)
    for algo in sizes:
        print('%-*s   %*s' % (witdth1, algo, witdth2, sizes[algo]), file=output)
