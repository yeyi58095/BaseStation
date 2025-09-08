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
		ShowMessage(IntToStr(t->ItemIndex));
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
	this->ComboBox1->ItemIndex = s->ITdistri;
	this->ComboBox2->ItemIndex = s->STdistri;

	manageVisible(this->ComboBox1);
	manageVisible(this->ComboBox2);
}
//---------------------------------------------------------------------------

