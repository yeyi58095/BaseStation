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
			s->enqueueArrival();
			// the funciton will add a new packet (struct) into the deque while the volume of deque is available
			// with setting its packet id, its servcie, and how much energy(ep) will it cost

			// the following part is for printing log
			int pid = s->lastEnqId(); // it just get the id lasest pushed, if is empty, will return -1
			if (pid >= 0) { //whetheter it is empty or not
				arrivedIds[sid].push_back(pid);   // push this packet id to corresponding sensor id
				if ((int)arrivedIds[sid].size() > keepLogIds) arrivedIds[sid].erase(arrivedIds[sid].begin());
				//avoiding out of memory, cutting the showed information if reached the max volume
				logArrival(now, sid, pid, (int)s->q.size(), s->energy);
			}
			arrivals[sid] += 1;   // arrivals (A for each sensor) +1

            // next arrival
			{ double dt = s->sampleIT(); if (dt <= EPS) dt = EPS; // setting the time interval that next event will arrival
			  felPush(now + dt, EV_DP_ARRIVAL, sid); }    // pushing that time, event, and id to fel

			felPush(now + EPS, EV_HAP_POLL, -1);    // push the polling event the fel at the unit time
													// so that it will polling or called updating each moment
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

			felPush(now + EPS, EV_HAP_POLL, -1);  //
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
				felPush(now + EPS, EV_HAP_POLL, -1);
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

            // EP with charging slope
			double epVis = (double)s->energy;
			if (charging[i]) {
				double capRemain = std::max(0, s->E_cap - s->energy);
				if (capRemain > 0) {
					double denom = (chargeNextDt[i] > EPS ? chargeNextDt[i] : 1.0); // 避免除 0
					double prog  = (now - chargeStartT[i]) / denom;  // 0..1 的線性進度，對應「這一顆 EP」的實際等待時間
					if (prog < 0) prog = 0;
					if (prog > 1.0) prog = 1.0;
					if (prog > capRemain) prog = capRemain;          // 接近滿時，格子變小
					epVis += prog;
				}
			}
			if (epVis > s->E_cap) epVis = s->E_cap;
			traceE[i].push_back(epVis);

            traceRtx[i].push_back( (double)s->r_tx );
		}
	}

	// ← for (int i=0; i<N; ++i) {...} 結束後新增
	if (hapTxBusy && hapTxSid >= 0 && hapTxSid < (int)busySidInt.size()) {
		busySidInt[hapTxSid] += dt;
	}


	busySumTx      += dt * (hapTxBusy ? 1 : 0);
    chargeCountInt += dt * (double)chargeActive;

	if (recordTrace) {
		traceT_all.push_back(now);
		traceQ_all.push_back(totalQ);

		double sumQtot = 0.0;
		for (int i=0;i<N;++i) sumQtot += sumQ[i];
		traceMeanQ_all.push_back((now > 0) ? (sumQtot / now) : 0.0);

	double EsumVis = 0.0;
	for (int i=0;i<N;++i) {
		Sensor* s = (*sensors)[i];
		double epVis = (double)s->energy;
		if (charging[i]) {
			double capRemain = std::max(0, s->E_cap - s->energy); // 距離滿還能加多少
			if (capRemain > 0) {
				// 針對「當前這一格」用實際抽到的等待時間 chargeNextDt[i]
				double denom = (chargeNextDt[i] > EPS ? chargeNextDt[i] : 1.0);
				double prog  = (now - chargeStartT[i]) / denom;   // 0..1 線性進度
				if (prog < 0)   prog = 0;
				if (prog > 1.0) prog = 1.0;
				if (prog > capRemain) prog = capRemain;           // 接近滿，最後一格 < 1
				epVis += prog;
			}
		}
		if (epVis > s->E_cap) epVis = s->E_cap;
		EsumVis += epVis;
	}
	traceE_all.push_back(EsumVis);
	traceEavg_all.push_back( (N>0)? (EsumVis / N) : 0.0 );
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
     * Per-sensor (single sensor) report
     *
     * Definitions (per sensor i = sid):
     *   T                : run horizon in seconds (now)
     *   A (=arrivals[i]) : total offered packet arrivals to sensor i
     *   D (=s->drops)    : packets dropped at queue-full (tail-drop) for sensor i
     *   S (=served[i])   : packets successfully transmitted (completed) from sensor i
     *   B                : backlog at end-of-run for sensor i
     *                      = queue length + 1 if being served else 0
	 *
     *   sumQ[i]          : time-integral of queue length of sensor i
     *   busySidInt[i]    : time-integral of the {0/1} indicator that sensor i
     *                      is *in service* (i.e., being transmitted by the HAP).
     *
     *   Lq_hat           : mean queue size (this sensor, waiting only)
     *                      = sumQ[i] / T
     *
	 *   lambda_off       : offered arrival rate for this sensor
     *                      = A / T
     *   lambda_car       : carried rate (throughput) for this sensor
     *                      = S / T
     *   loss_rate        : drop ratio = D / A  (defined as 0 if A==0)
     *
	 *   Wq_hat           : mean waiting time in queue (carried packets only)
     *                      Little’s law with carried rate: Wq_hat = Lq_hat / lambda_car
     *
     *   L_hat            : mean system size (this sensor)
     *                      = mean # in queue + mean # in service
     *                      = (sumQ[i] + busySidInt[i]) / T
     *                      NOTE: This is what most textbooks call “mean number in system”.
     *
     *   W_hat            : mean system time (waiting + service), carried packets
     *                      = L_hat / lambda_car
     *
     *   rho(sensor i)    : fraction of time sensor i is being served
     *                      = busySidInt[i] / T
     *
     *   Sanity check     : A ?= D + S + B
     *                      (In a stable long run, this mass balance should hold
     *                       up to initial preloads and end effects.)
     */

    AnsiString out;
    if (!sensors || sid < 0 || sid >= (int)sensors->size()) return "Invalid sensor id.\n";

    const Sensor* s = (*sensors)[sid];
    double T = now;

	// Basic counters
    int    A = arrivals[sid];
    int    D = s->drops;
    int    S = served[sid];
    int    B = (int)s->q.size() + (s->serving ? 1 : 0);

    // Time-averaged metrics
    double Lq_hat  = (T > 0) ? (sumQ[sid] / T) : 0.0;      // mean queue size (waiting only)
	double lam_off = (T > 0) ? ((double)A / T) : 0.0;      // offered rate
	double lam_car = (T > 0) ? ((double)S / T) : 0.0;      // carried (throughput)
    double loss    = (A > 0) ? ((double)D / (double)A) : 0.0;

    // Mean waiting time in queue (Little’s law with carried rate)
    double Wq_hat  = (lam_car > 0) ? (Lq_hat / lam_car) : 0.0;

    // Mean system size for this sensor (queue + in service)
    double L_hat   = (T > 0) ? ((sumQ[sid] + busySidInt[sid]) / T) : 0.0;
    // Mean system time (waiting + service) for carried packets of this sensor
    double W_hat   = (lam_car > 0) ? (L_hat / lam_car) : 0.0;

    // Fraction of time this sensor is being served
    double rho_i   = (T > 0) ? (busySidInt[sid] / T) : 0.0;

    out += "Sensor " + IntToStr(sid+1) + "\n";
    out += "T                = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)      = " + IntToStr(A) + "  (offered)\n";
    out += "drops(D)         = " + IntToStr(D) + "\n";
    out += "served(S)        = " + IntToStr(S) + "  (carried)\n";
    out += "backlog(B)       = " + IntToStr(B) + "  (end of run)\n";

    // Mean queue size (this sensor)
    out += "Lq_hat           = " + FloatToStrF(Lq_hat,  ffFixed, 7, 4) + "  (= mean queue size, waiting only)\n";

    // Rates
	out += "lambda_off       = " + FloatToStrF(lam_off, ffFixed, 7, 4) + " = offered arrival rate = A / T \n";
	out += "lambda_car       = " + FloatToStrF(lam_car, ffFixed, 7, 4) + "  (= throughput) = S/T\n";
    out += "loss_rate        = " + FloatToStrF(loss,    ffFixed, 7, 4) + "  (= D/A)\n";

    // Mean waiting time (queue) from Little’s law
	out += "Wq_hat           = " + FloatToStrF(Wq_hat,  ffFixed, 7, 4) + "  (Little: Lq_hat / lambda_car)\n";

    // Mean system size & time (this sensor)
    out += "L_hat            = " + FloatToStrF(L_hat,   ffFixed, 7, 4) + "  (= mean system size: queue + in-service)\n";
    out += "W_hat            = " + FloatToStrF(W_hat,   ffFixed, 7, 4) + "  (Little: L_hat / lambda_car)\n";

    // Service share for this sensor
    out += "rho(sensor)      = " + FloatToStrF(rho_i,   ffFixed, 7, 4) + "  (fraction of time being served)\n";

    // Flow conservation check
	out += "A ?= D + S + B   →  " + IntToStr(A) + " ?= " + IntToStr(D + S + B) + "\n";
    out += "(Shared-HAP w/ FDM charging; no simple closed form)\n";
    return out;
}

AnsiString Master::reportAll() const {
    /*
     * System-wide report (aggregated across all sensors)
	 *
     * N                : number of sensors
     * T                : run horizon in seconds (now)
     *
     * Totals:
     *   A = Σ_i arrivals[i]     (total offered arrivals)
     *   D = Σ_i drops_i         (total drops across sensors)
     *   S = Σ_i served[i]       (total served / throughput count)
     *   B = Σ_i backlog_i(end)  (sum of end-of-run backlogs)
     *
     *   sumQtot = Σ_i sumQ[i]   (integral of total queue length)
     *
     * Mean queue sizes:
     *   Lq_all            = sumQtot / T
     *                       → mean total # of waiting packets in the whole system
     *   Lq_avg_per_sensor = (N>0) ? Lq_all / N : 0
     *                       → mean queue size per sensor (averaged over sensors)
     *
     * Rates:
     *   lambda_off (overall) = A / T
     *   lambda_car (overall) = S / T
	 *
     * Waiting-time (system):
     *   Wq_all = (lambda_car>0) ? Lq_all / lambda_car : 0
     *            → mean waiting time in queue, for carried packets, system-wide
     *
     * HAP/charging utilizations:
     *   TX busy      = busySumTx / T
     *                  → fraction of time the HAP was transmitting (system-level)
	 *   avg charging = chargeCountInt / T
     *                  → time-average number of sensors being charged in parallel
     *
     * Mean system size/time (system):
     *   L_all = (sumQtot + busySumTx) / T
     *           → mean # in system = waiting + in-service
     *             (There is at most one in-service packet system-wide; its integral is busySumTx.)
     *   W_all = (lambda_car>0) ? L_all / lambda_car : 0
     *           → mean system time (waiting + service), carried packets, system-wide
     *
     * Sanity:
     *   A ?= D + S + B
     */

    AnsiString out;
    int N = (sensors ? (int)sensors->size() : 0);
    if (N == 0) return "No sensors.\n";

    double T = now;

    // System totals
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

    // Queue means (system)
    double Lq_all            = (T > 0) ? (sumQtot / T) : 0.0;                 // mean total waiting in system
    double Lq_avg_per_sensor = (N > 0) ? (Lq_all / N) : 0.0;                  // mean waiting per sensor

    // Rates (system)
    double lam_off  = (T > 0) ? ((double)A / T) : 0.0;
    double lam_car  = (T > 0) ? ((double)S / T) : 0.0;

    // Mean waiting time in queue (system)
	double Wq_all   = (lam_car > 0) ? (Lq_all / lam_car) : 0.0;

    // HAP busy & charging
    double txBusy   = (T > 0) ? (busySumTx / T) : 0.0;
    double avgCharg = (T > 0) ? (chargeCountInt / T) : 0.0;

    // Mean system size/time (system)
    double L_all    = (T > 0) ? ((sumQtot + busySumTx) / T) : 0.0;            // mean system size
    double W_all    = (lam_car > 0) ? (L_all / lam_car) : 0.0;                // mean system time

    out += "=== Overall (All sensors) ===\n";
    out += "N                = " + IntToStr(N) + "\n";
    out += "T                = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "arrivals(A)      = " + IntToStr((int)A) + "\n";
    out += "drops(D)         = " + IntToStr((int)D) + "\n";
    out += "served(S)        = " + IntToStr((int)S) + "\n";
    out += "backlog(B)       = " + IntToStr((int)B) + "\n";

    // Mean queue sizes (system)
    out += "Lq_all           = " + FloatToStrF(Lq_all,            ffFixed, 7, 4) + "  (= mean total waiting in system)\n";
	out += "Lq_avg_per_sensor= " + FloatToStrF(Lq_avg_per_sensor, ffFixed, 7, 4) + "  (= mean queue size per sensor)\n";

    // Rates
	out += "lambda_off       = " + FloatToStrF(lam_off,  ffFixed, 7, 4) + " = offered arrival rate = A/T \n";
	out += "lambda_car       = " + FloatToStrF(lam_car,  ffFixed, 7, 4) + "  (= throughput) = S/T \n";

	// Mean waiting time (system)
    out += "Wq_all           = " + FloatToStrF(Wq_all,   ffFixed, 7, 4) + "  (Little: Lq_all / lambda_car)\n";

	// HAP & charging utilizations
    out += "TX busy          = " + FloatToStrF(txBusy,   ffFixed, 7, 4) + "  (fraction of time HAP transmits)\n";
    out += "avg charging     = " + FloatToStrF(avgCharg, ffFixed, 7, 4) + "  (avg # of sensors charging)\n";

    // Mean system size/time (system)
    out += "L_all            = " + FloatToStrF(L_all,    ffFixed, 7, 4) + "  (= mean system size: waiting + in-service)\n";
    out += "W_all            = " + FloatToStrF(W_all,    ffFixed, 7, 4) + "  (Little: L_all / lambda_car)\n";

	// Flow conservation check
    out += "A ?= D + S + B   →  " + IntToStr((int)A) + " ?= " + IntToStr((int)(D+S+B)) + "\n";
    out += "(Shared-HAP w/ FDM charging; no simple closed form)\n";
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
           + ", slots=" + IntToStr(maxChargingSlots)
           + ", N=" + IntToStr(sensors ? (int)sensors->size() : 0) );
    sl->Add("");

    sl->Add("# === Timeline ===");
    for (size_t i=0;i<timeline.size();++i) sl->Add(timeline[i]);
	sl->Add("");

    AnsiString out = sl->Text;
    delete sl;
    return out;
}

AnsiString Master::stateLine() const {
	int N = sensors ? (int)sensors->size() : 0;
    AnsiString s = "  [";
    for (int i = 0; i < N; ++i) {
        const Sensor* x = (*sensors)[i];

        double epVis = x->energy;
		if ((size_t)N == charging.size() && charging[i]) {
			double capRemain = std::max(0, x->E_cap - x->energy);
			if (capRemain > 0) {
				double denom = (chargeNextDt[i] > EPS ? chargeNextDt[i] : 1.0);
				double prog  = (now - chargeStartT[i]) / denom;   // 0..1
				if (prog < 0)   prog = 0;
				if (prog > 1.0) prog = 1.0;
				if (prog > capRemain) prog = capRemain;
				epVis += prog;
			}
		}


		s += "S" + IntToStr(i) + "=" + x->queueToStr();
        if (x->serving) s += "(S:" + IntToStr(x->servingId) + ")";
        s += " EP=" + IntToStr(x->energy);
		if (epVis != x->energy) s += " (" + f2(epVis,3) + ")";
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

