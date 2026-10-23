#!/usr/bin/env python3
"""Exercise native touch policy in an active, muted QA session (game input only)."""
import json
import pathlib
import socket
import time

with socket.create_connection(("127.0.0.1",19842),timeout=6) as sock:
    channel=sock.makefile("rwb")
    def query(**request):
        channel.write((json.dumps(request)+"\n").encode());channel.flush()
        answer=json.loads(channel.readline())
        if not answer.get("ok"):
            raise RuntimeError(answer)
        return answer
    state=query(cmd="touch_status")
    w,h=state["drawable"]; left,top,right,bottom=state["insets"]
    # Default 100% layout; use it only on a test installation with that setting.
    u=min(h/5.5,w/12); pad=max(12,u*.12)
    dx=left+pad+u; dy=h-bottom-pad-u
    x1=w-right-pad; y1=h-bottom-pad
    buttons={
        "A":(x1-u*.46,y1-u*1.65,1),
        "B":(x1-u*1.55,y1-u*.46,2),
        "Right":(dx+u*.65,dy,16), "Left":(dx-u*.65,dy,32),
        "Up":(dx,dy-u*.65,64), "Down":(dx,dy+u*.65,128),
        "R":(x1-u*.65,top+pad+u*.27,256),
        "L":(left+pad+u*.65,top+pad+u*.27,512),
    }
    results=[]
    for name,(x,y,mask) in buttons.items():
        frame=query(cmd="frame")["frame"]
        query(cmd="touch_down",pointer=100,space="drawable",x=x,y=y)
        time.sleep(.2)
        query(cmd="touch_up",pointer=100,space="drawable",x=x,y=y)
        time.sleep(.08)
        samples=query(cmd="key_synth",since_frame=frame,limit=128)["samples"]
        assert any(s["composed"] & mask==0 for s in samples), name
        results.append({"button":name,"mask":mask,"passed":True})
    frame=query(cmd="frame")["frame"]
    query(cmd="touch_down",pointer=101,space="drawable",x=dx+u*.6,y=dy-u*.6)
    ax,ay,_=buttons["A"]
    query(cmd="touch_down",pointer=102,space="drawable",x=ax,y=ay)
    time.sleep(.25)
    query(cmd="touch_up",pointer=101,space="drawable",x=dx+u*.6,y=dy-u*.6)
    query(cmd="touch_up",pointer=102,space="drawable",x=ax,y=ay)
    time.sleep(.12)
    samples=query(cmd="key_synth",since_frame=frame,limit=128)["samples"]
    assert any(s["composed"] & 0x51==0 for s in samples), "diagonal + A"
    assert query(cmd="touch_status")["active_pointers"]==0
    assert samples[-1]["synth"]==0x3ff, "stuck key after release"
    results.append({"button":"Up + Right + A","mask":0x51,"passed":True})
    report={"tests":results,"coverage":query(cmd="misses")}
    pathlib.Path("artifacts").mkdir(exist_ok=True)
    pathlib.Path("artifacts/touch-validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(f"{len(results)} touch checks passed; all pointers released")
