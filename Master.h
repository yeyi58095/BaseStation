#pragma once
#include <vector>
#include <System.hpp>   // AnsiString

class Sensor;

namespace sim {

// events
enum EventType {
    EV_DP_ARRIVAL = 0,   // data packet arrival
    EV_TX_DONE    = 1,   // HAP finished a transmission
    EV_CHARGE_END = 2,   // charging finished (enough EP to tx one DP)
    EV_HAP_POLL   = 3    // trigger scheduler
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
    // external sensors (not owned)
    std::vector<Sensor*>* sensors;

    // FEL (dummy head)
    EvNode*  felHead;

    // time/settings
    double   endTime;
    double   now;
    double   prev;
    static double EPS;

    // HAP TX pipeline (one-at-a-time)
    bool   hapTxBusy;      // TX busy flag
    int    hapTxSid;       // which sid is being transmitted
    double switchover;     // TX switchover overhead (seconds)

    // Charging pipeline (FDM, can be many in parallel)
    int    maxChargingSlots; // 0 = unlimited
    int    chargeActive;     // how many are currently charging
    std::vector<bool> charging;     // per-sid "is charging?"
    std::vector<int>  pendCharge;   // pending EP units to add on CHARGE_END

    // priority weights
    double dpCoef; // weight for queue length
    double epCoef; // weight for energy

    // statistics per sensor
    std::vector<double> sumQ;     // integral of queue length
    std::vector<int>    served;   // TX completions
    std::vector<int>    arrivals; // DP arrivals

    // HAP stats
    double busySumTx;       // integral of TX busy (0/1)
    double chargeCountInt;  // integral of "how many charging in parallel"

    // traces for plots
    bool recordTrace;
    std::vector< std::vector<double> > traceT, traceQ, traceMeanQ; // per sensor
	std::vector<double> traceT_all, traceQ_all, traceMeanQ_all;    // overall

	// �C�� sensor �� EP �ɶ��ǦC
	std::vector< std::vector<double> > traceE;     // energy(t) �H double �s
	std::vector< std::vector<double> > traceRtx;   // r_tx ���e�]�e�����u�ΡF�C�I���ЦP�ȡ^

	// ����E�X
	std::vector<double> traceE_all;      // ���� EP �`�M
	std::vector<double> traceEavg_all;   // ���� EP ����


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

    // scheduler (FDM): TX and CHARGE are decided independently
    void scheduleIfIdle();

    // reports
    AnsiString reportOne(int sid) const;
	AnsiString reportAll() const;

public: // for logger
	// --- logging�]public �� UI �i���^ ---
	int keepLogIds;                         // �C�� list �̦h�O�d�X�� id
	std::vector<AnsiString> timeline;             // �ƥ�v��
	std::vector< std::vector<int> > arrivedIds;     // �C sensor ����F id
	std::vector< std::vector<int> > servedIds;      // �C sensor �����} id

	void logArrival(double t,int sid,int pid,int q,int ep);
	void logStartTx(double t,int sid,int pid,double st,int epBefore);
	void logEndTx(double t,int sid,int pid,int q,int epNow);
	void logChargeStart(double t,int sid,int need,int active,int cap);
	void logChargeEnd(double t,int sid,int add,int epNow);

	// �� timeline + summary �ꦨ�@�q��r
	AnsiString dumpLogWithSummary() const;

};

} // namespace sim

