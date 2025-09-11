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

    // ��l�ƻPø��
    void InitChart();
    void BeginReplayAll(bool useAvg);
    void DrawToIndex(int k);        // ��ϵe��� k �V
    void AppendPoint(int i);        // �u�[�@�I�]�[�t�Ρ^
    void UpdateLabelForTime(double t, int k);

    // �بƥ�֨��]LOG_HUMAN �ɡ^
    void BuildEventsCacheFromDump();
	static double ParseLeadingTime(const AnsiString& line); // �ѪR "t=xx" �ɶ�
public:		// User declarations
	__fastcall TReplay(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TReplay *Replay;
//---------------------------------------------------------------------------
#endif
