#pragma once
#include <vector>
#include <cstdio>       // FILE*, fopen, fprintf, fclose
#include <System.hpp>   // AnsiString
#include "RandomVar.h"  // �� rv::exponential()

class Sensor;

namespace sim {

// events
enum EventType {
    EV_DP_ARRIVAL = 0,   // data packet arrival
    EV_TX_DONE    = 1,   // HAP finished a transmission
    EV_CHARGE_END = 2,   // charging finished for this segment (�ثe���ϥ�)
    EV_HAP_POLL   = 3,   // trigger scheduler
    EV_CHARGE_STEP= 4,   // �R�q�v�B (+1 EP)
    EV_TX_TICK    = 5    // �� �ǰe�v�B (-1 EP)
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

	// �ֿn�uEP==0�v���ɶ��]�`�M�^
	double zeroEP_sum;              // �� (EP==0 ���P������) dt
	std::vector<double> zeroEP_time;// �U�P���� �� 1[EP==0] dt�]�p�� per-sensor�^


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
    std::vector<double> chargeNextDt;  // ���� +1EP ����˵��ݮɶ��]�u����ı�Ρ^

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

	// �� TX�v�B���Ϊ����A
    std::vector<int>    txEpRemain;   // �|�ݦ����� EP ����
    std::vector<double> txTickPeriod; // �C�� 1EP ���ɶ� (= 1 / R)
    std::vector<double> txStartT;     // �u���}�l�ǰe (�L�� switchover)
    std::vector<double> txEndT;       // �u�������ǰe (= start + ST)

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
    // === �s�G�Ҧ��P�ɮ� ===
	enum LogMode { LOG_HUMAN = 0, LOG_CSV = 1, LOG_NONE = 2 };
	LogMode     logMode    ;         // �w�] CSV
	FILE*       flog         ;       // CSV �ɮץy�`
	AnsiString  logFileName ; // CSV ���|

    // �¡]�H���iŪ�^�Ҧ��~�|�Ψ�
    int keepLogIds;
    std::vector<AnsiString> timeline;
    std::vector< std::vector<int> > arrivedIds;
    std::vector< std::vector<int> > servedIds;

    // human ���]�O�d�^
    void logArrival(double t,int sid,int pid,int q,int ep);
    void logStartTx(double t,int sid,int pid,double st,int epBefore,int epCost);
    void logEndTx(double t,int sid,int pid,int q,int epNow);
    void logChargeStart(double t,int sid,int need,int active,int cap);
    void logChargeEnd(double t,int sid,int add,int epNow);
    void logDrop(double t,int sid,int q,int qmax,int ep);
    void logSnapshot(double t, const char* tag);
    AnsiString dumpLogWithSummary() const;
    bool logStateEachEvent;

    // CSV ���]�s�W�^
    void logCSV(double t, const char* ev, int sid, int pid, int q, int ep, double x1=0.0, double x2=0.0);
    void logTxTick(double t,int sid,int pid,int epRemain,int epNow);

    // misc
    AnsiString stateLine() const;

    int  needEPForHead(int sid) const;      // �ثe�^�� r_tx�F�Y�n�� ST �����A�b���ק�
	void startChargeToFull(int sid);        // �Ƥ@�q�u�ɨ캡�v���R�q
	AnsiString leftPanelSummary() const;
    // ���񭫸�ơFkeepSensors=true ��ܫO�d Sensor ���СA���R����
	void purgeHeavyData(bool keepSensors);

	// �M���R�� sensors�]delete ��M vector�^
	void freeSensors();
	// �b class Master �� public �ϰ�[�W�G
	void shrinkToPlotOnly(bool keepSensors);
public:
	struct KPIs {
		double L;           // �����t�Τ��ƶq
		double W;           // �����t�γr�d�ɶ�(��)
		double Lq;          // �������C����
		double Wq;          // �������ݮɶ�(��)
		double loss_rate;   // ��]�v D/A
		double EP_mean;     // EP �����]���饭���^
		double avg_delay_ms;// �D�n��X(�@��) = W*1000
		int    S_total;     // �A���`��
		int    A_total;     // ��F�`��
		 double P_es;
	};

	// �ھڲ{���έp���p�� KPI�]���g�ɡB���ʤ������A�^
	bool computeKPIs(KPIs& out) const;
	};




} // namespace sim

