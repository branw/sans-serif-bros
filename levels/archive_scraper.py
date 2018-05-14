# This script automatically scrapes the Internet Archive for all custom levels
# and downloads them

import re
import requests

# The levels went up to about 7000 before the shutdown, but it seems like no
# levels above 5000 were archived
for i in range(1, 7000):
    # The correct URL is automatically resolved
    r = requests.get(
        'https://web.archive.org/web/20061009071802/http://www.foon.co.uk/farcade/ssb/?lev=' + str(
            i))

    if r.status_code == 200:
        m = re.search("var resp = '(.*)';\n", r.text)
        if m is not None:
            print(f'{i} found!')
            with open(f'{i}_.txt', 'w') as f:
                f.write(m.group(1).replace('\\\\', '\\').replace('\\n', '\n'))
            continue

    print(f'{i} not found')
