#!/usr/bin/env python3

import os

flags = ['-w', '-n']
tests = ['1', '2', '3', '4']
shebang = '#!/bin/bash'
script_name = './tests.sh'
prog_name = './freemem'
out_dir = 'results'

buffer=''

buffer += f'{shebang}\n\n'
for free_flag in flags:
    for nofree_flag in flags:
        for test in tests:
            free_abrv = 'fw' if '-w' == free_flag else 'fnw'
            nofree_abrv = 'nfw' if '-w' == nofree_flag else 'nfnw'
            buffer += f'{prog_name} {free_flag} {nofree_flag} {test} > ./{out_dir}/out_{test}_{free_abrv}_{nofree_abrv}.txt\n'

with open(script_name, 'w') as fd:
    fd.write(buffer)

