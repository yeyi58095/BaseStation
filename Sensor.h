#pragma once
#include <System.hpp>  // AnsiString
#include "RandomVar.h"
#include <deque>
enum DistKind {
    DIST_NORMAL      = 0,
    DIST_EXPONENTIAL = 1,
    DIST_UNIFORM     = 2,
};

class Sensor {
public:
    explicit Sensor(int id);

    // IT = inter-arrival time�FST = service time
    void setIT(int method, double para1, double para2 = 0.0);
    void setST(int method, double para1, double para2 = 0.0);

    // ��K�b�� exponential �ɨϥ�
    void setArrivalExp(double lambda) { setIT(DIST_EXPONENTIAL, lambda); }
	void setServiceExp(double mu)     { setST(DIST_EXPONENTIAL, mu); }
	AnsiString toString() const;
	static AnsiString dict(int );
	double sampleIT() const;
	double sampleST() const;

public:
    int id;
    int max_queue_size;   // �Ϋغc�l���w�]��
    int ITdistri;
    int STdistri;
    double ITpara1, ITpara2;
	double STpara1, STpara2;

public:
    // ... �A�쥻���������O�d

    // ---- Master �|�Ψ쪺 4 �Ӱʧ@ ----
    void    enqueueArrival();      // ��F�G��s�ʥ]��i��C
    bool    canServe() const;      // �O�_�i�H�}�l�A�ȡ]�w�]�G���f�B�S�b�A�^
    double  startService();        // �q��C���@�ӡB�аO�A�Ȥ��A�^�� service time
    void    finishService();       // �@�ӪA�ȧ����A�M���X��

    // ��ө�ˡG�A�w�g�� sampleIT()/sampleST()

public: //�]�A���Q�� public�A�N��o��^
	std::deque<int> q;   // ��ʥ] id�]���� int counter �N���^
	bool  serving ;
	int   nextPktId ;
};
