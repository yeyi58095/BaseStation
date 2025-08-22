//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "ChooseITorST.h"
#include "Unit5.h"
#include "ChooseMethodDialog.h"
#include "ParaSetterDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TmethodChooser *methodChooser;
//---------------------------------------------------------------------------
__fastcall TmethodChooser::TmethodChooser(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TmethodChooser::FormShow(TObject *Sender)
{
	AnsiString id = Form5->selectSensorComboBox->ItemIndex;
	int style = Chooser->state;

	AnsiString str = "" ;

	if(style== 1){
		str = "IT ";
	}else{
		str = "ST ";
	}

	methodChooser->Label1->Caption = "Sensor :" + id + ", is for " + str;

}
//---------------------------------------------------------------------------
void __fastcall TmethodChooser::ComboBox1Change(TObject *Sender)
{
	parameterSetter->Show();
	Close();

}
//---------------------------------------------------------------------------
