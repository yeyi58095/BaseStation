#pragma once
#include <vector>

// �e�m�ŧi�]�קK�`�� include�^
class Sensor;

namespace sim {

// �ƥ󫬧O�G�{�b�u�Ψ�u��F / ���}�v���
enum EventType { EV_ARRIVAL = 0, EV_DEPARTURE = 1 };

// FEL �`�I�]�Φ��� linked list�A�ɶ��p���b�e�^
struct EvNode {
    double   t;    // �ƥ�ɶ�
    EventType tp;  // �ƥ󫬧O
    int      sid;  // ���� sensor ���ƥ�]0..N-1�^
    EvNode*  next; // �U�@�Ӹ`�I
    EvNode(double tt, EventType ty, int id) : t(tt), tp(ty), sid(id), next(0) {}
};

// Master�G�ƥ�Ƶ{���]������A�����^
// - �C�� sensor �����ۤv����F/�A�ȡA�����z�Z
// - FEL�]future event list�^�������b Master ��
class Master {
public:
    // ===== ���� public�A��K�A�b IDE �ʬ� =====
    std::vector<Sensor*>* sensors; // �~���֦��FMaster ���t�d delete
    EvNode*  felHead;              // FEL dummy head�]head->next �O�Ĥ@�Өƥ�^
    double   endTime;              // �����פ�ɶ��]��^
    double   now;                  // �ثe�����ɶ�
    double   switchover;           // �����}�P�]��^�F������A���q�`�] 0
    // �]Shared �����|�Ψ쪺�X�СA�o�̫O�d�����ϥΡ^
    bool     serverBusy;

    // ===== �έp =====
    double   prev;       // �W�@�����έp�n�����ɶ�
    double   sumQ;       // �� ������C�����`�M dt  �� Lq_total = sumQ / T
    double   sumSystem;  // �� (������C + ���L���A����) dt �� L_total = sumSystem / T
    double   busySum;    // �� ���L���A�����u�ƶq�v dt       �� �����C�x���L�v = busySum / (T * N)
    int      arrivals;   // ��F�ʥ]�`�ơ]���� sensor �X�p�^
    int      served;     // �����ʥ]�`�ơ]���� sensor �X�p�^

    // ===== ���� =====
    Master();
    ~Master();

    void setSensors(std::vector<Sensor*>* v);
    void setEndTime(double T);
    void setSwitchOver(double s);

    void reset();           // �M FEL�B�ɶ��B�έp
    void run_parallel();    // �u������A���v�Ҧ��G�U sensor �W�ߪA��

    // ===== FEL �ާ@�]���}�A��K debug/�X�R�^=====
    void felClear();
    void felPush(double t, EventType tp, int sid);
    bool felPop(double& t, EventType& tp, int& sid);

    // ===== �Ƶ{�ʧ@ =====
    void startService(int sid); // �� sid �}�l�A�ȡA�ñƥ��ۤv�� EV_DEPARTURE

    // ===== �έp�n�� =====
    void accumulateQ();     // �� [prev, now] �o�q��s Lq/L �P���L���A���ƪ��n��

    static double EPS;      // �קK���D���ɶ��Ϊ��̤p�ȡ]�Ҧp 1e-9�^
};

} // namespace sim

