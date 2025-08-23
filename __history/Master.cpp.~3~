#include "Master.h"
#include "Sensor.h"
#include <System.SysUtils.hpp>  // IntToStr / FloatToStrF

using namespace sim;

double Master::EPS = 1e-9;

Master::Master()
: sensors(0), felHead(0), endTime(10000.0), now(0.0), prev(0.0)
{
    felHead = new EvNode(0.0, EV_ARRIVAL, -1); // dummy
    felHead->next = 0;
}

Master::~Master() {
    felClear();
    delete felHead;
    felHead = 0;
}

void Master::setSensors(std::vector<Sensor*>* v) {
    sensors = v;

    // 依 sensors 數量配置/清零統計
    int N = (sensors ? (int)sensors->size() : 0);
    sumQ.assign(N, 0.0);
    sumL.assign(N, 0.0);
    busySum.assign(N, 0.0);
    served.assign(N, 0);
    arrivals.assign(N, 0);
}

void Master::reset() {
    now = 0.0;
    prev = 0.0;
    felClear();

    int N = (sensors ? (int)sensors->size() : 0);
    for (int i = 0; i < N; ++i) {
        sumQ[i] = 0.0;
        sumL[i] = 0.0;
        busySum[i] = 0.0;
        served[i] = 0;
        arrivals[i] = 0;
    }
}

// 主迴圈（每個 sensor 自己一條 M/M/1）
// - EV_ARRIVAL(sid): 該 sid 佇列+1；若可服務就立刻開始，並排 DEPARTURE(sid)
//                    同時排下一個 ARRIVAL(sid)
// - EV_DEPARTURE(sid): 該 sid 完成一次；若佇列>0 就直接接下一個
void Master::run() {
    if (!sensors || sensors->empty()) return;

    reset();

    // 1) 初始：每個 sid 先排第一個 ARRIVAL
    for (int i = 0; i < (int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT();
        if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_ARRIVAL, i);
    }

    // 2) 事件迴圈
    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) { now = endTime; accumulate(); break; }
        now = t;
        accumulate(); // 在進行狀態變更之前，先把上一段時間的 L/Q/Busy 積分起來

        Sensor* s = (*sensors)[sid];

        switch (tp) {
            case EV_ARRIVAL: {
                // 入佇列
                s->enqueueArrival();
                arrivals[sid] += 1;

                // 同時排下一個到達
                {
                    double dt2 = s->sampleIT();
                    if (dt2 <= EPS) dt2 = EPS;
                    felPush(now + dt2, EV_ARRIVAL, sid);
                }

                // 若沒在服務，立刻開始
                if (s->canServe()) {
                    double st = s->startService();
                    if (st <= EPS) st = EPS;
                    felPush(now + st, EV_DEPARTURE, sid);
                }
                break;
            }

            case EV_DEPARTURE: {
                // 完成一次服務
                s->finishService();
                served[sid] += 1;

                // 若佇列還有貨，立刻接下一個（同一條線不需要等別人）
                if (s->canServe()) {
                    double st = s->startService();
                    if (st <= EPS) st = EPS;
                    felPush(now + st, EV_DEPARTURE, sid);
                }
                break;
            }
        }

        if (now >= endTime) break;
    }

    // 收尾：若時間還沒走到 endTime，把最後一段積分補上
    if (now < endTime) { now = endTime; accumulate(); }
}

// ========== FEL ==========

void Master::felClear() {
    EvNode* c = felHead->next;
    while (c) { EvNode* d = c; c = c->next; delete d; }
    felHead->next = 0;
}

void Master::felPush(double t, EventType tp, int sid) {
    EvNode* n = new EvNode(t, tp, sid);
    EvNode* cur = felHead;
    while (cur->next && cur->next->t <= t) cur = cur->next;
    n->next = cur->next;
    cur->next = n;
}

bool Master::felPop(double& t, EventType& tp, int& sid) {
    EvNode* n = felHead->next;
    if (!n) return false;
    felHead->next = n->next;
    t = n->t; tp = n->tp; sid = n->sid;
    delete n;
    return true;
}

// ========== 統計（面積法）==========
// 把 [prev, now] 這段時間，對每個 sid：
//   sumQ[i]    += dt * queue_len_i
//   sumL[i]    += dt * (queue_len_i + (serving?1:0))
//   busySum[i] += dt * (serving?1:0)
void Master::accumulate() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int N = (sensors ? (int)sensors->size() : 0);
    for (int i = 0; i < N; ++i) {
        Sensor* s = (*sensors)[i];
        int qlen = (int)s->q.size();      // 你現在把 q/serving 做成 public，這樣讀最快
        int sys  = qlen + (s->serving ? 1 : 0);
        sumQ[i]    += dt * qlen;
        sumL[i]    += dt * sys;
        busySum[i] += dt * (s->serving ? 1 : 0);
    }

    prev = now;
}

// ========== 產生單一 sid 的報表 ==========
AnsiString Master::reportOne(int sid) const {
    AnsiString out;

    if (!sensors || sid < 0 || sid >= (int)sensors->size()) {
        return "Invalid sensor id.\n";
    }

    double T = now;
    double Lq_hat   = (T > 0) ? (sumQ[sid]   / T) : 0.0;
    double L_hat    = (T > 0) ? (sumL[sid]   / T) : 0.0;
    double busy_hat = (T > 0) ? (busySum[sid]/ T) : 0.0;
    double thr_hat  = (T > 0) ? ((double)served[sid]   / T) : 0.0;
    double lam_hat  = (T > 0) ? ((double)arrivals[sid] / T) : 0.0;
    double mu_hat   = (busySum[sid] > 0) ? ((double)served[sid] / busySum[sid]) : 0.0;

    out += "Sensor " + IntToStr(sid+1) + "\n";
    out += "T            = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "served       = " + IntToStr(served[sid]) + "\n";
    out += "arrivals     = " + IntToStr(arrivals[sid]) + "\n";
    out += "Lq_hat       = " + FloatToStrF(Lq_hat,   ffFixed, 7, 4) + "\n";
    out += "L_hat        = " + FloatToStrF(L_hat,    ffFixed, 7, 4) + "\n";
    out += "busy_hat     = " + FloatToStrF(busy_hat, ffFixed, 7, 4) + "\n";
    out += "throughput   = " + FloatToStrF(thr_hat,  ffFixed, 7, 4) + " (? ≈ λ if stable)\n";
    out += "lambda_hat   = " + FloatToStrF(lam_hat,  ffFixed, 7, 4) + "\n";
    out += "mu_hat       = " + FloatToStrF(mu_hat,   ffFixed, 7, 4) + "\n";

    // 如果該 sensor 是 Exp(λ)/Exp(μ)，給理論值
    const Sensor* s = (*sensors)[sid];
    bool theory = (s->ITdistri == 1 && s->STdistri == 1); // 1 = EXPONENTIAL（照你的定義）
    if (theory) {
        double lambda = s->ITpara1;
        double mu     = s->STpara1;
        if (mu > 0 && lambda < mu) {
            double rho = lambda / mu;
            double Lq  = (rho*rho) / (1.0 - rho);
            double L   = rho / (1.0 - rho);
            out += "— Theory (M/M/1) —\n";
            out += "lambda      = " + FloatToStrF(lambda, ffFixed, 7, 4) + "\n";
            out += "mu          = " + FloatToStrF(mu,     ffFixed, 7, 4) + "\n";
            out += "rho         = " + FloatToStrF(rho,    ffFixed, 7, 4) + "\n";
            out += "Lq(theory)  = " + FloatToStrF(Lq,     ffFixed, 7, 4) + "\n";
            out += "L(theory)   = " + FloatToStrF(L,      ffFixed, 7, 4) + "\n";
        } else {
            out += "(No closed-form: unstable or mu<=0)\n";
        }
    } else {
        out += "(No closed-form theory for non-exponential)\n";
    }
    return out;
}

AnsiString Master::reportAll() const
{
    AnsiString out;

    int N = (sensors ? (int)sensors->size() : 0);
    if (N == 0) return "No sensors.\n";

    double T = now;
    long long served_tot = 0, arrivals_tot = 0;
    double sumQ_tot = 0.0, sumL_tot = 0.0, busy_tot = 0.0;

    for (int i = 0; i < N; ++i) {
        served_tot   += served[i];
        arrivals_tot += arrivals[i];
        sumQ_tot     += sumQ[i];
        sumL_tot     += sumL[i];
        busy_tot     += busySum[i];
    }

    double Lq_hat   = (T > 0) ? (sumQ_tot / T) : 0.0;           // 全系統平均排隊人數 (sum over sensors)
    double L_hat    = (T > 0) ? (sumL_tot / T) : 0.0;           // 全系統平均系統人數
    double thr_hat  = (T > 0) ? ((double)served_tot   / T) : 0.0; // 總吞吐
    double lam_hat  = (T > 0) ? ((double)arrivals_tot / T) : 0.0; // 總到達率
    double mu_hat   = (busy_tot > 0) ? ((double)served_tot / busy_tot) : 0.0; // 平均服務率(以忙碌時間加權)
    double busy_avg = (N > 0 && T > 0) ? (busy_tot / (N * T)) : 0.0; // 每台server平均忙碌比例

    out += "=== Overall (sum/average across sensors) ===\n";
    out += "T            = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
    out += "served_tot   = " + IntToStr((int)served_tot) + "\n";
    out += "arrivals_tot = " + IntToStr((int)arrivals_tot) + "\n";
    out += "Lq_hat       = " + FloatToStrF(Lq_hat,   ffFixed, 7, 4) + "\n";
    out += "L_hat        = " + FloatToStrF(L_hat,    ffFixed, 7, 4) + "\n";
    out += "throughput   = " + FloatToStrF(thr_hat,  ffFixed, 7, 4) + "\n";
    out += "lambda_hat   = " + FloatToStrF(lam_hat,  ffFixed, 7, 4) + "\n";
    out += "mu_hat       = " + FloatToStrF(mu_hat,   ffFixed, 7, 4) + "  (served_tot / busy_tot)\n";
    out += "busy_avg     = " + FloatToStrF(busy_avg, ffFixed, 7, 4) + "  (average server busy fraction)\n";

    // 可選：若所有 sensor 都是 Exp(λ_i)/Exp(μ_i) 且穩定，給理論總和 Lq/L
    bool theory_ok = true;
    double Lq_th_sum = 0.0, L_th_sum = 0.0;
    for (int i = 0; i < N; ++i) {
        const Sensor* s = (*sensors)[i];
        if (!(s->ITdistri == 1 && s->STdistri == 1)) { theory_ok = false; break; } // 1=EXPONENTIAL
        double lambda = s->ITpara1, mu = s->STpara1;
        if (!(mu > 0 && lambda < mu)) { theory_ok = false; break; }
        double rho = lambda / mu;
        Lq_th_sum += (rho*rho) / (1.0 - rho);
        L_th_sum  +=  rho      / (1.0 - rho);
    }
    if (theory_ok) {
        out += "--- Theory sum (independent M/M/1 per sensor) ---\n";
        out += "Lq(theory,sum) = " + FloatToStrF(Lq_th_sum, ffFixed, 7, 4) + "\n";
        out += "L(theory,sum)  = " + FloatToStrF(L_th_sum,  ffFixed, 7, 4) + "\n";
    } else {
        out += "(No closed-form theory: non-exponential or unstable at least one sensor)\n";
    }

    return out;
}

