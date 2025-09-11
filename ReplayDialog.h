//---------------------------------------------------------------------------

#ifndef ReplayDialogH
#define ReplayDialogH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <VCLTee.Chart.hpp>
#include <VCLTee.Series.hpp>
#include <VclTee.TeeGDIPlus.hpp>
#include <VCLTee.TeEngine.hpp>
#include <VCLTee.TeeProcs.hpp>

#include <map>
#include <cmath>
#include <vector>   // �Y�M�רS����L�a��]�t�A��ĳ�[�W

//---------------------------------------------------------------------------

struct ReplayEvent {
    double t;
    int    sid;       // -1 ��ܵL sid�]�Ҧp All/STAT�^
    int    pid;       // �ʥ]id�F������ -1
    AnsiString type;  // "ARR","START_TX","TX_TICK","END_TX","DROP","CHARGE_START","CHARGE_END","STATE","STAT"
    AnsiString text;  // ��l��εu�ԭz
};

class TReplay : public TForm
{
__published:	// IDE-managed Components
	TChart *ReplayChart;
	TFastLineSeries *SeriesQ;
	TFastLineSeries *SeriesMeanQ;
	TFastLineSeries *SeriesEP;
	TButton *btnStep;
	TLabel *lblInfo;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall btnStepClick(TObject *Sender);
private:	// User declarations
    // �^�񪬺A�]All sensors �Ҧ��^
    int  curIdx;           // �ثe�V���ޡ]�� traceT_all�^
    int  lastDrawnIdx;     // �̫�wø����@�V�]�[�t�^
    bool epAverage;        // true=EP �� avg�Afalse=EP �� sum

    // �ƥ�֨��]�q dumpLogWithSummary() �ѪR�^
	std::vector<double>     evtT;
	    std::vector<int>    evtKind;   // 0:ARR 1:STX 2:TTK 3:ETX 4:DROP 5:CST 6:CEND 7:STAT 8:UNKNOWN
    std::vector<int>    evtSid;
    std::vector<int>    evtPid;
    std::vector<int>    evtQ;
    std::vector<int>    evtEP;
    std::vector<double> evtX1;
	std::vector<double> evtX2;
    std::vector<AnsiString> evtMsg;
	bool hasEvents;         // �O�_���ѪR��ƥ�]LOG_HUMAN �ɬ��u�^

	std::vector<ReplayEvent> events;
	int  nextEventIdx;

	// �^�񪬺A�]�C�� sensor �@���^
	struct SidState {
        std::vector<int> q; // ��C�����ʥ] id
        int servingPid;     // -1 ��ܵL
        int ep;             // �ثe EP
        SidState(): servingPid(-1), ep(0) {}
    };
	std::map<int, SidState> S;


    // ��l�ƻPø��
    void InitChart();
	void BeginReplayAll(bool useAvg);
	void DrawToIndex(int k);        // ��ϵe��� k �V
	void AppendPoint(int i);        // �u�[�@�I�]�[�t�Ρ^
	void UpdateLabelForTime(double t, int k);

    // �بƥ�֨��]LOG_HUMAN �ɡ^
	void BuildEventsCacheFromDump();
	void BuildEventsFromCSV()         ;

	static double ParseLeadingTime(const AnsiString& line); // �ѪR "t=xx" �ɶ�
	static int    ParseIntAfter(const AnsiString& line, const AnsiString& key, int defVal);
	static AnsiString DetectEventType(const AnsiString& line);
    int  FindLastEventIndexLE(double tNow) const;
	void SetLabelFromEvent(const ReplayEvent& ev, double tNow, int frameIdx);

		// ����ƥ��СG�U�@���|����ܪ��ƥ�b evtT/evtMsg ������

    // ����G���Y�ɶ��W���s�սd�� [lo, hi]�]�t���I�^
    bool FindTimeGroupByIndex(int anchor, int& lo, int& hi) const;

	    // �P�@�ɶ��W�����l�B�J
    double curTimeGroupT;   // �ثe�o�ӡu�P�ɨ�v���ɶ��A��l�Ƭ� -1
	int    curTimeGroupPos; // �o�Ӯɶ��W���A�U�@���n��ܪ��ƥ�b�Ӹs�դ�����m�]0-based�^
	    // ��ܡG�Y�@���ƥ� �� ��s���A �� �^�Ǥ@����ܤ�r�]�t Q/EP�^
	AnsiString ApplyAndFormatEvent(int idx);

    // �p�u��G��X����Y�ɶ��W���ƥ���޽d�� [lo, hi]�]�t���I�^�A�䤣��^�� false
	bool   FindTimeGroup(double tNow, int& lo, int& hi) const;

	// �C�@�V�������ƥ���޲M��
	std::vector< std::vector<int> > frameEv;
	// �ƥ�O�_�w�g��X�L
	std::vector<char> evUsed;

public:		// User declarations
	__fastcall TReplay(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TReplay *Replay;
//---------------------------------------------------------------------------
#endif
