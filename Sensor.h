#pragma once
#include <System.hpp>  // AnsiString

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

public:
    int id;
    int max_queue_size;   // �Ϋغc�l���w�]��
    int ITdistri;
    int STdistri;
    double ITpara1, ITpara2;
    double STpara1, STpara2;
};
