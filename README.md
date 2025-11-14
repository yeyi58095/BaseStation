# BaseStation — Wireless Powered Sensor Network Simulator

BaseStation 是一個以 C++Builder (VCL) 開發的模擬器，用來研究「無線供電感測網路／能量收集佇列系統」的行為。  
系統中有一個 HAP/Base Station，對多個 Sensor 進行充電與資料收發；每個 Sensor 具有：

- 資料封包佇列（Data Packets, DP）
- 能量單位 (Energy Packets, EP)
- 到達時間／服務時間的隨機分佈
- 充電、傳送、丟棄等行為

本專案支援兩種運作模式：  
- **GUI 模式**：透過 VCL 視窗操作，觀察即時模擬  
- **Headless CLI 模式**：從命令列執行大批量模擬，輸出 CSV 給 Python 後處理

---

## 專案結構與檔案說明

### 核心程式檔案說明  
以下每個檔案都列出了其模組功能、以及其中幾個主要函式的用途說明。

#### `Sensor.h`  
用途：定義 Sensor 節點類別（`Sensor`），管理封包佇列與能量狀態。  
主要內容：  
- `class Sensor { … };`  
- 成員變數：例如 `energy`, `E_cap`, `q`, `serving`, `r_tx`, `charge_rate` 等  
- 分佈參數變數：`ITdistri`, `ITpara1`, `ITpara2`, `STdistri`, `STpara1`, `STpara2`  

主要函式：  
- `void setArrivalExp(double lambda);`  
  說明：設定封包到達（Inter-arrival time）為指數分佈，參數為 λ。  
- `double sampleIT();`  
  說明：從所設定的到達時間分佈抽樣一筆值（以秒為單位）。  
- `void enqueueArrival();`  
  說明：在節點上產生一個新的封包並加入佇列。  
- `bool canTransmit() const;`  
  說明：判別該節點是否滿足傳送條件（佇列非空、且能量 ≥ r_tx）。  
- `double startTx();`  
  說明：啟動一次傳送：從佇列取出封包、扣除能量、標記為「正在傳送」狀態，並回傳該封包所需服務時間（秒）。  
- `void finishTx();`  
  說明：完成傳送：標記「傳送完成」、釋放 serving 狀態。  
- `void addEnergy(double dt);`  
  說明：根據充電速率 `charge_rate`，在間隔 `dt` 秒內為節點增加能量。  
- `std::string toString() const;`  
  說明：回傳該節點目前狀態（能量、佇列長度、服務中封包 ID等）的文字描述，用於 log 或 GUI 顯示。  

---

#### `Sensor.cpp`  
用途：實作 `Sensor.h` 中宣告的所有函式。  
主要內容：實作分佈抽樣邏輯（依指數／常態／均勻分佈型態），維護封包佇列狀態、充電邏輯、傳送狀態轉移。  
主要函式（對應 `Sensor.h`）的補充說明：  
- `double sampleST();`  
  說明：從服務時間分佈中抽樣一筆值（該封包的服務時間）。  
- `int drops() const;`  
  說明：回傳該節點迄今因佇列滿或能量不足而被丟棄的封包數量。  
- `void reset();`  
  說明：將節點回復初始狀態（清空佇列、能量歸零或預設值、重置統計量）以重複模擬。  

---

#### `Engine.h`  
用途：模擬引擎核心類別，管理所有 Sensor 節點、統計量、時間推進。  
主要內容：  
- `class Engine { … };`  
- 成員變數：例如 `std::vector<Sensor> sensors`, `double clock`, `double T_end`, 排程策略變數、統計量變數（平均延遲、佇列長度、損失率）等。  
主要函式：  
- `void initialize(int N, double lambda, double mu, double C, double e, unsigned seed);`  
  說明：初始化模擬參數，建立 N 個 Sensor、設置亂數種子、重置時計與統計。  
- `void run();`  
  說明：以離散事件模擬或時間步進方式從 `clock = 0` 推進至 `T_end`，在過程中觸發封包到達、充電、傳送、完成。  
- `void step(double dt);`  
  說明：推進時間 `dt` 秒，對所有 Sensor 執行 `addEnergy(dt)`、檢查 `enqueueArrival()` 是否產生新封包、觸發傳送條件。  
- `void scheduleTransmission();`  
  說明：根據所選擇的排程策略（如 Round-Robin、最小佇列長度、最多能量）挑出要服務的 Sensor 開始傳送。  
- `void collectStatistics();`  
  說明：在模擬結束或某時間點收集系統統計量（如平均佇列長度 L, 平均等待時間 W, 損失率等）。  
- `std::string summary();`  
  說明：回傳模擬結果的摘要文字，可直接輸出至 log 或 GUI。  

---

#### `Engine.cpp`  
用途：實作 `Engine.h` 中宣告的所有函式。  
主要內容：封包到達事件管理（呼叫每個 Sensor 的 `enqueueArrival()`）、系統時計管理、傳送開始與完成邏輯、統計量計算、支援 GUI 更新與 Headless 模式輸出。  
主要函式補充說明：  
- `void prepareNextEvent();`  
  說明：計算下一個即將發生的事件（例如下一個到達時間、下一個傳送完成時間、下一次充電時間閾值）並將 `clock` 跳轉至該事件時間。  
- `void outputCSV(const std::string &filename);`  
  說明：將模擬結果（可能是封包數、延遲、能量等隨時間變化）輸出為 CSV 檔案供後處理。  
- `void resetAll();`  
  說明：重置所有 Sensor 與引擎的狀態，以便進行下一個模擬試跑。  

---

#### `Master.h`  
用途：定義 GUI 主視窗類別（通常為 `TForm` 派生），在 GUI 模式下提供使用者介面。  
主要內容：VCL 表單成員變數（例如按鈕、滑桿、圖表元件）、控制 `Engine` 的成員實體、與各種對話框的呼叫。  
主要函式：  
- `void __fastcall btnStartClick(TObject *Sender);`  
  說明：使用者按下「Start 模擬」按鈕後呼叫，讀取 UI 上設定參數，呼叫 Engine 的 `initialize()` 與 `run()`。  
- `void __fastcall btnStopClick(TObject *Sender);`  
  說明：使用者按下「Stop」停止模擬，可能要求 Engine 停止、或視窗禁止再進行迴圈。  
- `void updateUI();`  
  說明：模擬進行中被定期呼叫（例如 timer）以更新畫面上各 Sensor 的狀態、圖表、進度條。  
- `void displaySummary(const std::string &text);`  
  說明：在模擬結束或中斷後顯示統計摘要結果。  

---

#### `Master.cpp`  
用途：實作 `Master.h` 中的所有函式。包含建立與配置 UI 控件、調用各個設定對話框（如參數設定、排程方法選擇、分佈選擇等）、執行 Engine 並顯示結果。  
主要函式補充說明：  
- `void __fastcall FormCreate(TObject *Sender);`  
  說明：主視窗創建時初始化 UI 控件與預設值。  
- `void __fastcall btnSettingsClick(TObject *Sender);`  
  說明：使用者按下「設定」按鈕後彈出 `ParaSetterDialog` 讓使用者輸入模擬參數。  
- `void __fastcall btnChartClick(TObject *Sender);`  
  說明：使用者按下「顯示圖表」按鈕後，呼叫 `CharterDialog` 顯示統計圖形。  

---

#### `HeadlessBridge.h`  
用途：支援命令列 (CLI) 模式的橋接層，將 GUI 模擬器變為可被自動化批次呼叫的模式。  
主要內容：解析命令列參數、設定 Engine、選擇輸出檔案、避免顯示 GUI 介面。  
主要函式：  
- `bool parseArgs(int argc, char* argv[]);`  
  說明：解析所有 CLI 參數（如 `--T`, `--seed`, `--N`, `--lambda`, `--mu`, `--C`, `--e`, `--out` 等），並將設定填入內部變數。  
- `int runHeadless();`  
  說明：呼叫 Engine 的 `initialize()` 與 `run()`, 最後呼叫 `outputCSV()`。返回狀態碼（0 成功、其他為錯誤）。  
- `void printUsage();`  
  說明：若參數錯誤或使用者輸入 `--help`，印出 CLI 用法說明。  

---

#### `HeadlessBridge.cpp`  
用途：實作 `HeadlessBridge.h` 中的函式。會與 Engine 互動但不顯示任何視窗，完全透過命令列操作。  
主要函式補充說明：  
- `int main(int argc, char* argv[]);`  
  說明：程式入口；呼叫 `parseArgs()` → 若成功則呼叫 `runHeadless()` → 結束。  
- `void writeLog(const std::string &msg);`  
  說明：寫入 headless 模式專用的 log 檔案或標準輸出。  

---

#### `RandomVar.h`  
用途：定義亂數分佈工具類別／函式，用於各種隨機抽樣（如 Exponential, Uniform, Normal）。  
主要內容：函式宣告或 class 模板，例如 `double sampleExponential(double lambda)`, `double sampleUniform(double a, double b)`, `double sampleNormal(double mu, double sigma)`.  
主要函式：  
- `double sampleExponential(double lambda);`  
  說明：從指數分佈抽樣。  
- `double sampleUniform(double a, double b);`  
  說明：從均勻分佈 [a,b] 抽樣。  
- `double sampleNormal(double mu, double sigma);`  
  說明：從常態分佈 (mu, sigma) 抽樣。  

---

#### `distributionGenerator.h`  
用途：協助產生大量隨機樣本或直方圖用數據，常用於分佈檢查或 GUI 繪圖。  
主要內容：例如 `void generateSamples(int count, DistributionType type, double param1, double param2, std::vector<double> &out)`.  
主要函式：  
- `void generateSamples(int count, DistType type, double p1, double p2, std::vector<double> &out);`  
  說明：依參數產生 `count` 個樣本，儲入 `out`。  
- `void printHistogram(const std::vector<double> &data, int bins);`  
  說明：將樣本 `data` 分成 `bins` 組並印出文字直方圖（GUI 模式可能改成圖形）。  

---

#### `Person.h`  
用途：儲存單一「實體」（可能是封包／任務／顧客）在系統中的記錄，例如到達時間、開始服務時間、完成服務時間、延遲時間。  
主要內容：`struct Person { double arrivalTime; double serviceStartTime; double departureTime; … };`  
主要函式：  
- `double waitingTime() const;`  
  說明：回傳從到達至開始服務的等待時間。  
- `double totalTime() const;`  
  說明：回傳從到達至離開的總逗留時間。  

---

#### `Person.cpp`  
用途：若有實作函式，則為 `Person.h` 中成員函式的實作（如 `waitingTime()`, `totalTime()`）。若只是資料結構，則此檔案可能極簡。  

---

### Python 腳本說明  
- `csv2human.py`：將 headless 模式輸出的 raw CSV 檔案轉換為人類較易讀或 Excel 友善的格式。  
- `pyReadCSV.py`：讀取模擬結果 CSV，用 `pandas`、`matplotlib` 等做探索性資料分析或畫圖。  
- `requirements.txt`：列出 Python 依賴套件（如 `pandas`, `numpy`, `matplotlib` 等）。

---

## 使用方式（簡述）

### GUI 模式  
1. 用 C++Builder 開啟 `Project2.cbproj`。  
2. 編譯並執行。  
3. 在主視窗中：  
   - 設定節點數 N、到達率 λ、服務率 μ、電池容量 C、補充能量 e、傳送門檻 r_tx 等。  
   - 按下「Start 模擬」。  
   - 使用「顯示圖表」來檢視結果。

### Headless CLI 模式（Windows）  
在 `Win32/Debug` 目錄執行：

```bash
.\Project2.exe --headless --T=1000000 --seed=12345 --N=10 --lambda=0.9 --mu=2.4 --C=2 --e=1.6 --out=output.csv --version=BaseStation
