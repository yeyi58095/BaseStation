//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <vector>
#include "Unit5.h"
#include "RandomVar.h"
#include "Sensor.h"
#include "ChooseITorST.h"

#include "Person.h"
//#include "MMEngine.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm5 *Form5;
int sensorAmount;
// 建議：在表單成員放
std::vector<Sensor*> sensors;
sim::Master master;
//---------------------------------------------------------------------------
__fastcall TForm5::TForm5(TComponent* Owner)
	: TForm(Owner)
{
	sensorAmount = 1;
	Form5->selectSensorComboBox->ItemIndex = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm5::Button1Click(TObject *Sender)
{
	Person* p = new Person(L"Ji", 12);

	double sum = 0;
	int n = 100000;

	for(int i = 0; i < n; i++){
		sum += rv::exponential(1);
	}
	double result = (double) sum / (double) n;

	ShowMessage(result);


	delete p;
}
//---------------------------------------------------------------------------

void __fastcall TForm5::sensorAmountEditChange(TObject *Sender)
{
	if(Form5->sensorAmountEdit->Text != ""){
		sensorAmount = StrToInt(Form5->sensorAmountEdit->Text);
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm5::generatorButtonClick(TObject *Sender)
{
	    // 清掉舊 sensor
    for (size_t i=0;i<sensors.size();++i) delete sensors[i];
	sensors.clear();
	this->selectSensorComboBox->Clear();
	this->selectVisitComboBox->Clear();
	int n = StrToIntDef(sensorAmountEdit->Text, 1);
	for (int i=0;i<n;++i) {
        Sensor* s = new Sensor(i);
        s->setArrivalExp(1.0);  // λ_i，UI 再改
        s->setServiceExp(1.5);  // μ_i，UI 再改
        sensors.push_back(s);
		selectSensorComboBox->Items->Add(IntToStr(i+1));
		selectVisitComboBox->Items->Add(IntToStr(i+1));
	}
    master.setSensors(&sensors);
    master.setEndTime(10000);
	if (n>0) selectSensorComboBox->ItemIndex = 0;
}

//---------------------------------------------------------------------------

// for setting as txt file
#include <System.Classes.hpp>  // TStringList

void SaveMsgToFile(const AnsiString& msg, const AnsiString& fileName)
{
	TStringList* sl = new TStringList();
	sl->Text = msg;
    sl->SaveToFile(fileName);   // 預設 ANSI；要 UTF-8 可改用 TEncoding
    delete sl;
}


void __fastcall TForm5::DubugClick(TObject *Sender)
{
	rv::reseed(12345);
	master.run();

	AnsiString all = master.reportAll();
	ShowMessage(all);
}
//---------------------------------------------------------------------------


void __fastcall TForm5::selectSensorComboBoxChange(TObject *Sender)
{
	Sensor* s =  sensors[Form5->selectSensorComboBox->ItemIndex];
	AnsiString msg  = s->toString();
	Form5->DebugLabel->Caption = msg;
	Chooser->Show();
}
//---------------------------------------------------------------------------






void __fastcall TForm5::selectVisitComboBoxChange(TObject *Sender)
{
	int sid = this->selectVisitComboBox->ItemIndex;
	AnsiString msg = master.reportOne(sid);
	ShowMessage(msg);
}
//---------------------------------------------------------------------------

