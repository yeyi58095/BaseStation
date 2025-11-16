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
#include <string>

// ================= 全域策略狀態 =================
static int      g_useD      = 0;        // 1=啟用距離模式
static char     g_dmode[16] = "";       // "uniform|normal|lognormal|exponential"
static double   g_dmin      = 0.0;
static double   g_dmax      = 0.0;
static double   g_dmean     = 0.0;
static double   g_dsigma    = 0.0;
static unsigned g_dseed     = 0;
static double   g_pt        = 1.0;      // power law 常數：charge_rate = e * pt / d^alpha
static double   g_alpha     = 1.0;      // power law 指數 alpha
static int      g_rBase     = -1;       // 非距離模式固定 txCostBase（<0 表未指定）
static int      g_queue_max = -1;
static double   g_tau       = 0.0;      // switchover (polling) latency
static int      g_policy    = 0;        // 0=RR, 1=DF, 2=CEDF

// 給 debug 用：這次 run 的 outPath
static std::string g_lastOutPath;

// ----------------------------------------------------------------------
// 路徑處理 & debug dump 小工具
// ----------------------------------------------------------------------

// 把 "Paper2/Fig6/out/xxx.csv" 變成 "Paper2/Fig6"
static std::string figure_dir_from_out(const std::string& outPath)
{
    std::string dir = outPath;

    // 優先找 "/out/" 這一段
    size_t pos = dir.find("/out/");
    if (pos == std::string::npos) {
        pos = dir.find("\\out\\"); // Windows 風格
    }
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);  // 留到 FigX 那層
    } else {
        // 找不到 out/ 就退而求其次：砍到最後一個斜線
        size_t last = dir.find_last_of("/\\");
        if (last != std::string::npos) {
            dir = dir.substr(0, last);
        }
    }
    return dir;
}

// 把目前 sensors 的關鍵參數 dump 成文字檔：PaperX/FigY/sensors_debug.txt
static void dump_sensors_debug(const std::vector<Sensor*>& sensors,
                               double lambda, double mu, int C)
{
    if (g_lastOutPath.empty()) return;

    std::string figDir   = figure_dir_from_out(g_lastOutPath);
    std::string dumpPath = figDir + "/sensors_debug.txt";

    std::ofstream fout(dumpPath.c_str());   // 預設 trunc → 每次覆蓋
    if (!fout.is_open()) {
        return;
    }

    int Qmax_effective = (g_queue_max > 0 ? g_queue_max : 100000);

    fout << "# Sensor configuration debug dump\n";
    fout << "# lambda = " << lambda
         << ", mu = "    << mu
         << ", C = "     << C
         << ", Q_max = " << Qmax_effective
         << "\n";
    fout << "# useD = "  << g_useD
         << ", dmode = " << g_dmode
         << ", pt = "    << g_pt
         << ", alpha = " << g_alpha
         << ", tau = "   << g_tau
         << ", policy = " << g_policy
         << "\n\n";

    for (size_t i = 0; i < sensors.size(); ++i) {
        const Sensor* s = sensors[i];
        fout << "Sensor " << i << ":\n";
        fout << "  txCostBase (r_i) = " << s->txCostBase << "\n";
        fout << "  charge_rate (e_i) = " << s->charge_rate << "\n";
        fout << "  E_cap = " << s->E_cap << "\n";
        fout << "  Q_max = " << Qmax_effective << "\n";
        fout << "  r_tx = " << s->r_tx << "\n";
        fout << "\n";
    }
}

// ----------------------------------------------------------------------
// Queue / switchover / policy 相關 API
// ----------------------------------------------------------------------

extern "C" void HB_SetQueueMax(int qmax) {
    if (qmax <= 0) g_queue_max = -1;  // 0 或負數 → 不覆蓋
    else g_queue_max = qmax;
}

extern "C" void HB_SetSwitchOver(double tau) {
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
    g_pt    = pt;
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

// ----------------------------------------------------------------------
// 距離取樣工具
// ----------------------------------------------------------------------

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

// ----------------------------------------------------------------------
// 建立 sensors（這裡套用 power law + r_i = ceil(d^2)）
// ----------------------------------------------------------------------

static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed, int N, int r_tx, int slots, bool alwaysCharge,
                                     int useD, int dDistKind, double dP1, double dP2, int rBase,
                                     std::vector<Sensor*>& out)
{
    (void)useD; (void)dDistKind; (void)dP1; (void)dP2; // 實際吃的是全域 g_*

    if (seed == 0) seed = (unsigned int)std::time(NULL);
    rv::reseed(seed);

    if (N <= 0) N = 1;
    for (int i = 0; i < N; ++i) {
        Sensor* s = new Sensor(i);
        s->setArrivalExp(lambda);
        s->setServiceExp(mu);

        s->E_cap        = C;
        s->r_tx         = (r_tx > 0 ? r_tx : 1);
        s->txCostPerSec = 0.0;
        s->txCostBase   = 1.0;
        s->energy       = 0;

        if (g_useD) {
            // 抽距離 d_i
            if (g_dseed != 0) rv::reseed(g_dseed + (unsigned)i);
            double d = sample_distance();
            if (d <= 0.0) d = 1e-12;

            // 1) r_i = ceil(d_i^2)  —— 對應論文裡的 r_i
            double r_energy = std::ceil(d * d);
            if (r_energy < 1.0) r_energy = 1.0;
            s->txCostBase = r_energy;

            // 2) e_i = e * pt / d_i^alpha —— 對應 e_i ∝ Pt / d_i^alpha
            double denom = std::pow(d, g_alpha);
            if (denom <= 0.0) denom = 1e-12;
            double charge = e * g_pt / denom;
            s->charge_rate = charge;
        } else {
            // 沒開距離模式時保持舊行為（homogeneous）
            s->charge_rate = e;
            if (g_rBase > 0) s->txCostBase = (double)g_rBase;
            else             s->txCostBase = (s->r_tx > 0 ? (double)s->r_tx : 1.0);
        }

        if (g_queue_max > 0) s->setQmax(g_queue_max);
        else                 s->setQmax(100000);
        s->setPreloadInit(0);
        out.push_back(s);
    }
}

// ----------------------------------------------------------------------
// 核心（新版簽名）
// ----------------------------------------------------------------------

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
    m.setChargingSlots(slots);     // 0=不限
    m.setSwitchOver(g_tau);        // polling latency
    m.setPolicy(g_policy);         // RR / DF / CEDF
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

    // 在刪掉 sensors 前，把這次的 sensor 設定 dump 出去
    dump_sensors_debug(sensors, lambda, mu, C);

    for (size_t i = 0; i < sensors.size(); ++i) delete sensors[i];
    sensors.clear();

    return ok ? 0 : -20;
}

// ----------------------------------------------------------------------
// 核心（舊簽名 overloading）
// ----------------------------------------------------------------------

int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es)
{
    // 直接使用全域策略 g_useD / g_rBase ... 轉呼叫新版
    return RunSimulationCore(mu, e, C, lambda, T, seed,
                             N, r_tx, slots, alwaysChargeFlag,
                             /*useD*/ g_useD, /*dDistKind*/ 0,
                             /*dP1*/ 0.0, /*dP2*/ 0.0, /*rBase*/ g_rBase,
                             avg_delay_ms, L, W, loss_rate, EP_mean, P_es);
}

// ----------------------------------------------------------------------
// 外層封裝：輸出 CSV（沿用舊 signature）
// ----------------------------------------------------------------------

extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr)
{
    // 記住這次模擬輸出的 CSV 路徑，給 debug dump 用
    g_lastOutPath = (outPath && outPath[0]) ? outPath : "result.csv";

    double avg_delay_ms = 0, L = 0, W = 0, loss_rate = 0, EP_mean = 0, P_es = 0;
    int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
                               N, r_tx, slots, alwaysChargeFlag,
                               &avg_delay_ms, &L, &W, &loss_rate, &EP_mean, &P_es);
    if (rc != 0) return rc;

    const char* ver = (versionStr && versionStr[0]) ? versionStr : "BaseStation";
    std::ofstream f(outPath ? outPath : "result.csv");
    if (!f.is_open()) return -30;

    // 跟 Engine.cpp 原本一樣：有 P_es 欄位
    f << "mu,e,C,lambda,T,seed,avg_delay_ms,L,W,loss_rate,EP_mean,P_es,version,timestamp\n";
    f << mu << "," << e << "," << C << "," << lambda << "," << T << "," << seed
      << "," << avg_delay_ms << "," << L << "," << W << "," << loss_rate << "," << EP_mean
      << "," << P_es
      << "," << "\"" << ver << "\"" << "," << (long long)std::time(NULL) << "\n";
    f.close();

    return 0;
}

