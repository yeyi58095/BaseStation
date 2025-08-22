#pragma once
#include <vector>

// 前置宣告（避免循環 include）
class Sensor;

namespace sim {

// 事件型別：現在只用到「到達 / 離開」兩種
enum EventType { EV_ARRIVAL = 0, EV_DEPARTURE = 1 };

// FEL 節點（用有序 linked list，時間小的在前）
struct EvNode {
    double   t;    // 事件時間
    EventType tp;  // 事件型別
    int      sid;  // 哪個 sensor 的事件（0..N-1）
    EvNode*  next; // 下一個節點
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

// Master：事件排程器（平行伺服器版）
// - 每個 sensor 都有自己的到達/服務，互不干擾
// - FEL（future event list）仍集中在 Master 管
class Master {
public:
    // ===== 全部 public，方便你在 IDE 監看 =====
    std::vector<Sensor*>* sensors; // 外部擁有；Master 不負責 delete
    EvNode*  felHead;              // FEL dummy head（head->next 是第一個事件）
    double   endTime;              // 模擬終止時間（秒）
    double   now;                  // 目前模擬時間
    double   switchover;           // 切換開銷（秒）；平行伺服器通常設 0
    // （Shared 版本會用到的旗標，這裡保留但不使用）
    bool     serverBusy;

    // ===== 統計 =====
    double   prev;       // 上一次做統計積分的時間
    double   sumQ;       // ∫ 全部佇列長度總和 dt  → Lq_total = sumQ / T
    double   sumSystem;  // ∫ (全部佇列 + 忙碌伺服器數) dt → L_total = sumSystem / T
    double   busySum;    // ∫ 忙碌伺服器的「數量」 dt       → 平均每台忙碌率 = busySum / (T * N)
    int      arrivals;   // 到達封包總數（全部 sensor 合計）
    int      served;     // 完成封包總數（全部 sensor 合計）

    // ===== 介面 =====
    Master();
    ~Master();

    void setSensors(std::vector<Sensor*>* v);
    void setEndTime(double T);
    void setSwitchOver(double s);

    void reset();           // 清 FEL、時間、統計
    void run_parallel();    // 「平行伺服器」模式：各 sensor 獨立服務

    // ===== FEL 操作（公開，方便 debug/擴充）=====
    void felClear();
    void felPush(double t, EventType tp, int sid);
    bool felPop(double& t, EventType& tp, int& sid);

    // ===== 排程動作 =====
    void startService(int sid); // 讓 sid 開始服務，並排它自己的 EV_DEPARTURE

    // ===== 統計積分 =====
    void accumulateQ();     // 用 [prev, now] 這段更新 Lq/L 與忙碌伺服器數的積分

    static double EPS;      // 避免抽到非正時間用的最小值（例如 1e-9）
};

} // namespace sim

