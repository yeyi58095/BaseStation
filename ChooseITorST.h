//---------------------------------------------------------------------------

#ifndef ChooseITorSTH
#define ChooseITorSTH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TChooser : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TButton *Button1;
	TButton *Button2;
	TLabel *Label2;
	TLabel *Label3;
	TEdit *qmaxEdit;
	TLabel *Label4;
	TLabel *preLoad;
	TEdit *preloadEdit;
	TEdit *ECapEdit;
	TLabel *EP;
	TLabel *E_Cap;
	TLabel *InitEnergy;
	TEdit *EnergyEdit;
	TLabel *r_tx;
	TEdit *rTxEdit;
	TLabel *Label5;
	TEdit *chargingRateEdit;
	TButton *Button3;
	TLabel *Label6;
	TEdit *costBaseEdit;
	TLabel *CostPerSec;
	TCheckBox *CheckBox1;
	TEdit *costPerSecEdit;
	void __fastcall OnShow(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
	void __fastcall qmaxEditChange(TObject *Sender);
	void __fastcall preloadEditChange(TObject *Sender);
	void __fastcall ECapEditChange(TObject *Sender);
	void __fastcall EnergyEditChange(TObject *Sender);
	void __fastcall rTxEditChange(TObject *Sender);
	void __fastcall chargingRateEditChange(TObject *Sender);
	void __fastcall Button3Click(TObject *Sender);
	void __fastcall costBaseEditChange(TObject *Sender);
	void __fastcall costPerSecEditChange(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TChooser(TComponent* Owner);
	int state; //
};
//---------------------------------------------------------------------------
extern PACKAGE TChooser *Chooser;
//---------------------------------------------------------------------------
#endif
