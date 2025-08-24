#pragma once
#include <deque>
#include <System.hpp>  // AnsiString

// distributions
enum { DIST_NORMAL = 0, DIST_EXPONENTIAL = 1, DIST_UNIFORM = 2 };

class Sensor {
public:
    int id;

    // ====== queue: store full packet info ======
    struct Packet {
        int    id;       // packet id
        double st;       // service time (sec)
        int    needEP;   // energy cost in EP units
    };
    std::deque<Packet> q;

    bool  serving;       // being served by HAP?
    int   servingId;

    int pktSeq;
    int init_preload;

    // arrival/service distributions
    int    ITdistri, STdistri;
    double ITpara1, ITpara2;   // arrival
    double STpara1, STpara2;   // service

    // energy model (integer EP)
    int    energy;       // current EP units
    int    E_cap;        // battery capacity
    int    r_tx;         // gate threshold to allow TX
    double charge_rate;  // EP / second when charging

    // ====== NEW: energy cost model ======
    // needEP = ceil(txCostBase + txCostPerSec * st)
    double txCostBase;     // default = r_tx (downward-compatible)
    double txCostPerSec;   // default = 0

public:
    explicit Sensor(int id);

    // config
    void setIT(int method, double p1, double p2 = 0.0);
    void setST(int method, double p1, double p2 = 0.0);
    void setArrivalExp(double lambda) { setIT(DIST_EXPONENTIAL, lambda); }
    void setServiceExp(double mu)     { setST(DIST_EXPONENTIAL, mu); }

    // samples
    double sampleIT() const;
    double sampleST() const;

    // queue ops
    void   enqueueArrival();
    bool   canTransmit() const;        // q>0, energy>=max(r_tx, needEP(front)), !serving
    double startTx();                  // pop one, consume needEP, set serving, return st
    void   finishTx();

    // energy helpers
    void   addEnergy(int units);

    // info
    AnsiString toString() const;
    AnsiString queueToStr() const;

    // queue limits
    int  Qmax;   // -1 = unlimited
    int  drops;  // tail-drops
    void setQmax(int m) { Qmax = m; }
    void preloadDP(int n) { while (n-- > 0) enqueueArrival(); }

    // runtime helpers
    void resetDynamic();
    void setPreloadInit(int n) { init_preload = (n < 0 ? 0 : n); }
    int  lastEnqId() const { return q.empty() ? -1 : q.back().id; }
    int  currentServingId() const { return serving ? servingId : -1; }

    // front peek
    int    frontId()     const { return q.empty() ? -1  : q.front().id; }
    double frontST()     const { return q.empty() ? 0.0 : q.front().st; }
    int    frontNeedEP() const { return q.empty() ? 0   : q.front().needEP; }

    // energy cost for a given st
    int energyForSt(double st) const;
};

