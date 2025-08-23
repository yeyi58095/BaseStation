#pragma once
#include <vector>
#include <System.hpp>   // for AnsiString
// 前置宣告，避免循環 include；Master.cpp 再 #include "Sensor.h"
class Sensor;

namespace sim {

// 事件型別：只有「到達/離開」兩種即可（每個 sid 獨立服務）
enum EventType { EV_ARRIVAL = 0, EV_DEPARTURE = 1 };

// FEL 節點（時間遞增的單向串列）
struct EvNode {
    double    t;     // 事件時間
    EventType tp;    // 類型
    int       sid;   // 哪個 sensor
    EvNode*   next;  // 下一個
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

// =======================
// Master：事件管理 + 統計
//（每個 Sensor 各自一台伺服器，不互相排他）
// =======================
class Master {
public:  // 你指定全部公開，方便監看
    // ---- 外部提供的 sensors（Master 不擁有生命週期） ----
    std::vector<Sensor*>* sensors;

    // ---- FEL（dummy head）----
    EvNode*  felHead;

    // ---- 模擬設定/狀態 ----
    double   endTime;      // 模擬終止時間
    double   now;          // 目前時間
    double   prev;         // 上一次積分時間
    static double EPS;     // 最小正時間

    // ---- 每個 sensor 的統計（length = sensors->size()）----
    // 面積法：sumQ[i] = ∫ queue_i(t) dt
    //         sumL[i] = ∫ systemSize_i(t) dt（= queue_i + (serving?1:0)）
    // busySum[i] = ∫ 1_{serving_i}(t) dt
    std::vector<double> sumQ;
    std::vector<double> sumL;
    std::vector<double> busySum;

    // 計數
    std::vector<int>    served;     // 完成數
    std::vector<int>    arrivals;   // 到達數

public:
    Master();
    ~Master();

    // 指定 sensors 容器後，Master 會根據大小配置統計陣列
    void setSensors(std::vector<Sensor*>* v);

    void setEndTime(double T) { endTime = T; }

    // 重新歸零狀態與 FEL（不刪 sensors）
    void reset();

    // 主迴圈：排每個 sid 的第一個 ARRIVAL，然後一直 pop 最早事件
    void run();

    // ===== FEL 操作（公開，方便你 debug）=====
    void felClear();                                  // 清空（保留 dummy）
    void felPush(double t, EventType tp, int sid);    // 依時間插入
    bool felPop(double& t, EventType& tp, int& sid);  // 取出最早事件

    // 積分統計：把 [prev, now] 這段對每個 sid 的 Q/L/Busy 都加起來
    void accumulate();

    // 給 UI 的單一 Sensor 報表（回傳 AnsiString）
	AnsiString reportOne(int sid) const;
	AnsiString reportAll() const;



// ... 你的 Master 類別其它成員都保留

public:
    bool recordTrace;  // 是否紀錄軌跡（預設 true）

    // 每個 sensor 的時間序列：t、即時Q、平均Q
    std::vector< std::vector<double> > traceT;
    std::vector< std::vector<double> > traceQ;
    std::vector< std::vector<double> > traceMeanQ;

    // 全體加總的時間序列
    std::vector<double> traceT_all;
    std::vector<double> traceQ_all;
	std::vector<double> traceMeanQ_all;
};

} // namespace sim

