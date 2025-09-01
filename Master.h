#pragma once
#include <vector>
#include <System.hpp>   // AnsiString
#include "RandomVar.h"   // �� rv::exponential()

class Sensor;

namespace sim {

// events
enum EventType {
    EV_DP_ARRIVAL = 0,   // data packet arrival
    EV_TX_DONE    = 1,   // HAP finished a transmission
    EV_CHARGE_END = 2,   // charging finished for this segment
	EV_HAP_POLL   = 3,    // trigger scheduler
	EV_CHARGE_STEP = 4   // �C��tick(+1 EP) ���γo�Өƥ�

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
    std::vector<bool> charging;
    std::vector<int>  pendCharge;   // planned added EP for current segment

	std::vector<double> chargeStartT;
	std::vector<double> chargeEndT;
	std::vector<double> chargeNextDt;   // ���� +1EP ����˵��ݮɶ��]�u����ı�Ρ^

    // scheduling weights
    double dpCoef;
    double epCoef;

    // stats per sensor
    std::vector<double> sumQ;
    std::vector<int>    served;
	std::vector<int>    arrivals;

	// NEW: per-sensor time integral of "being served"
	std::vector<double> busySidInt;

    // HAP stats
    double busySumTx;
    double chargeCountInt;

    // traces
    bool recordTrace;
    std::vector< std::vector<double> > traceT, traceQ, traceMeanQ;
    std::vector<double> traceT_all, traceQ_all, traceMeanQ_all;

    std::vector< std::vector<double> > traceE;    // EP(t) with charging slope
	std::vector< std::vector<double> > traceRtx;  // r_tx for plotting
    std::vector<double> traceE_all;               // sum EP
    std::vector<double> traceEavg_all;            // avg EP

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
    int keepLogIds;
    std::vector<AnsiString> timeline;
    std::vector< std::vector<int> > arrivedIds;
    std::vector< std::vector<int> > servedIds;

    void logArrival(double t,int sid,int pid,int q,int ep);
    void logStartTx(double t,int sid,int pid,double st,int epBefore,int epCost);
    void logEndTx(double t,int sid,int pid,int q,int epNow);
    void logChargeStart(double t,int sid,int need,int active,int cap);
	void logChargeEnd(double t,int sid,int add,int epNow);
	void logDrop(double t, int sid, int q, int qmax, int ep);

    AnsiString dumpLogWithSummary() const;

    bool logStateEachEvent;
    AnsiString stateLine() const;
	void logSnapshot(double t, const char* tag);

	int needEPForHead(int sid) const;      // �ثe���^�� r_tx�A����A�n���u�� ST �����v�i�b�o�̧�
	void startChargeToFull(int sid);       // �⦨�@�Ө禡�G�Ƥ@�q�u�ɨ캡�v���R�q
};

} // namespace sim

