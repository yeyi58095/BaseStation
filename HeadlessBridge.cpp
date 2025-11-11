#include "HeadlessBridge.h"

#include "Master.h"
#include "Sensor.h"
#include "RandomVar.h"

#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <fstream>

// ================= 全域策略狀態 =================
static int      g_useD      = 0;        // 1=啟用距離模式
static char     g_dmode[16] = "";       // "uniform|normal|lognormal|exponential"
static double   g_dmin      = 0.0;
static double   g_dmax      = 0.0;
static double   g_dmean     = 0.0;
static double   g_dsigma    = 0.0;
static unsigned g_dseed     = 0;
static double   g_pt        = 1.0;      // charge_rate = pt / d^alpha（若你要用）
static double   g_alpha     = 1.0;
static int      g_rBase     = -1;       // 非距離模式固定 txCostBase（<0 表未指定）
static int g_queue_max = -1;
static double g_tau    = 0.0;     // switchover (polling) latency
static int g_policy = 0; // 0=RR, 1=DF, 2=CEDF


extern "C" void HB_SetQueueMax(int qmax) {
    if (qmax <= 0) g_queue_max = -1;  // 0 或負數 → 不覆蓋
    else g_queue_max = qmax;
}

void HB_SetSwitchOver(double tau) {
    if (tau < 0.0) tau = 0.0;
    g_tau = tau;
}


extern "C" void HB_ResetDistancePolicy(void) {
    g_useD = 0;
    g_dmode[0] = '\0';
    g_dmin = g_dmax = g_dmean = g_dsigma = 0.0;
    g_dseed = 0;
    g_pt = 1.0;
    g_alpha = 1.0;
    g_rBase = -1;
}
extern "C" void HB_SetDistanceList(const char* csv_dlist) {
    (void)csv_dlist; // 此版保留接口，不使用
}
extern "C" void HB_SetPowerLaw(double pt, double alpha) {
    g_pt = pt;
    g_alpha = alpha;
}
extern "C" void HB_SetRandomDistance(const char* dmode,
                                     double dmin, double dmax,
                                     double dmean, double dsigma,
                                     unsigned int dseed,
                                     double pt, double alpha) {
    g_useD = 1;
    if (dmode && dmode[0]) {
        size_t n = std::strlen(dmode);
        if (n > sizeof(g_dmode)-1) n = sizeof(g_dmode)-1;
        std::memcpy(g_dmode, dmode, n);
        g_dmode[n] = '\0';
    } else {
        g_dmode[0] = '\0';
    }
    g_dmin  = dmin;
    g_dmax  = dmax;
    g_dmean = dmean;
    g_dsigma= dsigma;
    g_dseed = dseed;
    g_pt    = pt;
    g_alpha = alpha;
}
extern "C" void HB_SetFixedRBase(int rBase) {
    g_rBase = rBase;
}

// ================ 距離取樣工具 ================
static double sample_uniform(double a, double b) {
    if (b < a) { double t=a; a=b; b=t; }
    return rv::uniform(a, b);
}
static double sample_normal_pos(double m, double s) {
    if (s <= 0) s = 1e-9;
    double x = std::fabs(rv::normal(m, s));
    if (x <= 0.0) x = 1e-9;
    return x;
}
static double sample_lognormal(double mu, double sigma) {
    if (sigma <= 0) sigma = 1e-9;
    double z = rv::normal(mu, sigma);
    double d = std::exp(z);
    if (d <= 0.0) d = 1e-12;
    return d;
}
static double sample_exponential_mean(double mean) {
    double m = (mean > 0 ? mean : 1.0);
    double rate = 1.0 / m;
    double d = rv::exponential(rate);
    if (d <= 0.0) d = 1e-12;
    return d;
}
static double sample_distance() {
    if (!g_useD) return 1.0;
    if (std::strcmp(g_dmode, "uniform") == 0 || g_dmode[0] == '\0') {
        return sample_uniform(g_dmin, g_dmax);
    } else if (std::strcmp(g_dmode, "normal") == 0) {
        return sample_normal_pos(g_dmean, g_dsigma);
    } else if (std::strcmp(g_dmode, "lognormal") == 0) {
        return sample_lognormal(g_dmean, g_dsigma);
    } else if (std::strcmp(g_dmode, "exponential") == 0) {
        return sample_exponential_mean(g_dmean);
    }
    return sample_uniform(g_dmin, g_dmax);
}

// ================ 建立 sensors ================
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed, int N, int r_tx, int slots, bool alwaysCharge,
                                     int useD, int dDistKind, double dP1, double dP2, int rBase,
                                     std::vector<Sensor*>& out)
{
    (void)dDistKind; (void)dP1; (void)dP2; // 此版不用（我們吃全域 g_*）
    if (seed == 0) seed = (unsigned int)std::time(NULL);
    rv::reseed(seed);

    if (N <= 0) N = 1;
    for (int i = 0; i < N; ++i) {
        Sensor* s = new Sensor(i);
        s->setArrivalExp(lambda);
        s->setServiceExp(mu);

        s->E_cap        = C;
        s->charge_rate  = e;
        s->r_tx         = (r_tx > 0 ? r_tx : 1);
        s->txCostPerSec = 0.0;         // 目前不隨封包長度
        s->txCostBase   = 1.0;         // 預設相容 r=1
		s->energy       = 0;

        if (g_useD) {
            if (g_dseed != 0) rv::reseed(g_dseed + (unsigned)i);
            double d = sample_distance();
            int r = (int)std::ceil(d);
            if (r < 1) r = 1;
            s->txCostBase = (double)r;
        } else {
            if (g_rBase > 0) s->txCostBase = (double)g_rBase;
            else             s->txCostBase = (s->r_tx > 0 ? (double)s->r_tx : 1.0);
		}


		if (g_queue_max > 0) s->setQmax(g_queue_max);
		else                 s->setQmax(100000);
        s->setPreloadInit(0);
        out.push_back(s);
    }
}

// ================ 核心（新版簽名） ================
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    int useD, int dDistKind, double dP1, double dP2, int rBase,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es)
{
    if (!avg_delay_ms || !L || !W || !loss_rate || !EP_mean) return -10;

    std::vector<Sensor*> sensors;
    CreateSensorsForHeadless(lambda, mu, e, C, seed, N, r_tx, slots, alwaysChargeFlag != 0,
                             useD, dDistKind, dP1, dP2, rBase, sensors);

	sim::Master m;
    m.setSensors(&sensors);
    m.setEndTime((double)T);

    m.logMode           = sim::Master::LOG_NONE;
    m.recordTrace       = false;
    m.logStateEachEvent = false;
    m.alwaysCharge      = (alwaysChargeFlag != 0);
    m.setChargingSlots(slots); // 0=不限
	m.setSwitchOver(g_tau);
	m.setPolicy(g_policy);
    m.reset();
	m.run();

    sim::Master::KPIs kpi;
    int ok = m.computeKPIs(kpi) ? 1 : 0;

    if (ok) {
        *avg_delay_ms = kpi.avg_delay_ms;
        *L            = kpi.L;
        *W            = kpi.W;
        *loss_rate    = kpi.loss_rate;
        *EP_mean      = kpi.EP_mean;
        if (P_es) *P_es = kpi.P_es;
    }

    for (size_t i = 0; i < sensors.size(); ++i) delete sensors[i];
    sensors.clear();

    return ok ? 0 : -20;
}

// ================ 核心（舊簽名 overloading） ================
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es)
{
    // 直接使用全域策略 g_useD / g_rBase ... 轉呼叫新版
	return RunSimulationCore(mu, e, C, lambda, T, seed,
							 N, r_tx, slots, alwaysChargeFlag,
                             /*useD*/ g_useD, /*dDistKind*/ 0, /*dP1*/ 0.0, /*dP2*/ 0.0, /*rBase*/ g_rBase,
                             avg_delay_ms, L, W, loss_rate, EP_mean, P_es);
}

// ================ 外層封裝：輸出 CSV（沿用舊 signature） ================
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr)
{
    double avg_delay_ms = 0, L = 0, W = 0, loss_rate = 0, EP_mean = 0, P_es = 0;
    int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
                               N, r_tx, slots, alwaysChargeFlag,
                               &avg_delay_ms, &L, &W, &loss_rate, &EP_mean, &P_es);
    if (rc != 0) return rc;

    const char* ver = (versionStr && versionStr[0]) ? versionStr : "BaseStation";
    std::ofstream f(outPath ? outPath : "result.csv");
    if (!f.is_open()) return -30;

    f << "mu,e,C,lambda,T,seed,avg_delay_ms,L,W,loss_rate,EP_mean,version,timestamp\n";
    f << mu << "," << e << "," << C << "," << lambda << "," << T << "," << seed
      << "," << avg_delay_ms << "," << L << "," << W << "," << loss_rate << "," << EP_mean
      << "," << "\"" << ver << "\"" << "," << (long long)std::time(NULL) << "\n";
    f.close();

    return 0;
}

extern "C" void HB_SetPolicy(const char* name) {
    g_policy = 0; // default RR
	if (!name) return;

#ifdef _WIN32
    if (stricmp(name, "df") == 0)        g_policy = 1;
    else if (stricmp(name, "cedf") == 0) g_policy = 2;
#else
    if (strcasecmp(name, "df") == 0)        g_policy = 1;
    else if (strcasecmp(name, "cedf") == 0) g_policy = 2;
#endif
}

extern "C" int HB_GetPolicy(void) {
    return g_policy;
}
