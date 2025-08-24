//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ChooseITorST.h"
#include "ChooseMethodDialog.h"
#include "Unit5.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TChooser *Chooser;
Sensor* s;

//---------------------------------------------------------------------------
__fastcall TChooser::TChooser(TComponent* Owner)
	: TForm(Owner)
{

}
//---------------------------------------------------------------------------
void __fastcall TChooser::OnShow(TObject *Sender)
{
	 s = Form5->sensors[Form5->selectSensorComboBox->ItemIndex] ;
	Chooser->Label1->Caption = "Sensor : " + IntToStr(Form5->selectSensorComboBox->ItemIndex);
}
//---------------------------------------------------------------------------

void __fastcall TChooser::Button1Click(TObject *Sender)
{
	state = 1;
	methodChooser->Show();

}
//---------------------------------------------------------------------------
void __fastcall TChooser::Button2Click(TObject *Sender)
{
	state = 2;
	methodChooser->Show();
}
//---------------------------------------------------------------------------

void __fastcall TChooser::qmaxEditChange(TObject *Sender)
{
	int qmax = StrToIntDef(this->qmaxEdit->Text, -1);
	s->setQmax(qmax);
}
//---------------------------------------------------------------------------


void __fastcall TChooser::preloadEditChange(TObject *Sender)
{
	int preload = StrToIntDef(this->preloadEdit->Text, 0);
	s->preloadDP(preload);
}
//---------------------------------------------------------------------------

void __fastcall TChooser::ECapEditChange(TObject *Sender)
{
	int e_cap = StrToIntDef(this->ECapEdit->Text, s->E_cap);
	s->E_cap = e_cap;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::EnergyEditChange(TObject *Sender)
{
	int eng = StrToIntDef(this->EnergyEdit->Text, s->energy);
	s->energy = eng;
}
//---------------------------------------------------------------------------




void __fastcall TChooser::rTxEditChange(TObject *Sender)
{
	int r = StrToIntDef(this->rTxEdit->Text, s->r_tx);
	s->r_tx = r;
}
//---------------------------------------------------------------------------


void __fastcall TChooser::Edit1Change(TObject *Sender)
{
	int rate = StrToIntDef(this->Edit1->Text, s->charge_rate);
	s->charge_rate = rate;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::Button3Click(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

