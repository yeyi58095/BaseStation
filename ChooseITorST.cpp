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
	this->qmaxEdit->Text = s->Qmax;
	this->ECapEdit->Text = s->E_cap;
	this->EnergyEdit->Text = s->energy;
	this->rTxEdit->Text = s->r_tx;
	this->chargingRateEdit->Text = s->charge_rate;
	this->costBaseEdit->Text = s->txCostBase;
	this->costPerSecEdit->Text = s->txCostPerSec;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::Button1Click(TObject *Sender)
{
	state = 1;
	methodChooser->Show();   Form5->runned = false;

}
//---------------------------------------------------------------------------
void __fastcall TChooser::Button2Click(TObject *Sender)
{
	state = 2;
	methodChooser->Show();  Form5->runned = false;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::qmaxEditChange(TObject *Sender)
{
	int qmax = StrToIntDef(this->qmaxEdit->Text, -1);
	s->setQmax(qmax);         Form5->runned = false;
}
//---------------------------------------------------------------------------


	void __fastcall TChooser::preloadEditChange(TObject *Sender)
{
    int preload = StrToIntDef(this->preloadEdit->Text, 0);
    s->setPreloadInit(preload);
    Form5->runned = false;
}

//---------------------------------------------------------------------------

void __fastcall TChooser::ECapEditChange(TObject *Sender)
{
	int e_cap = StrToIntDef(this->ECapEdit->Text, s->E_cap);
	s->E_cap = e_cap;   Form5->runned = false;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::EnergyEditChange(TObject *Sender)
{
	int eng = StrToIntDef(this->EnergyEdit->Text, s->energy);
	s->energy = eng;    Form5->runned = false;
}
//---------------------------------------------------------------------------




void __fastcall TChooser::rTxEditChange(TObject *Sender)
{
	int r = StrToIntDef(this->rTxEdit->Text, s->r_tx);
	s->r_tx = r;      Form5->runned = false;
}
//---------------------------------------------------------------------------


void __fastcall TChooser::chargingRateEditChange(TObject *Sender)
{
	int rate = StrToIntDef(this->chargingRateEdit->Text, s->charge_rate);
	s->charge_rate = rate;
	Form5->runned = false;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::Button3Click(TObject *Sender)
{
	Form5->log->Caption = s->toString();
	Close();
}
//---------------------------------------------------------------------------


void __fastcall TChooser::costBaseEditChange(TObject *Sender)
{
	int cb = StrToIntDef(this->costBaseEdit->Text, s->txCostBase);
	s->txCostBase = cb;
}
//---------------------------------------------------------------------------

void __fastcall TChooser::costPerSecEditChange(TObject *Sender)
{
	int cps = StrToIntDef(this->costPerSecEdit->Text, s->txCostPerSec);
	s->txCostPerSec = cps;
}
//---------------------------------------------------------------------------


