#include "Sensor.h"
#include "RandomVar.h"
#include <System.SysUtils.hpp>  // IntToStr, FloatToStrF

Sensor::Sensor(int id)
: id(id),
  serving(false),
  ITdistri(DIST_EXPONENTIAL), STdistri(DIST_EXPONENTIAL),
  ITpara1(1.0), ITpara2(0.0),
  STpara1(1.5), STpara2(0.0),
  energy(0), E_cap(100), r_tx(1), charge_rate(1.0),
  Qmax(-1), drops(0) ,               // <-- 新增預設
  pktSeq(0), init_preload(0)
  {
}


void Sensor::setIT(int method, double p1, double p2) {
    ITdistri = method; ITpara1 = p1; ITpara2 = p2;
}
void Sensor::setST(int method, double p1, double p2) {
    STdistri = method; STpara1 = p1; STpara2 = p2;
}

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

void Sensor::enqueueArrival() {
	static int nextId = 1;
    if (Qmax >= 0 && (int)q.size() >= Qmax) {  // 滿了就丟
        drops++;
        return;
    }
	q.push_back(++pktSeq);
}

bool Sensor::canTransmit() const {
    return (!serving) && ((int)q.size() > 0) && (energy >= r_tx);
}

double Sensor::startTx() {
    if (serving || q.empty() || energy < r_tx) return 0.0;

    int pid = q.front();
    q.pop_front();

    serving   = true;
    servingId = pid;

    // 扣除發送所需能量（整數）
    energy -= r_tx;
    if (energy < 0) energy = 0;

    // 抽服務時間
    double st = sampleST();
    if (st <= 0) st = 1e-9;
    return st;
}


void Sensor::finishTx() {
	serving = false;
	servingId = -1;
}

void Sensor::addEnergy(int units) {
    energy += (units > 0 ? units : 0);
    if (energy > E_cap) energy = E_cap;
}

AnsiString Sensor::toString() const {
    AnsiString msg;
    msg += "Sensor: " + IntToStr(id+1) + "\n";

    msg += "IT: ";
    switch (ITdistri) {
        case DIST_NORMAL:
            msg += "mean=" + FloatToStrF(ITpara1, ffFixed, 7, 3) +
                   ", stddev=" + FloatToStrF(ITpara2, ffFixed, 7, 3);
            break;
        case DIST_EXPONENTIAL:
            msg += "lambda=" + FloatToStrF(ITpara1, ffFixed, 7, 3);
            break;
        case DIST_UNIFORM:
            msg += "a=" + FloatToStrF(ITpara1, ffFixed, 7, 3) +
                   ", b=" + FloatToStrF(ITpara2, ffFixed, 7, 3);
            break;
        default: msg += "(unknown)"; break;
    }

    msg += "\nST: ";
    switch (STdistri) {
        case DIST_NORMAL:
            msg += "mean=" + FloatToStrF(STpara1, ffFixed, 7, 3) +
                   ", stddev=" + FloatToStrF(STpara2, ffFixed, 7, 3);
            break;
        case DIST_EXPONENTIAL:
            msg += "mu=" + FloatToStrF(STpara1, ffFixed, 7, 3);
            break;
        case DIST_UNIFORM:
            msg += "a=" + FloatToStrF(STpara1, ffFixed, 7, 3) +
                   ", b=" + FloatToStrF(STpara2, ffFixed, 7, 3);
            break;
        default: msg += "(unknown)"; break;
    }

	msg += "\nDP: Qmax=" + IntToStr(this->Qmax) +
			"\nEP: energy=" + IntToStr(energy) +
		   ", cap=" + IntToStr(E_cap) +
		   ", r_tx=" + IntToStr(r_tx) +
		   ", charge_rate=" + FloatToStrF(charge_rate, ffFixed, 7, 3) + "\n";
	return msg;
}



