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
    // 回放狀態（All sensors 模式）
    int  curIdx;           // 目前幀索引（對 traceT_all）
    int  lastDrawnIdx;     // 最後已繪到哪一幀（加速）
    bool epAverage;        // true=EP 用 avg，false=EP 用 sum

    // 事件快取（從 dumpLogWithSummary() 解析）
    std::vector<double>     evtT;
    std::vector<AnsiString> evtMsg;
    bool hasEvents;         // 是否有解析到事件（LOG_HUMAN 時為真）

    // 初始化與繪圖
    void InitChart();
    void BeginReplayAll(bool useAvg);
    void DrawToIndex(int k);        // 把圖畫到第 k 幀
    void AppendPoint(int i);        // 只加一點（加速用）
    void UpdateLabelForTime(double t, int k);

    // 建事件快取（LOG_HUMAN 時）
    void BuildEventsCacheFromDump();
	static double ParseLeadingTime(const AnsiString& line); // 解析 "t=xx" 時間
public:		// User declarations
	__fastcall TReplay(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TReplay *Replay;
//---------------------------------------------------------------------------
#endif
