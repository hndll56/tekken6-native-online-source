import ctypes
import os
import signal
import subprocess
import sys
import time
# Adopt and reap this test's orphan child; no dependency on a container PID 1 reaper.
assert ctypes.CDLL(None).prctl(36, 1, 0, 0, 0) == 0
parent = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, text=True)
child = None
try:
    child = int(parent.stdout.readline().strip())
    parent.kill()
    parent.wait(timeout=3)
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        pid, status = os.waitpid(child, os.WNOHANG)
        if pid:
            assert os.WIFSIGNALED(status) and os.WTERMSIG(status) == signal.SIGKILL, status
            child = None
            print('launcher crash kills its game child through PR_SET_PDEATHSIG: PASS')
            break
        time.sleep(.01)
    else:
        raise AssertionError('game child survived parent crash')
finally:
    if parent.poll() is None:
        parent.kill()
        parent.wait()
    if child:
        try:
            os.kill(child, signal.SIGKILL)
            os.waitpid(child, 0)
        except ProcessLookupError:
            pass
