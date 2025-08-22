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

    // IT = inter-arrival time；ST = service time
    void setIT(int method, double para1, double para2 = 0.0);
    void setST(int method, double para1, double para2 = 0.0);

    // 方便在選 exponential 時使用
    void setArrivalExp(double lambda) { setIT(DIST_EXPONENTIAL, lambda); }
	void setServiceExp(double mu)     { setST(DIST_EXPONENTIAL, mu); }
	AnsiString toString() const;
	static AnsiString dict(int );
	double sampleIT() const;
	double sampleST() const;

public:
    int id;
    int max_queue_size;   // 用建構子給預設值
    int ITdistri;
    int STdistri;
    double ITpara1, ITpara2;
	double STpara1, STpara2;

public:
    // ... 你原本的介面都保留

    // ---- Master 會用到的 4 個動作 ----
    void    enqueueArrival();      // 到達：把新封包放進佇列
    bool    canServe() const;      // 是否可以開始服務（預設：有貨且沒在服）
    double  startService();        // 從佇列取一個、標記服務中，回傳 service time
    void    finishService();       // 一個服務完成，清除旗標

    // 兩個抽樣：你已經有 sampleIT()/sampleST()

public: //（你說想全 public，就放這邊）
	std::deque<int> q;   // 放封包 id（先用 int counter 代替）
	bool  serving ;
	int   nextPktId ;
};
