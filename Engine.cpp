#include "Engine.h"
#include "Master.h"
#include <fstream>
#include <time.h>
#include <vcl.h>
#include "HeadlessBridge.h"  // <== 新增這行

// ---- 修正：前向宣告要包含 P_es ----
static void write_csv(const char* path,
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    double avg_delay_ms, double L, double W, double loss_rate, double EP_mean, double P_es,
    const char* versionStr);

int RunHeadlessEngine(
        double mu, double e, int C, double lambda, int T, unsigned int seed,
        int N, int r_tx, int slots, int alwaysChargeFlag,
        const char* outPath, const char* versionStr)
{
    if (outPath == NULL) {
        return -99;
    }

    double avg_delay_ms = 0.0;
    double L = 0.0, W = 0.0, loss = 0.0, EP_mean = 0.0;
    double P_es = 0.0;

    int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
                               N, r_tx, slots, alwaysChargeFlag,
                               &avg_delay_ms, &L, &W, &loss, &EP_mean, &P_es);
    if (rc != 0) {
        return rc;
    }

    // 🔄 CSV 輸出（帶上 P_es）
    write_csv(outPath, mu, e, C, lambda, T, seed,
              avg_delay_ms, L, W, loss, EP_mean, P_es, versionStr);

    return 0;
}

// ---- 修正：定義也要包含 P_es，且在 P_es 後面要有逗號 ----
static void write_csv(const char* path,
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    double avg_delay_ms, double L, double W, double loss_rate, double EP_mean, double P_es,
    const char* versionStr)
{
    bool fileExists = false;
    {
        std::ifstream fin(path);
        fileExists = fin.good();
    }

    std::ofstream f(path, std::ios::app); // append 模式
    if (!f.is_open()) {
        // MessageBoxA(NULL, "write_csv: cannot open output file!", "Error", MB_OK);
        return;
    }

    // 第一次寫入時加上表頭
    if (!fileExists) {
        f << "mu,e,C,lambda,T,seed,avg_delay_ms,L,W,loss_rate,EP_mean,P_es,version,timestamp\n";
    }

    long timestamp = (long)time(NULL);
    const char* safeVer = (versionStr != NULL) ? versionStr : "BaseStation";

    f << mu << ','
      << e << ','
      << C << ','
      << lambda << ','
      << T << ','
      << seed << ','
      << avg_delay_ms << ','
      << L << ','
      << W << ','
      << loss_rate << ','
      << EP_mean << ','
      << P_es << ','
      << '"' << safeVer << '"' << ','
      << timestamp << '\n';

    f.close();
}

