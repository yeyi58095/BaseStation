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
#include <vector>   // 若專案沒有其他地方包含，建議加上

//---------------------------------------------------------------------------

struct ReplayEvent {
    double t;
    int    sid;       // -1 表示無 sid（例如 All/STAT）
    int    pid;       // 封包id；未知用 -1
    AnsiString type;  // "ARR","START_TX","TX_TICK","END_TX","DROP","CHARGE_START","CHARGE_END","STATE","STAT"
    AnsiString text;  // 原始行或短敘述
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
    // 回放狀態（All sensors 模式）
    int  curIdx;           // 目前幀索引（對 traceT_all）
    int  lastDrawnIdx;     // 最後已繪到哪一幀（加速）
    bool epAverage;        // true=EP 用 avg，false=EP 用 sum

    // 事件快取（從 dumpLogWithSummary() 解析）
	std::vector<double>     evtT;
	    std::vector<int>    evtKind;   // 0:ARR 1:STX 2:TTK 3:ETX 4:DROP 5:CST 6:CEND 7:STAT 8:UNKNOWN
    std::vector<int>    evtSid;
    std::vector<int>    evtPid;
    std::vector<int>    evtQ;
    std::vector<int>    evtEP;
    std::vector<double> evtX1;
	std::vector<double> evtX2;
    std::vector<AnsiString> evtMsg;
	bool hasEvents;         // 是否有解析到事件（LOG_HUMAN 時為真）

	std::vector<ReplayEvent> events;
	int  nextEventIdx;

	// 回放狀態（每個 sensor 一份）
	struct SidState {
        std::vector<int> q; // 佇列內的封包 id
        int servingPid;     // -1 表示無
        int ep;             // 目前 EP
        SidState(): servingPid(-1), ep(0) {}
    };
	std::map<int, SidState> S;


    // 初始化與繪圖
    void InitChart();
	void BeginReplayAll(bool useAvg);
	void DrawToIndex(int k);        // 把圖畫到第 k 幀
	void AppendPoint(int i);        // 只加一點（加速用）
	void UpdateLabelForTime(double t, int k);

    // 建事件快取（LOG_HUMAN 時）
	void BuildEventsCacheFromDump();
	void BuildEventsFromCSV()         ;

	static double ParseLeadingTime(const AnsiString& line); // 解析 "t=xx" 時間
	static int    ParseIntAfter(const AnsiString& line, const AnsiString& key, int defVal);
	static AnsiString DetectEventType(const AnsiString& line);
    int  FindLastEventIndexLE(double tNow) const;
	void SetLabelFromEvent(const ReplayEvent& ev, double tNow, int frameIdx);

		// 全域事件游標：下一筆尚未顯示的事件在 evtT/evtMsg 的索引

    // 幫手：找到某時間戳的群組範圍 [lo, hi]（含端點）
    bool FindTimeGroupByIndex(int anchor, int& lo, int& hi) const;

	    // 同一時間戳內的子步驟
    double curTimeGroupT;   // 目前這個「同時刻」的時間，初始化為 -1
	int    curTimeGroupPos; // 這個時間戳內，下一筆要顯示的事件在該群組中的位置（0-based）
	    // 顯示：吃一筆事件 → 更新狀態 → 回傳一行顯示文字（含 Q/EP）
	AnsiString ApplyAndFormatEvent(int idx);

    // 小工具：找出等於某時間戳的事件索引範圍 [lo, hi]（含端點），找不到回傳 false
	bool   FindTimeGroup(double tNow, int& lo, int& hi) const;

	// 每一幀對應的事件索引清單
	std::vector< std::vector<int> > frameEv;
	// 事件是否已經輸出過
	std::vector<char> evUsed;

public:		// User declarations
	__fastcall TReplay(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TReplay *Replay;
//---------------------------------------------------------------------------
#endif
