// HeadlessBridge.h
#pragma once

// 這裡不需要特別 include VCL，只要 C interface 就好

#ifdef __cplusplus
extern "C" {
#endif

// ============ Queue / polling ============

// 設定感測器 queue 最大長度（<=0 表示沿用預設）
void HB_SetQueueMax(int qmax);

// 設定 HAP switchover latency tau（輪詢延遲）
void HB_SetSwitchOver(double tau);

// ============ Distance / power-law ============

// 重置距離與 power-law 策略（恢復預設 homogeneous）
void HB_ResetDistancePolicy(void);

// 預留：CSV 距離列表版本，目前不使用
void HB_SetDistanceList(const char* csv_dlist);

// 設定 power-law 參數：charge_rate ∝ pt / d^alpha
void HB_SetPowerLaw(double pt, double alpha);

// 設定距離分佈與 power-law：
// dmode: "uniform" / "normal" / "lognormal" / "exponential"
// 參數意義：
//   uniform:    dmin, dmax
//   normal:     dmean, dsigma
//   lognormal:  dmean, dsigma（對數空間參數）
//   exponential:dmean（平均值）
// dseed:  距離亂數種子
// pt,alpha: power-law 參數
void HB_SetRandomDistance(const char* dmode,
                          double dmin, double dmax,
                          double dmean, double dsigma,
                          unsigned int dseed,
                          double pt, double alpha);

// 固定 rBase（沒開距離模式時用）；<0 表示關閉
void HB_SetFixedRBase(int rBase);

// ============ Policy ============

// name: "rr" / "df" / "cedf" ...
void HB_SetPolicy(const char* name);

// 回傳目前 policy code（0=RR,1=DF,2=CEDF）
int  HB_GetPolicy(void);

// ============ Log mode ============
//
// --log=none  → HB_LogNone()
// --log=csv   → HB_LogCSV()
// --log=human → HB_LogHuman()
//
void HB_LogNone(void);
void HB_LogCSV(void);
void HB_LogHuman(void);

// ============ Headless 入口 ============
//
// mu,e,C,lambda,T,seed,N,r_tx,slots,alwaysChargeFlag 由 CLI 傳入
// outPath:  輸出的 CSV 檔
// versionStr: CSV 裡的 version 欄位
//
int RunHeadlessEngine(double mu, double e, int C,
                      double lambda, int T, unsigned int seed,
                      int N, int r_tx, int slots, int alwaysChargeFlag,
                      const char* outPath, const char* versionStr);

#ifdef __cplusplus
}
#endif

