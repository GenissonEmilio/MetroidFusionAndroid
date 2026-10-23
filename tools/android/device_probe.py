#!/usr/bin/env python3
"""Observe an already-running QA session via adb forward (no game-state writes)."""
import argparse
import json
import pathlib
import socket
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--adb", default="adb")
p.add_argument("--seconds", type=int, default=20)
p.add_argument("--port", type=int, default=19842)
p.add_argument("--out", type=pathlib.Path, default=pathlib.Path("artifacts/device-probe.json"))
a = p.parse_args()
if not 1 <= a.seconds <= 50:
    p.error("Use a bounded 1..50 second sample")
subprocess.run([a.adb, "forward", f"tcp:{a.port}", "tcp:19842"], check=True)
with socket.create_connection(("127.0.0.1", a.port), timeout=10) as sock:
    channel = sock.makefile("rwb")
    def query(cmd):
        channel.write((json.dumps({"cmd": cmd})+"\n").encode()); channel.flush()
        line = channel.readline()
        if not line:
            raise RuntimeError("Game observer disconnected")
        answer = json.loads(line)
        if not answer.get("ok"):
            raise RuntimeError(answer)
        return answer
    samples=[]
    for n in range(a.seconds+1):
        answer=query("frame")
        samples.append({"host_seconds":time.monotonic(), "frame":answer["frame"]})
        if n<a.seconds:
            time.sleep(1)
    elapsed=samples[-1]["host_seconds"]-samples[0]["host_seconds"]
    result={"guest_fps":(samples[-1]["frame"]-samples[0]["frame"])/elapsed,
            "seconds":elapsed,"samples":samples,"coverage":query("misses"),
            "touch":query("touch_status")}
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({"guest_fps":round(result["guest_fps"],2),"seconds":round(elapsed,2),
                      "coverage":result["coverage"].get("coverage"),"report":str(a.out)}))
