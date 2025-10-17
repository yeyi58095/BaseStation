
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


// �A�{���� Sensor ���O
class Sensor;

// === �A�ݭn�Ρu�ۤv�{���� GUI �ͦ��{���X�v��@�o�Ӥu�t ===
//   �� GUI �W�쥻�Ψӫإ�/��l�ƨC�� Sensor�]�f, �g, e, C ���^���q�h�i�ӡC
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed,
                                     int N,           // <-- �s�W
                                     int r_tx,        // <-- �s�W
                                     int slots,       // <-- �D���禡�ΡA���O�d�@�P��
                                     bool alwaysCharge, // <-- �D���禡�ΡA���O�d�@�P��
                                     std::vector<Sensor*>& out)
{
    // �Ȧb�o�� reseed�A�@���N�n�F���л\�ϥΪ̪� seed
    rv::reseed(seed);

    if (N <= 0) N = 1;
    for (int i = 0; i < N; ++i) {
        Sensor* s = new Sensor(i);
        s->setArrivalExp(lambda);
        s->setServiceExp(mu);

        // Energy model�]�P GUI ����G�T�w�C�] 1EP�^
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
        int N, int r_tx, int slots, int alwaysChargeFlag,  // <-- �s�W
        double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean)
{
    if (!avg_delay_ms || !L || !W || !loss_rate || !EP_mean) return -10;
    if (seed == 0) seed = (unsigned int)time(NULL);

    std::vector<Sensor*> sensors;
    CreateSensorsForHeadless(lambda, mu, e, C, seed, N, r_tx, slots, alwaysChargeFlag != 0, sensors);

    sim::Master m;
    m.setSensors(&sensors);
    m.setEndTime((double)T);

    // �� CLI ����A�w�]�M GUI ����Gtrue
    m.logMode           = sim::Master::LOG_NONE;
    m.recordTrace       = false;
    m.logStateEachEvent = false;
    m.alwaysCharge      = (alwaysChargeFlag != 0);
    m.setChargingSlots(slots);      // 0 ��ܡu�����v�A�M�A�{���y�N�@�P

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

