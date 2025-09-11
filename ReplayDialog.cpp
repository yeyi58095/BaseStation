//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Master.h"
#include "ReplayDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TReplay *Replay;
//---------------------------------------------------------------------------

#include <System.SysUtils.hpp>

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
    curIdx = -1;
    lastDrawnIdx = -1;
    epAverage = true;
    hasEvents = false;
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
}
//---------------------------------------------------------------------------

void TReplay::BeginReplayAll(bool useAvg)
{
    epAverage = useAvg;
    InitChart();

    curIdx = 0;
    lastDrawnIdx = -1;

    // 建立事件快取（如果使用者切到 LOG_HUMAN 才會有）
    BuildEventsCacheFromDump();

    // 畫第一幀（如果有資料）
    DrawToIndex(0);
}
void __fastcall TReplay::btnStepClick(TObject *Sender)
{
	     // 每按一次往下一幀
	const std::vector<double>& tAll = master.traceT_all;
	if (tAll.empty()) {
		lblInfo->Caption = "No trace data. Run simulation first.";
        return;
    }
    if (curIdx < 0) curIdx = 0;
    if (curIdx >= (int)tAll.size() - 1) {
        // 到尾端
        curIdx = (int)tAll.size() - 1;
        DrawToIndex(curIdx);
        lblInfo->Caption = lblInfo->Caption + "\n(End)";
        return;
    }

    curIdx++;
	DrawToIndex(curIdx);
}
//---------------------------------------------------------------------------

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

    // 更新上方 Label
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
    // 顯示目前幀的統計（All）
    AnsiString base =
        "t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
        "   frame=" + IntToStr(k);

    // 如果有事件快取，找最後一筆 evtT <= tNow
    if (hasEvents && !evtT.empty()) {
        int j;
        int pick = -1;
        for (j = (int)evtT.size()-1; j >= 0; --j) {
            if (evtT[j] <= tNow + 1e-9) { pick = j; break; }
        }
        if (pick >= 0) {
            lblInfo->Caption = base + "\n" + evtMsg[pick];
            return;
        }
    }

    // 沒事件（或 LOG_CSV 模式），就簡單秀幀資訊
    const std::vector<double>& q  = master.traceQ_all;
    const std::vector<double>& m  = master.traceMeanQ_all;
    const std::vector<double>& eA = master.traceEavg_all;
    const std::vector<double>& eS = master.traceE_all;

    double epVal = epAverage ? ( (k < (int)eA.size()) ? eA[k] : 0.0 )
                             : ( (k < (int)eS.size()) ? eS[k] : 0.0 );
    AnsiString info = base +
        "\nQ=" + FloatToStrF(q[k], ffFixed, 7, 3) +
        "  MeanQ=" + FloatToStrF(m[k], ffFixed, 7, 3) +
        (epAverage ? "  EP_avg=" : "  EP_sum=") + FloatToStrF(epVal, ffFixed, 7, 3);
    lblInfo->Caption = info;
}

void TReplay::BuildEventsCacheFromDump()
{
    // 先試 Human 文字（dump）
    evtT.clear();
    evtMsg.clear();
    hasEvents = false;

    AnsiString dump = master.dumpLogWithSummary();
    TStringList* sl = new TStringList();
    sl->Text = dump;

    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i].Trim();
        if (line.Length() < 3) continue;
        if (!(line.SubString(1,2) == "t=")) continue;

		double tt = ParseLeadingTime(line);
        if (tt < 0.0) continue;

        // 做個簡短的人類可讀摘要：Sensor ? — TYPE (...)，並保留原行前 80 字輔助
        int sid = ParseIntAfter(line, "sensor=", -1);
        int pid = ParseIntAfter(line, "pkg=", -1);
        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid + 1)) : AnsiString("All/HAP");
        AnsiString typ = DetectEventType(line);

		if (typ == "STATE" || typ == "STAT") continue;
        AnsiString shortTxt = line;
        if (shortTxt.Length() > 80) shortTxt = shortTxt.SubString(1, 80) + "...";

        AnsiString msg = who + " — " + typ;
        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";
        msg += "\n" + shortTxt;

        evtT.push_back(tt);
        evtMsg.push_back(msg);
    }
    delete sl;

    if (!evtT.empty()) {
        hasEvents = true;
        return; // Human 模式 OK
    }

	// Human 模式沒抓到 → fallback: 用 CSV 來建
	BuildEventsFromCSV();
}

// 從 CSV 建立事件快取（填入 evtT / evtMsg）
void TReplay::BuildEventsFromCSV()
{
    evtT.clear();
    evtMsg.clear();

    // 嘗試兩個常見路徑：當前目錄、Win32/Debug
    AnsiString csvPath;
    if (!FileExistsMulti("run_log.csv", "Win32\\Debug\\run_log.csv", csvPath)) {
        // 也許在專案輸出資料夾（如果你用別的路徑，改這裡）
        if (!FileExistsMulti("..\\..\\Win32\\Debug\\run_log.csv", "..\\..\\run_log.csv", csvPath)) {
            // 找不到 CSV
            hasEvents = false;
            return;
        }
    }

    TStringList* sl = new TStringList();
    try {
        sl->LoadFromFile(csvPath);
    } catch(...) {
        delete sl;
        hasEvents = false;
        return;
    }

    std::vector<AnsiString> cols;
    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i].Trim();
        if (line.IsEmpty()) continue;

        // 跳過標頭（以 't,' 或 't,event' 判定；也防止人為插入的註解）
        if (line.Pos("t,") == 1 || line.Pos("t,event") == 1) continue;

        cols.clear();
        splitCSV(line, cols);
        if ((int)cols.size() < 8) continue;

        double t   = StrToFloatDef(cols[0], 0.0);
		CsvEvKind k= parseKind(cols[1]);
		if (k == CEV_STAT) continue;
        int sid    = StrToIntDef(cols[2], -1);
        int pid    = StrToIntDef(cols[3], -1);
        int q      = StrToIntDef(cols[4], -1);
        int ep     = StrToIntDef(cols[5], -1);
        double x1  = StrToFloatDef(cols[6], 0.0);
        double x2  = StrToFloatDef(cols[7], 0.0);

        // 組一行人類可讀的簡短訊息（與你原本 Label 類似）
        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid+1)) : AnsiString("All/HAP");
        AnsiString typ = kindToText(k);
        AnsiString msg = who + " — " + typ;

        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";

        // 也可加一點輔助資訊（可選）
        if (k == CEV_STX) {
            msg += "  st=" + FloatToStrF(x1, ffFixed, 7, 3) +
                   "  EP_before=" + IntToStr(ep) +
                   "  cost=" + IntToStr((int)x2);
        } else if (k == CEV_TTK) {
            msg += "  EP=" + IntToStr(ep) + "  remain=" + IntToStr((int)x1);
        } else if (k == CEV_ETX) {
            msg += "  EP=" + IntToStr(ep) + "  Q=" + IntToStr(q);
        } else if (k == CEV_CEND) {
            msg += "  EP=" + IntToStr(ep) + "  +=" + IntToStr((int)x1) + "EP";
        } else if (k == CEV_DROP) {
            if (q >= 0)  msg += "  Q=" + IntToStr(q);
            if (ep >= 0) msg += "  EP=" + IntToStr(ep);
        }

        evtT.push_back(t);
        evtMsg.push_back(msg);
    }

    delete sl;

    hasEvents = !evtT.empty();
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


