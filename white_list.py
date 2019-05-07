#!/usr/bin/env python
import os
# Reset
os.system("git checkout Testing/")
# Files to keep. Note an empty white-list is treated as NO filter, will download everything.
whitelist = ['SANS2D00022048.nxs', 'SANS2D00022041.nxs', 'SANS2D00022024.nxs', 'SANS2D00022023.nxs', 'MaskSANS2DReductionGUI_LimitEventsTime.txt', 'DIRECTM1_15785_12m_31Oct12_v12.dat']
whitelist_search = ['{}.md5'.format(f) for f in whitelist]
if whitelist:
    for r, d, f in os.walk('Testing'):
         for file in f:
             if file not in whitelist_search:
                 if 'md5' in file:
                     os.remove(os.path.join(r, file)) 

