
#include "Master.h"
#include <vector>
#include <stdlib.h>
#include <time.h>

#include "Sensor.h"
#include "Master.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include "HeadlessBridge.h"

#include "Master.h"
#include "Sensor.h"
#include <vector>
#include <cstdlib>
#include <ctime>


// 你現有的 Sensor 類別
class Sensor;

// === 你需要用「自己現有的 GUI 生成程式碼」實作這個工廠 ===
//   把 GUI 上原本用來建立/初始化每個 Sensor（λ, μ, e, C 等）那段搬進來。
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed,
                                     int N,           // <-- 新增
                                     int r_tx,        // <-- 新增
                                     int slots,       // <-- 非此函式用，但保留一致性
                                     bool alwaysCharge, // <-- 非此函式用，但保留一致性
                                     std::vector<Sensor*>& out)
{
    // 僅在這裡 reseed，一次就好；不覆蓋使用者的 seed
    rv::reseed(seed);

    if (N <= 0) N = 1;
    for (int i = 0; i < N; ++i) {
        Sensor* s = new Sensor(i);
        s->setArrivalExp(lambda);
        s->setServiceExp(mu);

        // Energy model（與 GUI 對齊：固定每包 1EP）
        s->E_cap = C;
        s->charge_rate = e;
        s->r_tx = (r_tx > 0 ? r_tx : 1);
        s->txCostBase = 0;
        s->txCostPerSec = 1.0;
        s->energy = 0;
        s->setQmax(100000);
        s->setPreloadInit(0);

        out.push_back(s);
    }
}


int RunSimulationCore(
        double mu, double e, int C, double lambda, int T, unsigned int seed,
        int N, int r_tx, int slots, int alwaysChargeFlag,  // <-- 新增
        double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean)
{
    if (!avg_delay_ms || !L || !W || !loss_rate || !EP_mean) return -10;
    if (seed == 0) seed = (unsigned int)time(NULL);

    std::vector<Sensor*> sensors;
    CreateSensorsForHeadless(lambda, mu, e, C, seed, N, r_tx, slots, alwaysChargeFlag != 0, sensors);

    sim::Master m;
    m.setSensors(&sensors);
    m.setEndTime((double)T);

    // 由 CLI 控制，預設和 GUI 對齊：true
    m.logMode           = sim::Master::LOG_NONE;
    m.recordTrace       = false;
    m.logStateEachEvent = false;
    m.alwaysCharge      = (alwaysChargeFlag != 0);
    m.setChargingSlots(slots);      // 0 表示「不限」，和你現有語意一致

    m.reset();
    m.run();

    sim::Master::KPIs kpi;
    if (!m.computeKPIs(kpi)) {
        int i;
        for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
        sensors.clear();
        return -20;
    }

    *avg_delay_ms = kpi.avg_delay_ms;
    *L            = kpi.L;
    *W            = kpi.W;
    *loss_rate    = kpi.loss_rate;
    *EP_mean      = kpi.EP_mean;

    int i;
    for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
    sensors.clear();

    return 0;
}

