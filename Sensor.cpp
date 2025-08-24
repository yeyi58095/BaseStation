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
  Qmax(-1), drops(0),
  pktSeq(0), init_preload(0),
  // default energy-cost model: downward-compatible
  txCostBase(r_tx), txCostPerSec(0.0)
{
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
    AnsiString msg;
    msg += "Sensor: " + IntToStr(id+1) + "\n";

    msg += "IT: ";
    switch (ITdistri) {
        case DIST_NORMAL:      msg += "mean=" + FloatToStrF(ITpara1, ffFixed, 7, 3) + ", stddev=" + FloatToStrF(ITpara2, ffFixed, 7, 3); break;
        case DIST_EXPONENTIAL: msg += "lambda=" + FloatToStrF(ITpara1, ffFixed, 7, 3); break;
        case DIST_UNIFORM:     msg += "a=" + FloatToStrF(ITpara1, ffFixed, 7, 3) + ", b=" + FloatToStrF(ITpara2, ffFixed, 7, 3); break;
        default: msg += "(unknown)"; break;
    }

    msg += "\nST: ";
    switch (STdistri) {
        case DIST_NORMAL:      msg += "mean=" + FloatToStrF(STpara1, ffFixed, 7, 3) + ", stddev=" + FloatToStrF(STpara2, ffFixed, 7, 3); break;
        case DIST_EXPONENTIAL: msg += "mu=" + FloatToStrF(STpara1, ffFixed, 7, 3); break;
        case DIST_UNIFORM:     msg += "a=" + FloatToStrF(STpara1, ffFixed, 7, 3) + ", b=" + FloatToStrF(STpara2, ffFixed, 7, 3); break;
        default: msg += "(unknown)"; break;
    }

    msg += "\nDP: Qmax=" + IntToStr(this->Qmax) +
           "\nEP: energy=" + IntToStr(energy) +
           ", cap=" + IntToStr(E_cap) +
           ", r_tx=" + IntToStr(r_tx) +
           ", charge_rate=" + FloatToStrF(charge_rate, ffFixed, 7, 3) + "\n";

    msg += "EP-cost: base=" + FloatToStrF(txCostBase, ffFixed, 7, 3) +
           ", perSec=" + FloatToStrF(txCostPerSec, ffFixed, 7, 3) + "\n";
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

    if (energy < 0) energy = 0;
    if (energy > E_cap) energy = E_cap;

	for (int i = 0; i < init_preload; ++i) enqueueArrival();
}

