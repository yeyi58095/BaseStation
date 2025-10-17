// HeadlessBridge.h  �X�X ��@�u�ۨӷ��]SSOT�^
#ifndef HEADLESS_BRIDGE_H
#define HEADLESS_BRIDGE_H

// �o�ӬO���� C++ �Ϊ��A���n extern "C"�]�קK C linkage overload �Ĭ�^
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean
);

// �o�Ӥ~�ݭn���~���]�Ψ�L�y���^�I�s�A�� C linkage�F**�M�פ��Ҧ��a�賣 include �o�@���N�n�A����A�U�ۤ�g�O�� extern "C" �ŧi**
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

