#include "Engine.h"
#include "Master.h"
#include <fstream>
#include <time.h>
#include <vcl.h>
#include "HeadlessBridge.h"  // <== 新增這行

   static void write_csv(const char* path,
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	double avg_delay_ms, double L, double W, double loss_rate, double EP_mean,
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

    int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
                               N, r_tx, slots, alwaysChargeFlag,
                               &avg_delay_ms, &L, &W, &loss, &EP_mean);
    if (rc != 0) {
        return rc;
    }

    // 🔄 改成 CSV 版本
    write_csv(outPath, mu, e, C, lambda, T, seed,
              avg_delay_ms, L, W, loss, EP_mean, versionStr);

    return 0;
}

static void write_csv(const char* path,
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	double avg_delay_ms, double L, double W, double loss_rate, double EP_mean,
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
        f << "mu,e,C,lambda,T,seed,avg_delay_ms,L,W,loss_rate,EP_mean,version,timestamp\n";
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
      << '"' << safeVer << '"' << ','
      << timestamp << '\n';

    f.close();
}

