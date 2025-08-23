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
	TButton *Button1;
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
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall sensorAmountEditChange(TObject *Sender);
	void __fastcall generatorButtonClick(TObject *Sender);
	void __fastcall DubugClick(TObject *Sender);
	void __fastcall selectSensorComboBoxChange(TObject *Sender);
	void __fastcall selectVisitComboBoxChange(TObject *Sender);
	void __fastcall plotButtonClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm5(TComponent* Owner);
	std::vector<Sensor*> sensors;
};
//---------------------------------------------------------------------------
extern PACKAGE TForm5 *Form5;
//---------------------------------------------------------------------------
#endif
