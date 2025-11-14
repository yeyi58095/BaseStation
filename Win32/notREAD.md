下面這一整段就是可以直接貼進 README（或拆成幾節）的說明稿，你可以之後再自己調順語氣或刪減細節。

---

### Project2.cpp

本檔案主要是整個程式的「入口點」與「模式切換」：

* 當命令列參數中有 `--headless` 時，會走 `DoHeadless()`，透過 `RunHeadlessEngine(...)` 以 CLI 方式跑模擬並輸出 CSV。
* 否則就啟動 VCL GUI 介面（`Form5` 等表單）。

#### GetArgD(key, val)

```cpp
static bool GetArgD(const UnicodeString &key, double &val)
```

* 用途：從命令列參數中讀取 **double** 型態的參數，例如 `--mu=3.0`、`--lambda=1.2`。
* 參數說明：

  * `key`：參數名稱（不含 `--`），例如 `L"mu"`。
  * `val`（輸出）：若找到並成功轉換，會被設為對應的浮點數。
* 回傳值：

  * `true`：找到符合 `--key=...` 的參數，且成功轉成浮點數。
  * `false`：沒找到或轉型失敗。

#### GetArgI(key, val)

```cpp
static bool GetArgI(const UnicodeString &key, int &val)
```

* 用途：從命令列參數中讀取 **int** 型態的參數，例如 `--C=5`、`--T=100000`。
* 參數說明：

  * `key`：參數名稱。
  * `val`（輸出）：讀到的整數值。
* 回傳值：

  * `true`：成功取得並轉換。
  * `false`：失敗或不存在。

#### GetArgU(key, val)

```cpp
static bool GetArgU(const UnicodeString &key, unsigned int &val)
```

* 用途：從命令列參數中讀取 **unsigned int** 參數，例如 `--seed=12345`。
* 參數說明：

  * `key`：參數名稱。
  * `val`（輸出）：非負整數（若使用者輸入負值會先取絕對值再轉成 unsigned）。
* 回傳值：

  * `true`：成功取得並轉換。
  * `false`：失敗或不存在。

#### GetArgS(key, val)

```cpp
static bool GetArgS(const UnicodeString &key, AnsiString &val)
```

* 用途：從命令列參數中讀取 **字串** 參數，例如 `--out=result.csv`、`--version=BaseStation`、`--policy=rr`。
* 參數說明：

  * `key`：參數名稱。
  * `val`（輸出）：轉成 `AnsiString` 的剩餘字串。
* 回傳值：

  * `true`：找到並成功取得。
  * `false`：不存在。

#### HasFlag(flag)

```cpp
static bool HasFlag(const UnicodeString &flag)
```

* 用途：檢查命令列是否包含某個 **布林旗標**，例如 `--headless` 或 `--alwaysCharge`。
* 參數說明：

  * `flag`：旗標名稱（不含 `--`），例如 `L"headless"`。
* 回傳值：

  * `true`：命令列中有 `--flag`。
  * `false`：沒有。

#### DoHeadless()

```cpp
static int DoHeadless()
```

* 用途：**整個 headless 模式的主流程**。

  * 設定預設參數（`mu, e, C, lambda, T, seed, outPath, N, r_tx, slots, alwaysChargeFlag` 等）。
  * 使用上述的 `GetArg*` / `HasFlag` 從 CLI 覆寫預設值。
  * 設定 polling policy、距離分佈、隊列長度上限、switch-over time 等 headless 相關全域設定（透過 `HB_SetPolicy`, `HB_SetRandomDistance`, `HB_SetFixedRBase`, `HB_SetQueueMax`, `HB_SetSwitchOver` 等）。
  * 將最終參數記錄到 `headless_log.txt`。
  * 呼叫 `RunHeadlessEngine(...)` 進行模擬與輸出 CSV。
* 回傳值：

  * `RunHeadlessEngine(...)` 的傳回碼（0 代表成功，非 0 代表錯誤）。

#### _tWinMain(...)

```cpp
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
```

* 用途：整個 Windows 應用程式的入口點。
* 行為：

  * 若命令列含 `--headless`，直接呼叫 `DoHeadless()`，**不啟動 GUI**。
  * 否則初始化 VCL，建立多個 Form（`Form5`, `Charter`, `Chooser`, ...），然後 `Application->Run()` 跑 GUI 模式。
* 回傳值：

  * 程式結束時的狀態碼。

---

### HeadlessBridge.h

本檔案定義了 **headless 模式與核心引擎之間的 API**：

* 對外提供 `RunHeadlessEngine(...)` 這個 C 介面（給 Project2 / CLI 使用）。
* 對內則提供 `RunSimulationCore(...)` 與一組 `HB_*` 函式來設定距離、隊列上限、policy 等全域模擬參數。

#### RunHeadlessEngine(...)

```cpp
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr
);
```

* 用途：headless 模式對外的**主要入口函式**。

  * 接收來自 CLI 的所有主要模擬參數。
  * 執行模擬並將統計結果寫出到 `outPath` 指定的 CSV 檔。
* 參數重點：

  * `mu`：服務率（Service rate）。
  * `e`：充電速率 / 每單位時間能量。
  * `C`：感測節點電池容量（EP 上限）。
  * `lambda`：到達率（Arrival rate）。
  * `T`：模擬總時間（time slots）。
  * `seed`：亂數種子。
  * `N`：感測節點數量。
  * `r_tx`：每次傳送所需的 EP（或基本傳輸成本單位）。
  * `slots`：每個 polling cycle 中用於充電的 slot 數。
  * `alwaysChargeFlag`：是否「永遠允許充電」。
  * `outPath`：輸出 CSV 路徑。
  * `versionStr`：版本字串（會寫在 CSV 裡）。
* 回傳值：

  * 0：成功。
  * 負值：錯誤碼（如輸出檔案開啟失敗、模擬錯誤等）。

#### RunSimulationCore(...)（完整參數版）

```cpp
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    int useD, int dDistKind, double dP1, double dP2, int rBase,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es
);
```

* 用途：核心模擬函式，**直接接受所有控制距離與發射成本的參數**。
* 參數重點（除了前面 queue 參數外）：

  * `useD`：是否啟用距離分佈（1=啟用，0=不啟用）。
  * `dDistKind`：距離分佈種類（目前實作中實際是透過 `g_dmode` 控制，這個參數可視為保留位）。
  * `dP1`, `dP2`：距離分佈參數（視 `dDistKind` / `dmode` 的種類而有不同意義）。
  * `rBase`：手動指定傳輸成本基準（txCostBase），若 <0 則使用預設。
  * `avg_delay_ms`：平均延遲（輸出）。
  * `L`：系統平均長度（輸出）。
  * `W`：平均等待時間（輸出）。
  * `loss_rate`：封包遺失率（輸出）。
  * `EP_mean`：平均能量儲存量（輸出）。
  * `P_es`：能量儲存穩定機率（輸出）。
* 回傳值：

  * 0：成功。
  * <0：錯誤（例如輸出指標為 `nullptr` 等）。

#### RunSimulationCore(...)（簡化版）

```cpp
int RunSimulationCore(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    double* avg_delay_ms, double* L, double* W, double* loss_rate,
    double* EP_mean, double* P_es
);
```

* 用途：較常用的版本，由函式內部自動讀取 `g_useD`, `g_rBase` 等全域設定。
* 參數：

  * 與前一版相同，但沒有 `useD`、`dDistKind`、`dP1`、`dP2`、`rBase`，改由事先呼叫 `HB_*` 函式設定。
* 回傳值：同上。

#### HB_ResetDistancePolicy()

```cpp
void HB_ResetDistancePolicy(void);
```

* 用途：重置所有「距離相關」的全域設定（`g_useD`, `g_dmode`, `g_dmin`, `g_dmax`, `g_dmean`, `g_dsigma`, `g_dseed`, `g_pt`, `g_alpha`, `g_rBase`）。
* 通常在 headless 主程式開始時先呼叫一次，確保環境乾淨。

#### HB_SetDistanceList(csv_dlist)

```cpp
void HB_SetDistanceList(const char* csv_dlist);
```

* 用途：預留功能，原意是給定一串距離列表，目前實作中僅忽略參數。
* 目前不影響模擬。

#### HB_SetPowerLaw(pt, alpha)

```cpp
void HB_SetPowerLaw(double pt, double alpha);
```

* 用途：設定 power-law 形式的充電率公式 `charge_rate = pt / d^alpha` 中的 `pt` 與 `alpha`。
* 參數說明：

  * `pt`：比例常數。
  * `alpha`：衰減指數（`alpha=0` 代表與距離無關）。

#### HB_SetRandomDistance(dmode, dmin, dmax, dmean, dsigma, dseed, pt, alpha)

```cpp
void HB_SetRandomDistance(const char* dmode,
                          double dmin, double dmax,
                          double dmean, double dsigma,
                          unsigned int dseed,
                          double pt, double alpha);
```

* 用途：啟用並設定距離分佈，用於產生各個 sensor 與基地台之間的距離，進而決定 `txCostBase`。
* 參數說明：

  * `dmode`：距離分佈模式 `"uniform" | "normal" | "lognormal" | "exponential"`。
  * `dmin`, `dmax`：uniform 模式的最小 / 最大距離。
  * `dmean`, `dsigma`：normal / lognormal / exponential 模式的參數。
  * `dseed`：距離亂數的種子。
  * `pt`, `alpha`：會同時設定 power-law 的參數。

#### HB_SetFixedRBase(rBase)

```cpp
void HB_SetFixedRBase(int rBase);
```

* 用途：不使用距離分佈時，直接指定所有 sensor 的 `txCostBase`。
* 參數說明：

  * `rBase`：固定的傳輸成本基準，>0 時生效，<=0 則表示不設定。

#### HB_SetQueueMax(qmax)

```cpp
void HB_SetQueueMax(int qmax);
```

* 用途：設定每個 sensor 的最大隊列長度上限。
* 參數說明：

  * `qmax`：>0 時設定為該上限；<=0 則表示不限制（預設使用 100000）。

#### HB_SetSwitchOver(tau)

```cpp
void HB_SetSwitchOver(double tau);
```

* 用途：設定 polling system 的 switch-over time（server 在不同 sensor 之間切換的延遲）。
* 參數說明：

  * `tau`：延遲時間，若 <0 會自動被裁成 0。

#### HB_SetPolicy(name)

```cpp
void HB_SetPolicy(const char* name);
```

* 用途：設定 polling policy。
* 參數說明：

  * `name`：字串 `"rr" | "df" | "cedf"`，分別對應 Round-Robin、DF、CEDF。
* 影響：會更新全域變數 `g_policy`，後續 `RunSimulationCore` 會依此執行不同排程策略。

#### HB_GetPolicy()

```cpp
int HB_GetPolicy(void);
```

* 用途：取得目前設定的 polling policy。
* 回傳值：

  * `0`：RR
  * `1`：DF
  * `2`：CEDF

---

### HeadlessBridge.cpp

本檔案實作了 HeadlessBridge.h 宣告的所有函式，並且包含：

* 距離 / power-law / queue-max 等全域設定。
* `CreateSensorsForHeadless(...)`：依照設定產生感測器。
* `RunSimulationCore(...)`：包裝 `sim::Master` 的模擬流程。
* 舊版的 `RunHeadlessEngine(...)`（單次輸出 CSV 的版本）。

#### 全域變數（距離與 policy 狀態）

* `g_useD`：是否啟用距離分佈。
* `g_dmode[]`：距離分佈模式字串。
* `g_dmin, g_dmax, g_dmean, g_dsigma`：距離分佈參數。
* `g_dseed`：距離亂數種子。
* `g_pt, g_alpha`：power-law 參數。
* `g_rBase`：固定 txCostBase。
* `g_queue_max`：隊列最大長度。
* `g_tau`：switch-over time。
* `g_policy`：polling policy（0=RR, 1=DF, 2=CEDF）。

（這些都透過前面 `HB_*` 函式來修改）

#### sample_uniform(a, b)

```cpp
static double sample_uniform(double a, double b);
```

* 用途：根據 `[a, b]` 產生一個均勻分佈的距離值（內部使用 `rv::uniform`）。

#### sample_normal_pos(m, s)

```cpp
static double sample_normal_pos(double m, double s);
```

* 用途：從常態分佈抽樣，取絕對值且保證 >0，用於距離。

#### sample_lognormal(mu, sigma)

```cpp
static double sample_lognormal(double mu, double sigma);
```

* 用途：lognormal 分佈距離。

#### sample_exponential_mean(mean)

```cpp
static double sample_exponential_mean(double mean);
```

* 用途：以指定平均值的指數分佈產生距離。

#### sample_distance()

```cpp
static double sample_distance();
```

* 用途：根據目前的 `g_useD` 與 `g_dmode` 等全域設定，選擇對應的抽樣方式，回傳一個距離。

#### CreateSensorsForHeadless(...)

```cpp
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed, int N, int r_tx, int slots, bool alwaysCharge,
                                     int useD, int dDistKind, double dP1, double dP2, int rBase,
                                     std::vector<Sensor*>& out);
```

* 用途：建立 headless 模擬中使用的 `Sensor` 物件列表，將指標放進 `out`。
* 主要行為：

  * 初始化亂數種子。
  * 為每個 sensor 設定到達率、服務率、電池容量、充電速率、傳輸成本 `txCostBase`、隊列長度上限等。
  * 如果啟用距離分佈，會為每個 sensor 抽一個距離並轉成 `txCostBase`。

#### RunSimulationCore(...)（兩個版本）

已在 HeadlessBridge.h 說明，這裡是實作：

* 建立 sensors → `sim::Master` → 設定 `endTime`、`policy`、`switchOver` 等 → `run()` → 計算 KPI → 回填輸出參數 → 刪除 sensors。

#### RunHeadlessEngine(...)（舊版）

```cpp
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr);
```

* 用途：早期版本的 headless 入口，直接呼叫 `RunSimulationCore(...)`，並使用 `std::ofstream` 寫出單一列的 CSV。
* 在目前程式中，**Engine.cpp 也提供了新版 `RunHeadlessEngine`，改用 append 方式寫 CSV 且額外輸出 `P_es` 欄位**。
* 實際專案中只要保留／使用其中一個版本即可，README 可依你最後採用的版本為主。

---

### Engine.cpp

本檔案提供新版的 `RunHeadlessEngine(...)` 實作與共用的 CSV 輸出函式 `write_csv(...)`。

* 主要差異在於：`write_csv` 會在檔案存在時進行 **append**，並自動在第一次寫入時補上 CSV 標頭。

#### RunHeadlessEngine(...)（新版）

```cpp
int RunHeadlessEngine(
        double mu, double e, int C, double lambda, int T, unsigned int seed,
        int N, int r_tx, int slots, int alwaysChargeFlag,
        const char* outPath, const char* versionStr);
```

* 用途：目前建議使用的 headless 入口，配合 CLI / `DoHeadless()` 來跑模擬。
* 行為：

  1. 檢查 `outPath` 是否為 `NULL`（是則回傳 -99）。
  2. 宣告 KPI 變數（`avg_delay_ms, L, W, loss, EP_mean, P_es`）。
  3. 呼叫 `RunSimulationCore(...)` 取得模擬結果。
  4. 呼叫 `write_csv(...)` 以 **append** 方式把結果寫入 CSV。
* 回傳值：

  * 0：成功。
  * 其他：`RunSimulationCore` 回傳的錯誤碼。

#### write_csv(...)

```cpp
static void write_csv(const char* path,
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    double avg_delay_ms, double L, double W, double loss_rate, double EP_mean, double P_es,
    const char* versionStr);
```

* 用途：將單次模擬的 KPI 追加寫入 `path` 指定的 CSV 檔案。
* 行為：

  * 若檔案不存在：先寫入表頭
    `mu,e,C,lambda,T,seed,avg_delay_ms,L,W,loss_rate,EP_mean,P_es,version,timestamp`。
  * 然後寫入一列數據，包含版本字串與時間戳記。

---

### Engine.h

目前只是個 placeholder header：

```cpp
// Engine.h
#ifndef ENGINE_H
#define ENGINE_H
// …（註解說明未來 Engine 本體可放這裡）…
#endif
```

* 用途：保留 `Engine` 相關宣告的位置，並且讓 `Engine.cpp` 可以 `#include` 自己專案的 Engine 介面。
* 實際的 queue 模擬邏輯目前是透過 `sim::Master`、`Sensor` 等類別進行，而不是放在這個檔案中。

---

### MMEngine.h

`MMEngine.h` 是一個 **header-only 的 M/M/1 排隊模擬引擎**，與 BaseStation 的 Headless 模式無直接關係，主要給 GUI 或教學用來做標準 M/M/1 模型。

#### namespace mm1::ISimObserver

```cpp
class ISimObserver {
public:
    virtual ~ISimObserver() {}
    virtual void onPoint(double t, int queue_size) {}
    virtual void onMean(double t, double meanQ, double meanS) {}
    virtual void onStep(double t, int q, int s, double meanQ, double meanS) {}
};
```

* 用途：觀察者介面，讓外部程式可以在模擬過程中收到回呼並更新圖表或 UI。

#### mm1::FEL（Future Event List）

```cpp
struct EventNode { ... };
class FEL { ... };
```

* 用途：簡易的事件鏈結串列，用來以時間排序管理 ARRIVAL / SERVICE / DEPARTURE 事件。

#### mm1::Engine

```cpp
class Engine {
public:
    enum { ARRIVAL = 0, SERVICE = 1, DEPARTURE = 2 };
    // ...
    void setObserver(ISimObserver* obs);
    void setInterArrival(SampleFn fn, void* user);
    void setService    (SampleFn fn, void* user);
    void reset();
    void run(double end_time);
    double now() const;
    int    queue() const;
    int    system() const;
    double meanQ() const;
    double meanS() const;
    // ...
};
```

* 用途：以事件驅動方式模擬 M/M/1 排隊系統，可外掛任意 inter-arrival / service time 分佈。
* 主要介面：

  * `setObserver(...)`：註冊觀察者。
  * `setInterArrival(...)`、`setService(...)`：設定分佈生成函式。
  * `reset()`：重設狀態。
  * `run(end_time)`：從 t=0 模擬到指定時間。
  * `meanQ()` / `meanS()`：取得平均排隊長度與平均系統內人數。

---

如果你接下來還會貼 `Master` / `Sensor` / `RandomVar` 等檔案，我也可以依照同一種格式幫你補上「檔案用途 + 主要函式用途」的 README 區段。
