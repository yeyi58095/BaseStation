#include "Engine.h"
#include "Master.h"
#include <fstream>
#include <time.h>
#include <vcl.h>
#include "HeadlessBridge.h"  // <== 新增這行


static void write_json(const char* path,
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    double avg_delay_ms, double L, double W, double loss_rate, double EP_mean,
    const char* versionStr)
{
    std::ofstream f(path);
    if (!f.is_open()) {
		//MessageBoxA(NULL, "write_json: cannot open output file!", "Error", MB_OK);
        return;
    }

    // ---- 安全檢查 versionStr ----
    const char* safeVer = "BaseStation";
    // 檢查指標是否看起來可讀
    if (versionStr != NULL) {
		__try {
            // 嘗試讀取頭幾個位元組，避免懸空 crash
            volatile char c = versionStr[0];
            (void)c;
            safeVer = versionStr;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // crash 就留 BaseStation
			//MessageBoxA(NULL, "write_json: invalid versionStr pointer, using default.", "Warning", MB_OK);
        }
    }

    f << "{\n";
    f << "  \"mu\":" << mu << ",\n";
    f << "  \"e\":" << e << ",\n";
    f << "  \"C\":" << C << ",\n";
    f << "  \"lambda\":" << lambda << ",\n";
    f << "  \"T\":" << T << ",\n";
    f << "  \"seed\":" << seed << ",\n";
    f << "  \"metrics\": {\n";
    f << "    \"avg_delay_ms\":" << avg_delay_ms << ",\n";
    f << "    \"L\":" << L << ",\n";
    f << "    \"W\":" << W << ",\n";
    f << "    \"loss_rate\":" << loss_rate << ",\n";
    f << "    \"EP_mean\":" << EP_mean << "\n";
    f << "  },\n";
    f << "  \"version\": \"" << safeVer << "\",\n";
    f << "  \"timestamp\": " << (long)time(NULL) << "\n";
    f << "}\n";

    f.close();
}

extern "C" int RunHeadlessEngine(
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	const char* outPath, const char* versionStr)
{
	//MessageBoxA(NULL, "Entered RunHeadlessEngine()", "Debug", MB_OK);

	if (outPath == NULL) {
		//MessageBoxA(NULL, "outPath is NULL!", "Error", MB_OK);
		return -99;
	}

	double avg_delay_ms=0, L=0, W=0, loss=0, EP_mean=0;

	// ===== 檢查呼叫 RunSimulationCore 前後 =====
	//MessageBoxA(NULL, "Before RunSimulationCore()", "Debug", MB_OK);
	int rc = RunSimulationCore(mu, e, C, lambda, T, seed,
							   &avg_delay_ms, &L, &W, &loss, &EP_mean);
	//MessageBoxA(NULL, "After RunSimulationCore()", "Debug", MB_OK);

	if (rc != 0) {
		char buf[128];
		sprintf(buf, "RunSimulationCore returned %d", rc);
		MessageBoxA(NULL, buf, "Error", MB_OK);
		return rc;
	}

	//MessageBoxA(NULL, "Before write_json()", "Debug", MB_OK);

	write_json(outPath, mu, e, C, lambda, T, seed,
			   avg_delay_ms, L, W, loss, EP_mean, versionStr);

	//MessageBoxA(NULL, "After write_json()", "Debug", MB_OK);

	return 0;
}
