#!/usr/bin/env python
import os
# Files to keep
whitelist = ['SANS2D00022048.nxs', 'SANS2D00022041.nxs', 'SANS2D00022024.nxs', 'SANS2D00022023.nxs']
whitelist_search = ['{}.md5'.format(f) for f in whitelist]
for r, d, f in os.walk('Testing'):
     for file in f:
         if file not in whitelist_search:
             if 'md5' in file:
                 os.remove(os.path.join(r, file)) 

