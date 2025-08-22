#include "Master.h"
#include "Sensor.h"  // �o�̤~�ݭn����w�q�]�Ψ� sampleIT ���^

using namespace sim;

// �p�`�ƪ�l��
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
    // �إ� FEL �� dummy head�]��ڲĤ@�Өƥ�b head->next�^
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

// �N�ɶ�/�έp/FEL �M�^�_�l���A
void Master::reset() {
    now = 0.0;
    serverBusy = false;
    prev = 0.0;
    sumQ = 0.0;
    served = 0;
    felClear();
}

// �D�j��G1) �C�� sensor ���ƲĤ@�Ө�F�F2) ���_ pop �̦��ƥ�óB�z
void Master::run() {
    if (!sensors || sensors->empty()) return;

    reset();

    // 1) ��l�ơG���C�� sensor �ơu�Ĥ@�Ө�F�v
    for (int i = 0; i < (int)sensors->size(); ++i) {
        double dt = (*sensors)[i]->sampleIT();
        if (dt <= EPS) dt = EPS;
        felPush(now + dt, EV_ARRIVAL, i);
    }

    // 2) �D�ƥ�j��
    double t; EventType tp; int sid;
    while (felPop(t, tp, sid)) {
        if (t > endTime) {                 // �W�L�פ�ɶ��N����
            now = endTime;
            accumulateQ();                 // �ɳ̫�@�q
            break;
        }

        now = t;
        accumulateQ();                     // �� [prev, now] �o�q��s���n

        switch (tp) {
            case EV_ARRIVAL: {
                // �� sensor �Ӥ@�ӫʥ]�G��i��C
                Sensor* s = (*sensors)[sid];
                s->enqueueArrival();

                // �ߨ�ƤU�@�Ӹ� sensor ����F
                double dt = s->sampleIT();
                if (dt <= EPS) dt = EPS;
                felPush(now + dt, EV_ARRIVAL, sid);

                // �Y���A���šA���ն}�u
                if (!serverBusy) {
                    int pick = chooseNext();
                    if (pick >= 0) startService(pick);
                }
                break;
            }

            case EV_DEPARTURE: {
                // ���@�ӪA�ȧ���
                Sensor* s = (*sensors)[sid];
                s->finishService();
                serverBusy = false;
                served += 1;

                // �ݤU�@�ӭn�A�Ƚ�
                int pick = chooseNext();
                if (pick >= 0) startService(pick);
                break;
            }
        }

        if (now >= endTime) break;
    }

    // �����G�p�G FEL ���ŤF�A���ɶ��٨S��A�ɳ̫�@�q�έp
    if (now < endTime) {
        now = endTime;
        accumulateQ();
    }
}

//===== FEL �ާ@ =====
void Master::felClear() {
    EvNode* c = felHead->next;
    while (c) {
        EvNode* d = c;
        c = c->next;
        delete d;
    }
    felHead->next = 0;
}

// �̮ɶ����W���J�]í�w�ƧǡG�ۦP�ɶ������ӥ��A�ȡ^
void Master::felPush(double t, EventType tp, int sid) {
    EvNode* n = new EvNode(t, tp, sid);
    EvNode* cur = felHead;
    while (cur->next && cur->next->t <= t) cur = cur->next;
    n->next = cur->next;
    cur->next = n;
}

// ���X�̦����@���ƥ�F�Y�L�ƥ�^ false
bool Master::felPop(double& t, EventType& tp, int& sid) {
    EvNode* n = felHead->next;
    if (!n) return false;
    felHead->next = n->next;
    t = n->t; tp = n->tp; sid = n->sid;
    delete n;
    return true;
}

//===== �����G�D�u��C�̦h�B�i�A�ȡv�� sensor =====
// �i���������A�� DF/CDF/CEF/CEDF �����C
int Master::chooseNext() {
    if (!sensors) return -1;
    int pick = -1;
    int bestQLen = -1;

    for (int i = 0; i < (int)sensors->size(); ++i) {
        Sensor* s = (*sensors)[i];
        if (!s->canServe()) continue;
        int qlen = (int)s->q.size(); // �A�{�b q �O public�F���Q���S�N�� accessor
        if (qlen > bestQLen) { bestQLen = qlen; pick = i; }
    }
    return pick;
}

// �� sid �}�l�A�ȡG�� Sensor �^�ǪA�Ȯɶ��AMaster �� DEPARTURE
void Master::startService(int sid) {
    Sensor* s = (*sensors)[sid];
    double st = s->startService();             // �| pop ��C�Y�ç� serving=true
    serverBusy = true;
    // ²�ơG�� �n �����[�b�����ɶ��F�Y�n���ǡA�i�H���Ƥ@�� EV_SERVICE �ƥ�C
    felPush(now + switchover + ((st > EPS) ? st : EPS), EV_DEPARTURE, sid);
}

//===== �έp�]���n�k�^�G�� [prev, now] �o�q�� totalQueue �n���_�� =====
void Master::accumulateQ() {
    double dt = now - prev;
    if (dt <= 0) { prev = now; return; }

    int totalQ = 0;
    if (sensors) {
        for (int i = 0; i < (int)sensors->size(); ++i) {
            // �ثe�� total queue = �U sensor ��C�����`�M
            totalQ += (int)(*sensors)[i]->q.size();
        }
    }
    sumQ += dt * totalQ;
    prev = now;
}

