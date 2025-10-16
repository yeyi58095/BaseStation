
                                                         // EngineAdapter.h
#ifndef ENGINE_ADAPTER_H
#define ENGINE_ADAPTER_H

extern "C" int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean);

#endif
