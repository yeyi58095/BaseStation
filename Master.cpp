#include "Master.h"
#include "Sensor.h"

#include <algorithm>
#include <System.SysUtils.hpp>
#include <System.Classes.hpp>

using namespace sim;

static AnsiString f2(double x, int prec=3) {
    return FloatToStrF(x, ffFixed, 7, prec);
}

inline double sampleChargeStepDt(double rate, double EPS) {
    if (rate <= 0) return 0.0;
    double dt = rv::exponential(rate);   // 參數是 λ（rate），平均 1/λ
    if (dt <= EPS) dt = EPS;
    return dt;
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
  keepLogIds(200),
  logStateEachEvent(true)  // default ON
{
    felHead = new EvNode(0.0, EV_DP_ARRIVAL, -1);
	felHead->next = 0;

	logMode = 0;            // 預設 CSV
	flog    = NULL;            // CSV 檔案句柄
	logFileName = "run_log.csv";  // CSV 路徑
}

Master::~Master() {
    felClear();
    delete felHead; felHead = 0;
	if (flog) { fclose(flog); flog = NULL; }
}

void Master::setSensors(std::vector<Sensor*>* v) {
    sensors = v;
    const int N = (sensors ? (int)sensors->size() : 0);

    sumQ.assign(N, 0.0);
    served.assign(N, 0);
    arrivals.assign(N, 0);
    busySidInt.assign(N, 0.0);

    chargeNextDt.assign(N, 0.0);

    charging.assign(N, false);
    pendCharge.assign(N, 0);
    chargeStartT.assign(N, 0.0);
    chargeEndT.assign(N, 0.0);

    busySumTx = 0.0;
    chargeActive = 0;
    chargeCountInt = 0.0;

    traceT.assign(N, std::vector<double>());
    traceQ.assign(N, std::vector<double>());
    traceMeanQ.assign(N, std::vector<double>());
    traceT_all.clear(); traceQ_all.clear(); traceMeanQ_all.clear();

    traceE.assign(N, std::vector<double>());
    traceRtx.assign(N, std::vector<double>());
    traceE_all.clear();
    traceEavg_all.clear();

    arrivedIds.assign(N, std::vector<int>());
    servedIds.assign(N, std::vector<int>());
    timeline.clear();

    sumE.assign(N, 0.0);
    sumE_tot = 0.0;

	// ★ TX逐步扣
    txEpRemain.assign(N, 0);
    txTickPeriod.assign(N, 0.0);
    txStartT.assign(N, 0.0);
    txEndT.assign(N, 0.0);
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
        chargeStartT[i] = 0.0; chargeEndT[i] = 0.0;
        busySidInt[i] = 0.0;
        sumE[i] = 0.0;

        chargeNextDt[i] = 0.0;

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

    for (int i=0;i<N;++i) {
        arrivedIds[i].clear();
        servedIds[i].clear();
    }
    timeline.clear();

    // TX逐步扣
    for (int i=0;i<N;++i) {
        txEpRemain[i]   = 0;
        txTickPeriod[i] = 0.0;
        txStartT[i]     = 0.0;
        txEndT[i]       = 0.0;
    }
}

void Master::run() {
    if (!sensors || sensors->empty()) return;

    reset();

    // ★ CSV 開檔
    if (logMode == LOG_CSV) {
		if (flog) { fclose(flog); flog = NULL; }
        flog = fopen(logFileName.c_str(), "wb");
        if (flog) {
            fprintf(flog, "t,event,sid,pid,q,ep,x1,x2\n");
        }
	}

    // initial arrivals
    for (int i=0; i<(int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT(); if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_DP_ARRIVAL, i);
    }
    felPush(now + EPS, EV_HAP_POLL, -1);

    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) { now = endTime; accumulate(); break; }
        now = t;
        accumulate();

        switch (tp) {

        case EV_DP_ARRIVAL: {
            Sensor* s = (*sensors)[sid];

            int d0 = s->drops;
            s->enqueueArrival();

            if (s->drops > d0) {
                logDrop(now, sid, (int)s->q.size(), s->Qmax, s->energy);
            } else {
                int pid = s->lastEnqId();
                if (pid >= 0) {
                    arrivedIds[sid].push_back(pid);
                    if ((int)arrivedIds[sid].size() > keepLogIds)
                        arrivedIds[sid].erase(arrivedIds[sid].begin());
                    logArrival(now, sid, pid, (int)s->q.size(), s->energy);
                }
            }

            arrivals[sid] += 1;

            { double dt2 = s->sampleIT(); if (dt2 <= EPS) dt2 = EPS;
              felPush(now + dt2, EV_DP_ARRIVAL, sid); }

            felPush(now, EV_HAP_POLL, -1);
            logSnapshot(now, "after ARRIVAL");
            break;
        }

		case EV_TX_DONE: {
			Sensor* s = (*sensors)[sid];

			// ★ 在結束瞬間補扣尚未扣完的 EP（不花時間）
			if (sid >= 0 && sid < (int)txEpRemain.size()) {
				int rem = txEpRemain[sid];
				if (rem > 0) {
					// 若你一開始就要求 energy >= frontNeedEP()，這裡不會負數；
					// 但保險仍夾 0
					s->energy -= rem;
					if (s->energy < 0) s->energy = 0;

					// 清空本次 TX 的逐步扣狀態
					txEpRemain[sid]   = 0;
					txTickPeriod[sid] = 0.0;
					txStartT[sid]     = 0.0;
					txEndT[sid]       = 0.0;
				}
			}

			int pid = s->currentServingId();
			logEndTx(now, sid, pid, (int)s->q.size(), s->energy);

			s->finishTx();
			served[sid] += 1;

			servedIds[sid].push_back(pid);
			if ((int)servedIds[sid].size() > keepLogIds) servedIds[sid].erase(servedIds[sid].begin());

			hapTxBusy = false; hapTxSid = -1;

			felPush(now, EV_HAP_POLL, -1);
			logSnapshot(now, "after END_TX");
			break;
		}


        case EV_CHARGE_END: {
            // 目前不用（保留未來多段充電）
            break;
        }

        case EV_CHARGE_STEP: {
            if (sid < 0 || sid >= (int)sensors->size()) break;
            Sensor* s = (*sensors)[sid];
            if (!s) break;
            if (!charging[sid]) break;

            int before = s->energy;
            s->addEnergy(1);

            if (s->energy >= s->E_cap) {
				logChargeEnd(now, sid, s->E_cap - before, s->energy);
                charging[sid] = false;
                if (chargeActive > 0) chargeActive--;
                pendCharge[sid]  = 0;
                chargeStartT[sid]= 0.0;
                chargeEndT[sid]  = 0.0;
                chargeNextDt[sid]= 0.0;

                felPush(now + EPS, EV_HAP_POLL, -1);
                logSnapshot(now, "after CHARGE_FULL");
            } else {
                chargeStartT[sid] = now;
                double capRemain = std::max(0, s->E_cap - s->energy);
                pendCharge[sid]  = (capRemain > 1 ? 1 : (int)capRemain);

                double dtc = sampleChargeStepDt(s->charge_rate, EPS);
                chargeStartT[sid] = now;
                chargeNextDt[sid] = dtc;
                felPush(now + dtc, EV_CHARGE_STEP, sid);

                felPush(now, EV_HAP_POLL, -1);
                logSnapshot(now, "after CHARGE_TICK");
            }
            break;
        }

		case EV_TX_TICK: {
            if (sid < 0 || sid >= (int)sensors->size()) break;
            if (!hapTxBusy || hapTxSid != sid) break; // 保護：只處理當前在傳的那顆
            Sensor* s = (*sensors)[sid];
            if (!s || !s->serving) break;

            // 尚未進入服務（還在 switchover）就對齊開始點
            if (now < txStartT[sid] - EPS) {
                felPush(txStartT[sid], EV_TX_TICK, sid);
                break;
            }

            // 若已到終點或不需再扣，由 EV_TX_DONE 收尾
			if (now >= txEndT[sid] - EPS || txEpRemain[sid] <= 0) {
                break;
            }

            // ★ 扣 1 EP
            if (s->energy > 0) s->energy -= 1;
            if (s->energy < 0) s->energy = 0;
            txEpRemain[sid]--;

            logTxTick(now, sid, s->currentServingId(), txEpRemain[sid], s->energy);

            // 安排下一個 tick（不可越過 txEndT）
            if (txEpRemain[sid] > 0) {
                double remT = txEndT[sid] - now;
                if (remT > EPS) {
                    double dt = std::min(txTickPeriod[sid], std::max(EPS, remT));
                    felPush(now + dt, EV_TX_TICK, sid);
                }
            }
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

	// ★ CSV 關檔
	if (flog) { fclose(flog); flog = NULL; }
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

    const int N = (sensors ? (int)sensors->size() : 0);
    int totalQ = 0;
    int totalEP_inst = 0;

    double sliceEPsum = 0.0;

    for (int i = 0; i < N; ++i) {
        Sensor* s = (*sensors)[i];
        int qlen = (int)s->q.size();

        totalQ       += qlen;
        totalEP_inst += s->energy;

        sumQ[i] += dt * qlen;

		sumE[i] += dt * (double)s->energy;
        sliceEPsum += (double)s->energy;

        if (recordTrace) {
            traceT[i].push_back(now);
            traceQ[i].push_back(qlen);
            double meanQ = (now > 0) ? (sumQ[i] / now) : 0.0;
            traceMeanQ[i].push_back(meanQ);

            traceE[i].push_back((double)s->energy);
            traceRtx[i].push_back((double)s->r_tx);
        }
    }

    sumE_tot += dt * sliceEPsum;

    if (hapTxBusy && hapTxSid >= 0 && hapTxSid < (int)busySidInt.size()) {
        busySidInt[hapTxSid] += dt;
    }

    busySumTx      += dt * (hapTxBusy ? 1 : 0);
    chargeCountInt += dt * (double)chargeActive;

    if (recordTrace) {
        traceT_all.push_back(now);
        traceQ_all.push_back(totalQ);

        double sumQtot = 0.0;
        for (int i = 0; i < N; ++i) sumQtot += sumQ[i];
        traceMeanQ_all.push_back((now > 0) ? (sumQtot / now) : 0.0);

        traceE_all.push_back((double)totalEP_inst);
        traceEavg_all.push_back((N > 0) ? ((double)totalEP_inst / N) : 0.0);
    }

    prev = now;
}

void Master::scheduleIfIdle() {
	if (!sensors || sensors->empty()) return;
    const int N = (int)sensors->size();

    // (A) TX: if idle, pick one
    if (!hapTxBusy) {
        int pickTX = -1; double bestScore = -1.0;
        for (int i=0;i<N; ++i) {
            Sensor* s = (*sensors)[i];
            if (!s->canTransmit()) continue;
            double score = dpCoef * (double)s->q.size()
                         * epCoef * (double)s->energy;
            if (score > bestScore) { bestScore = score; pickTX = i; }
        }
        if (pickTX >= 0) {
            Sensor* s = (*sensors)[pickTX];

            int    pid      = s->frontId();
            double st_need  = s->frontST(); if (st_need <= EPS) st_need = EPS;
            int    epCost   = s->frontNeedEP();
            int    epBefore = s->energy;

            double st = s->startTx(); // ★ 不再在這裡扣 EP（交由 EV_TX_TICK）
            if (st <= EPS) st = st_need;
            hapTxBusy = true; hapTxSid = pickTX;

            // 設定逐步扣參數
            txEpRemain[pickTX]   = epCost;
            txTickPeriod[pickTX] = (s->txCostPerSec > 0 ? 1.0 / s->txCostPerSec : 1e9);
            txStartT[pickTX]     = now + switchover;
            txEndT[pickTX]       = now + switchover + st_need;

            logStartTx(now, pickTX, pid, st_need, epBefore, epCost);
            logSnapshot(now, "after START_TX");

            // 完成事件
            felPush(txEndT[pickTX], EV_TX_DONE, pickTX);

            // 第一個 TX_TICK 安排在開始後（並對齊尾端）
            if (txEpRemain[pickTX] > 0) {
				double firstDt = std::min(txTickPeriod[pickTX], std::max(EPS, txEndT[pickTX] - txStartT[pickTX]));
                felPush(txStartT[pickTX] + firstDt, EV_TX_TICK, pickTX);
            }
        }
    }

	// (B) Charging pipeline (FDM)
    int freeSlots = (maxChargingSlots <= 0) ? 0x3fffffff : (maxChargingSlots - chargeActive);

    for (int i=0; i<N && freeSlots > 0; ++i) {
        Sensor* s = (*sensors)[i];
        if (!s) continue;
		if (s->charge_rate <= 0) continue;
        if (charging[i]) continue;
		if (s->energy >= s->E_cap) continue;

		bool headBlocked = ((int)s->q.size() > 0) && (s->energy < s->frontNeedEP());
        int gate = std::max(s->r_tx, needEPForHead(i));
		if (!headBlocked && s->energy >= gate) continue;

        startChargeToFull(i);
        --freeSlots;
    }
}

AnsiString Master::reportOne(int sid) const {
    AnsiString out;
    if (!sensors || sid < 0 || sid >= (int)sensors->size()) return "Invalid sensor id.\n";

    const Sensor* s = (*sensors)[sid];
    double T = now;

    int    A = arrivals[sid];
    int    D = s->drops;
    int    S = served[sid];
    int    B = (int)s->q.size() + (s->serving ? 1 : 0);

    double Lq_hat = (T > 0) ? (sumQ[sid] + 0.0) / T : 0.0;
    double L_hat  = (T > 0) ? (sumQ[sid] + busySidInt[sid]) / T : 0.0;
    double thr    = (T > 0) ? ( (S > 0) ? ((double)S / T) : 0.0 ) : 0.0;
	double Wq_hat = (thr > 0) ? (Lq_hat / thr) : 0.0;
    double W_hat  = (thr > 0) ? (L_hat  / thr) : 0.0;
    double loss   = (A > 0) ? ((double)D / (double)A) : 0.0;

    out += "Sensor " + IntToStr(sid+1) + "\n";
    out += "T                 = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)       = " + IntToStr(A) + "\n";
    out += "loss_rate         = " + FloatToStrF(loss, ffFixed, 7, 4) + "  (= D/A)\n";
    out += "Lq_hat            = " + FloatToStrF(Lq_hat, ffFixed, 7, 4) + "  <-- mean queue size\n";
    out += "Wq_hat            = " + FloatToStrF(Wq_hat, ffFixed, 7, 4) + "  (Little: Lq_hat / (S/T))\n";
    out += "L_hat             = " + FloatToStrF(L_hat,  ffFixed, 7, 4) + "  <-- mean system size (queue + in-service)\n";
    out += "W_hat             = " + FloatToStrF(W_hat,  ffFixed, 7, 4) + "  (Little: L_hat / (S/T))\n";

    double EP_mean = (T > 0) ? (sumE[sid] / T) : 0.0;
    out += "EP_mean           = " + FloatToStrF(EP_mean, ffFixed, 7, 4) + "  <-- mean energy level (time-avg)\n";

    return out;
}

AnsiString Master::reportAll() const {
    AnsiString out;
    int N = (sensors ? (int)sensors->size() : 0);
    if (N == 0) return "No sensors.\n";

    double T = now;

    long long A = 0, D = 0, S = 0;
    double sumQtot = 0.0;
    for (int i = 0; i < N; ++i) {
        const Sensor* s = (*sensors)[i];
        A += arrivals[i];
        D += s->drops;
        S += served[i];
        sumQtot += sumQ[i];
    }

    double Lq_all            = (T > 0) ? (sumQtot / T) : 0.0;
    double Lq_avg_per_sensor = (N > 0) ? (Lq_all / N) : 0.0;
    double thr               = (T > 0) ? ( (S > 0) ? ((double)S / T) : 0.0 ) : 0.0;
	double Wq_all            = (thr > 0) ? (Lq_all / thr) : 0.0;
    double L_all             = (T > 0) ? ((sumQtot + busySumTx) / T) : 0.0;
    double W_all             = (thr > 0) ? (L_all / thr) : 0.0;
    double loss_all          = (A > 0) ? ((double)D / (double)A) : 0.0;

    out += "=== Overall (All sensors) ===\n";
    out += "N                 = " + IntToStr(N) + "\n";
    out += "T                 = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)       = " + IntToStr((int)A) + "\n";
    out += "loss_rate         = " + FloatToStrF(loss_all, ffFixed, 7, 4) + "  (= D/A)\n";

    out += "Lq_all            = " + FloatToStrF(Lq_all,            ffFixed, 7, 4) + "  <-- mean total queue size (system)\n";
    out += "Lq_avg_per_sensor = " + FloatToStrF(Lq_avg_per_sensor, ffFixed, 7, 4) + "  <-- mean queue size per sensor\n";
    out += "Wq_all            = " + FloatToStrF(Wq_all,            ffFixed, 7, 4) + "  (Little: Lq_all / (S/T))\n";

    out += "L_all             = " + FloatToStrF(L_all,             ffFixed, 7, 4) + "  <-- mean system size (waiting + in-service)\n";
    out += "W_all             = " + FloatToStrF(W_all,             ffFixed, 7, 4) + "  (Little: L_all / (S/T))\n";

    double EP_sum_mean        = (T > 0) ? (sumE_tot / T) : 0.0;
    double EP_mean_per_sensor = (T > 0 && N > 0) ? (sumE_tot / (T * N)) : 0.0;

    out += "EP_sum_mean       = " + FloatToStrF(EP_sum_mean,        ffFixed, 7, 4) + "  <-- mean total energy (time-avg)\n";
    out += "EP_mean_per_sensor= " + FloatToStrF(EP_mean_per_sensor, ffFixed, 7, 4) + "  <-- mean energy per sensor (time-avg)\n";

    return out;
}

/* ==================== Logger ==================== */

// === CSV 版 ===
void Master::logCSV(double t, const char* ev, int sid, int pid, int q, int ep, double x1, double x2) {
    if (logMode != LOG_CSV || !flog) return;
    fprintf(flog, "%.6f,%s,%d,%d,%d,%d,%.6f,%.6f\n", t, ev, sid, pid, q, ep, x1, x2);
}

void Master::logTxTick(double t,int sid,int pid,int epRemain,int epNow){
    if (logMode == LOG_CSV) {
        logCSV(t, "TTK", sid, pid, -1, epNow, (double)epRemain, 0.0);
    } else {
        timeline.push_back(
            AnsiString("t=") + f2(t,3) +
			"  TX_TICK   sensor=" + IntToStr(sid) +
            "  pkg=" + IntToStr(pid) +
            "  EP=" + IntToStr(epNow) +
            "  remain=" + IntToStr(epRemain)
        );
    }
}


// === 人類可讀（保留；CSV 模式不會用 timeline） ===
void Master::logArrival(double t,int sid,int pid,int q,int ep){
    if (logMode == LOG_CSV) { logCSV(t, "ARR", sid, pid, q, ep); return; }
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  pkg=" + IntToStr(pid) +
        "  ARRIVAL      Q=" + IntToStr(q) +
        "  EP=" + IntToStr(ep)
    );
}

void Master::logStartTx(double t,int sid,int pid,double st,int epBefore,int epCost){
    if (logMode == LOG_CSV) { logCSV(t, "STX", sid, pid, -1, epBefore, st, (double)epCost); return; }
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  HAP      START_TX sensor=" + IntToStr(sid) +
        "  pkg=" + IntToStr(pid) +
        "  st=" + f2(st,3) +
        "  EP_before=" + IntToStr(epBefore) +
        "  cost=" + IntToStr(epCost) + " (will be ticked)"
    );
}

void Master::logEndTx(double t,int sid,int pid,int q,int epNow){
    if (logMode == LOG_CSV) { logCSV(t, "ETX", sid, pid, q, epNow); return; }
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  HAP      END_TX   sensor=" + IntToStr(sid) +
        "  pkg=" + IntToStr(pid) +
        "  served=" + IntToStr(served[sid] + 1) +
        "  Q=" + IntToStr(q) +
        "  EP=" + IntToStr(epNow)
    );
}

void Master::logChargeStart(double t,int sid,int need,int active,int cap){
    if (logMode == LOG_CSV) { logCSV(t, "CST", sid, -1, -1, -1, (double)active, (double)cap); return; }
	AnsiString capStr = (cap <= 0) ? AnsiString("inf") : AnsiString((int)cap);
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  CHARGE_START need=" + IntToStr(need) +
        "  active=" + IntToStr(active) + AnsiString("/") + capStr
    );
}

void Master::logChargeEnd(double t,int sid,int add,int epNow){
    if (logMode == LOG_CSV) { logCSV(t, "CEND", sid, -1, -1, epNow, (double)add, 0.0); return; }
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  CHARGE_END   +" + IntToStr(add) + "EP" +
        "  EP=" + IntToStr(epNow)
    );
}

void Master::logDrop(double t,int sid,int q,int qmax,int ep){
    if (logMode == LOG_CSV) { logCSV(t, "DROP", sid, -1, q, ep, (double)qmax, 0.0); return; }
    AnsiString qStr = (qmax >= 0)
        ? (AnsiString(IntToStr(q)) + "/" + IntToStr(qmax))
        : AnsiString(IntToStr(q));
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  DROP         Q=" + qStr +
        "  EP=" + IntToStr(ep)
    );
}

AnsiString Master::stateLine() const {
    int N = sensors ? (int)sensors->size() : 0;
    AnsiString s = "  [";
    for (int i = 0; i < N; ++i) {
        const Sensor* x = (*sensors)[i];

        s += "S" + IntToStr(i) + "=" + x->queueToStr();
		if (x->serving) s += "(S:" + IntToStr(x->servingId) + ")";
        s += " EP=" + IntToStr(x->energy);
        if (charging.size()==(size_t)N && charging[i]) s += " (chg)";
        if (i + 1 < N) s += " | ";
    }
    s += "]  HAP=";
    if (hapTxBusy && hapTxSid >= 0) {
        const Sensor* hs = (*sensors)[hapTxSid];
        s += "sid=" + IntToStr(hapTxSid) + ", pkg=" +
             (hs->currentServingId() >= 0 ? AnsiString(IntToStr(hs->currentServingId())) : AnsiString("-"));
    } else {
        s += "idle";
    }
    return s;
}

void Master::logSnapshot(double t, const char* tag) {
    if (!logStateEachEvent) return;
    if (logMode == LOG_CSV) {
        // 精簡 STAT：x1=hapBusy(0/1), x2=chargeActive；q/ep 用最新全系統即時值
        int Qtot = traceQ_all.empty() ? 0 : (int)traceQ_all.back();
        int EPtot= traceE_all.empty() ? 0 : (int)traceE_all.back();
        logCSV(t, "STAT", hapTxSid, -1, Qtot, EPtot, hapTxBusy?1.0:0.0, (double)chargeActive);
        return;
    }
    timeline.push_back(AnsiString("t=") + f2(t,3) + "  STATE " + AnsiString(tag));
    timeline.push_back(stateLine());
}

AnsiString Master::dumpLogWithSummary() const {
    TStringList *sl = new TStringList();

    sl->Add("# Run header");
    sl->Add( AnsiString("T=") + f2(now,2)
           + ", tau=" + f2(switchover,3)
           + ", N=" + IntToStr(sensors ? (int)sensors->size() : 0) );
    sl->Add("");

    // ===== Meeting-style Summary (static params only) =====
	if (sensors && !sensors->empty()) {
        const Sensor* s0 = (*sensors)[0];

        double itMean = -1.0, lam = -1.0;
        if (s0->ITdistri == DIST_EXPONENTIAL && s0->ITpara1 > 0) lam = s0->ITpara1;
        else {
            if (s0->ITdistri == DIST_NORMAL)   itMean = s0->ITpara1;
            if (s0->ITdistri == DIST_UNIFORM)  itMean = 0.5*(s0->ITpara1 + s0->ITpara2);
            if (itMean > 0) lam = 1.0 / itMean;
        }

        double stMean = -1.0, mu = -1.0;
        if (s0->STdistri == DIST_EXPONENTIAL && s0->STpara1 > 0) mu = s0->STpara1;
        else {
            if (s0->STdistri == DIST_NORMAL)   stMean = s0->STpara1;
            if (s0->STdistri == DIST_UNIFORM)  stMean = 0.5*(s0->STpara1 + s0->STpara2);
            if (stMean > 0) mu = 1.0 / stMean;
        }

        sl->Add("# Summary");
        sl->Add("DP:");
        sl->Add("  buffer size   = " + AnsiString(s0->Qmax));
        if (lam > 0) sl->Add("  arrival rate  = " + f2(lam,3));
        if (mu  > 0) sl->Add("  service rate  = " + f2(mu,3));

        sl->Add("EP:");
        sl->Add("  capacity      = " + AnsiString(s0->E_cap));
        sl->Add("  charging rate = " + f2(s0->charge_rate,3));
        sl->Add("  R (EP/sec)    = " + f2(s0->txCostPerSec,3));
        sl->Add("  threshold     = " + AnsiString(s0->r_tx));
        sl->Add("");
    }

    // CSV 模式下，不再輸出大段 timeline（避免 OOM）
    if (logMode == LOG_HUMAN) {
        sl->Add("# === Timeline ===");
        for (size_t i=0;i<timeline.size();++i) sl->Add(timeline[i]);
        sl->Add("");
    } else {
		sl->Add("# log written to: " + logFileName);
    }

    AnsiString out = sl->Text;
    delete sl;
    return out;
}

int Master::needEPForHead(int sid) const {
    const Sensor* s = (*sensors)[sid];
    if (!s) return 0;
    // 若佇列空，拿 r_tx 當門檻；有頭封包就用它的實際需求
    if ((int)s->q.size() == 0) return s->r_tx;
    int need = s->frontNeedEP();                 // 頭封包的能量需求（ceil(R*ST)）
    return std::max(s->r_tx, need);              // 與 r_tx 取最大
}


void Master::startChargeToFull(int sid) {
    Sensor* s = (*sensors)[sid];
    if (!s || charging[sid]) return;
    if (s->charge_rate <= 0) return;
    if (s->energy >= s->E_cap) return;

    charging[sid]   = true;
    chargeActive++;

    double capRemain = std::max(0, s->E_cap - s->energy);
    pendCharge[sid]  = (capRemain > 1 ? 1 : (int)capRemain);

    chargeStartT[sid] = now;
    chargeEndT[sid]   = 0.0;

    logChargeStart(now, sid, s->E_cap - s->energy,
                   chargeActive, (maxChargingSlots<=0 ? -1 : maxChargingSlots));
    logSnapshot(now, "after CHARGE_START");

	double dt = sampleChargeStepDt(s->charge_rate, EPS);
    chargeNextDt[sid] = dt;
	felPush(now + dt, EV_CHARGE_STEP, sid);
}

AnsiString Master::leftPanelSummary() const {
    TStringList* sl = new TStringList();

    const int N = (sensors ? (int)sensors->size() : 0);
    const double T = now;

    if (N == 0 || T <= 0.0) {
        sl->Add("No sensors or T=0.");
        AnsiString out = sl->Text; delete sl; return out;
    }

    long long A_tot = 0, D_tot = 0, S_tot = 0;
    double sumQtot = 0.0;
    for (int i = 0; i < N; ++i) {
        const Sensor* s = (*sensors)[i];
        A_tot   += arrivals[i];
        D_tot   += s->drops;
        S_tot   += served[i];
        sumQtot += sumQ[i];
    }

    const double Lq_all  = sumQtot / T;
    const double Wq_all  = (S_tot > 0) ? (Lq_all / ((double)S_tot / T)) : 0.0;
    const double loss_all= (A_tot > 0) ? ((double)D_tot / (double)A_tot) : 0.0;
    const double EP_mean_per_sensor =
        (N > 0) ? ((T > 0 ? (sumE_tot / (T * N)) : 0.0)) : 0.0;

    sl->Add("=== Summary ===");
    sl->Add("DP Queuing Size  : " + FloatToStrF(Lq_all, ffFixed, 7, 4));
    sl->Add("DP Waiting time (Wq): " + FloatToStrF(Wq_all, ffFixed, 7, 4));
    sl->Add("DP Loss rate     : " + FloatToStrF(loss_all, ffFixed, 7, 4));
    sl->Add("EP Capacity Mean : " + FloatToStrF(EP_mean_per_sensor, ffFixed, 7, 4));

    AnsiString out = sl->Text;
    delete sl;
    return out;
}

