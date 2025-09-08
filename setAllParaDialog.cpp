//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "Unit5.h"
#include "Sensor.h"
#include "setAllParaDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TsetAllPara *setAllPara;
Sensor *s;
void manageVisible(TComboBox* t);
//---------------------------------------------------------------------------
__fastcall TsetAllPara::TsetAllPara(TComponent* Owner)
	: TForm(Owner)
{

}

void manageVisible(TComboBox* t){
	if(t == setAllPara->ComboBox1){
		if(t->ItemIndex == 1){
			setAllPara->ITpara2Edit->Visible = false;
			setAllPara->ITpara2->Visible = false;
		}else{
			setAllPara->ITpara2Edit->Visible = true;
			setAllPara->ITpara2->Visible = true;
		}

		switch(t->ItemIndex){
			case 0: // uniform
				setAllPara->ITpara1->Caption = "mean";
				setAllPara->ITpara2->Caption = "stdvar";
				break;

			case 1:
				setAllPara->ITpara1->Caption = "lambda";
				break;

			case 2:
				setAllPara->ITpara1->Caption = "a";
				setAllPara->ITpara2->Caption = "b";
				break;

			default:
				break;
		}

	}

	if(t == setAllPara->ComboBox2){
		if(t->ItemIndex == 1){
			setAllPara->STpara2Edit->Visible = false;
			setAllPara->STpara2->Visible = false;
		}else{
			setAllPara->STpara2Edit->Visible = true;
			setAllPara->STpara2->Visible = true;
		}

		switch(t->ItemIndex){
			case 0:
				setAllPara->STpara1->Caption = "mean";
				setAllPara->STpara2->Caption = "stdvar";
				break;

			case 1:
				setAllPara->STpara1->Caption = "mu";
				break;

			case 2:
				setAllPara->STpara1->Caption = "a";
				setAllPara->STpara2->Caption = "b";
				break;

			default:
				break;
		}
	}
}

//---------------------------------------------------------------------------
void __fastcall TsetAllPara::OnShow(TObject *Sender)
{
	if(Form5->sensorAmountEdit->Text != 0){
		s = Form5->sensors[0];
	}
	this->BufferSizeEdit->Text = s->Qmax;
	this->ComboBox1->ItemIndex = s->ITdistri;
	this->ComboBox2->ItemIndex = s->STdistri;

	this->ITpara1Edit->Text = s->ITpara1;
	this->ITpara2Edit->Text = s->ITpara2;

	this->STpara1Edit->Text = s->STpara1;
	this->STpara2Edit->Text = s->STpara2;

	manageVisible(this->ComboBox1);
	manageVisible(this->ComboBox2);
}
//---------------------------------------------------------------------------

void __fastcall TsetAllPara::OKClick(TObject *Sender)
{
	int N = Form5->sensors.size();

	for(int i = 0; i < N; i++){
		Sensor *sensor = Form5->sensors[i];
		sensor->Qmax = StrToIntDef(this->BufferSizeEdit->Text, 10);
		sensor->ITdistri = this->ComboBox1->ItemIndex;
		sensor->ITpara1 = StrToFloat(this->ITpara1Edit->Text);
		sensor->ITpara2 = StrToFloat(this->ITpara2Edit->Text);

		sensor->STdistri = this->ComboBox2->ItemIndex;
		sensor->STpara1 = StrToFloat(this->STpara1Edit->Text);
		sensor->STpara2 = StrToFloat(this->STpara2Edit->Text);
	}


	Form5->DebugLabel->Caption = Form5->sensors[0]->toString();
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TsetAllPara::ComboBox1Change(TObject *Sender)
{
	manageVisible(setAllPara->ComboBox1);
}
//---------------------------------------------------------------------------
void __fastcall TsetAllPara::ComboBox2Change(TObject *Sender)
{
	manageVisible(setAllPara->ComboBox2);
}
//---------------------------------------------------------------------------
