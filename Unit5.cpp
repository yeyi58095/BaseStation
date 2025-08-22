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
	sim::Master master;
	int n = sensorAmount;
	// 清掉舊的物件，避免記憶體漏
	for (size_t i = 0; i < sensors.size(); ++i) delete sensors[i];
	sensors.clear();

	// 重建
	for (int i = 0; i < n; ++i) {
		Sensor* s = new Sensor(i);
		s->setArrivalExp(1.0);
		s->setServiceExp(1.5);
		sensors.push_back(s);
	}
	master.setSensors(&sensors);

	// 建 sensors 的地方（維持你現在的寫法）
	//sensors->clear();

	for (int i = 0; i < n; ++i) {
		Sensor* s = new Sensor(i);
		s->setArrivalExp(1.0);  // 每條線自己的 λ
		s->setServiceExp(1.5);  // 每條線自己的 μ
		sensors.push_back(s);
	}
	master.setSensors(&sensors);
	master.setEndTime(10000);
	master.setSwitchOver(0.0);  // 平行線通常 0

	// 跑
	master.run_parallel();

	// 報表（總量）
	double T = master.now;
	int N = (int)sensors.size();

	double Lq_total   = (T>0) ? master.sumQ / T       : 0.0;
	double L_total    = (T>0) ? master.sumSystem / T  : 0.0;
	double busy_avg   = (T>0 && N>0) ? master.busySum / (T * N) : 0.0; // 平均每台忙碌率
	double thr_total  = (T>0) ? (double)master.served   / T : 0.0;      // 總吞吐
	double lambda_tot = (T>0) ? (double)master.arrivals / T : 0.0;

	AnsiString msg;
	msg += "T            = " + FloatToStrF(T, ffFixed, 7, 2) + "\n";
	msg += "served       = " + IntToStr(master.served) + "\n";
	msg += "arrivals     = " + IntToStr(master.arrivals) + "\n";
	msg += "Lq_total     = " + FloatToStrF(Lq_total,  ffFixed, 7, 4) + "\n";
	msg += "L_total      = " + FloatToStrF(L_total,   ffFixed, 7, 4) + "\n";
	msg += "busy_avg     = " + FloatToStrF(busy_avg,  ffFixed, 7, 4) + "  (平均每台)\n";
	msg += "throughput   = " + FloatToStrF(thr_total, ffFixed, 7, 4) + "  (總)\n";
	msg += "lambda_tot   = " + FloatToStrF(lambda_tot,ffFixed, 7, 4) + "\n";

	// 只有當所有 Sensor 的 IT/ST 都是 Exponential 時做理論對照
	bool theory_ok = true;
	double Lq_th_sum = 0.0, L_th_sum = 0.0;
	for (int i = 0; i < N; ++i) {
		Sensor* s = sensors[i];
		if (s->ITdistri != DIST_EXPONENTIAL || s->STdistri != DIST_EXPONENTIAL) { theory_ok = false; break; }
		double lambda = s->ITpara1;
		double mu     = s->STpara1;
		double rho    = lambda / mu;
		if (rho >= 1.0) { theory_ok = false; break; }
		Lq_th_sum += (rho*rho) / (1.0 - rho);
		L_th_sum  +=  rho      / (1.0 - rho);
	}
	if (theory_ok) {
		msg += "— Theory (sum of per-queue M/M/1) —\n";
		msg += "Lq_total(th) = " + FloatToStrF(Lq_th_sum, ffFixed, 7, 4) + "\n";
		msg += "L_total(th)  = " + FloatToStrF(L_th_sum,  ffFixed, 7, 4) + "\n";
	} else {
		msg += "(No closed-form theory: non-exp or some rho>=1)\n";
	}
	SaveMsgToFile(msg, "report.txt");
	ShowMessage(msg);
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




