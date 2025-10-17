//---------------------------------------------------------------------------

#ifndef Unit5H
#define Unit5H
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
#include <vector>
#include "Sensor.h"

#include "Master.h"
//---------------------------------------------------------------------------
class TForm5 : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TEdit *sensorAmountEdit;
	TButton *generatorButton;
	TComboBox *selectSensorComboBox;
	TButton *Dubug;
	TLabel *DebugLabel;
	TLabel *Label2;
	TComboBox *selectVisitComboBox;
	TChart *Chart1;
	TFastLineSeries *SeriesCurrent;
	TFastLineSeries *SeriesMean;
	TButton *plotButton;
	TLabel *Label3;
	TEdit *endTimeEdit;
	TLabel *Label4;
	TEdit *switchOverEdit;
	TLabel *Label5;
	TEdit *maxCharginSlotEdit;
	TLabel *Label6;
	TLabel *log;
	TFastLineSeries *SeriesThreshold;
	TFastLineSeries *SeriesEP;
	TCheckBox *chkQ;
	TCheckBox *ckbMean;
	TCheckBox *ckbRtx;
	TCheckBox *ckbEP;
	TButton *setParaButton;
	TLabel *leftPanel;
	TRadioGroup *LogMode;
	TButton *replayButton;
	TButton *FreeMemory;
	TRadioGroup *ChargeModeGroup;
	TLabel *Label7;
	TEdit *SeedEdit;
	void __fastcall sensorAmountEditChange(TObject *Sender);
	void __fastcall generatorButtonClick(TObject *Sender);
	void __fastcall DubugClick(TObject *Sender);
	void __fastcall selectSensorComboBoxChange(TObject *Sender);
	void __fastcall selectVisitComboBoxChange(TObject *Sender);
	void __fastcall plotButtonClick(TObject *Sender);
	void __fastcall endTimeEditChange(TObject *Sender);
	void __fastcall switchOverEditChange(TObject *Sender);
	void __fastcall maxCharginSlotEditChange(TObject *Sender);
	void __fastcall chkQClick(TObject *Sender);
	void __fastcall ckbMeanClick(TObject *Sender);
	void __fastcall ckbRtxClick(TObject *Sender);
	void __fastcall ckbEPClick(TObject *Sender);
	void __fastcall setParaButtonClick(TObject *Sender);
	void __fastcall LogModeClick(TObject *Sender);
	void __fastcall replayButtonClick(TObject *Sender);
	void __fastcall FreeMemoryClick(TObject *Sender);
	void __fastcall ChargeModeGroupClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm5(TComponent* Owner);
	std::vector<Sensor*> sensors;
	bool runned;
};
//---------------------------------------------------------------------------
extern PACKAGE TForm5 *Form5;
//---------------------------------------------------------------------------
#endif
