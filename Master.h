#pragma once
#include <vector>
#include <System.hpp>   // for AnsiString
// �e�m�ŧi�A�קK�`�� include�FMaster.cpp �A #include "Sensor.h"
class Sensor;

namespace sim {

// �ƥ󫬧O�G�u���u��F/���}�v��اY�i�]�C�� sid �W�ߪA�ȡ^
enum EventType { EV_ARRIVAL = 0, EV_DEPARTURE = 1 };

// FEL �`�I�]�ɶ����W����V��C�^
struct EvNode {
    double    t;     // �ƥ�ɶ�
    EventType tp;    // ����
    int       sid;   // ���� sensor
    EvNode*   next;  // �U�@��
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

// =======================
// Master�G�ƥ�޲z + �έp
//�]�C�� Sensor �U�ۤ@�x���A���A�����۱ƥL�^
// =======================
class Master {
public:  // �A���w�������}�A��K�ʬ�
    // ---- �~�����Ѫ� sensors�]Master ���֦��ͩR�g���^ ----
    std::vector<Sensor*>* sensors;

    // ---- FEL�]dummy head�^----
    EvNode*  felHead;

    // ---- �����]�w/���A ----
    double   endTime;      // �����פ�ɶ�
    double   now;          // �ثe�ɶ�
    double   prev;         // �W�@���n���ɶ�
    static double EPS;     // �̤p���ɶ�

    // ---- �C�� sensor ���έp�]length = sensors->size()�^----
    // ���n�k�GsumQ[i] = �� queue_i(t) dt
    //         sumL[i] = �� systemSize_i(t) dt�]= queue_i + (serving?1:0)�^
    // busySum[i] = �� 1_{serving_i}(t) dt
    std::vector<double> sumQ;
    std::vector<double> sumL;
    std::vector<double> busySum;

    // �p��
    std::vector<int>    served;     // ������
    std::vector<int>    arrivals;   // ��F��

public:
    Master();
    ~Master();

    // ���w sensors �e����AMaster �|�ھڤj�p�t�m�έp�}�C
    void setSensors(std::vector<Sensor*>* v);

    void setEndTime(double T) { endTime = T; }

    // ���s�k�s���A�P FEL�]���R sensors�^
    void reset();

    // �D�j��G�ƨC�� sid ���Ĥ@�� ARRIVAL�A�M��@�� pop �̦��ƥ�
    void run();

    // ===== FEL �ާ@�]���}�A��K�A debug�^=====
    void felClear();                                  // �M�š]�O�d dummy�^
    void felPush(double t, EventType tp, int sid);    // �̮ɶ����J
    bool felPop(double& t, EventType& tp, int& sid);  // ���X�̦��ƥ�

    // �n���έp�G�� [prev, now] �o�q��C�� sid �� Q/L/Busy ���[�_��
    void accumulate();

    // �� UI ����@ Sensor ����]�^�� AnsiString�^
	AnsiString reportOne(int sid) const;
	AnsiString reportAll() const;



// ... �A�� Master ���O�䥦�������O�d

public:
    bool recordTrace;  // �O�_�����y��]�w�] true�^

    // �C�� sensor ���ɶ��ǦC�Gt�B�Y��Q�B����Q
    std::vector< std::vector<double> > traceT;
    std::vector< std::vector<double> > traceQ;
    std::vector< std::vector<double> > traceMeanQ;

    // ����[�`���ɶ��ǦC
    std::vector<double> traceT_all;
    std::vector<double> traceQ_all;
	std::vector<double> traceMeanQ_all;
};

} // namespace sim

