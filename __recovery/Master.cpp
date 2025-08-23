#include "Master.h"
#include "Sensor.h"  // 這裡才需要完整定義（用到 sampleIT 等）

using namespace sim;

// 小常數初始化
double Master::EPS = 1e-9;

Master::Master()
: sensors(0),
  felHead(0),
  endTime(10000.0),
  switchover(0.0),
  now(0.0),
  serverBusy(false),
  prev(0.0),
  sumQ(0.0),
  served(0)
{
    // 建立 FEL 的 dummy head（實際第一個事件在 head->next）
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

// 將時間/統計/FEL 清回起始狀態
void Master::reset() {
    now = 0.0;
    serverBusy = false;
    prev = 0.0;
    sumQ = 0.0;
    served = 0;
	felClear();

	// for detailly record
	sumSystem = 0.0;
	busySum   = 0.0;
	arrivals  = 0;

}

// 主迴圈：1) 每個 sensor 先排第一個到達；2) 不斷 pop 最早事件並處理
void Master::run() {
    if (!sensors || sensors->empty()) return;

    reset();

    // 1) 初始化：為每個 sensor 排「第一個到達」
    for (int i = 0; i < (int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT();
        if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_ARRIVAL, i);
    }

    // 2) 主事件迴圈
    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) {                 // 超過終止時間就結束
            now = endTime;
            accumulateQ();                 // 補最後一段
            break;
        }

        now = t;
        accumulateQ();                     // 用 [prev, now] 這段更新面積

        switch (tp) {
            case EV_ARRIVAL: {
                // 該 sensor 來一個封包：丟進佇列
                Sensor* s = (*sensors)[sid];
                s->enqueueArrival();

                // 立刻排下一個該 sensor 的到達
                double dt = s->sampleIT();
                if (dt <= EPS) dt = EPS;
                felPush(now + dt, EV_ARRIVAL, sid);

                // 若伺服器空，嘗試開工
                if (!serverBusy) {
                    int pick = chooseNext();
                    if (pick >= 0) startService(pick);
				}
				arrivals += 1;
                break;
            }

            case EV_DEPARTURE: {
                // 有一個服務完成
                Sensor* s = (*sensors)[sid];
                s->finishService();
                serverBusy = false;
                served += 1;

                // 看下一個要服務誰
                int pick = chooseNext();
                if (pick >= 0) startService(pick);
                break;
            }
        }

        if (now >= endTime) break;
    }

    // 收尾：如果 FEL 先空了，但時間還沒到，補最後一段統計
    if (now < endTime) {
        now = endTime;
        accumulateQ();
    }
}

//===== FEL 操作 =====
void Master::felClear() {
    EvNode* c = felHead->next;
    while (c) {
        EvNode* d = c;
        c = c->next;
        delete d;
    }
    felHead->next = 0;
}

// 依時間遞增插入（穩定排序：相同時間按先來先服務）
void Master::felPush(double t, EventType tp, int sid) {
    EvNode* n = new EvNode(t, tp, sid);
    EvNode* cur = felHead;
    while (cur->next && cur->next->t <= t) cur = cur->next;
    n->next = cur->next;
    cur->next = n;
}

// 取出最早的一筆事件；若無事件回 false
bool Master::felPop(double& t, EventType& tp, int& sid) {
    EvNode* n = felHead->next;
    if (!n) return false;
    felHead->next = n->next;
    t = n->t; tp = n->tp; sid = n->sid;
    delete n;
    return true;
}

//===== 策略：挑「佇列最多且可服務」的 sensor =====
// 可直接換成你的 DF/CDF/CEF/CEDF 評分。
int Master::chooseNext() {
    if (!sensors) return -1;
    int pick = -1;
    int bestQLen = -1;

    for (int i = 0; i < (int)sensors->size(); ++i) {
        Sensor* s = (*sensors)[i];
        if (!s->canServe()) continue;
        int qlen = (int)s->q.size(); // 你現在 q 是 public；不想暴露就做 accessor
        if (qlen > bestQLen) { bestQLen = qlen; pick = i; }
    }
    return pick;
}

// 讓 sid 開始服務：由 Sensor 回傳服務時間，Master 排 DEPARTURE
void Master::startService(int sid) {
    Sensor* s = (*sensors)[sid];
    double st = s->startService();             // 會 pop 佇列頭並把 serving=true
    serverBusy = true;
    // 簡化：把 τ 直接加在完成時間；若要更精準，可以先排一個 EV_SERVICE 事件。
    felPush(now + switchover + ((st > EPS) ? st : EPS), EV_DEPARTURE, sid);
}

//===== 統計（面積法）：把 [prev, now] 這段的 totalQueue 積分起來 =====
void Master::accumulateQ() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int totalQ = 0;
    for (int i = 0; sensors && i < (int)sensors->size(); ++i)
        totalQ += (int)(*sensors)[i]->q.size();

    int systemSize = totalQ + (serverBusy ? 1 : 0);

    sumQ      += dt * totalQ;      // Lq 積分
    sumSystem += dt * systemSize;  // L 積分
    busySum   += dt * (serverBusy ? 1 : 0);

    prev = now;
}


