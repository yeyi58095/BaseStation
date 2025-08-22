#include "Master.h"
#include "Sensor.h"   // 用到 sampleIT/sampleST、enqueueArrival/canServe/startService/finishService

using namespace sim;

// 小常數
double Master::EPS = 1e-9;

Master::Master()
: sensors(0),
  felHead(0),
  endTime(10000.0),
  now(0.0),
  switchover(0.0),
  serverBusy(false),  // 平行模式不會使用
  prev(0.0),
  sumQ(0.0),
  sumSystem(0.0),
  busySum(0.0),
  arrivals(0),
  served(0)
{
    // 建立 FEL 的 dummy head
    felHead = new EvNode(0.0, EV_ARRIVAL, -1);
    felHead->next = 0;
}

Master::~Master() {
    felClear();
    delete felHead;
    felHead = 0;
}

void Master::setSensors(std::vector<Sensor*>* v) { sensors = v; }
void Master::setEndTime(double T)                { endTime = T; }
void Master::setSwitchOver(double s)             { switchover = s; }

void Master::reset() {
    now = 0.0;
    prev = 0.0;
    sumQ = 0.0;
    sumSystem = 0.0;
    busySum = 0.0;
    arrivals = 0;
    served = 0;
    serverBusy = false; // 平行模式不使用，先歸零
    felClear();
}

void Master::run_parallel() {
    if (!sensors || sensors->empty()) return;

    reset();

    // 1) 為每個 sensor 排第一個到達
    for (int i = 0; i < (int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT();
        if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_ARRIVAL, i);
    }

    // 2) 主迴圈：一直 pop 最早事件，直到 FEL 空或超過 endTime
    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) {  // 時間到
            now = endTime;
            accumulateQ();  // 補最後一段
            break;
        }

        now = t;
        accumulateQ();      // 用 [prev, now] 這段更新統計

        switch (tp) {
            case EV_ARRIVAL: {
                Sensor* s = (*sensors)[sid];

                // 該 sensor 來一包
                s->enqueueArrival();
                arrivals += 1;

                // 立刻排下一個該 sensor 的到達
                double dt = s->sampleIT();
                if (dt <= EPS) dt = EPS;
                felPush(now + dt, EV_ARRIVAL, sid);

                // 平行模式：該 sensor 自己有貨且不在服務 → 立刻開工
                if (s->canServe()) {
                    startService(sid);
                }
                break;
            }

            case EV_DEPARTURE: {
                Sensor* s = (*sensors)[sid];
                s->finishService();
                served += 1;

                // 平行模式：若佇列還有，就讓同一條線接著開工
                if (s->canServe()) {
                    startService(sid);
                }
                break;
            }
        }

        if (now >= endTime) break;
    }

    // 補尾：若 FEL 先空但時間未到
    if (now < endTime) {
        now = endTime;
        accumulateQ();
    }
}

// ===== FEL =====
void Master::felClear() {
    EvNode* c = felHead->next;
    while (c) {
        EvNode* d = c;
        c = c->next;
        delete d;
    }
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

// ===== 排程：讓 sid 開始服務並排自己的離開事件 =====
void Master::startService(int sid) {
    Sensor* s = (*sensors)[sid];
    double st = s->startService();     // 會 pop 佇列頭並把 serving=true
    if (st <= EPS) st = EPS;
    // 平行模式：沒有 serverBusy 這個限制；每條線都是獨立伺服器
    felPush(now + switchover + st, EV_DEPARTURE, sid);
}

// ===== 統計：Lq/L 與忙碌伺服器「數量」的積分 =====
void Master::accumulateQ() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int totalQ = 0;
    int busyCount = 0; // 同時在服務的「伺服器數」（= 正在服務的 sensor 數）
    if (sensors) {
        for (int i = 0; i < (int)sensors->size(); ++i) {
            Sensor* s = (*sensors)[i];
            totalQ   += (int)s->q.size();
            if (s->serving) busyCount += 1;
        }
    }

    int systemSize = totalQ + busyCount;

    sumQ      += dt * totalQ;
    sumSystem += dt * systemSize;
    busySum   += dt * busyCount;

    prev = now;
}

