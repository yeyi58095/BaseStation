#include "Sensor.h"
#include "RandomVar.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <cmath>

Sensor::Sensor(int id)
: id(id),
  serving(false),
  ITdistri(DIST_EXPONENTIAL), STdistri(DIST_EXPONENTIAL),
  ITpara1(1.0), ITpara2(0.0),
  STpara1(1.5), STpara2(0.0),
  energy(0), E_cap(100), r_tx(5), charge_rate(1.0),
  Qmax(100000), drops(0),
  pktSeq(0), init_preload(0),
  // default energy-cost model: downward-compatible
  txCostBase(0), txCostPerSec(1.0)
{
}

// 將浮點數格式化為固定小數、並轉成 AnsiString
static AnsiString FixStr(double v, int prec = 3) {
    return AnsiString(FloatToStrF(v, ffFixed, 7, prec));
}

void Sensor::setIT(int method, double p1, double p2) { ITdistri = method; ITpara1 = p1; ITpara2 = p2; }
void Sensor::setST(int method, double p1, double p2) { STdistri = method; STpara1 = p1; STpara2 = p2; }

double Sensor::sampleIT() const {
    switch (ITdistri) {
        case DIST_NORMAL:      return rv::normal(ITpara1, ITpara2);
        case DIST_EXPONENTIAL: return rv::exponential(ITpara1);
        case DIST_UNIFORM:     return rv::uniform(ITpara1, ITpara2);
        default:               return rv::exponential(1.0);
    }
}
double Sensor::sampleST() const {
    switch (STdistri) {
        case DIST_NORMAL:      return rv::normal(STpara1, STpara2);
        case DIST_EXPONENTIAL: return rv::exponential(STpara1);
        case DIST_UNIFORM:     return rv::uniform(STpara1, STpara2);
        default:               return rv::exponential(1.0);
    }
}

int Sensor::energyForSt(double st) const {
    if (st < 0) st = 0;
	double need =  txCostPerSec * st;
	int needInt = (int)std::ceil(need - 1e-12);
    if (needInt < 1) needInt = 1;
    return needInt;
}

void Sensor::enqueueArrival() {
    if (Qmax >= 0 && (int)q.size() >= Qmax) { drops++; return; }
    double st = sampleST(); if (st <= 0) st = 1e-9;
    Packet p;
    p.id     = ++pktSeq;
    p.st     = st;
	p.needEP = energyForSt(st);
    q.push_back(p);
}

bool Sensor::canTransmit() const {
	if (serving || q.empty()) return false;
	int need = frontNeedEP();
	int gate = (need > r_tx ? need : r_tx);  // keep r_tx as a gate
	gate = std::min(gate, E_cap);            // ★ 不能比容量還大
	return energy >= gate;
}

double Sensor::startTx() {
    if (!canTransmit()) return 0.0;
    Packet p = q.front(); q.pop_front();
    serving   = true;
    servingId = p.id;

    // consume actual energy of this packet
	//energy -= p.needEP;
	//if (energy < 0) energy = 0;

    return p.st;
}

void Sensor::finishTx() {
    serving = false;
    servingId = -1;
}

void Sensor::addEnergy(int units) {
    if (units > 0) energy += units;
    if (energy > E_cap) energy = E_cap;
}

AnsiString Sensor::toString() const {
    AnsiString msg;
    msg += "Sensor " + IntToStr(id+1) + "\n";

    // ===== DP (traffic) =====
    msg += "DP:\n";
    msg += "  Buffer size = " + IntToStr(Qmax) + "\n";

    // Arrival (IT) -> show distribution + arrival rate (if available)
    msg += "  Arrival: ";
    double itMean = -1.0; // mean inter-arrival
    switch (ITdistri) {
        case DIST_EXPONENTIAL:
            msg += "exponential, rate λ=" + FloatToStrF(ITpara1, ffFixed, 7, 3);
            if (ITpara1 > 0) itMean = 1.0 / ITpara1;
            break;
        case DIST_NORMAL:
            msg += "normal, mean=" + FloatToStrF(ITpara1, ffFixed, 7, 3)
                + ", stddev=" + FloatToStrF(ITpara2, ffFixed, 7, 3);
            itMean = ITpara1;
            break;
        case DIST_UNIFORM:
            msg += "uniform, a=" + FloatToStrF(ITpara1, ffFixed, 7, 3)
                + ", b=" + FloatToStrF(ITpara2, ffFixed, 7, 3);
            itMean = 0.5 * (ITpara1 + ITpara2);
            break;
        default:
            msg += "(unknown)";
            break;
    }
    if (itMean > 0) {
        double lam = 1.0 / itMean; // implied arrival rate
        msg += "\n  Arrival rate λ ≈ " + FloatToStrF(lam, ffFixed, 7, 3);
    }

    // Service time (ST) -> show distribution + service rate (if available)
    msg += "\n  Service time: ";
    double stMean = -1.0; // mean service time
    switch (STdistri) {
        case DIST_EXPONENTIAL:
            msg += "exponential, rate μ=" + FloatToStrF(STpara1, ffFixed, 7, 3);
            if (STpara1 > 0) stMean = 1.0 / STpara1;  // mean = 1/μ
            break;
        case DIST_NORMAL:
            msg += "normal, mean=" + FloatToStrF(STpara1, ffFixed, 7, 3)
                + ", stddev=" + FloatToStrF(STpara2, ffFixed, 7, 3);
            stMean = STpara1;
            break;
        case DIST_UNIFORM:
            msg += "uniform, a=" + FloatToStrF(STpara1, ffFixed, 7, 3)
                + ", b=" + FloatToStrF(STpara2, ffFixed, 7, 3);
            stMean = 0.5 * (STpara1 + STpara2);
            break;
        default:
            msg += "(unknown)";
            break;
    }
    if (stMean > 0) {
        double mu = 1.0 / stMean; // implied service rate
        msg += "\n  Service rate μ ≈ " + FloatToStrF(mu, ffFixed, 7, 3);
    }

    // ===== EP (energy/power) =====
    // Rename per meeting:
    //   E_cap -> Capacity
    //   r_tx  -> Threshold
    //   R     -> energy per second during service (use txCostPerSec as R)
    msg += "\nEP:\n";
    msg += "  Capacity       = " + IntToStr(E_cap) + "\n";
    msg += "  Charging rate  = " + FloatToStrF(charge_rate, ffFixed, 7, 3) + "  (exp waiting, mean 1/λ)\n";
    msg += "  R (EP/sec)     = " + FloatToStrF(txCostPerSec, ffFixed, 7, 3) + "\n";
    msg += "  Threshold      = " + IntToStr(r_tx) + "  (min EP to start a transmission)\n";
    msg += "  Energy model   = ceil(R * service_time)\n";

    return msg;
}

AnsiString Sensor::queueToStr() const {
    AnsiString s = "{";
    for (size_t i = 0; i < q.size(); ++i) {
        s += IntToStr(q[i].id);
        if (i + 1 < q.size()) s += ", ";
    }
    s += "}";
    return s;
}

void Sensor::resetDynamic() {
    q.clear();
	drops   = 0;

	serving = false;
	servingId = -1;
	pktSeq  = 0;
	energy = 0;
    if (energy < 0) energy = 0;
    if (energy > E_cap) energy = E_cap;

	for (int i = 0; i < init_preload; ++i) enqueueArrival();
}

