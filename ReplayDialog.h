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
    std::vector<AnsiString> evtMsg;
	bool hasEvents;         // �O�_���ѪR��ƥ�]LOG_HUMAN �ɬ��u�^

	std::vector<ReplayEvent> events;


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
public:		// User declarations
	__fastcall TReplay(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TReplay *Replay;
//---------------------------------------------------------------------------
#endif
