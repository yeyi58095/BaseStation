import pandas as pd

def replay(csv_path):
    df = pd.read_csv(csv_path)
    df = df.sort_values('t', kind='mergesort')  # 保持事件順序

    # 狀態：每個 sensor 的 queue、serving、EP
    queues = {}
    serving = {}
    ep = {}

    def qstr(sid):
        q = queues.get(sid, [])
        s = serving.get(sid)
        base = f"S{sid}={{" + ", ".join(map(str, q)) + "}"
        if s is not None:
            base += f"(S:{s})"
        base += f" EP={ep.get(sid,0)}"
        return base

    for _, r in df.iterrows():
        t = r.t; ev = r.event; sid = int(r.sid)
        pid = int(r.pid) if r.pid >= 0 else None
        q   = int(r.q)   if r.q >= 0 else None
        EP  = int(r.ep)  if r.ep >= 0 else None
        x1  = r.x1; x2 = r.x2

        if ev == 'ARR':
            queues.setdefault(sid, [])
            queues[sid].append(pid)
            ep[sid] = EP
            print(f"t={t:.3f}  sensor={sid}  pkg={pid}  ARRIVAL      Q={len(queues[sid])}  EP={EP}")
            print(f"t={t:.3f}  STATE after ARRIVAL")
            state_str = " | ".join(qstr(s) for s in sorted(queues.keys()))
            print(f"  [{state_str}]  HAP=idle")

        elif ev == 'STX':
            queues.setdefault(sid, [])
            if queues[sid] and queues[sid][0] == pid:
                queues[sid].pop(0)
            serving[sid] = pid
            ep_before = EP
            print(f"t={t:.3f}  HAP      START_TX sensor={sid}  pkg={pid}  st={x1:.3f}  EP_before={ep_before}  cost={int(x2)} (will be ticked)")
            print(f"t={t:.3f}  STATE after START_TX")
            state_str = " | ".join(qstr(s) for s in sorted(queues.keys()))
            print(f"  [{state_str}]  HAP=sid={sid}, pkg={pid}")

        elif ev == 'TTK':
            ep[sid] = EP
            # 如果想顯示逐步扣，可以解開下一行
            # print(f"t={t:.3f}  TX_TICK   sensor={sid}  pkg={serving.get(sid)}  EP={EP}  remain={int(x1)}")

        elif ev == 'ETX':
            if sid in serving:
                del serving[sid]
            ep[sid] = EP
            queues.setdefault(sid, [])
            print(f"t={t:.3f}  HAP      END_TX   sensor={sid}  pkg={pid}  served={pid}  Q={len(queues[sid])}  EP={EP}")
            print(f"t={t:.3f}  STATE after END_TX")
            state_str = " | ".join(qstr(s) for s in sorted(queues.keys()))
            print(f"  [{state_str}]  HAP=idle")

        elif ev == 'CST':
            cap = "inf" if int(x2) < 0 else str(int(x2))
            print(f"t={t:.3f}  sensor={sid}  CHARGE_START need=?  active={int(x1)}/{cap}")
            print(f"t={t:.3f}  STATE after CHARGE_START")
            state_str = " | ".join(qstr(s) for s in sorted(queues.keys()))
            print(f"  [{state_str}]  HAP=idle")

        elif ev == 'CEND':
            ep[sid] = EP
            print(f"t={t:.3f}  sensor={sid}  CHARGE_END +{int(x1)}EP  EP={EP}")
            print(f"t={t:.3f}  STATE after CHARGE_END")
            state_str = " | ".join(qstr(s) for s in sorted(queues.keys()))
            print(f"  [{state_str}]  HAP=idle")

        elif ev == 'STAT':
            # 可用來校驗，但這裡先忽略
            pass

if __name__ == "__main__":
    replay("Win32/Debug/run_log.csv")
