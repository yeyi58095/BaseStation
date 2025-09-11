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
    // ���b�GQueue�A�k�b�GEP
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
    // �w�]�� EP �����F�A�n�令 sum �N�� false
    BeginReplayAll(true);

    // ��ܼ��D
	ReplayChart->Title->Caption = epAverage ? "All sensors (EP avg)" : "All sensors (EP sum)";
}
//---------------------------------------------------------------------------

void TReplay::BeginReplayAll(bool useAvg)
{
    epAverage = useAvg;
    InitChart();

    curIdx = 0;
    lastDrawnIdx = -1;

    // �إߨƥ�֨��]�p�G�ϥΪ̤��� LOG_HUMAN �~�|���^
    BuildEventsCacheFromDump();

    // �e�Ĥ@�V�]�p�G����ơ^
    DrawToIndex(0);
}
void __fastcall TReplay::btnStepClick(TObject *Sender)
{
	     // �C���@�����U�@�V
	const std::vector<double>& tAll = master.traceT_all;
	if (tAll.empty()) {
		lblInfo->Caption = "No trace data. Run simulation first.";
        return;
    }
    if (curIdx < 0) curIdx = 0;
    if (curIdx >= (int)tAll.size() - 1) {
        // �����
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

    // ���F�קK�C�V O(N) �M�ŭ��e�A�e�i�ɥu�ɷs�I
    if (lastDrawnIdx < 0) {
        // �Ĥ@���G�� 0..k ���e�W�h
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
        // ���e�@�B�G�u�� k �o�@�I
        AppendPoint(k);
    } else if (k < lastDrawnIdx) {
        // �]�z�פW���y�{�u�����e�F�Y�n�䴩���^�A�o�������e�^
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

    // ��s�W�� Label
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
    // ��ܥثe�V���έp�]All�^
    AnsiString base =
        "t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
        "   frame=" + IntToStr(k);

    // �p�G���ƥ�֨��A��̫�@�� evtT <= tNow
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

    // �S�ƥ�]�� LOG_CSV �Ҧ��^�A�N²��q�V��T
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

/*** �ƥ�֨��]�q dumpLogWithSummary() �ѪR "t=..." ��^ ***/
void TReplay::BuildEventsCacheFromDump()
{
    evtT.clear();
    evtMsg.clear();
    hasEvents = false;

    AnsiString dump = master.dumpLogWithSummary();
    // dump �b LOG_HUMAN �Ҧ��|�t "# === Timeline ===" �P�j�q "t=..." ��
    TStringList* sl = new TStringList();
    sl->Text = dump;

    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i];
        // �u���H "t=" �}�Y����
        if (line.Length() >= 3 && line.SubString(1,2) == "t=") {
            double tt = ParseLeadingTime(line);
            // �L�o�Ůɶ�
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
    // line �Ҧp�G "t=12.345  HAP      START_TX sensor=1 pkg=..."
    // �� "t=" �᪽��U�@�Ӫťլ���
    int p = line.Pos("t=");
    if (p <= 0) return -1.0;
    int s = p + 2; // �ƭȰ_�I�]1-based�^
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


