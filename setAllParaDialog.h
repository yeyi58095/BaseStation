//---------------------------------------------------------------------------

#ifndef setAllParaDialogH
#define setAllParaDialogH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TsetAllPara : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TLabel *Label2;
	TComboBox *ComboBox1;
	TLabel *ITpara1;
	TEdit *ITpara1Edit;
	TLabel *ITpara2;
	TEdit *ITpara2Edit;
	TLabel *Label3;
	TComboBox *ComboBox2;
	TLabel *STpara1;
	TLabel *STpara2;
	TEdit *STpara1Edit;
	TEdit *STpara2Edit;
	TLabel *BufferSize;
	TEdit *BufferSizeEdit;
	TButton *OK;
	TLabel *EP;
	TLabel *Capacity;
	TEdit *CapacityEdit;
	TLabel *Charging_Rate;
	TEdit *chargingRateEdit;
	TLabel *R;
	TEdit *REdit;
	TLabel *Threshold;
	TEdit *thresholdEdit;
	void __fastcall OnShow(TObject *Sender);
	void __fastcall OKClick(TObject *Sender);
	void __fastcall ComboBox1Change(TObject *Sender);
	void __fastcall ComboBox2Change(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TsetAllPara(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TsetAllPara *setAllPara;
//---------------------------------------------------------------------------
#endif
