import re
import socket
import subprocess
import sys

def reserve():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('127.0.0.1', 0))
    return s

s1, s2 = reserve(), reserve()
p1, p2 = s1.getsockname()[1], s2.getsockname()[1]
s1.close()
s2.close()
children = []
try:
    for player, local, remote in [(1, p1, p2), (2, p2, p1)]:
        children.append(subprocess.Popen([sys.argv[1], str(player), str(local), str(remote)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))
    rows = []
    for child in children:
        out, err = child.communicate(timeout=30)
        if child.returncode:
            raise RuntimeError(f'peer failed {child.returncode}: {out}\n{err}')
        found = re.search(r'frame=(\d+) checksum=(\d+) rollbacks=(\d+)', out)
        assert found, (out, err)
        rows.append(tuple(map(int, found.groups())))
    assert rows[0][:2] == rows[1][:2], rows
    assert sum(row[2] for row in rows) > 0, f'no rollback exercised: {rows}'
    print(f'240 frames over real loopback UDP, checksums agree, rollback loads: {rows[0][2]}, {rows[1][2]}')
finally:
    for child in children:
        if child.poll() is None:
            child.kill()
            child.wait()
