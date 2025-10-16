
#include "Master.h"
#include <vector>
#include <stdlib.h>
#include <time.h>

// 你現有的 Sensor 類別
class Sensor;

// === 你需要用「自己現有的 GUI 生成程式碼」實作這個工廠 ===
//   把 GUI 上原本用來建立/初始化每個 Sensor（λ, μ, e, C 等）那段搬進來。
//   注意：不要用任何 VCL/TeeChart/表單物件。
//   下面先給一個空殼，防止編譯爆炸，你要把 TODO 換成真的。
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed,
                                     std::vector<Sensor*>& out)
{
	// TODO: 依你的設計 push_back N 個 Sensor*
	//       並設定它們的到達率/服務率/EP 參數/容量 C 等。
	// 範例（假設 1 個 sensor；你可改成多個）：
	// out.push_back(new Sensor(lambda, mu, e, C, seed));
	(void)lambda; (void)mu; (void)e; (void)C; (void)seed;
}

extern "C" int RunSimulationCore(
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean)
{
	if (!avg_delay_ms || !L || !W || !loss_rate || !EP_mean) return -10;
	if (seed == 0) seed = (unsigned int)time(NULL);

	// 1) 建立 sensors（用你原本的邏輯）
	std::vector<Sensor*> sensors;
	CreateSensorsForHeadless(lambda, mu, e, C, seed, sensors);

	// 2) 建立 Master、關掉重負荷紀錄，設定模擬時間
	sim::Master m;
	m.setSensors(&sensors);
	m.setEndTime((double)T);

	// 一些常用的「headless 清爽」設定（可依你需求調整）
	m.logMode           = sim::Master::LOG_NONE;
	m.recordTrace       = false;
	m.logStateEachEvent = false;
	m.alwaysCharge      = false;        // 依你的策略
	m.setChargingSlots(0);              // 0=unlimited（若你有多工充電）

	// 3) 跑模擬
	m.reset();
	m.run();

	// 4) 算 KPI
	sim::Master::KPIs kpi;
	if (!m.computeKPIs(kpi)) {
		// 清理 sensors
		int i;
		for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
		sensors.clear();
		return -20;
	}

	// 5) 填回呼叫者需要的統計
	*avg_delay_ms = kpi.avg_delay_ms;
	*L            = kpi.L;
	*W            = kpi.W;
	*loss_rate    = kpi.loss_rate;
	*EP_mean      = kpi.EP_mean;

	// 6) 清理 sensors（避免記憶體外洩）
	int i;
	for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
	sensors.clear();

	return 0;
}
