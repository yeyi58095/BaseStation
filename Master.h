#pragma once
#include <vector>
#include <cstdio>       // FILE*, fopen, fprintf, fclose
#include <System.hpp>   // AnsiString
#include "RandomVar.h"  // 用 rv::exponential()

class Sensor;

namespace sim {

// events
enum EventType {
    EV_DP_ARRIVAL = 0,   // data packet arrival
    EV_TX_DONE    = 1,   // HAP finished a transmission
    EV_CHARGE_END = 2,   // charging finished for this segment (目前未使用)
    EV_HAP_POLL   = 3,   // trigger scheduler
    EV_CHARGE_STEP= 4,   // 充電逐步 (+1 EP)
    EV_TX_TICK    = 5    // ★ 傳送逐步 (-1 EP)
};

// FEL node
struct EvNode {
    double    t;
    EventType tp;
    int       sid;
    EvNode*   next;
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

class Master {
public:
    std::vector<Sensor*>* sensors;

	EvNode*  felHead;
	bool alwaysCharge;

    // time/settings
    double   endTime;
    double   now;
    double   prev;
    static double EPS;

    // HAP TX pipeline (one at a time)
    bool   hapTxBusy;
    int    hapTxSid;
    double switchover;

    // Charging pipeline (FDM)
    int    maxChargingSlots;   // 0 = unlimited
    int    chargeActive;       // how many charging now
    std::vector<bool>  charging;
    std::vector<int>   pendCharge;     // planned added EP for current segment
    std::vector<double> chargeStartT;
    std::vector<double> chargeEndT;
    std::vector<double> chargeNextDt;  // 本次 +1EP 的抽樣等待時間（只給視覺用）

    // scheduling weights
    double dpCoef;
    double epCoef;

    // stats per sensor
    std::vector<double> sumQ;
    std::vector<int>    served;
    std::vector<int>    arrivals;

    // per-sensor time integral of "being served"
    std::vector<double> busySidInt;

    // HAP stats
    double busySumTx;
    double chargeCountInt;

    // traces
    bool recordTrace;
    std::vector< std::vector<double> > traceT, traceQ, traceMeanQ;
    std::vector<double> traceT_all, traceQ_all, traceMeanQ_all;

    std::vector< std::vector<double> > traceE;    // EP(t)
	std::vector< std::vector<double> > traceRtx;  // r_tx
    std::vector<double> traceE_all;               // sum EP (instant)
    std::vector<double> traceEavg_all;            // avg EP (instant)

    // EP time integral
    std::vector<double> sumE;   // \int EP_i(t) dt
    double sumE_tot;            // \int \sum_i EP_i(t) dt

	// ★ TX逐步扣用的狀態
    std::vector<int>    txEpRemain;   // 尚需扣除的 EP 次數
    std::vector<double> txTickPeriod; // 每扣 1EP 的時間 (= 1 / R)
    std::vector<double> txStartT;     // 真正開始傳送 (過完 switchover)
    std::vector<double> txEndT;       // 真正結束傳送 (= start + ST)

public:
    Master();
    ~Master();

    void setSensors(std::vector<Sensor*>* v);
    void setEndTime(double T)        { endTime = T; }
    void setSwitchOver(double s)     { switchover = s; }
    void setChargingSlots(int slots) { maxChargingSlots = slots; } // 0 = unlimited

    void reset();
    void run();

    // FEL ops
    void felClear();
    void felPush(double t, EventType tp, int sid);
    bool felPop(double& t, EventType& tp, int& sid);

    // stats
    void accumulate();

    // scheduler
    void scheduleIfIdle();

    // reports
    AnsiString reportOne(int sid) const;
    AnsiString reportAll() const;

public: // logger
    // === 新：模式與檔案 ===
	enum LogMode { LOG_HUMAN = 0, LOG_CSV = 1, LOG_NONE = 2 };
	LogMode     logMode    ;         // 預設 CSV
	FILE*       flog         ;       // CSV 檔案句柄
	AnsiString  logFileName ; // CSV 路徑

    // 舊（人類可讀）模式才會用到
    int keepLogIds;
    std::vector<AnsiString> timeline;
    std::vector< std::vector<int> > arrivedIds;
    std::vector< std::vector<int> > servedIds;

    // human 版（保留）
    void logArrival(double t,int sid,int pid,int q,int ep);
    void logStartTx(double t,int sid,int pid,double st,int epBefore,int epCost);
    void logEndTx(double t,int sid,int pid,int q,int epNow);
    void logChargeStart(double t,int sid,int need,int active,int cap);
    void logChargeEnd(double t,int sid,int add,int epNow);
    void logDrop(double t,int sid,int q,int qmax,int ep);
    void logSnapshot(double t, const char* tag);
    AnsiString dumpLogWithSummary() const;
    bool logStateEachEvent;

    // CSV 版（新增）
    void logCSV(double t, const char* ev, int sid, int pid, int q, int ep, double x1=0.0, double x2=0.0);
    void logTxTick(double t,int sid,int pid,int epRemain,int epNow);

    // misc
    AnsiString stateLine() const;

    int  needEPForHead(int sid) const;      // 目前回傳 r_tx；若要跟 ST 有關，在此修改
	void startChargeToFull(int sid);        // 排一段「補到滿」的充電
	AnsiString leftPanelSummary() const;
    // 釋放重資料；keepSensors=true 表示保留 Sensor 指標，不刪物件
	void purgeHeavyData(bool keepSensors);

	// 專門刪掉 sensors（delete 後清 vector）
	void freeSensors();
	// 在 class Master 的 public 區域加上：
	void shrinkToPlotOnly(bool keepSensors);

	};

} // namespace sim

