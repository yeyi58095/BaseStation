//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <math.h> // 或 cmath
#include "Master.h"
#include "ReplayDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TReplay *Replay;
//---------------------------------------------------------------------------

#include <System.SysUtils.hpp>


#include <float.h>   // for _finite
// #include <math.h> // 若需要其它數學函式再加

// ---- helpers for bcc32 ----
static bool IsFinite(double x) {
    // _finite: 非 0 表示是有限數；0 表示 NaN/Inf
    return _finite(x) != 0;
}

static int myRound(double x) {
    // 四捨五入；同時處理負數
    return (int)(x >= 0.0 ? x + 0.5 : x - 0.5);
}

template<typename T>
static T myMax(T a, T b) { return (a > b) ? a : b; }





// ===== CSV 解析（無引號處理；你的 CSV 無引號）=====
static void splitCSV(const AnsiString& line, std::vector<AnsiString>& out) {
    out.clear();
    int n = line.Length();
    int i = 1;
    int start = 1;
    while (i <= n) {
        if (line[i] == ',') {
            out.push_back(line.SubString(start, i - start));
            start = i + 1;
        }
        ++i;
    }
    if (start <= n+1)
        out.push_back(line.SubString(start, n - start + 1));
}

enum CsvEvKind {
    CEV_ARR, CEV_STX, CEV_TTK, CEV_ETX,
    CEV_DROP, CEV_CST, CEV_CEND, CEV_STAT, CEV_UNKNOWN
};

static CsvEvKind parseKind(const AnsiString& s) {
    if (s == "ARR")  return CEV_ARR;
    if (s == "STX")  return CEV_STX;
    if (s == "TTK")  return CEV_TTK;
    if (s == "ETX")  return CEV_ETX;
    if (s == "DROP") return CEV_DROP;
    if (s == "CST")  return CEV_CST;
    if (s == "CEND") return CEV_CEND;
    if (s == "STAT") return CEV_STAT;
    return CEV_UNKNOWN;
}

static AnsiString kindToText(CsvEvKind k) {
    switch (k) {
        case CEV_ARR:  return "ARR";
        case CEV_STX:  return "START_TX";
        case CEV_TTK:  return "TX_TICK";
        case CEV_ETX:  return "END_TX";
        case CEV_DROP: return "DROP";
        case CEV_CST:  return "CHARGE_START";
        case CEV_CEND: return "CHARGE_END";
        case CEV_STAT: return "STAT";
        default:       return "EVENT";
    }
}

static bool FileExistsMulti(const AnsiString& p1, const AnsiString& p2, AnsiString& outPath) {
    if (FileExists(p1)) { outPath = p1; return true; }
    if (FileExists(p2)) { outPath = p2; return true; }
    return false;
}


using namespace sim;
extern Master master;
__fastcall TReplay::TReplay(TComponent* Owner)
	: TForm(Owner)
{
	runTimer   = NULL;   // 用 nullptr 比 NULL 好
	isRunning  = false;
	isStepping = false;
	stepsPerSec= 5.0;
	this->edSpeed->Text = 5.0;


    curIdx = -1;
    lastDrawnIdx = -1;
    epAverage = true;
    hasEvents = false;
    // 計時器成員在需要時建立
}

void TReplay::InitChart()
{
    // 左軸：Queue，右軸：EP
    ReplayChart->LeftAxis->Title->Caption = "Queue size";
    ReplayChart->RightAxis->Visible = true;
    ReplayChart->RightAxis->Title->Caption = "Energy (EP)";

    SeriesQ->Stairs = true;
    SeriesMeanQ->Stairs = true;
    SeriesEP->Stairs = true;

    SeriesQ->VertAxis = aLeftAxis;
    SeriesMeanQ->VertAxis = aLeftAxis;
    SeriesEP->VertAxis = aRightAxis;

    ReplayChart->Legend->Visible = true;
    SeriesQ->Title = "Q (current)";
    SeriesMeanQ->Title = "Q (mean)";
    SeriesEP->Title = epAverage ? "EP (avg)" : "EP (sum)";

    ReplayChart->BottomAxis->Automatic = true;
    ReplayChart->LeftAxis->Automatic   = true;
    ReplayChart->RightAxis->Automatic  = true;
    ReplayChart->TopAxis->Automatic    = true;

    SeriesQ->Clear();
    SeriesMeanQ->Clear();
    SeriesEP->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TReplay::FormShow(TObject *Sender)
{
    // 預設用 EP 平均；你要改成 sum 就傳 false
    BeginReplayAll(true);

    // 顯示標題
	ReplayChart->Title->Caption = epAverage ? "All sensors (EP avg)" : "All sensors (EP sum)";

    // 初始 UI
    UpdateRunUI();
}
//---------------------------------------------------------------------------
void TReplay::BeginReplayAll(bool useAvg)
{
    nextEventIdx = 0;
    epAverage = useAvg;
    InitChart();

    curIdx = 0;
    lastDrawnIdx = -1;
    S.clear();

    BuildEventsCacheFromDump();
    if (!hasEvents) BuildEventsFromCSV();

    // ★ 建立事件→幀的 bucket
    frameEv.clear();
    evUsed.clear();
    if (hasEvents) {
        const std::vector<double>& tA = master.traceT_all;
        int K = (int)tA.size();
        frameEv.resize(K);
        evUsed.resize((int)evtT.size(), 0);

        if (K > 0) {
            int i, k = 0;
            for (i = 0; i < (int)evtT.size(); ++i) {
                double te = evtT[i];
                // 找到最小的 k 使 te <= tA[k]
                while (k < K && te - tA[k] > 1e-9) ++k;
                if (k >= K) k = K - 1;      // 太晚的事件就塞到最後一幀
                if (k < 0)  k = 0;
                frameEv[k].push_back(i);
            }
        }
    }

    DrawToIndex(0);
    UpdateRunUI();
}

void __fastcall TReplay::btnStepClick(TObject *Sender)
{
    // 若正在播放 → 先停
    if (isRunning) {
        StopRun();
    }

    DoOneStep(false); // 顯示模式
    UpdateRunUI();
}

// ====== Start/Stop 單鍵切換 ======
void __fastcall TReplay::btnStartClick(TObject* Sender)
{
	if (isRunning) {
        StopRun();
    } else {
		StartRun();
    }
}

void TReplay::EnsureRunTimer() {
    if (runTimer) return;
    runTimer = new TTimer(this);
    runTimer->Enabled = false;
    runTimer->Interval = 200;              // 預設值，StartRun 會重設
    runTimer->OnTimer = OnRunTimer;
}

static double ParseStepsPerSecFromEdit(TEdit* ed, double fallback/*=5.0*/) {
    double v = StrToFloatDef(ed->Text.Trim(), fallback);
    if (!IsFinite(v)) v = fallback; // 取代 std::isfinite

    if (v < 0.1)  v = 0.1;   // 下限，避免太慢
    if (v > 200.) v = 200.;  // 上限，避免 CPU 飆高
    return v;
}



void TReplay::StartRun() {
	const std::vector<double>& tAll = master.traceT_all;
    if (tAll.empty()) {
        lblInfo->Caption = "No trace data. Run simulation first.";
        return;
    }
    if (curIdx < 0) curIdx = 0;

	stepsPerSec = ParseStepsPerSecFromEdit(edSpeed, stepsPerSec);
	EnsureRunTimer();
	int interval = myMax(1, myRound(1000.0 / stepsPerSec));
runTimer->Interval = interval;
;
	isRunning = true;
    runTimer->Enabled = true;
    UpdateRunUI();
}

void TReplay::StopRun() {
    if (runTimer) runTimer->Enabled = false;
    isRunning = false;
    UpdateRunUI();
}

void TReplay::UpdateRunUI() {
    // 切換 Start/Stop 文字與可按狀態
    if (isRunning) {
        btnStart->Caption = "Stop";
    } else {
        btnStart->Caption = "Start";
    }
    // 邏輯：只要還沒播到尾端，就允許 Start / Step
    bool hasMore = !master.traceT_all.empty() && curIdx < (int)master.traceT_all.size();
    btnStart->Enabled = hasMore || isRunning;
    btnStep->Enabled  = hasMore;
    edSpeed->Enabled  = !isRunning;
}

// 封裝「一次單步」的行為，供 Step 按鈕與 Timer 共用
bool TReplay::DoOneStep(bool silent)
{
    const std::vector<double>& tAll = master.traceT_all;
    if (tAll.empty()) {
        lblInfo->Caption = "No trace data. Run simulation first.";
        return false;
    }
    if (curIdx < 0) curIdx = 0;

    // 為避免極端情況，設個保護上限
    int guard = 0, GUARD_MAX = (int)tAll.size() + (int)evtT.size() + 8;

    while (guard++ < GUARD_MAX) {
        // 先把圖畫到目前幀
        DrawToIndex(curIdx);

        // 嘗試在「目前幀」吐一筆事件；成功就結束這次單步
        if (EmitOneEventAtCurrentFrame()) {
            return true;
        }

        // 本幀沒有事件或都吐完 → 若還有下一幀就前進，繼續找直到吐出一筆事件或到結尾
        if (curIdx < (int)tAll.size() - 1) {
            curIdx++;
            continue;
        }

        // 走到尾了也沒事件可吐
        lblInfo->Caption = lblInfo->Caption + "\n(End)";
        return false;
    }

    // 安全保護：理論上不會到這
    lblInfo->Caption = lblInfo->Caption + "\n(guard hit)";
    return false;
}

void __fastcall TReplay::OnRunTimer(TObject* Sender)
{
    if (isStepping) return;     // 防 reentrancy
    isStepping = true;
    try {
        // 播放時用 silent 模式減少重繪成本（如需，也可改成 false）
        bool ok = DoOneStep(/*silent=*/true);
        if (!ok) {
            // 到尾或 guard hit → 停止播放
            StopRun();
        }
    } __finally {
        isStepping = false;
    }
}

// ------------------ 既有繪圖與事件處理 ------------------

void TReplay::DrawToIndex(int k)
{
    const std::vector<double>& t  = master.traceT_all;
    const std::vector<double>& q  = master.traceQ_all;
    const std::vector<double>& m  = master.traceMeanQ_all;
    const std::vector<double>& eA = master.traceEavg_all;
    const std::vector<double>& eS = master.traceE_all;

    if (t.empty()) return;
    if (k < 0) k = 0;
    if (k >= (int)t.size()) k = (int)t.size() - 1;

    // 為了避免每幀 O(N) 清空重畫，前進時只補新點
    if (lastDrawnIdx < 0) {
        // 第一次：把 0..k 都畫上去
        ReplayChart->AutoRepaint = false;
        SeriesQ->Clear();
        SeriesMeanQ->Clear();
        SeriesEP->Clear();

        int i;
        for (i = 0; i <= k; ++i) {
            AppendPoint(i);
        }
        ReplayChart->AutoRepaint = true;
        ReplayChart->Repaint();
    } else if (k > lastDrawnIdx) {
        // 往前一步：只補 k 這一點
        AppendPoint(k);
    } else if (k < lastDrawnIdx) {
        // （理論上本流程只有往前；若要支援往回，這裡應重畫）
        ReplayChart->AutoRepaint = false;
        SeriesQ->Clear();
        SeriesMeanQ->Clear();
        SeriesEP->Clear();
        int i2;
        for (i2 = 0; i2 <= k; ++i2) AppendPoint(i2);
        ReplayChart->AutoRepaint = true;
        ReplayChart->Repaint();
    }

    lastDrawnIdx = k;

    // 更新上方 Label（播放時 silent 依然維持這個 Head 也沒關係）
    UpdateLabelForTime(t[k], k);
}

void TReplay::AppendPoint(int i)
{
    const std::vector<double>& t  = master.traceT_all;
    const std::vector<double>& q  = master.traceQ_all;
    const std::vector<double>& m  = master.traceMeanQ_all;
    const std::vector<double>& eA = master.traceEavg_all;
    const std::vector<double>& eS = master.traceE_all;

    double epVal = epAverage ? ( (i < (int)eA.size()) ? eA[i] : 0.0 )
                             : ( (i < (int)eS.size()) ? eS[i] : 0.0 );

    SeriesQ->AddXY(t[i], q[i], "", clTeeColor);
    SeriesMeanQ->AddXY(t[i], m[i], "", clTeeColor);
    SeriesEP->AddXY(t[i], epVal, "", clTeeColor);
}

void TReplay::UpdateLabelForTime(double tNow, int k)
{
    const std::vector<double>& q  = master.traceQ_all;
    const std::vector<double>& m  = master.traceMeanQ_all;
    const std::vector<double>& eA = master.traceEavg_all;
    const std::vector<double>& eS = master.traceE_all;

    double epVal = epAverage ? ((k < (int)eA.size()) ? eA[k] : 0.0)
                             : ((k < (int)eS.size()) ? eS[k] : 0.0);

    lblInfo->Caption =
        "t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
        "   frame=" + IntToStr(k) +
        "\nQ=" + FloatToStrF(q[k], ffFixed, 7, 3) +
        "  MeanQ=" + FloatToStrF(m[k], ffFixed, 7, 3) +
        (epAverage ? "  EP_avg=" : "  EP_sum=") +
        FloatToStrF(epVal, ffFixed, 7, 3);
}

void TReplay::BuildEventsCacheFromDump()
{
    evtT.clear(); evtKind.clear(); evtSid.clear(); evtPid.clear();
    evtQ.clear(); evtEP.clear(); evtX1.clear(); evtX2.clear();
    evtMsg.clear();
    hasEvents = false;

    AnsiString dump = master.dumpLogWithSummary();
    TStringList* sl = new TStringList();
    sl->Text = dump;

    for (int i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i].Trim();
        if (line.IsEmpty()) continue;

        // 只吃以 "t=" 開頭的 timeline 行；# Summary / STATE / STAT 都跳過
        if (line.Pos("t=") != 1) continue;
        if (line.Pos(" STATE ") > 0) continue;  // "STATE after ..." 不吃
        if (line.Pos(" STAT") > 0)  continue;   // CSV 的 STAT 也不吃

        // 事件時間
        double t = ParseLeadingTime(line);
        if (t < 0) continue;

        // 判斷事件種類
        AnsiString typ = DetectEventType(line);
        int kind = 8;
        if (typ == "ARR") kind = 0;
        else if (typ == "START_TX") kind = 1;
        else if (typ == "TX_TICK")  kind = 2;
        else if (typ == "END_TX")   kind = 3;
        else if (typ == "DROP")     kind = 4;
        else if (typ == "CHARGE_START") kind = 5;
        else if (typ == "CHARGE_END")   kind = 6;
        else continue; // 其它不吃

        // 通用欄位
        int sid = ParseIntAfter(line, "sensor=", -1);
        int pid = ParseIntAfter(line, "pkg=",    -1);
        int q   = -1;
        int ep  = -1;
        double x1 = 0.0, x2 = 0.0;

        // 依事件類型補齊欄位
        if (kind == 0) { // ARR: "... ARRIVAL      Q=1  EP=4"
            q  = ParseIntAfter(line, "Q=",  -1);
            ep = ParseIntAfter(line, "EP=", -1);
        }
        else if (kind == 1) { // START_TX: "st=0.466  EP_before=5  cost=1"
            x1 = StrToFloatDef(AnsiString(ParseIntAfter(line, "st=", -99999)), -99999); // 先保留數值，下一行補更準確
            // st 可能是小數 → 用 float 解析
            int p = line.Pos("st=");
            if (p > 0) {
                int s = p + 3, e = s;
                while (e <= line.Length()) {
                    WideChar ch = line[e];
                    if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') e++;
                    else break;
                }
                x1 = StrToFloatDef(line.SubString(s, e - s), 0.0);    // st
            }
            ep = ParseIntAfter(line, "EP_before=", -1);               // 事件前 EP
            x2 = ParseIntAfter(line, "cost=", 0);                     // cost (整數)
        }
        else if (kind == 2) { // TX_TICK: "... TX_TICK ... EP=5  remain=..."
            ep = ParseIntAfter(line, "EP=", -1);
            x1 = ParseIntAfter(line, "remain=", 0);
        }
        else if (kind == 3) { // END_TX: "... END_TX ... Q=0  EP=4"
            q  = ParseIntAfter(line, "Q=",  -1);
            ep = ParseIntAfter(line, "EP=", -1);
        }
        else if (kind == 4) { // DROP: "... DROP   Q=1/10  EP=3"（Human 版可能只看到 EP）
            ep = ParseIntAfter(line, "EP=", -1);
        }
        else if (kind == 5) { // CHARGE_START: "... CHARGE_START need=100  active=1/inf"
            x1 = ParseIntAfter(line, "need=", 0);
            // active/cap 目前 UI 不用，略過
        }
        else if (kind == 6) { // CHARGE_END: "... CHARGE_END   +1EP  EP=5"
            ep = ParseIntAfter(line, "EP=", -1);
            int pplus = line.Pos("+");
            if (pplus > 0) {
                int s = pplus + 1, e = s;
                while (e <= line.Length() && line[e] >= '0' && line[e] <= '9') e++;
                x1 = StrToIntDef(line.SubString(s, e - s), 0); // 本次充了幾格
            }
        }

        // 沒抓到 sid/pid 的 ARR/START/END → 跳過避免出現 "All/HAP — ARR (pkg=-1)"
        if ((kind == 0 || kind == 1 || kind == 3) && sid < 0)
            continue;

        // 寫入事件陣列
        evtT.push_back(t);
        evtKind.push_back(kind);
        evtSid.push_back(sid);
        evtPid.push_back(pid);
        evtQ.push_back(q);
        evtEP.push_back(ep);
        evtX1.push_back(x1);
        evtX2.push_back(x2);

        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid + 1)) : AnsiString("All/HAP");
        AnsiString msg = who + " — " + typ;
        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";
        evtMsg.push_back(msg);
    }
    delete sl;

    // Human dump 本來就按時間輸出，保險起見做一次穩定插入排序
    int n = (int)evtT.size();
    for (int i = 1; i < n; ++i) {
        double   t = evtT[i];   int k = evtKind[i], sid = evtSid[i], pid = evtPid[i];
        int      q = evtQ[i],   ep = evtEP[i];
        double   x1 = evtX1[i], x2 = evtX2[i];
        AnsiString m = evtMsg[i];
        int j = i - 1;
        while (j >= 0 && evtT[j] > t) {
            evtT[j+1]=evtT[j]; evtKind[j+1]=evtKind[j]; evtSid[j+1]=evtSid[j];
            evtPid[j+1]=evtPid[j]; evtQ[j+1]=evtQ[j]; evtEP[j+1]=evtEP[j];
            evtX1[j+1]=evtX1[j]; evtX2[j+1]=evtX2[j]; evtMsg[j+1]=evtMsg[j];
            --j;
        }
        evtT[j+1]=t; evtKind[j+1]=k; evtSid[j+1]=sid; evtPid[j+1]=pid;
        evtQ[j+1]=q; evtEP[j+1]=ep; evtX1[j+1]=x1; evtX2[j+1]=x2; evtMsg[j+1]=m;
    }

    hasEvents = !evtT.empty();
    nextEventIdx = 0;
    S.clear();
}

void TReplay::BuildEventsFromCSV()
{
    evtT.clear(); evtKind.clear(); evtSid.clear(); evtPid.clear();
    evtQ.clear(); evtEP.clear(); evtX1.clear(); evtX2.clear();
    evtMsg.clear();

    hasEvents = false;

    AnsiString csvPath = "run_log.csv";
    if (!FileExists(csvPath)) csvPath = "Win32\\Debug\\run_log.csv";
    if (!FileExists(csvPath)) return;

    TStringList* sl = new TStringList();
    try { sl->LoadFromFile(csvPath); } catch(...) { delete sl; return; }

    std::vector<AnsiString> cols;
    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i].Trim();
        if (line.IsEmpty()) continue;
        if (line.Pos("t,") == 1 || line.Pos("t,event") == 1) continue;

        cols.clear();
        splitCSV(line, cols);
        if ((int)cols.size() < 8) continue;

        double t   = StrToFloatDef(cols[0], 0.0);
        CsvEvKind k= parseKind(cols[1]);
        if (k == CEV_STAT) continue; // 不吃 STAT

        int kind = 8; // UNKNOWN -> 我們自己的編碼
        if (k == CEV_ARR)  kind = 0;
        if (k == CEV_STX)  kind = 1;
        if (k == CEV_TTK)  kind = 2;
        if (k == CEV_ETX)  kind = 3;
        if (k == CEV_DROP) kind = 4;
        if (k == CEV_CST)  kind = 5;
        if (k == CEV_CEND) kind = 6;

        int sid    = StrToIntDef(cols[2], -1);
        int pid    = StrToIntDef(cols[3], -1);
        int q      = StrToIntDef(cols[4], -1);
        int ep     = StrToIntDef(cols[5], -1);
        double x1  = StrToFloatDef(cols[6], 0.0);
        double x2  = StrToFloatDef(cols[7], 0.0);

        evtT.push_back(t);
        evtKind.push_back(kind);
        evtSid.push_back(sid);
        evtPid.push_back(pid);
        evtQ.push_back(q);
        evtEP.push_back(ep);
        evtX1.push_back(x1);
        evtX2.push_back(x2);

        // 預設訊息（非必要；真的要用狀態字串會重新組）
        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid+1)) : AnsiString("All/HAP");
        AnsiString typ = kindToText(k);
        AnsiString msg = who + " — " + typ;
        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";
        evtMsg.push_back(msg);
    }
    delete sl;

    // CSV 可能不是穩定時間序，做一下穩定排序（無 lambda 版本）
    int n = (int)evtT.size(), ii, jj;
    for (ii = 1; ii < n; ++ii) {
        double   tkey = evtT[ii];
        int      kkey = evtKind[ii], sid = evtSid[ii], pid = evtPid[ii];
        int      q = evtQ[ii], ep = evtEP[ii];
        double   x1 = evtX1[ii], x2 = evtX2[ii];
        AnsiString m = evtMsg[ii];
        jj = ii - 1;
        while (jj >= 0 && evtT[jj] > tkey) {
            evtT[jj+1]=evtT[jj]; evtKind[jj+1]=evtKind[jj]; evtSid[jj+1]=evtSid[jj];
            evtPid[jj+1]=evtPid[jj]; evtQ[jj+1]=evtQ[jj]; evtEP[jj+1]=evtEP[jj];
            evtX1[jj+1]=evtX1[jj]; evtX2[jj+1]=evtX2[jj]; evtMsg[jj+1]=evtMsg[jj];
            --jj;
        }
        evtT[jj+1]=tkey; evtKind[jj+1]=kkey; evtSid[jj+1]=sid; evtPid[jj+1]=pid;
        evtQ[jj+1]=q; evtEP[jj+1]=ep; evtX1[jj+1]=x1; evtX2[jj+1]=x2; evtMsg[jj+1]=m;
    }

    hasEvents = !evtT.empty();
    nextEventIdx = 0;
    S.clear();
}


double TReplay::ParseLeadingTime(const AnsiString& line)
{
    int p = line.Pos("t=");
    if (p <= 0) return -1.0;
    int s = p + 2;
    int len = line.Length();
    int e = s;
    while (e <= len) {
        WideChar ch = line[e];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') break;
        e++;
    }
    AnsiString num = line.SubString(s, e - s);
    return StrToFloatDef(num, -1.0);
}

int TReplay::ParseIntAfter(const AnsiString& line, const AnsiString& key, int defVal)
{
    int p = line.Pos(key);
    if (p <= 0) return defVal;
    int s = p + key.Length();
    int len = line.Length();
    // 跳過空白
    while (s <= len) {
        WideChar ch = line[s];
        if (ch != ' ' && ch != '\t' && ch != '=') break;
        s++;
    }
    int e = s;
    while (e <= len) {
        WideChar ch = line[e];
        if (ch < '0' || ch > '9') break;
        e++;
    }
    if (e <= s) return defVal;
    AnsiString num = line.SubString(s, e - s);
    return StrToIntDef(num, defVal);
}

AnsiString TReplay::DetectEventType(const AnsiString& line)
{
    // 依你的 Master::log*() 訊息關鍵詞判斷
    if (line.Pos("ARRIVAL") > 0)       return "ARR";
    if (line.Pos("START_TX") > 0)      return "START_TX";
    if (line.Pos("TX_TICK") > 0)       return "TX_TICK";
    if (line.Pos("END_TX") > 0)        return "END_TX";
    if (line.Pos("DROP") > 0)          return "DROP";
    if (line.Pos("CHARGE_START") > 0)  return "CHARGE_START";
    if (line.Pos("CHARGE_END") > 0)    return "CHARGE_END";
    if (line.Pos("STATE") > 0)         return "STATE";
    if (line.Pos("STAT") > 0)          return "STAT";  // 若未來也支援 CSV->人讀
    return "EVENT";
}

int TReplay::FindLastEventIndexLE(double tNow) const
{
    if (events.empty()) return -1;
    // 線性找就夠了（事件不多）；要加速可做二分
    int i;
    int pick = -1;
    for (i = 0; i < (int)events.size(); ++i) {
        if (events[i].t <= tNow + 1e-9) pick = i;
        else break;
    }
    return pick;
}

void TReplay::SetLabelFromEvent(const ReplayEvent& ev, double tNow, int frameIdx)
{
    // 統一在第一行顯示回放狀態
    AnsiString head = "t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
                      "   frame=" + IntToStr(frameIdx);

    // 第二行顯示「第幾個 sensor、事件是什麼」
    // 若 sid = -1，顯示 All/HAP/STATE 類
    AnsiString who = (ev.sid >= 0) ? (AnsiString("Sensor ") + IntToStr(ev.sid + 1))
                                   : AnsiString("All/HAP");
    AnsiString line2 = who + " — " + ev.type;

    // 適度加上 pid/短欄位
    if (ev.pid >= 0) line2 += "  (pkg=" + IntToStr(ev.pid) + ")";

    // 也可以加上你原始行的一小段，幫助除錯
    // 例如保留到 80 字元
    AnsiString shortTxt = ev.text;
    if (shortTxt.Length() > 80) shortTxt = shortTxt.SubString(1, 80) + "...";

    lblInfo->Caption = head + "\n" + line2 + "\n" + shortTxt;
}


bool TReplay::FindTimeGroup(double tNow, int& lo, int& hi) const
{
    lo = -1; hi = -1;
    if (!hasEvents || evtT.empty()) return false;

    // 先找到最後一筆 <= tNow
    int j;
    int lastLE = -1;
    for (j = (int)evtT.size() - 1; j >= 0; --j) {
        if (evtT[j] <= tNow + 1e-9) { lastLE = j; break; }
    }
    if (lastLE < 0) return false;

    // 這一筆的時間就是群組時間（可能 < tNow；若 < tNow，代表 tNow 沒事件，就回那個時間的最後群）
    double T = evtT[lastLE];

    // 往左找到同一個時間的起點
    int s = lastLE;
    while (s - 1 >= 0 && fabs(evtT[s - 1] - T) <= 1e-9) --s;

    // 往右找到同一個時間的終點
    int e = lastLE;
    while (e + 1 < (int)evtT.size() && fabs(evtT[e + 1] - T) <= 1e-9) ++e;

    lo = s; hi = e;
    return true;
}


bool TReplay::FindTimeGroupByIndex(int anchor, int& lo, int& hi) const
{
    lo = -1; hi = -1;
    if (!hasEvents || evtT.empty()) return false;
    if (anchor < 0 || anchor >= (int)evtT.size()) return false;

    double T = evtT[anchor];

    int s = anchor;
    while (s - 1 >= 0 && fabs(evtT[s - 1] - T) <= 1e-9) --s;

    int e = anchor;
    while (e + 1 < (int)evtT.size() && fabs(evtT[e + 1] - T) <= 1e-9) ++e;

    lo = s; hi = e;
    return true;
}

AnsiString TReplay::ApplyAndFormatEvent(int idx)
{
    int kind = evtKind[idx];
    int sid  = evtSid[idx];
    int pid  = evtPid[idx];
    int qcol = evtQ[idx];     // 事件後的 queue size（CSV 會有，Human 沒有）
    int epcol= evtEP[idx];
    double x1 = evtX1[idx];
    double x2 = evtX2[idx];

    SidState &st = S[sid];
    AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid+1)) : AnsiString("All/HAP");
    AnsiString line;

    switch (kind) {
        case 0: // ARR
            if (pid >= 0) st.q.push_back(pid);
            if (epcol >= 0) st.ep = epcol;
            line = who + " — ARR  (pkg=" + IntToStr(pid) + ")";
            break;

        case 1: // START_TX
            // 從 queue 拿掉要服務的那顆
            if (!st.q.empty() && pid >= 0 && st.q.front() == pid) st.q.erase(st.q.begin());
            st.servingPid = pid;
            line = who + " — START_TX  (pkg=" + IntToStr(pid) + ")"
                 + "  st=" + FloatToStrF(x1, ffFixed, 7, 3)
                 + "  EP_before=" + IntToStr(epcol)
                 + "  cost=" + IntToStr((int)x2);
            break;

        case 2: // TX_TICK
            if (epcol >= 0) st.ep = epcol;
            line = who + " — TX_TICK  (pkg=" + IntToStr(st.servingPid) + ")"
                 + "  EP=" + IntToStr(st.ep);
            break;

        case 3: // END_TX
            st.servingPid = -1;
            if (epcol >= 0) st.ep = epcol;
            line = who + " — END_TX  (pkg=" + IntToStr(pid) + ")";
            break;

        case 4: // DROP
            if (epcol >= 0) st.ep = epcol;
            line = who + " — DROP";
            break;

        case 5: // CHARGE_START
            line = who + " — CHARGE_START";
            break;

        case 6: // CHARGE_END
            if (epcol >= 0) st.ep = epcol;
            line = who + " — CHARGE_END  EP=" + IntToStr(st.ep);
            break;

        default:
            line = who + " — EVENT";
            break;
    }

    // ★ Q 對齊 traceQ_all：只算等待佇列，不含 in-service
    int qnow = (qcol >= 0) ? qcol : (int)st.q.size();

    // ★ EP：若 CSV 有 ep 欄位就用，否則用我們維持的狀態
    int epnow = (epcol >= 0) ? epcol : st.ep;

    line += "   |  Q=" + IntToStr(qnow) + "  EP=" + IntToStr(epnow);
    return line;
}


bool TReplay::EmitOneEventAtCurrentFrame()
{
    if (!hasEvents || curIdx < 0 || curIdx >= (int)frameEv.size())
        return false;

    std::vector<int>& bucket = frameEv[curIdx];

    for (int p = 0; p < (int)bucket.size(); ++p) {
        int evIdx = bucket[p];
        if (!evUsed[evIdx]) {
            AnsiString line = ApplyAndFormatEvent(evIdx);
            evUsed[evIdx] = 1;

            // ★ 用事件時間，而不是幀時間
            double te = evtT[evIdx];

            int sid     = evtSid[evIdx];
            int qAfter  = (evtQ[evIdx]  >= 0) ? evtQ[evIdx]  : (int)S[sid].q.size();
            int epAfter = (evtEP[evIdx] >= 0) ? evtEP[evIdx] : S[sid].ep;

            // MeanQ 取「事件當下之前的最近一個 trace 點」來近似
            int kForMean = curIdx;
            const std::vector<double>& tAll = master.traceT_all;
            while (kForMean > 0 && tAll[kForMean] > te + 1e-9) kForMean--;
            double meanQ = master.traceMeanQ_all.empty() ? 0.0
                             : master.traceMeanQ_all[kForMean];

            // EP 標頭：單 sensor 直接用事件後 EP
            double epHeader;
            int N = (master.sensors ? (int)master.sensors->size() : 1);
            if (N == 1 && sid >= 0) {
                epHeader = (double)epAfter;
            } else {
                epHeader = epAverage
                    ? ((curIdx < (int)master.traceEavg_all.size()) ? master.traceEavg_all[curIdx] : 0.0)
                    : ((curIdx < (int)master.traceE_all.size())     ? master.traceE_all[curIdx]     : 0.0);
            }

            // ★ 標頭用事件時間 te
            AnsiString header =
                "t=" + FloatToStrF(te, ffFixed, 12, 6) +
                "   frame=" + IntToStr(curIdx) +
                "\nQ=" + FloatToStrF(qAfter, ffFixed, 12, 6) +
                "  MeanQ=" + FloatToStrF(meanQ,  ffFixed, 12, 6) +
                (epAverage ? "  EP_avg=" : "  EP_sum=") +
                FloatToStrF(epHeader, ffFixed, 12, 6);

            lblInfo->Caption = header + "\n" + line;

            // 若你有右側事件清單，把這行也改成 te：
            // AppendEventLine( FloatToStrF(te, ffFixed, 12, 6) + "  " + line, color );

            return true;
        }
    }
    return false;
}


