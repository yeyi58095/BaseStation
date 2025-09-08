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
}

void Master::run() {
    if (!sensors || sensors->empty()) return;

	reset();

    // initial arrivals
    for (int i=0; i<(int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT(); if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_DP_ARRIVAL, i);
    }
    felPush(now + EPS, EV_HAP_POLL, -1);

	double t; EventType tp; int sid;
	//felPop is call-by-reference function, so that the parameter t, tp, sid
	// would be change after felPop
	// it will be the next time, next event type , and the operated sensor id
	while (felPop(t, tp, sid)) {
		// if over-due, the program had finished
		if (t > endTime) { now = endTime; accumulate(); break; }

		// update the the current time as the previous next time
		now = t;

		// accumulate
		accumulate();

		// operate different function on with different event type
		switch (tp) {

		// As Data Packet Arrival
		case EV_DP_ARRIVAL: {
			Sensor* s = (*sensors)[sid];

			// remember the previous drop count to detect tail-drop caused by THIS arrival
			int d0 = s->drops;

			s->enqueueArrival();
			// the function will add a new packet (struct) into the deque while the volume of deque is available
			// with setting its packet id, its service time, and how much energy(EP) it will cost

			// the following part is for printing log (ARRIVAL or DROP)
			if (s->drops > d0) {
				// this arrival was dropped because the queue was full → write a DROP line
				// Qmax could be -1 (infinite); logDrop handles how to print Q/Qmax nicely
				logDrop(now, sid, (int)s->q.size(), s->Qmax, s->energy);
			} else {
				int pid = s->lastEnqId(); // it just gets the id latest pushed; if empty, will return -1
				if (pid >= 0) {           // whether it is empty or not
					arrivedIds[sid].push_back(pid);   // push this packet id to corresponding sensor id
					if ((int)arrivedIds[sid].size() > keepLogIds)
						arrivedIds[sid].erase(arrivedIds[sid].begin());
					// avoiding out of memory, cutting the showed information if reaching the max volume
					logArrival(now, sid, pid, (int)s->q.size(), s->energy);
				}
			}

			arrivals[sid] += 1;   // arrivals (A for each sensor) +1 no matter dropped or not

			// next arrival
			{ double dt = s->sampleIT(); if (dt <= EPS) dt = EPS; // setting the time interval that next event will arrive
			  felPush(now + dt, EV_DP_ARRIVAL, sid); }            // pushing that time, event, and id to FEL

			felPush(now, EV_HAP_POLL, -1); // re-poll soon so that scheduling gets refreshed
			logSnapshot(now, "after ARRIVAL");
			break;
		}


		case EV_TX_DONE: {    // hap serviced done time
			Sensor* s = (*sensors)[sid];
			int pid = s->currentServingId();    // get the servicing id
			logEndTx(now, sid, pid, (int)s->q.size(), s->energy);

			s->finishTx();  // release the space for servicing other packet
			served[sid] += 1;     // the amount of serviced packet +1

			servedIds[sid].push_back(pid);
			if ((int)servedIds[sid].size() > keepLogIds) servedIds[sid].erase(servedIds[sid].begin());

			hapTxBusy = false; hapTxSid = -1;   // label it as it is available to service

			felPush(now , EV_HAP_POLL, -1);  //
			logSnapshot(now, "after END_TX");
			break;
		}

		case EV_CHARGE_END: {
			/*if (sid >= 0 && sid < (int)pendCharge.size()) {
				Sensor* s = (*sensors)[sid];

				// 先記 log（用加完後的值給 log），再真的加
				logChargeEnd(now, sid, pendCharge[sid], s->energy + pendCharge[sid]);
				s->addEnergy(pendCharge[sid]);    // 真正加 EP（整數，會 clamp 到 cap）
				pendCharge[sid] = 0;

				// 還沒滿 → 立刻排下一段續充（仍視為同一個 slot，chargeActive 不變）
				if (s->energy < s->E_cap) {
					int needRem = s->E_cap - s->energy;
					double tchg = needRem / std::max(s->charge_rate, 1e-9);
					if (needRem < 1) needRem = 1;
					if (tchg <= EPS) tchg = EPS;

					pendCharge[sid]  = needRem;
					chargeStartT[sid] = now;
					chargeEndT[sid]   = now + tchg;

					// 這裡 active 不變、不+1
					logChargeStart(now, sid, needRem, chargeActive, (maxChargingSlots<=0 ? -1 : maxChargingSlots));
					logSnapshot(now, "after CHARGE_CONTINUE");

					felPush(now + tchg, EV_CHARGE_END, sid);
				} else {
					// 滿了 → 結束這個充電 slot
					if (charging[sid]) { charging[sid] = false; chargeActive--; }
					chargeStartT[sid] = 0.0;
					chargeEndT[sid]   = 0.0;
				}
			}
			felPush(now + EPS, EV_HAP_POLL, -1);
			logSnapshot(now, "after CHARGE_END");  */
			break;
		}

		case EV_CHARGE_STEP: {
			if (sid < 0 || sid >= (int)sensors->size()) break;
			Sensor* s = (*sensors)[sid];
			if (!s) break;
			if (!charging[sid]) break; // 被外部中止就不動

			int before = s->energy;
			s->addEnergy(1);           // 真實 energy 直接 +1（已夾 cap）

			if (s->energy >= s->E_cap) {
				// 滿了 → 結束 slot
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
				// 還沒滿 → 排下一個 +1EP，並重置斜坡起點（只畫下一格）
				chargeStartT[sid] = now;
				double capRemain = std::max(0, s->E_cap - s->energy);
				pendCharge[sid]  = (capRemain > 1 ? 1 : (int)capRemain);

				double dt = sampleChargeStepDt(s->charge_rate, EPS);
				chargeStartT[sid] = now;        // 斜坡起點重設
				chargeNextDt[sid] = dt;         // 這一顆 EP 的實際等待時間
				felPush(now + dt, EV_CHARGE_STEP, sid);


				// 也讓排程看看是不是已經 >= r_tx 可以開傳
				felPush(now , EV_HAP_POLL, -1);
				logSnapshot(now, "after CHARGE_TICK");
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
    int totalQ = 0;        // 全系統即時佇列
    int totalEP_inst = 0;  // 這個瞬間全系統 EP（整數）

    // 這個時間切片內，要累加到 sumE_tot 的「片段總能量」
    double sliceEPsum = 0.0;

    for (int i = 0; i < N; ++i) {
        Sensor* s = (*sensors)[i];
        int qlen = (int)s->q.size();

        totalQ       += qlen;
        totalEP_inst += s->energy;

        // ←（等待中）佇列長度的時間積分（你原本就有）
        sumQ[i] += dt * qlen;

        // ★ 能量的時間積分（關鍵！）
        sumE[i] += dt * (double)s->energy;
        sliceEPsum += (double)s->energy;

        if (recordTrace) {
            traceT[i].push_back(now);
            traceQ[i].push_back(qlen);
            double meanQ = (now > 0) ? (sumQ[i] / now) : 0.0;
            traceMeanQ[i].push_back(meanQ);

            // EP 走勢：整數階梯
            traceE[i].push_back((double)s->energy);

            // 門檻線
            traceRtx[i].push_back((double)s->r_tx);
        }
    }

    // 片段總能量 × dt → 加到全系統能量的時間積分 sumE_tot
    sumE_tot += dt * sliceEPsum;

    // 服務中那顆 sensor 的 in-service 指示器積分
    if (hapTxBusy && hapTxSid >= 0 && hapTxSid < (int)busySidInt.size()) {
        busySidInt[hapTxSid] += dt;
    }

    // 系統層積分
    busySumTx      += dt * (hapTxBusy ? 1 : 0);
    chargeCountInt += dt * (double)chargeActive;

    if (recordTrace) {
        // 系統層（左圖）
        traceT_all.push_back(now);
        traceQ_all.push_back(totalQ);

        double sumQtot = 0.0;
        for (int i = 0; i < N; ++i) sumQtot += sumQ[i];
        traceMeanQ_all.push_back((now > 0) ? (sumQtot / now) : 0.0);

        // EP（整數）總量/平均（即時值，不是時間平均）
        traceE_all.push_back((double)totalEP_inst);
        traceEavg_all.push_back((N > 0) ? ((double)totalEP_inst / N) : 0.0);
    }

    prev = now;
}

void Master::scheduleIfIdle() {      // the polling part
	if (!sensors || sensors->empty()) return;
	const int N = (int)sensors->size();

	// (A) TX: if idle, pick one

	if (!hapTxBusy) {
		int pickTX = -1; double bestScore = -1.0;
		for (int i=0;i<N; ++i) {     // grab with priority
			Sensor* s = (*sensors)[i];
			if (!s->canTransmit()) continue;
			// in this part, it will check whether the energy in this sensor is enough to transmit the head packet
			// so that, the grabbed channel is all qualified to transmit,
			// don't worry for some unexpected condition
			double score = dpCoef * (double)s->q.size()
						 * epCoef * (double)s->energy;
			if (score > bestScore) { bestScore = score; pickTX = i; }
		}
		if (pickTX >= 0) {  // if grabbed
			Sensor* s = (*sensors)[pickTX];

			// peek before startTx() mutates the state
			int    pid      = s->frontId();
			double st_need  = s->frontST(); if (st_need <= EPS) st_need = EPS;
			int    epCost   = s->frontNeedEP();
			int    epBefore = s->energy;

			double st = s->startTx(); // Actually, this part is needed to be modified
			// this part means the energy will be cost suddenly if there have enough energy
			// but, in fact, it should be cost steppedly.

			if (st <= EPS) st = st_need;  // if transfer time is less than unit time, set it as unit time
			hapTxBusy = true; hapTxSid = pickTX;

			logStartTx(now, pickTX, pid, st_need, epBefore, epCost);
			logSnapshot(now, "after START_TX");

			felPush(now + switchover + st_need, EV_TX_DONE, pickTX);  // set tbe finish time, with switchover time added
		}
	}

	// (B) Charging pipeline (FDM)
	int freeSlots = (maxChargingSlots <= 0) ? 0x3fffffff : (maxChargingSlots - chargeActive); // whether more slot is available

	for (int i=0; i<N && freeSlots > 0; ++i) {
		Sensor* s = (*sensors)[i];
		if (!s) continue;
		if (s->charge_rate <= 0) continue;   // 不能充
		if (charging[i]) continue;           // 已在充

		//bool hasWork = ((int)s->q.size() > 0) || s->serving;
		//if (!hasWork) continue;              // 沒在運作就不充

		if (s->energy >= s->E_cap) continue; // 已滿當然不充

		// 判斷啟動條件：EP 低於門檻(或不夠下一包)
		int gate = std::max(s->r_tx, needEPForHead(i));
		if (s->energy >= gate) continue;     // 夠傳且未低於門檻 → 不啟動

		// ★ 符合條件才啟動，而且一啟動就補到滿
		startChargeToFull(i);
		--freeSlots;
	}
}

AnsiString Master::reportOne(int sid) const {
    /*
     * Per-sensor runtime report (no λ_off / λ_car printed).
     *
     * Notation (for sensor i = sid):
     *   T                 : run horizon (seconds)
     *   A, D, S, B        : arrivals, drops, served, end backlog
     *   sumQ[i]           : ∫ queue_length_i(t) dt  (waiting only)
     *   busySidInt[i]     : ∫ 1{sensor i in service}(t) dt
     *
     *   Lq_hat            : mean queue size (waiting only)
     *                       = sumQ[i] / T
     *   L_hat             : mean system size (queue + in-service)
     *                       = (sumQ[i] + busySidInt[i]) / T
     *   Wq_hat            : mean queue waiting time (carried pkts)
     *                       = Lq_hat / (S/T)  if S>0 else 0
     *   W_hat             : mean system time (waiting + service)
     *                       = L_hat / (S/T)   if S>0 else 0
     *   loss_rate         : D / A  (0 if A==0)
     */

    AnsiString out;
    if (!sensors || sid < 0 || sid >= (int)sensors->size()) return "Invalid sensor id.\n";

    const Sensor* s = (*sensors)[sid];
    double T = now;

    // Counters
    int    A = arrivals[sid];
    int    D = s->drops;
    int    S = served[sid];
    int    B = (int)s->q.size() + (s->serving ? 1 : 0);

    // Averages
    double Lq_hat = (T > 0) ? (sumQ[sid] + 0.0) / T : 0.0;           // mean queue size (waiting only)
    double L_hat  = (T > 0) ? (sumQ[sid] + busySidInt[sid]) / T : 0.0; // mean system size
    double thr    = (T > 0) ? ( (S > 0) ? ((double)S / T) : 0.0 ) : 0.0; // carried "rate" used internally
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

	// (Optional) keep a balance line for debugging; ok to remove if you want it lean:
	//out += "B(end)            = " + IntToStr(B) + "\n";
	// out += "A ?= D + S + B     →  " + IntToStr(A) + " ?= " + IntToStr(D + S + B) + "\n";
	double EP_mean = (T > 0) ? (sumE[sid] / T) : 0.0;
	out += "EP_mean           = " + FloatToStrF(EP_mean, ffFixed, 7, 4) + "  <-- mean energy level (time-avg)\n";

    return out;
}

AnsiString Master::reportAll() const {
    /*
     * System-wide runtime report (aggregated across all sensors).
     * No λ_off / λ_car printed; they are only used internally for Little's law.
     *
     * Totals:
     *   A = Σ arrivals[i],  D = Σ drops_i,  S = Σ served[i]
     *   sumQtot = Σ sumQ[i]
     *
     * Means (system):
     *   Lq_all  = sumQtot / T                     (mean total waiting in system)
     *   L_all   = (sumQtot + busySumTx) / T       (mean system size: waiting + in-service)
     *   Wq_all  = Lq_all / (S/T)   if S>0 else 0  (mean queue wait, carried pkts)
     *   W_all   = L_all  / (S/T)   if S>0 else 0  (mean system time)
     *   loss_all= D / A         (0 if A==0)
     *
     * Also:
     *   Lq_avg_per_sensor = (N>0) ? Lq_all / N : 0
     */

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

    double Lq_all            = (T > 0) ? (sumQtot / T) : 0.0;                  // mean total waiting in system
    double Lq_avg_per_sensor = (N > 0) ? (Lq_all / N) : 0.0;                   // mean waiting per sensor
    double thr               = (T > 0) ? ( (S > 0) ? ((double)S / T) : 0.0 ) : 0.0; // carried "rate" internal
    double Wq_all            = (thr > 0) ? (Lq_all / thr) : 0.0;
    double L_all             = (T > 0) ? ((sumQtot + busySumTx) / T) : 0.0;    // waiting + in-service
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

void Master::logArrival(double t,int sid,int pid,int q,int ep){
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  pkg=" + IntToStr(pid) +
        "  ARRIVAL      Q=" + IntToStr(q) +
        "  EP=" + IntToStr(ep)
    );
}

void Master::logStartTx(double t,int sid,int pid,double st,int epBefore,int epCost){
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  HAP      START_TX sensor=" + IntToStr(sid) +
        "  pkg=" + IntToStr(pid) +
        "  st=" + f2(st,3) +
        "  EP->" + IntToStr(epBefore - epCost)
    );
}

void Master::logEndTx(double t,int sid,int pid,int q,int epNow){
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
    AnsiString capStr = (cap <= 0) ? AnsiString("inf") : AnsiString((int)cap);
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  CHARGE_START need=" + IntToStr(need) +
        "  active=" + IntToStr(active) + AnsiString("/") + capStr
    );
}

void Master::logChargeEnd(double t,int sid,int add,int epNow){
    timeline.push_back(
        AnsiString("t=") + f2(t,3) +
        "  sensor=" + IntToStr(sid) +
        "  CHARGE_END   +" + IntToStr(add) + "EP" +
        "  EP=" + IntToStr(epNow)
    );
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

        // Arrival rate (parameter or implied)
        double itMean = -1.0, lam = -1.0;
        if (s0->ITdistri == DIST_EXPONENTIAL && s0->ITpara1 > 0) lam = s0->ITpara1;
        else {
            if (s0->ITdistri == DIST_NORMAL)   itMean = s0->ITpara1;
            if (s0->ITdistri == DIST_UNIFORM)  itMean = 0.5*(s0->ITpara1 + s0->ITpara2);
            if (itMean > 0) lam = 1.0 / itMean;
        }

        // Service rate (parameter or implied)
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

    sl->Add("# === Timeline ===");
    for (size_t i=0;i<timeline.size();++i) sl->Add(timeline[i]);
    sl->Add("");

    AnsiString out = sl->Text;
    delete sl;
    return out;
}

void Master::logDrop(double t,int sid,int q,int qmax,int ep){
    // Qmax 可能是 -1(無限)，這時就只印目前 Q
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
		s += " EP=" + IntToStr(x->energy);   // 只印整數 EP
		// 不再加 " (epVis)" 的小數顯示
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
    timeline.push_back(AnsiString("t=") + f2(t,3) + "  STATE " + AnsiString(tag));
    timeline.push_back(stateLine());
}

int Master::needEPForHead(int sid) const {
	const Sensor* s = (*sensors)[sid];
	// 現在先用固定成本 r_tx；未來若要「跟 ST 成本」，
    // 你可改成：return std::max(s->r_tx, txCostBase + txCostPerSec * E[ST]);
    return s->r_tx;
}

// Master.cpp
void Master::startChargeToFull(int sid) {  // this funciton will be triggered one time, when it is time to charge
	// then the run while-loop will only trigger EV_CHARGE_STEP until full
	Sensor* s = (*sensors)[sid];
	if (!s || charging[sid]) return;
    if (s->charge_rate <= 0) return;
    if (s->energy >= s->E_cap) return;

    charging[sid]   = true;
    chargeActive++;

    // 只用來畫「下一格」斜坡（最多 1EP；臨界時可能 <1）
	double capRemain = std::max(0, s->E_cap - s->energy);
	pendCharge[sid]  = (capRemain > 1 ? 1 : (int)capRemain);

    chargeStartT[sid] = now;      // 斜坡從現在開始
    chargeEndT[sid]   = 0.0;

    logChargeStart(now, sid, s->E_cap - s->energy,   // 給人看的 need=距滿差距
				   chargeActive, (maxChargingSlots<=0 ? -1 : maxChargingSlots));
    logSnapshot(now, "after CHARGE_START");

	// 排第一個 +1EP
	double dt = sampleChargeStepDt(s->charge_rate, EPS);  // the arrival event of ep is also random variable form
	chargeNextDt[sid] = dt;
	felPush(now + dt, EV_CHARGE_STEP, sid);
}


AnsiString Master::leftPanelSummary() const {
    TStringList* sl = new TStringList();

    const int N = (sensors ? (int)sensors->size() : 0);
    const double T = now;

    // 防呆
    if (N == 0 || T <= 0.0) {
        sl->Add("No sensors or T=0.");
        AnsiString out = sl->Text; delete sl; return out;
    }

    // 匯總 A, D, S 與 ∑Q 的時間積分
    long long A_tot = 0, D_tot = 0, S_tot = 0;
    double sumQtot = 0.0;
    for (int i = 0; i < N; ++i) {
        const Sensor* s = (*sensors)[i];
        A_tot   += arrivals[i];
        D_tot   += s->drops;
        S_tot   += served[i];
        sumQtot += sumQ[i];
    }

    // 指標計算
    const double Lq_all  = sumQtot / T;                         // DP Queuing Size（全系統平均等待佇列）
    const double Wq_all  = (S_tot > 0) ? (Lq_all / ((double)S_tot / T)) : 0.0;  // DP Waiting time（Little's law）
    const double loss_all= (A_tot > 0) ? ((double)D_tot / (double)A_tot) : 0.0; // DP Loss rate
    const double EP_mean_per_sensor =
        (N > 0) ? ((T > 0 ? (sumE_tot / (T * N)) : 0.0)) : 0.0;                 // EP Capacity Mean（時間平均、每感測器）

    // 輸出（你可以依喜好調整文案或小數位）
    sl->Add("=== Summary ===");
    sl->Add("DP Queuing Size  : " + FloatToStrF(Lq_all, ffFixed, 7, 4));
	sl->Add("DP Waiting time (Wq): " + FloatToStrF(Wq_all, ffFixed, 7, 4));
    sl->Add("DP Loss rate     : " + FloatToStrF(loss_all, ffFixed, 7, 4));
    sl->Add("EP Capacity Mean : " + FloatToStrF(EP_mean_per_sensor, ffFixed, 7, 4));

	AnsiString out = sl->Text;
    delete sl;
    return out;
}

