#pragma once
#include <deque>
#include <System.hpp>  // AnsiString

// distributions: keep same ids as your project
enum { DIST_NORMAL = 0, DIST_EXPONENTIAL = 1, DIST_UNIFORM = 2 };

class Sensor {
public:
    // identity
    int id;

    // data queue (DP queue)
    std::deque<int> q;
    bool  serving;       // true when this sensor is being served by HAP

    // arrival/service distributions
    int    ITdistri, STdistri;
    double ITpara1, ITpara2;   // for arrival
    double STpara1, STpara2;   // for service

    // energy model (integer EP)
    int    energy;       // current EP units
    int    E_cap;        // battery capacity (max EP)
    int    r_tx;         // EP units required per DP
    double charge_rate;  // EP units per second when HAP is charging this sensor

public:
    explicit Sensor(int id);

    // config (UI)
    void setIT(int method, double p1, double p2 = 0.0);
    void setST(int method, double p1, double p2 = 0.0);
    void setArrivalExp(double lambda) { setIT(DIST_EXPONENTIAL, lambda); }
    void setServiceExp(double mu)     { setST(DIST_EXPONENTIAL, mu); }

    // samples
    double sampleIT() const;
    double sampleST() const;

    // queue ops
    void   enqueueArrival();
    bool   canTransmit() const;        // have DP and enough EP and not serving
    double startTx();                  // pop 1 DP, consume EP, set serving, return service time
    void   finishTx();                 // clear serving flag

    // energy helpers
    void   addEnergy(int units);       // increase EP by integer units, clamp to cap

    // info
    AnsiString toString() const;
};

