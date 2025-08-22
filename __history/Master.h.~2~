#pragma once
#include <vector>

// 只做前置宣告，避免循環相依；Master.cpp 再 #include "Sensor.h"
class Sensor;

namespace sim {

//=== 事件型別：目前先用兩種（Arrival/Departure）。之後你要加 POLL/EP 都能擴。===
enum EventType { EV_ARRIVAL = 0, EV_DEPARTURE = 1 };

//=== FEL（Future Event List）的節點：用有序 linked list 以時間遞增插入。===
struct EvNode {
    double   t;    // 事件發生時間（秒）
    EventType tp;  // 事件種類
    int      sid;  // 哪一個 sensor（0..N-1）
    EvNode*  next; // 下一個節點
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

//=== Master：全域排程器（管理 FEL + 決定誰上機）。===
// 需求對應：
// - 你要所有成員 public → 全部 public，便於你在 Debug 視窗觀察。
// - Master 不擁有 sensors 的生命週期（指標由外部 vector 管）。
class Master {
public:
    // --------- 公開資料（方便你在 IDE 監看） ---------
    std::vector<Sensor*>* sensors; // 指向外部的 Sensor* 容器；不擁有！
    EvNode*  felHead;              // FEL 的 dummy head（head->next 是第一個事件）
    double   endTime;              // 模擬終止時間（秒）
    double   switchover;           // 切換/輪詢開銷 τ（秒），簡化處理：直接加在 service 完成時間
    double   now;                  // 目前模擬時間（秒）
    bool     serverBusy;           // 伺服器是否在服務某個 sensor

    // （可選）統計：用「面積法」計平均佇列長度；served 計完成數
    double   prev;                 // 上次更新統計的時間
    double   sumQ;                 // ∫ totalQueue(t) dt 的累積
    int      served;               // 完成服務的封包數

    // --------- 介面函式 ---------
    Master();                      // 設定預設值並建好 FEL 的 dummy head
    ~Master();                     // 釋放 FEL 節點

    void setSensors(std::vector<Sensor*>* v);  // 指定 sensors 容器（不複製）
    void setEndTime(double T);                 // 設定模擬時間上限
    void setSwitchOver(double s);              // 設定 τ
    void reset();                              // 將時間/狀態/FEL 清回起始
    void run();                                // 主迴圈：從 FEL 取事件一路跑到 endTime

    // --------- FEL 操作（公開讓你 debug/擴充容易） ---------
    void felClear();                           // 清空 FEL（保留 dummy）
    void felPush(double t, EventType tp, int sid); // 以時間排序插入
    bool felPop(double& t, EventType& tp, int& sid); // 取最早事件；若空回 false

    // --------- 排程策略（之後可換成多策略） ---------
    int  chooseNext();                         // 回傳要上機的 sensor id；-1 表示目前無人可服
    void startService(int sid);                // 讓 sid 開始服務，並排 DEPARTURE

    // --------- 統計（面積法） ---------
    void accumulateQ();                       // 在 now 時刻更新 sumQ/prev

    // 小常數：避免抽到非正時間
    static double EPS;
};

} // namespace sim

