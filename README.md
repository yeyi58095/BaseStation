# BaseStation — Wireless Powered Sensor Network Simulator

BaseStation 是一個以 **C++Builder (VCL)** 開發的模擬器，用來研究「無線供電感測網路 / 能量收集佇列系統」的行為。  
系統中有一個 HAP/Base Station，對多個 Sensor 進行充電與資料收發；每個 Sensor 具有：

- 資料封包佇列（Data Packets, DP）
- 整數化的能量單位（Energy Packets, EP）
- 到達時間 / 服務時間的隨機分佈
- 充電、傳送、丟棄等事件

本專案同時支援：

- **GUI 模式**：透過 VCL 視窗調整參數、觀察即時狀態
- **Headless CLI 模式**：從命令列執行大量模擬，輸出 CSV 給 Python 進行後處理與畫圖

---

## 專案結構

根據目前 repo 內容，所有程式檔都在 root 目錄下（另外有已編譯好的 `Win32/Debug` 執行檔）。  

- C++ / Header 檔案（核心邏輯 + GUI）
- `.dfm`：C++Builder 的表單版面配置
- Python 腳本：讀取 / 轉換 CSV
- `Win32/Debug/Project2.exe`：已編譯好的 Windows 執行檔

---

## 重要執行檔與專案檔

- `Project2.cbproj`  
  C++Builder 專案檔，開啟整個 GUI 專案用。

- `Project2.cpp`  
  **應用程式入口點**，負責初始化 VCL，建立主視窗（`Master` 或 `Unit5` 等）後進入 message loop。

- `Project2PCH1.h`  
  C++Builder 的 **precompiled header**，匯入常用 VCL / RTL 標頭，加速編譯用，沒有實際模擬邏輯。

- `Project2.res`, `Project2.stat`, `Project2.cbproj.local`  
  專案資源與 IDE 設定檔，包含圖示、版本資訊、IDE 個人化設定等。

- `Win32/Debug/Project2.exe`  
  已編譯好的 Windows 執行檔。  
  - 直接 double-click 可啟動 GUI 版模擬器  
  - 也可以從命令列搭配 `HeadlessBridge` 支援的參數執行 headless 模式

---

## 核心模擬邏輯（C++ / Header）

### `Sensor.h` / `Sensor.cpp`

**Sensor 節點模型**。每個感測器節點負責管理自身的封包佇列與能量狀態。主要功能包含：【從程式可見】  

- 內部 `struct Packet { id, st, needEP }`：  
  - `st`：該封包的服務時間（秒）  
  - `needEP`：該封包的傳送所需能量（EP 整數單位）  
- `std::deque<Packet> q`：封包佇列  
- 狀態：
  - `serving` / `servingId`：是否正在被 HAP 服務、目前傳送中的封包 ID  
  - `energy` / `E_cap`：目前 EP、電池容量  
  - `r_tx`：啟動傳送所需的門檻 EP  
  - `charge_rate`：充電速率（EP/sec）
- 分佈與參數：
  - `ITdistri, ITpara1, ITpara2`：到達時間（Inter-arrival Time）分佈與參數  
  - `STdistri, STpara1, STpara2`：服務時間（Service Time）分佈與參數  
- 主要方法：
  - `setIT() / setST()` / `setArrivalExp()` / `setServiceExp()`：設定分佈  
  - `sampleIT()` / `sampleST()`：抽樣到達 / 服務時間  
  - `enqueueArrival()`：產生一個新封包並放入佇列  
  - `canTransmit()`：檢查佇列與能量是否足以傳送  
  - `startTx()`：取出一個封包、扣除 EP、進入傳送中狀態，回傳 `st`  
  - `finishTx()`：完成傳送，重置 `serving` 狀態  
  - `addEnergy()` / `energyForSt()`：充電與計算傳送所需能量  
  - 佇列資訊輸出：`toString()`, `queueToStr()`  
  - 佇列限制 / 丟棄計數：`Qmax`, `drops` 與相關 setter

> 簡單來說：**`Sensor` 封裝了「封包佇列 + 能量模型 + 到達/服務時間分佈」的所有細節**，是整個系統中最關鍵的「節點」物件。

---

### `Engine.h` / `Engine.cpp`

**模擬引擎核心**。負責：

- 管理整個系統中的所有 `Sensor` 物件
- 維護 global simulation clock、事件迴圈
- 決定每一個 time step / event 要做的事情  
  - 充電（增加 sensor.energy）  
  - 觸發封包到達  
  - 檢查哪些 Sensor 符合傳送條件（`canTransmit()`）  
  - 依照排程策略決定服務對象
- 統計量計算：
  - 系統平均封包數量 `L`  
  - 平均等待時間 `W`  
  - 平均延遲、損失率、EP 平均值等
- 提供 API 給 GUI 和 Headless 模式：
  - 初始化參數、reset 模擬
  - run / step / stop
  - 回傳目前狀態與統計資訊

> 可以把 `Engine` 想成 **整個離散事件模擬器的「大腦」**，所有關於時間推進與事件觸發都在這裡處理。

---

### `Master.h` / `Master.cpp`

**主視窗（Main Form）邏輯**。在 GUI 模式下負責：

- 各種按鈕、選單與 UI 控制的事件處理
  - 開始 / 停止模擬
  - 開啟參數設定視窗
  - 切換繪圖 / Replay 畫面
- 持有一個 `Engine` 實例，並根據 UI 上的設定去呼叫 `Engine` API
- 讀寫使用者輸入的參數（N、λ、μ、C、e、充電模式、排程策略…）
- 顯示統計結果（例如平均延遲、封包數等）  

> 換句話說：`Master` 是整個 GUI 程式的「控制台」，把 UI 操作轉成對 `Engine` 的呼叫。

---

### `HeadlessBridge.h` / `HeadlessBridge.cpp`

**Headless 模式橋接層**。用來讓這個 GUI 專案可以變成：

```bash
Project2.exe --headless --T=1000000 --seed=12345 --N=1 --lambda=0.9 --mu=2.4 --C=2 --e=1.6 --out=result.csv --version=BaseStation
