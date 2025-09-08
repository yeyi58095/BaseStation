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
  energy(0), E_cap(100), r_tx(1), charge_rate(1.0),
  Qmax(10), drops(0),
  pktSeq(0), init_preload(0),
  // default energy-cost model: downward-compatible
  txCostBase(r_tx), txCostPerSec(0.0)
{
}

// 盢疊翴计Αて㏕﹚计锣Θ AnsiString
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
    double need = txCostBase + txCostPerSec * st;
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
    return energy >= gate;
}

double Sensor::startTx() {
    if (!canTransmit()) return 0.0;
    Packet p = q.front(); q.pop_front();
    serving   = true;
    servingId = p.id;

    // consume actual energy of this packet
    energy -= p.needEP;
    if (energy < 0) energy = 0;

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
    AnsiString out;

    out += "Sensor " + IntToStr(id + 1) + "  [static config]\n";

    // === IT (Interarrival Time) ===
    out += "IT RV: ";
    switch (ITdistri) {
        case DIST_NORMAL:
            out += "Normal(mean=" + FixStr(ITpara1) +
                   ", stddev="    + FixStr(ITpara2) + ")";
            break;
        case DIST_EXPONENTIAL: {
            double rate = ITpara1;
            AnsiString meanStr;
            if (rate > 0.0) meanStr = FixStr(1.0 / rate);
            else            meanStr = "inf";
            out += "Exponential(rate=" + FixStr(rate) +
                   ", mean=" + meanStr + ")";
            break;
        }
        case DIST_UNIFORM:
            out += "Uniform(a=" + FixStr(ITpara1) +
                   ", b="       + FixStr(ITpara2) + ")";
            break;
        default:
            out += "(unknown)";
            break;
    }
    out += "\n";

    // === ST (Service Time / TX time per packet) ===
    out += "ST RV: ";
    switch (STdistri) {
        case DIST_NORMAL:
            out += "Normal(mean=" + FixStr(STpara1) +
                   ", stddev="    + FixStr(STpara2) + ")";
            break;
        case DIST_EXPONENTIAL: {
            double rate = STpara1;
            AnsiString meanStr;
            if (rate > 0.0) meanStr = FixStr(1.0 / rate);
            else            meanStr = "inf";
            out += "Exponential(rate=" + FixStr(rate) +
                   ", mean=" + meanStr + ")";
            break;
        }
        case DIST_UNIFORM:
            out += "Uniform(a=" + FixStr(STpara1) +
                   ", b="       + FixStr(STpara2) + ")";
            break;
        default:
            out += "(unknown)";
            break;
    }
    out += "\n";

    // === DP (data queue) ===
    AnsiString qmaxStr;
    if (Qmax < 0) qmaxStr = "inf";
    else          qmaxStr = IntToStr(Qmax);
    out += "DP: Qmax=" + qmaxStr +
		   ", preload=" + IntToStr(init_preload) + "\n";

    // === EP (energy params) ===  “繰篈把计
    out += "EP: cap=" + AnsiString(IntToStr(E_cap)) +
           ", r_tx=" + AnsiString(IntToStr(r_tx)) +
           ", charge_rate=" + FixStr(charge_rate) + " (EP/sec)\n";

    // === EP-cost model ===
    out += "EP-cost: base=" + FixStr(txCostBase) +
           ", perSec=" + FixStr(txCostPerSec) +
		   "  \nneedEP = ceil(base + perSec * ST), min=1\n";

    return out;
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

    if (energy < 0) energy = 0;
    if (energy > E_cap) energy = E_cap;

	for (int i = 0; i < init_preload; ++i) enqueueArrival();
}

