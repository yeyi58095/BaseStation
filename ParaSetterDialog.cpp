//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "ChooseMethodDialog.h"
#include "ChooseITorST.h"
#include "ParaSetterDialog.h"
#include "Unit5.h"
#include "Sensor.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TparameterSetter *parameterSetter;
//---------------------------------------------------------------------------
__fastcall TparameterSetter::TparameterSetter(TComponent* Owner)
	: TForm(Owner)
{

}
//---------------------------------------------------------------------------
void __fastcall TparameterSetter::OnShow(TObject *Sender)
{
	parameterSetter->Label3->Caption = methodChooser->Label1->Caption + methodChooser->ComboBox1->ItemIndex;
	int index = Form5->selectSensorComboBox->ItemIndex;
	Sensor* s = Form5->sensors[index];


	if(Chooser->state == 1){  // IT
		parameterSetter->Edit1->Text = FloatToStr(s->ITpara1);
		parameterSetter->Edit2->Text = FloatToStr(s->ITpara2);
	}else if(Chooser->state == 2){
		parameterSetter->Edit1->Text = s->STpara1;
		parameterSetter->Edit2->Text = s->STpara2;
	}

	switch(methodChooser->ComboBox1->ItemIndex){
		case 0:                                       // normal
			parameterSetter->Label1->Caption = "mean";
			parameterSetter->Label2->Caption = "stdvar";
			parameterSetter->Label2->Visible = true;
			parameterSetter->Edit2->Visible = true;
			break;

		case 1:
			{AnsiString rep = "lambda";                // exponential
			if(Chooser->state == 2){
				rep = "mu";
			}
			parameterSetter->Label1->Caption = rep;
			parameterSetter->Label2->Visible = false;
			parameterSetter->Edit2->Visible = false;
			break;}

		case 2:                                     // uniform
			parameterSetter->Label1->Caption = "a";
			parameterSetter->Label2->Caption = "b";
			parameterSetter->Label2->Visible = true;
			parameterSetter->Edit2->Visible = true;
			break;

		default:
			break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TparameterSetter::Button1Click(TObject *Sender)
{
	int method = methodChooser->ComboBox1->ItemIndex;
	int index = Form5->selectSensorComboBox->ItemIndex;
	Sensor* s = Form5->sensors[index];
	double para1 = StrToFloat(parameterSetter->Edit1->Text);
	double para2 = StrToFloat(parameterSetter->Edit2->Text);
	if(Chooser->state == 1){ // IT
		s->setIT(method, para1, para2);
	}else if(Chooser->state == 2){
		s->setST(method, para1, para2);
	}
	Form5->DebugLabel->Caption = s->toString();
	Close();
}
//---------------------------------------------------------------------------
