#!/usr/bin/env python
#----------------------------------------------------------------------------
# pqcbench - Copyright (c) 2025, Thierry Lelegard
# BSD 2-Clause License, see LICENSE file.
# A Python module to get the wrapped key and signature size of algorithms.
#----------------------------------------------------------------------------

import os, sys, glob, subprocess

rootdir = os.path.dirname(os.path.abspath(__file__))

# Run a command and get stdout+stderr. Need a list of strings as command.
def run(cmd, err=subprocess.STDOUT, cwd=None):
    try:
        return subprocess.check_output(cmd, stderr=err, cwd=cwd).decode('utf-8')
    except:
        return ''

# Build the index from an algo name.
def indexof(name):
    return name.replace('-','').lower()

# For each algo, build a dict of dict's.
sizes = dict()

# Collect data sizes in bytes (encapsulated key, or signature).
for filename in glob.glob(rootdir + '/results/*.txt'):
    with open(filename, 'r') as input:
        algo = None
        for line in input:
            line = [field.strip() for field in line.split(':')]
            if len(line) >= 2:
                if line[0] == 'algo':
                    algo = indexof(line[1])
                    if algo not in sizes:
                        sizes[algo] = {'name': line[1], 'data': None, 'priv': None, 'pub': None}
                elif algo is not None and (line[0] == 'wrapped-size' or line[0] == 'signature-size'):
                    size = int(line[1])
                    if sizes[algo]['data'] is None:
                        sizes[algo]['data'] = size
                    elif sizes[algo]['data'] != size:
                        print('error: incompatible sizes for %s: %d and %d' % (algo, sizes[algo]['data'], size), file=sys.stderr)

# Load key sizes from key files.
for filename in glob.glob(rootdir + '/keys/*-prv.pem'):
    algo = indexof(os.path.basename(filename).replace('-prv.pem',''))
    if algo not in sizes:
        print('error: untested algo for %s' % filename, file=sys.stderr)
        continue
    typ = None
    for line in run(['openssl', 'pkey', '-in', filename, '-text', '-noout']).split('\n'):
        if not line.startswith(' ') and line.endswith(':'):
            typ = line[:-1]
            if typ == 'ek':
                typ = 'pub' # ML-KEM encryption key
            elif typ == 'dk':
                typ = 'priv' # ML-KEM decryption key
            if typ not in sizes[algo] or sizes[algo][typ] is None:
                sizes[algo][typ] = 0
        elif typ is not None and line.startswith('    '):
            sizes[algo][typ] += len([x for x in line.replace(' ','').split(':') if x != ''])

# Build formatted strings for integers.
for typ in ['data', 'priv', 'pub']:
    for algo in sizes:
        if sizes[algo][typ] is not None:
            sizes[algo][typ + '_bytes'] = '{:,}'.format(sizes[algo][typ])
            sizes[algo][typ + '_bits'] = '{:,}'.format(8 * sizes[algo][typ])
        else:
            sizes[algo][typ + '_bytes'] = ''
            sizes[algo][typ + '_bits'] = ''

# Headers and width.
headers = {
    'name': ['', '', 'Algorithm'],
    'data_bytes': ['Wrapped key', '/ signature', 'bytes'],
    'data_bits': ['Wrapped key', '/ signature', 'bits'],
    'priv_bytes': ['Private', 'key', 'bytes'],
    'priv_bits': ['Private', 'key', 'bits'],
    'pub_bytes': ['Public', 'key', 'bytes'],
    'pub_bits': ['Public', 'key', 'bits']
}
display_order = ['priv_bits', 'priv_bytes', 'pub_bits', 'pub_bytes', 'data_bits', 'data_bytes']
separator = '   '
widths = dict()
for n in headers:
    widths[n] = max(max([len(h) for h in headers[n]]), max([len(sizes[algo][n]) for algo in sizes]))

# Format the table in SIZES.txt.
with open(rootdir + '/SIZES.txt', 'w') as output:
    print('SIZE OF KEYS, WRAPPED KEYS, AND SIGNATURES', file=output)
    print('', file=output)
    # Print header text lines.
    for li in range(len(headers['name'])):
        line = '%-*s' % (widths['name'], headers['name'][li])
        for typ in display_order:
            line += '%s%*s' % (separator, widths[typ], headers[typ][li])
        print(line, file=output)
    # Print header underlines.
    line = widths['name'] * '-'
    for typ in display_order:
        line += separator + widths[typ] * '-'
    print(line, file=output)
    # Print data.
    for siz in sizes.values():
        line = '%-*s' % (widths['name'], siz['name'])
        for typ in display_order:
            line += '%s%*s' % (separator, widths[typ], siz[typ])
        print(line, file=output)
