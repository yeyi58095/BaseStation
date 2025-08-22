#include "Master.h"
#include "Sensor.h"   // �Ψ� sampleIT/sampleST�BenqueueArrival/canServe/startService/finishService

using namespace sim;

// �p�`��
double Master::EPS = 1e-9;

Master::Master()
: sensors(0),
  felHead(0),
  endTime(10000.0),
  now(0.0),
  switchover(0.0),
  serverBusy(false),  // ����Ҧ����|�ϥ�
  prev(0.0),
  sumQ(0.0),
  sumSystem(0.0),
  busySum(0.0),
  arrivals(0),
  served(0)
{
    // �إ� FEL �� dummy head
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
    serverBusy = false; // ����Ҧ����ϥΡA���k�s
    felClear();
}

void Master::run_parallel() {
    if (!sensors || sensors->empty()) return;

    reset();

    // 1) ���C�� sensor �ƲĤ@�Ө�F
    for (int i = 0; i < (int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT();
        if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_ARRIVAL, i);
    }

    // 2) �D�j��G�@�� pop �̦��ƥ�A���� FEL �ũζW�L endTime
    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) {  // �ɶ���
            now = endTime;
            accumulateQ();  // �ɳ̫�@�q
            break;
        }

        now = t;
        accumulateQ();      // �� [prev, now] �o�q��s�έp

        switch (tp) {
            case EV_ARRIVAL: {
                Sensor* s = (*sensors)[sid];

                // �� sensor �Ӥ@�]
                s->enqueueArrival();
                arrivals += 1;

                // �ߨ�ƤU�@�Ӹ� sensor ����F
                double dt = s->sampleIT();
                if (dt <= EPS) dt = EPS;
                felPush(now + dt, EV_ARRIVAL, sid);

                // ����Ҧ��G�� sensor �ۤv���f�B���b�A�� �� �ߨ�}�u
                if (s->canServe()) {
                    startService(sid);
                }
                break;
            }

            case EV_DEPARTURE: {
                Sensor* s = (*sensors)[sid];
                s->finishService();
                served += 1;

                // ����Ҧ��G�Y��C�٦��A�N���P�@���u���۶}�u
                if (s->canServe()) {
                    startService(sid);
                }
                break;
            }
        }

        if (now >= endTime) break;
    }

    // �ɧ��G�Y FEL ���Ŧ��ɶ�����
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

// ===== �Ƶ{�G�� sid �}�l�A�ȨñƦۤv�����}�ƥ� =====
void Master::startService(int sid) {
    Sensor* s = (*sensors)[sid];
    double st = s->startService();     // �| pop ��C�Y�ç� serving=true
    if (st <= EPS) st = EPS;
    // ����Ҧ��G�S�� serverBusy �o�ӭ���F�C���u���O�W�ߦ��A��
    felPush(now + switchover + st, EV_DEPARTURE, sid);
}

// ===== �έp�GLq/L �P���L���A���u�ƶq�v���n�� =====
void Master::accumulateQ() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int totalQ = 0;
    int busyCount = 0; // �P�ɦb�A�Ȫ��u���A���ơv�]= ���b�A�Ȫ� sensor �ơ^
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

