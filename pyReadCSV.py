import pandas as pd

df = pd.read_csv("Win32/Debug/run_log.csv")

queue = []
hap = None
ep = 0
charging = False

for _, row in df.iterrows():
    t, event, sid, pid, q, ep_val, x1, x2 = row
    t_str = f"t={t:.3f}"

    if event == "CST":
        charging = True
        print(f"{t_str}  sensor={sid}  CHARGE_START need=?  active={int(x1)}/{int(x2) if x2>0 else 'inf'}")
        print(f"{t_str}  STATE after CHARGE_START")
        print(f"  [S{sid}={{{','.join(map(str,queue))}}} EP={ep_val if ep_val>=0 else 0} (chg)]  HAP={'idle' if not hap else f'sid={sid}, pkg={hap}'}")

    elif event == "CEND":
        charging = False
        print(f"{t_str}  sensor={sid}  CHARGE_END +{int(x1)}EP  EP={ep_val}")
        print(f"{t_str}  STATE after CHARGE_END")
        print(f"  [S{sid}={{{','.join(map(str,queue))}}} EP={ep_val} ]  HAP={'idle' if not hap else f'sid={sid}, pkg={hap}'}")

    elif event == "ARR":
        queue.append(pid)
        ep = ep_val
        print(f"{t_str}  sensor={sid}  pkg={pid}  ARRIVAL      Q={len(queue)}  EP={ep}")
        print(f"{t_str}  STATE after ARRIVAL")
        print(f"  [S{sid}={{{','.join(map(str,queue))}}} EP={ep}{' (chg)' if charging else ''}]  HAP={'idle' if not hap else f'sid={sid}, pkg={hap}'}")

    elif event == "STX":
        if pid in queue:
            queue.remove(pid)
        hap = pid
        ep = ep_val
        print(f"{t_str}  HAP      START_TX sensor={sid}  pkg={pid}  st={x1:.3f}  EP_before={ep}  cost={int(x2)} (will be ticked)")
        print(f"{t_str}  STATE after START_TX")
        print(f"  [S{sid}={{{','.join(map(str,queue))}}}(S:{pid}) EP={ep}{' (chg)' if charging else ''}]  HAP=sid={sid}, pkg={pid}")

    elif event == "ETX":
        hap = None
        ep = ep_val
        print(f"{t_str}  HAP      END_TX   sensor={sid}  pkg={pid}  served={pid}  Q={len(queue)}  EP={ep}")
        print(f"{t_str}  STATE after END_TX")
        print(f"  [S{sid}={{{','.join(map(str,queue))}}} EP={ep}{' (chg)' if charging else ''}]  HAP=idle")

    elif event == "STAT":
        # 這裡只驗證，不生成完整輸出
        pass
