//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ChooseITorST.h"
#include "Unit5.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TChooser *Chooser;

//---------------------------------------------------------------------------
__fastcall TChooser::TChooser(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TChooser::OnShow(TObject *Sender)
{
	Chooser->Label1->Caption = "Sensor : " + IntToStr(Form5->selectSensorComboBox->ItemIndex);
}
//---------------------------------------------------------------------------

void __fastcall TChooser::Button1Click(TObject *Sender)
{
	state = 1;
}
//---------------------------------------------------------------------------
void __fastcall TChooser::Button2Click(TObject *Sender)
{
	state = 2;
}
//---------------------------------------------------------------------------
