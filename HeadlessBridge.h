// HeadlessBridge.h  —— 單一真相來源（SSOT）
#ifndef HEADLESS_BRIDGE_H
#define HEADLESS_BRIDGE_H

// 這個是內部 C++ 用的，不要 extern "C"（避免 C linkage overload 衝突）
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
	double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean,  double* P_es
);

// 這個才需要給外部（或其他語言）呼叫，用 C linkage；**專案內所有地方都 include 這一份就好，不能再各自手寫別的 extern "C" 宣告**
#ifdef __cplusplus
extern "C" {
#endif

int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HEADLESS_BRIDGE_H

