#include "Master.h"
#include "Sensor.h"

#include <System.SysUtils.hpp>   // FloatToStrF, IntToStr
#include <System.Classes.hpp>    // TStringList

using namespace sim;

// 小工具：格式化浮點
static AnsiString f2(double x, int prec=3) {
    return FloatToStrF(x, ffFixed, 7, prec);
}

double Master::EPS = 1e-9;

Master::Master()
: sensors(0), felHead(0),
  endTime(10000.0), now(0.0), prev(0.0),
  hapTxBusy(false), hapTxSid(-1), switchover(0.0),
  maxChargingSlots(0), chargeActive(0),
  dpCoef(1.0), epCoef(1.0),
  busySumTx(0.0), chargeCountInt(0.0),
  recordTrace(true),
  keepLogIds(200)
{
    felHead = new EvNode(0.0, EV_DP_ARRIVAL, -1); // dummy
    felHead->next = 0;
}

Master::~Master() {
    felClear();
    delete felHead; felHead = 0;
}

void Master::setSensors(std::vector<Sensor*>* v) {
    sensors = v;
    const int N = (sensors ? (int)sensors->size() : 0);

    sumQ.assign(N, 0.0);
    served.assign(N, 0);
    arrivals.assign(N, 0);

    charging.assign(N, false);
    pendCharge.assign(N, 0);

    busySumTx = 0.0;
    chargeActive = 0;
    chargeCountInt = 0.0;

    // traces
    traceT.assign(N, std::vector<double>());
    traceQ.assign(N, std::vector<double>());
    traceMeanQ.assign(N, std::vector<double>());
    traceT_all.clear(); traceQ_all.clear(); traceMeanQ_all.clear();

    traceE.assign(N, std::vector<double>());
    traceRtx.assign(N, std::vector<double>());
    traceE_all.clear();
    traceEavg_all.clear();

    // logging
    arrivedIds.assign(N, std::vector<int>());
    servedIds.assign(N,  std::vector<int>());
    timeline.clear();
}

void Master::reset() {
    now = 0.0; prev = 0.0;
    hapTxBusy = false; hapTxSid = -1;
    busySumTx = 0.0;
    chargeActive = 0;
    chargeCountInt = 0.0;

    felClear();

    const int N = (sensors ? (int)sensors->size() : 0);
    for (int i=0;i<N;++i) {
        sumQ[i] = 0.0; served[i] = 0; arrivals[i] = 0;
        charging[i] = false; pendCharge[i] = 0;
        if (recordTrace) {
            traceT[i].clear();
            traceQ[i].clear();
            traceMeanQ[i].clear();
            traceE[i].clear();
            traceRtx[i].clear();
        }
    }

    traceT_all.clear(); traceQ_all.clear(); traceMeanQ_all.clear();
    traceE_all.clear(); traceEavg_all.clear();

    // 清感測器的動態狀態（保留 UI 設定）
    for (int i=0;i<N;++i) {
        Sensor* s = (*sensors)[i];
        s->resetDynamic();
        charging[i]  = false;
        pendCharge[i] = 0;
    }
    hapTxBusy   = false; hapTxSid = -1;
    chargeActive = 0;
    felClear();
    now = prev = 0.0;
    busySumTx = 0.0;
    chargeCountInt = 0.0;

    // logger
    for (int i=0;i<N;++i) {
        arrivedIds[i].clear();
        servedIds[i].clear();
    }
    timeline.clear();
}

void Master::run() {
    if (!sensors || sensors->empty()) return;

    reset();

    // initial DP arrivals
    for (int i=0; i<(int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT(); if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_DP_ARRIVAL, i);
    }
    // initial poll
    felPush(now + EPS, EV_HAP_POLL, -1);

    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) { now = endTime; accumulate(); break; }
        now = t;
        accumulate();

        switch (tp) {
        case EV_DP_ARRIVAL: {
            Sensor* s = (*sensors)[sid];
            s->enqueueArrival();

            // logger: arrival
			int pid = s->lastEnqId();
			if (pid >= 0) {
                arrivedIds[sid].push_back(pid);
                if ((int)arrivedIds[sid].size() > keepLogIds) arrivedIds[sid].erase(arrivedIds[sid].begin());
                logArrival(now, sid, pid, (int)s->q.size(), s->energy);
            }

            arrivals[sid] += 1;

            // next arrival
            { double dt = s->sampleIT(); if (dt <= EPS) dt = EPS;
              felPush(now + dt, EV_DP_ARRIVAL, sid); }

            // try schedule
            felPush(now + EPS, EV_HAP_POLL, -1);
            break;
        }

        case EV_TX_DONE: {
            // 先抓 id 再 finish，避免 finishTx() 把狀態清掉
            Sensor* s = (*sensors)[sid];
            int pid = s->currentServingId();
            logEndTx(now, sid, pid, (int)s->q.size(), s->energy);

            s->finishTx();
            served[sid] += 1;

            // 記錄離開 id
            servedIds[sid].push_back(pid);
            if ((int)servedIds[sid].size() > keepLogIds) servedIds[sid].erase(servedIds[sid].begin());

            hapTxBusy = false; hapTxSid = -1;

            // after TX, try next TX/CHARGE
            felPush(now + EPS, EV_HAP_POLL, -1);
            break;
        }

        case EV_CHARGE_END: {
            // finish one charging session
            if (sid >= 0 && sid < (int)pendCharge.size()) {
                Sensor* s = (*sensors)[sid];

                // logger（先算加完後的值給 log）
                logChargeEnd(now, sid, pendCharge[sid], s->energy + pendCharge[sid]);

                s->addEnergy(pendCharge[sid]); // add integer EP
                pendCharge[sid] = 0;
                if (charging[sid]) { charging[sid] = false; chargeActive--; }
            }
            // after charge, try schedule
            felPush(now + EPS, EV_HAP_POLL, -1);
            break;
        }

        case EV_HAP_POLL: {
            scheduleIfIdle();
            break;
        }
        }

        if (now >= endTime) break;
    }

    if (now < endTime) { now = endTime; accumulate(); }
}

void Master::felClear() {
    EvNode* c = felHead->next;
    while (c) { EvNode* d = c; c = c->next; delete d; }
    felHead->next = 0;
}

void Master::felPush(double t, EventType tp, int sid) {
    EvNode* n = new EvNode(t, tp, sid);
    EvNode* cur = felHead;
    while (cur->next && cur->next->t <= t) cur = cur->next;
    n->next = cur->next; cur->next = n;
}

bool Master::felPop(double& t, EventType& tp, int& sid) {
    EvNode* n = felHead->next;
    if (!n) return false;
    felHead->next = n->next;
    t = n->t; tp = n->tp; sid = n->sid;
    delete n;
    return true;
}

void Master::accumulate() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int N = (sensors ? (int)sensors->size() : 0);
    int totalQ = 0;

    for (int i=0;i<N;++i) {
        Sensor* s = (*sensors)[i];
        int qlen = (int)s->q.size();
        totalQ += qlen;
        sumQ[i] += dt * qlen;

        if (recordTrace) {
            traceT[i].push_back(now);
            traceQ[i].push_back(qlen);
            double meanQ = (now > 0) ? (sumQ[i] / now) : 0.0;
            traceMeanQ[i].push_back(meanQ);

            // 每 sensor 的 EP 與門檻
            traceE[i].push_back( (double)s->energy );
            traceRtx[i].push_back( (double)s->r_tx );
        }
    }

    // HAP 忙碌度（TX）與平行充電數
    busySumTx      += dt * (hapTxBusy ? 1 : 0);
    chargeCountInt += dt * (double)chargeActive;

    if (recordTrace) {
        traceT_all.push_back(now);
        traceQ_all.push_back(totalQ);

        double sumQtot = 0.0;
        for (int i=0;i<N;++i) sumQtot += sumQ[i];
        traceMeanQ_all.push_back((now > 0) ? (sumQtot / now) : 0.0);

        double Esum = 0.0;
        for (int i=0;i<N;++i) Esum += (double)(*sensors)[i]->energy;
        traceE_all.push_back(Esum);
        traceEavg_all.push_back( (N>0)? (Esum / N) : 0.0 );
    }

    prev = now;
}

void Master::scheduleIfIdle() {
    if (!sensors || sensors->empty()) return;

    const int N = (int)sensors->size();

    // (A) TX pipeline: pick if idle
    if (!hapTxBusy) {
        int pickTX = -1; double bestScore = -1.0;
        for (int i=0;i<N; ++i) {
            Sensor* s = (*sensors)[i];
            if (!s->canTransmit()) continue; // need q>0, energy>=r_tx, !serving
            double score = dpCoef * (double)s->q.size()
                         * epCoef * (double)s->energy;
            if (score > bestScore) { bestScore = score; pickTX = i; }
        }
        if (pickTX >= 0) {
            Sensor* s = (*sensors)[pickTX];
            double st = s->startTx(); if (st <= EPS) st = EPS;
            int pid = s->currentServingId();     // 這次送哪顆封包
            hapTxBusy = true; hapTxSid = pickTX;

            // log: 開始傳（把扣能前的 EP 顯示在箭頭左側）
            logStartTx(now, pickTX, pid, st, s->energy + s->r_tx);

            felPush(now + switchover + st, EV_TX_DONE, pickTX);
        }
    }

    // (B) Charging pipeline: allow parallel charging up to slots
    int freeSlots = (maxChargingSlots <= 0)
                  ? 0x3fffffff
                  : (maxChargingSlots - chargeActive);
    for (int i=0; i<N && freeSlots > 0; ++i) {
        Sensor* s = (*sensors)[i];
        if ((int)s->q.size() <= 0) continue;     // only charge if we have data to send
        if (s->energy >= s->r_tx) continue;      // already enough
        if (s->charge_rate <= 0) continue;       // cannot charge
        if (charging[i]) continue;               // already charging

        // 充到上限（你也可以改成充 K 包）
        int target = s->E_cap;
        int need   = target - s->energy;         // integer deficit
        if (need < 1) need = 1;

        double tchg = need / std::max(s->charge_rate, 1e-9);
        if (tchg <= EPS) tchg = EPS;

        charging[i] = true;
        chargeActive++;
        pendCharge[i] = need;

        // log: 開始充電
        logChargeStart(now, i, need, chargeActive,
                       (maxChargingSlots<=0 ? -1 : maxChargingSlots));

        felPush(now + tchg, EV_CHARGE_END, i);
        freeSlots--;
    }
}

AnsiString Master::reportOne(int sid) const {
    AnsiString out;
    if (!sensors || sid < 0 || sid >= (int)sensors->size()) return "Invalid sensor id.\n";

    const Sensor* s = (*sensors)[sid];
    double T = now;

    int    A  = arrivals[sid];      // offered arrivals
    int    D  = s->drops;           // drops at queue-full
    int    S  = served[sid];        // completed
    int    B  = (int)s->q.size() + (s->serving ? 1 : 0); // backlog at end

    double Lq_hat  = (T > 0) ? (sumQ[sid] / T) : 0.0;
    double lam_off = (T > 0) ? ((double)A / T) : 0.0;    // offered rate
    double lam_car = (T > 0) ? ((double)S / T) : 0.0;    // carried rate (throughput)
    double loss    = (A > 0) ? ((double)D / (double)A) : 0.0;
    double Wq_hat  = (lam_car > 0) ? (Lq_hat / lam_car) : 0.0;

    out += "Sensor " + IntToStr(sid+1) + "\n";
    out += "T            = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)  = " + IntToStr(A) + "  (offered)\n";
    out += "drops(D)     = " + IntToStr(D) + "\n";
    out += "served(S)    = " + IntToStr(S) + "  (carried)\n";
    out += "backlog(B)   = " + IntToStr(B) + "  (end of run)\n";
    out += "Lq_hat       = " + FloatToStrF(Lq_hat,  ffFixed, 7, 4) + "\n";
    out += "lambda_off   = " + FloatToStrF(lam_off, ffFixed, 7, 4) + "\n";
    out += "lambda_car   = " + FloatToStrF(lam_car, ffFixed, 7, 4) + "  (= throughput)\n";
    out += "loss_rate    = " + FloatToStrF(loss,    ffFixed, 7, 4) + "  (= D/A)\n";
    out += "Wq_hat       = " + FloatToStrF(Wq_hat,  ffFixed, 7, 4) + "  (Little: Lq/λ_car)\n";
    out += "A ?= D + S + B  →  " + IntToStr(A) + " ?= " + IntToStr(D + S + B) + "\n";
    out += "(Shared-HAP w/ FDM charging; no simple closed form)\n";
    return out;
}

AnsiString Master::reportAll() const {
    AnsiString out;
    int N = (sensors ? (int)sensors->size() : 0);
    if (N == 0) return "No sensors.\n";
    double T = now;

    long long A=0, D=0, S=0, B=0;
    double sumQtot = 0.0;
    for (int i=0;i<N;++i) {
        const Sensor* s = (*sensors)[i];
        A += arrivals[i];
        D += s->drops;
        S += served[i];
        B += (int)s->q.size() + (s->serving ? 1 : 0);
        sumQtot += sumQ[i];
    }

    double Lq_hat   = (T > 0) ? (sumQtot / T) : 0.0;
    double lam_off  = (T > 0) ? ((double)A / T) : 0.0;
    double lam_car  = (T > 0) ? ((double)S / T) : 0.0;
    double loss     = (A > 0) ? ((double)D / (double)A) : 0.0;
    double Wq_hat   = (lam_car > 0) ? (Lq_hat / lam_car) : 0.0;
    double txBusy   = (T > 0) ? (busySumTx / T) : 0.0;
    double avgCharg = (T > 0) ? (chargeCountInt / T) : 0.0;

    out += "=== Overall ===\n";
    out += "T            = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)  = " + IntToStr((int)A) + "\n";
    out += "drops(D)     = " + IntToStr((int)D) + "\n";
    out += "served(S)    = " + IntToStr((int)S) + "\n";
	out += "backlog(B)   = " + IntToStr((int)B) + "\n";
	out += "Lq_hat       = " + FloatToStrF(Lq_hat,   ffFixed, 7, 4) + "\n";
	out += "lambda_off   = " + FloatToStrF(lam_off,  ffFixed, 7, 4) + "\n";
	out += "lambda_car   = " + FloatToStrF(lam_car,  ffFixed, 7, 4) + "\n";
	out += "loss_rate    = " + FloatToStrF(loss,     ffFixed, 7, 4) + "  (= D/A)\n";
	out += "Wq_hat       = " + FloatToStrF(Wq_hat,   ffFixed, 7, 4) + "  (Little: Lq/λ_car)\n";
	out += "TX busy      = " + FloatToStrF(txBusy,   ffFixed, 7, 4) + "\n";
	out += "avg charging = " + FloatToStrF(avgCharg, ffFixed, 7, 4) + "\n";
	out += "A ?= D + S + B  →  " + IntToStr((int)A) + " ?= " + IntToStr((int)(D+S+B)) + "\n";
	out += "(Shared-HAP w/ FDM charging; no simple closed form)\n";
	return out;
}

/* ==================== Logger ==================== */

void Master::logArrival(double t,int sid,int pid,int q,int ep){
	timeline.push_back("t=" + f2(t,3) + "  sensor=" + IntToStr(sid) +
		"  pkg=" + IntToStr(pid) + "  ARRIVAL      Q=" + IntToStr(q) + "  EP=" + IntToStr(ep));
}

void Master::logStartTx(double t,int sid,int pid,double st,int epBefore){
    AnsiString line = AnsiString("t=") + f2(t,3)
        + "  HAP      START_TX sensor=" + IntToStr(sid)
        + "  pkg=" + IntToStr(pid)
        + "  st=" + f2(st,3)
        + "  EP->" + IntToStr(epBefore - (*sensors)[sid]->r_tx);
    timeline.push_back(line);
}

void Master::logEndTx(double t,int sid,int pid,int q,int epNow){
    AnsiString line = AnsiString("t=") + f2(t,3)
        + "  HAP      END_TX   sensor=" + IntToStr(sid)
        + "  pkg=" + IntToStr(pid)
        + "  served=" + IntToStr(served[sid]+1)
        + "  Q=" + IntToStr(q)
        + "  EP=" + IntToStr(epNow);
    timeline.push_back(line);
}

void Master::logChargeStart(double t,int sid,int need,int active,int cap){
    AnsiString capStr = (cap<=0 ? AnsiString("inf") : AnsiString(IntToStr(cap)));
    AnsiString line = AnsiString("t=") + f2(t,3)
        + "  sensor=" + IntToStr(sid)
        + "  CHARGE_START need=" + IntToStr(need)
        + "  active=" + IntToStr(active) + "/" + capStr;
    timeline.push_back(line);
}

void Master::logChargeEnd(double t,int sid,int add,int epNow){
    AnsiString line = AnsiString("t=") + f2(t,3)
        + "  sensor=" + IntToStr(sid)
        + "  CHARGE_END   +" + IntToStr(add) + "EP"
        + "  EP=" + IntToStr(epNow);
    timeline.push_back(line);
}

AnsiString Master::dumpLogWithSummary() const {
    TStringList *sl = new TStringList();

    // header
    sl->Add("# Run header");
    sl->Add( AnsiString("T=") + f2(now,2)
           + ", tau=" + f2(switchover,3)
           + ", slots=" + IntToStr(maxChargingSlots)
           + ", N=" + IntToStr(sensors ? (int)sensors->size() : 0) );
    sl->Add("");

    // timeline
    sl->Add("# === Timeline ===");
    for (size_t i=0;i<timeline.size();++i) sl->Add(timeline[i]);
    sl->Add("");

    // ...後面你的 Summary 保持不變...
    // （如果有 const char* + AnsiString 的地方，同樣把左邊包成 AnsiString("...")）

    AnsiString out = sl->Text;
    delete sl;
    return out;
}

