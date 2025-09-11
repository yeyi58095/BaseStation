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

// ===== CSV �ѪR�]�L�޸��B�z�F�A�� CSV �L�޸��^=====
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
    nextEventIdx = 0;
    epAverage = useAvg;
    InitChart();

    curIdx = 0;
    lastDrawnIdx = -1;
    S.clear();

    BuildEventsCacheFromDump();
    if (!hasEvents) BuildEventsFromCSV();

    // �� �إߨƥ���V�� bucket
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
                // ���̤p�� k �� te <= tA[k]
                while (k < K && te - tA[k] > 1e-9) ++k;
                if (k >= K) k = K - 1;      // �ӱߪ��ƥ�N���̫�@�V
                if (k < 0)  k = 0;
                frameEv[k].push_back(i);
            }
        }
    }

    DrawToIndex(0);
}

void __fastcall TReplay::btnStepClick(TObject *Sender)
{
    const std::vector<double>& tAll = master.traceT_all;
    if (tAll.empty()) { lblInfo->Caption = "No trace data. Run simulation first."; return; }
    if (curIdx < 0) curIdx = 0;

    // �e��ثe�V
    DrawToIndex(curIdx);
    double tNow = tAll[curIdx];

    // �����զR�X�u�o�@�V�v���|����X���ƥ�]�@���u�R�@���^
    if (hasEvents && curIdx < (int)frameEv.size()) {
        std::vector<int>& bucket = frameEv[curIdx];

        int p;
        for (p = 0; p < (int)bucket.size(); ++p) {
            int evIdx = bucket[p];
            if (!evUsed[evIdx]) {
                // 1) �M�Ψƥ�]��s���A�B�o��ƥ��^
                AnsiString line = ApplyAndFormatEvent(evIdx);
                evUsed[evIdx] = 1;

				// 2) �Ρu�ƥ��v�� Q/EP ���ؼ��Y�]���W��Ʀr�P�ƥ�@�P�^
				// ...�w���GAnsiString line = ApplyAndFormatEvent(evIdx);  // �o�B�|��s S[sid].ep
				int sid    = evtSid[evIdx];
				int qAfter = (evtQ[evIdx]  >= 0) ? evtQ[evIdx]  : (int)S[sid].q.size();
				int epAfter= (evtEP[evIdx] >= 0) ? evtEP[evIdx] : S[sid].ep;

				// ���� trace �� MeanQ�B�]�O�d EP ���u�@����
				double meanQ = master.traceMeanQ_all[curIdx];

				// �� �o�̬O����G��@ sensor �����Ψƥ�᪺ EP �� EP_avg
				double epHeader;
				int N = (master.sensors ? (int)master.sensors->size() : 1);
				if (N == 1 && sid >= 0) {
					epHeader = (double)epAfter;             // �� sensor�GEP_avg = �� sensor EP�]�ƥ��^
				} else {
					// �h sensor �ɡA���� trace ���A�A�Υi���o�� delta �L�ա]�i��^
					double epTrace = epAverage
						? ((curIdx < (int)master.traceEavg_all.size()) ? master.traceEavg_all[curIdx] : 0.0)
						: ((curIdx < (int)master.traceE_all.size())     ? master.traceE_all[curIdx]     : 0.0);

					int delta = 0;
					if (evtKind[evIdx] == 2) {          // TTK: �C tick �� 1
						delta = -1;
					} else if (evtKind[evIdx] == 6) {   // CEND: x1 �O�R�i�h�� EP ��
						delta = (int)evtX1[evIdx];
					}
					epHeader = epTrace + (epAverage ? ((double)delta)/N : (double)delta);
				}

				// ���ؼ��Y�]�� qAfter / epHeader�^
				AnsiString header =
					"t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
					"   frame=" + IntToStr(curIdx) +
					"\nQ=" + FloatToStrF(qAfter, ffFixed, 7, 3) +
					"  MeanQ=" + FloatToStrF(meanQ,  ffFixed, 7, 3) +
					(epAverage ? "  EP_avg=" : "  EP_sum=") +
					FloatToStrF(epHeader, ffFixed, 7, 3);

				lblInfo->Caption = header + "\n" + line;
				return;

            }
        }
    }

    // �o�@�V�S���ƥ�B�Τw�R�� �� ����U�@�V
    if (curIdx < (int)tAll.size() - 1) {
        curIdx++;
        DrawToIndex(curIdx);
    } else {
        lblInfo->Caption = lblInfo->Caption + "\n(End)";
    }
}

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

    int i;
    for (i = 0; i < sl->Count; ++i) {
        AnsiString line = sl->Strings[i].Trim();
        if (line.Length() < 3) continue;
        if (!(line.SubString(1,2) == "t=")) continue;

        double tt = ParseLeadingTime(line);
        if (tt < 0.0) continue;

        AnsiString typ = DetectEventType(line);
        if (typ == "STATE" || typ == "STAT") continue; // �^�񤣦Y

        int sid = ParseIntAfter(line, "sensor=", -1);
        int pid = ParseIntAfter(line, "pkg=", -1);

        int kind = 8; // UNKNOWN
        if (typ == "ARR") kind = 0;
        else if (typ == "START_TX") kind = 1;
        else if (typ == "TX_TICK")  kind = 2;
        else if (typ == "END_TX")   kind = 3;
        else if (typ == "DROP")     kind = 4;
        else if (typ == "CHARGE_START") kind = 5;
        else if (typ == "CHARGE_END")   kind = 6;

        evtT.push_back(tt);
        evtKind.push_back(kind);
        evtSid.push_back(sid);
        evtPid.push_back(pid);
        evtQ.push_back(-1);     // Human �L q/ep ��A��Ѫ��A��
        evtEP.push_back(-1);
        evtX1.push_back(0.0);
        evtX2.push_back(0.0);

        // �i�d�@��²�u��r�]�D���n�^
        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid+1)) : AnsiString("All/HAP");
        AnsiString msg = who + " �X " + typ;
        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";
        evtMsg.push_back(msg);
    }
    delete sl;

    hasEvents = !evtT.empty();
    nextEventIdx = 0;
    S.clear();
    // �ƥ���ɶ��w�g���ǡ]dump �쥻�N�O�Ӯɶ��^�A�Y���ݭn�i��í�w�Ƨ�
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
        if (k == CEV_STAT) continue; // ���Y STAT

        int kind = 8; // UNKNOWN -> �ڭ̦ۤv���s�X
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

        // �w�]�T���]�D���n�F�u���n�Ϊ��A�r��|���s�ա^
        AnsiString who = (sid >= 0) ? (AnsiString("Sensor ") + IntToStr(sid+1)) : AnsiString("All/HAP");
        AnsiString typ = kindToText(k);
        AnsiString msg = who + " �X " + typ;
        if (pid >= 0) msg += "  (pkg=" + IntToStr(pid) + ")";
        evtMsg.push_back(msg);
    }
    delete sl;

    // CSV �i�ण�Oí�w�ɶ��ǡA���@�Uí�w�Ƨǡ]�L lambda �����^
    // ��²�洡�J�Ƨǡ]��ƶq�q�`���j�F�Y�ܤj�i�����禡 + stable_sort�^
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
    // ���L�ť�
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
    // �̧A�� Master::log*() �T��������P�_
    if (line.Pos("ARRIVAL") > 0)       return "ARR";
    if (line.Pos("START_TX") > 0)      return "START_TX";
    if (line.Pos("TX_TICK") > 0)       return "TX_TICK";
    if (line.Pos("END_TX") > 0)        return "END_TX";
    if (line.Pos("DROP") > 0)          return "DROP";
    if (line.Pos("CHARGE_START") > 0)  return "CHARGE_START";
    if (line.Pos("CHARGE_END") > 0)    return "CHARGE_END";
    if (line.Pos("STATE") > 0)         return "STATE";
    if (line.Pos("STAT") > 0)          return "STAT";  // �Y���Ӥ]�䴩 CSV->�HŪ
    return "EVENT";
}

int TReplay::FindLastEventIndexLE(double tNow) const
{
    if (events.empty()) return -1;
    // �u�ʧ�N���F�]�ƥ󤣦h�^�F�n�[�t�i���G��
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
    // �Τ@�b�Ĥ@����ܦ^�񪬺A
    AnsiString head = "t=" + FloatToStrF(tNow, ffFixed, 7, 3) +
                      "   frame=" + IntToStr(frameIdx);

    // �ĤG����ܡu�ĴX�� sensor�B�ƥ�O����v
    // �Y sid = -1�A��� All/HAP/STATE ��
    AnsiString who = (ev.sid >= 0) ? (AnsiString("Sensor ") + IntToStr(ev.sid + 1))
                                   : AnsiString("All/HAP");
    AnsiString line2 = who + " �X " + ev.type;

    // �A�ץ[�W pid/�u���
    if (ev.pid >= 0) line2 += "  (pkg=" + IntToStr(ev.pid) + ")";

    // �]�i�H�[�W�A��l�檺�@�p�q�A���U����
    // �Ҧp�O�d�� 80 �r��
    AnsiString shortTxt = ev.text;
    if (shortTxt.Length() > 80) shortTxt = shortTxt.SubString(1, 80) + "...";

    lblInfo->Caption = head + "\n" + line2 + "\n" + shortTxt;
}


bool TReplay::FindTimeGroup(double tNow, int& lo, int& hi) const
{
    lo = -1; hi = -1;
    if (!hasEvents || evtT.empty()) return false;

    // �����̫�@�� <= tNow
    int j;
    int lastLE = -1;
    for (j = (int)evtT.size() - 1; j >= 0; --j) {
        if (evtT[j] <= tNow + 1e-9) { lastLE = j; break; }
    }
    if (lastLE < 0) return false;

    // �o�@�����ɶ��N�O�s�ծɶ��]�i�� < tNow�F�Y < tNow�A�N�� tNow �S�ƥ�A�N�^���Ӯɶ����̫�s�^
    double T = evtT[lastLE];

    // �������P�@�Ӯɶ����_�I
    int s = lastLE;
    while (s - 1 >= 0 && fabs(evtT[s - 1] - T) <= 1e-9) --s;

    // ���k���P�@�Ӯɶ������I
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
    int qcol = evtQ[idx];     // �ƥ�᪺ queue size�]CSV �|���AHuman �S���^
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
            line = who + " �X ARR  (pkg=" + IntToStr(pid) + ")";
            break;

        case 1: // START_TX
            // �q queue �����n�A�Ȫ�����
            if (!st.q.empty() && pid >= 0 && st.q.front() == pid) st.q.erase(st.q.begin());
            st.servingPid = pid;
            line = who + " �X START_TX  (pkg=" + IntToStr(pid) + ")"
                 + "  st=" + FloatToStrF(x1, ffFixed, 7, 3)
                 + "  EP_before=" + IntToStr(epcol)
                 + "  cost=" + IntToStr((int)x2);
            break;

        case 2: // TX_TICK
            if (epcol >= 0) st.ep = epcol;
            line = who + " �X TX_TICK  (pkg=" + IntToStr(st.servingPid) + ")"
                 + "  EP=" + IntToStr(st.ep);
            break;

        case 3: // END_TX
            st.servingPid = -1;
            if (epcol >= 0) st.ep = epcol;
            line = who + " �X END_TX  (pkg=" + IntToStr(pid) + ")";
            break;

        case 4: // DROP
            if (epcol >= 0) st.ep = epcol;
            line = who + " �X DROP";
            break;

        case 5: // CHARGE_START
            line = who + " �X CHARGE_START";
            break;

        case 6: // CHARGE_END
            if (epcol >= 0) st.ep = epcol;
            line = who + " �X CHARGE_END  EP=" + IntToStr(st.ep);
            break;

        default:
            line = who + " �X EVENT";
            break;
    }

    // �� Q ��� traceQ_all�G�u�ⵥ�ݦ�C�A���t in-service
    int qnow = (qcol >= 0) ? qcol : (int)st.q.size();

    // �� EP�G�Y CSV �� ep ���N�ΡA�_�h�Χڭ̺��������A
    int epnow = (epcol >= 0) ? epcol : st.ep;

    line += "   |  Q=" + IntToStr(qnow) + "  EP=" + IntToStr(epnow);
    return line;
}


