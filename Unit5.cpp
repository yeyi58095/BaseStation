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
void PlotTraceOne(int);
void PlotTraceAll();
__fastcall TForm5::TForm5(TComponent* Owner)
	: TForm(Owner)
{
	this->Dubug->Caption = "Run";
	sensorAmount = 1;
	Form5->selectSensorComboBox->ItemIndex = 0;
}
//---------------------------------------------------------------------------
/*void __fastcall TForm5::Button1Click(TObject *Sender)
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
}  */
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
		s->setArrivalExp(0.4);  // λ_i，UI 再改
		s->setServiceExp(1.5);  // μ_i，UI 再改
		sensors.push_back(s);
		selectSensorComboBox->Items->Add(IntToStr(i+1));
	}

    selectVisitComboBox->Clear();
	selectVisitComboBox->Items->Add("All sensors"); // index 0
	for (int i=0;i<n;++i)
		selectVisitComboBox->Items->Add("Sensor " + IntToStr(i+1));
	selectVisitComboBox->ItemIndex = 0;

    master.setSensors(&sensors);
    double T = StrToFloatDef(endTimeEdit->Text, 10000.0);
	master.setEndTime(T);
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
	this->Dubug->Caption = "Run";
	rv::reseed(12345);

	master.run();

	//AnsiString all = master.reportAll();
	//ShowMessage(all);
	AnsiString msg = FloatToStr(master.switchover)+ " \n" + master.reportOne(0);
	SaveMsgToFile(msg, "report.txt");
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
	if(sid == 0){
		AnsiString msg = master.reportAll();
		this->log->Caption = msg;
		 PlotTraceAll();
		return ;
	}
	PlotTraceOne(sid-1);
	AnsiString msg = IntToStr(master.maxChargingSlots) + "\n" + master.reportOne(sid - 1);
	//ShowMessage(msg);
	this->log->Caption = msg;
	}
//---------------------------------------------------------------------------

void PlotTraceAll() {
    Form5->SeriesCurrent->Clear();
    Form5->SeriesMean->Clear();

    Form5->Chart1->BottomAxis->Title->Caption = "Time";
	Form5->Chart1->LeftAxis->Title->Caption   = "Queue size";

	const std::vector<double>& t = master.traceT_all;
	const std::vector<double>& q = master.traceQ_all;
	const std::vector<double>& m = master.traceMeanQ_all;

	for (size_t k=0;k<t.size();++k) {
		Form5->SeriesCurrent->AddXY(t[k], q[k], "", clRed);
		Form5->SeriesMean->AddXY(t[k],    m[k], "", clBlue);
	}
}

void PlotTraceOne(int sid) {
	Form5->SeriesCurrent->Clear();
	Form5->SeriesMean->Clear();

	Form5->Chart1->BottomAxis->Title->Caption = "Time";
	Form5->Chart1->LeftAxis->Title->Caption   = "Queue size";

	const std::vector<double>& t = master.traceT[sid];
	const std::vector<double>& q = master.traceQ[sid];
	const std::vector<double>& m = master.traceMeanQ[sid];

	for (size_t k=0;k<t.size();++k) {
		Form5->SeriesCurrent->AddXY(t[k], q[k], "", clRed);
		Form5->SeriesMean->AddXY(t[k],    m[k], "", clBlue);
	}
}

void __fastcall TForm5::plotButtonClick(TObject *Sender)
{
		 PlotTraceAll();
}
//---------------------------------------------------------------------------


void __fastcall TForm5::endTimeEditChange(TObject *Sender)
{
	double T = StrToFloatDef(endTimeEdit->Text, master.endTime);
	master.setEndTime(T);

}
//---------------------------------------------------------------------------

void __fastcall TForm5::switchOverEditChange(TObject *Sender)
{
	double sw = StrToFloatDef(this->switchOverEdit->Text, master.switchover);
	master.setSwitchOver(sw);
}
//---------------------------------------------------------------------------


void __fastcall TForm5::maxCharginSlotEditChange(TObject *Sender)
{
	int mc = StrToIntDef(this->maxCharginSlotEdit->Text, master.maxChargingSlots);
	master.setChargingSlots(mc);
}
//---------------------------------------------------------------------------



