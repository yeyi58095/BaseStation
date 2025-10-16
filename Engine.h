#ifndef ENGINE_H
#define ENGINE_H
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    const char* outPath, const char* versionStr);
#endif

