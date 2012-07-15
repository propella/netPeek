#!/usr/bin/env python
#
# Show "PERFORCE-LOOKS-GOOD-AND-SMOOTH" if p4 is ready,
# otherwise show "SOMETHING-IS-TECHNICALLY-WRONG"
#
# It only test whether if the server becomes timeout or not, and it
# doesn't acually logs in it.
#
# This CGI is deployed at
# http://192.168.0.1/~tyamamiya/p4peek.cgi
#
# Based on http://code.pui.ch/2007/02/19/set-timeout-for-a-shell-command-in-python/

def timeout_command(command, timeout, env):
    """call shell-command and either return its output or kill it
    if it doesn't normally exit within timeout seconds and return None"""
    import subprocess, datetime, os, time, signal
    start = datetime.datetime.now()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    while process.poll() is None:
        time.sleep(0.1)
        now = datetime.datetime.now()
        if (now - start).seconds> timeout:
            os.kill(process.pid, signal.SIGKILL)
            os.waitpid(-1, os.WNOHANG)
            return None
#    print process.stderr.read()
#    print process.stdout.read()
    return process.poll()

status = timeout_command(["/usr/local/bin/p4", "revert", "-n", "..."], 5, {"P4CONFIG":"p4config"})

if status == 0:
    response = "PERFORCE-LOOKS-GOOD-AND-SMOOTH"
else:
    response = "SOMETHING-IS-TECHNICALLY-WRONG"

print "Content-Type: Text/plain"
print
print response
#print status
