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
    // 1) 先清 UI
    selectSensorComboBox->Clear();

    // 2) 停止/重置 master（最簡單：直接重建一個）
	//master = sim::Master();           // 觸發預設建構，等同 reset
	//master.setEndTime(10000);         // 依需要再設參數
	//master.setSwitchOver(0.0);

    // 3) 釋放舊的 sensors
    for (size_t i = 0; i < sensors.size(); ++i) {
        delete sensors[i];
    }
    sensors.clear();

    // 4) 從 UI 讀數量（這裡示範從一個 Edit 讀）
    int n = StrToIntDef(sensorAmountEdit->Text, 1);
    if (n < 0) n = 0;

    sensors.reserve(n);
    for (int i = 0; i < n; ++i) {
        // 建新 sensor，讓它在建構時 register 到 master（依你的設計）
		Sensor* s = new Sensor(i);   // 或 new Sensor(i) 看你的建構子
        // 給預設分佈（之後可從 UI 餵）
        s->setArrivalExp(1.0);            // λ
        s->setServiceExp(1.5);            // μ
        sensors.push_back(s);

        // UI: ComboBox 加上顯示用文字（Add 要字串）
        selectSensorComboBox->Items->Add(IntToStr(i));
    }

    if (n > 0) selectSensorComboBox->ItemIndex = 0;
}

//---------------------------------------------------------------------------

void __fastcall TForm5::DubugClick(TObject *Sender)
{
	/*Sensor* s1 = new Sensor(1);
	Sensor* s2 = new Sensor(2);
	//s1->setIT(1, 2);
	//s1->setST(2, 1.2, 4);

	AnsiString msg = "" ;
	//msg += s1->toString();
	//msg += "\n";
	//msg += s2->toString();

	 std::vector<int> v;
	 for(int i = 0; i< 3; i++){
		v.push_back(i);
	 }

	 for(int i = 0; i < v.size(); i++){
		msg += IntToStr(v[i]) + ", ";
	 }
	Form5->DebugLabel->Caption = msg;   */

	std::vector<Sensor*> sensors;
	sim::Master master;

	// generatorButtonClick 裡：
	for (int i = 0; i < sensorAmount; ++i) {
		Sensor* s = new Sensor(i);
		s->setArrivalExp(1.0);
		s->setServiceExp(1.5);
		sensors.push_back(s);
		selectSensorComboBox->Items->Add(IntToStr(i+1));
	}
	master.setSensors(&sensors);
	master.setEndTime(10000);
	master.setSwitchOver(0.0);

	// 跑
	master.run();
	double meanQ = (master.now > 0) ? (master.sumQ / master.now) : 0.0;
	ShowMessage("served=" + IntToStr(master.served) +
				", meanQ=" + FloatToStrF(meanQ, ffFixed, 7, 4));
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




