                                                         #ifndef HEADLESS_BRIDGE_H
#define HEADLESS_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// 既有 headless 入口（簽名不變）
int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr
);

#ifdef __cplusplus
} // extern "C"
#endif

// ================= 供 Engine / 單元測試呼叫的核心 =================

// 新版：包含距離/固定 rBase 等參數
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    int useD, int dDistKind, double dP1, double dP2, int rBase,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es
);

// 舊簽名 overloading（保持向下相容；內部轉呼叫新版並吃全域策略）
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es
);

// ================== 距離/能耗 策略（由 Project2.cpp 設定） ==================
#ifdef __cplusplus
extern "C" {
#endif

// 重置策略（關閉距離、清空清單、恢復預設 power-law 參數與 rBase）
void HB_ResetDistancePolicy(void);

// 直接提供距離清單（目前先保留接口，暫不使用）
void HB_SetDistanceList(const char* csv_dlist);

// charge_rate = pt / d^alpha（若 alpha=0 等價常數）
void HB_SetPowerLaw(double pt, double alpha);

// 亂數距離
// dmode: "uniform" | "normal" | "lognormal" | "exponential"
void HB_SetRandomDistance(const char* dmode,
                          double dmin, double dmax,     // uniform
                          double dmean, double dsigma,  // normal/lognormal/exponential(mean)
                          unsigned int dseed,
                          double pt, double alpha);

// 非距離模式：固定 txCostBase
void HB_SetFixedRBase(int rBase);
void HB_SetQueueMax(int qmax);
void HB_SetSwitchOver(double tau);
// ---- polling policy ----
// name: "rr" | "df" | "cedf"（大小寫不敏感）
void HB_SetPolicy(const char* name);
int  HB_GetPolicy(void);                    // 0=RR, 1=DF, 2=CEDF
#ifdef __cplusplus
} // extern "C"
#endif

#endif // HEADLESS_BRIDGE_H

