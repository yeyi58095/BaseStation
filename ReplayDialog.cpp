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

/*** 事件快取（從 dumpLogWithSummary() 解析 "t=..." 行） ***/
void TReplay::BuildEventsCacheFromDump()
{
    evtT.clear();
    evtMsg.clear();
    hasEvents = false;

    AnsiString dump = master.dumpLogWithSummary();
    // dump 在 LOG_HUMAN 模式會含 "# === Timeline ===" 與大量 "t=..." 行
    TStringList* sl = new TStringList();
    sl->Text = dump;

    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i];
        // 只收以 "t=" 開頭的行
        if (line.Length() >= 3 && line.SubString(1,2) == "t=") {
            double tt = ParseLeadingTime(line);
            // 過濾空時間
            if (tt >= 0.0) {
                evtT.push_back(tt);
                evtMsg.push_back(line);
            }
        }
    }
    delete sl;

    if (!evtT.empty() && !evtMsg.empty()) hasEvents = true;
}

double TReplay::ParseLeadingTime(const AnsiString& line)
{
    // line 例如： "t=12.345  HAP      START_TX sensor=1 pkg=..."
    // 取 "t=" 後直到下一個空白為止
    int p = line.Pos("t=");
    if (p <= 0) return -1.0;
    int s = p + 2; // 數值起點（1-based）
    int len = line.Length();
    int e = s;
    while (e <= len) {
        WideChar ch = line[e];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') break;
        e++;
    }
    AnsiString num = line.SubString(s, e - s);
    double val = StrToFloatDef(num, -1.0);
    return val;
}


