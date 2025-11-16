#if 0
#include "Engine.h"
#include "Master.h"
#include <fstream>
#include <time.h>
#include <vcl.h>
#include "HeadlessBridge.h"  // 必須包含，取得舊簽名 overloading

static void write_csv(const char* path,
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	double avg_delay_ms, double L, double W, double loss_rate, double EP_mean, double P_es,
	const char* versionStr);

#if 0
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

	// 使用舊簽名：由 HeadlessBridge 內部取 g_useD/g_rBase 等全域策略
	int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
							   N, r_tx, slots, alwaysChargeFlag,
							   &avg_delay_ms, &L, &W, &loss, &EP_mean, &P_es);
	if (rc != 0) {
		return rc;
	}

	write_csv(outPath, mu, e, C, lambda, T, seed,
			  avg_delay_ms, L, W, loss, EP_mean, P_es, versionStr);

	return 0;
}

#endif
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

	std::ofstream f(path, std::ios::app); // append
	if (!f.is_open()) {
		return;
	}

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

#endif
