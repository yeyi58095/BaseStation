//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <vector>
#include "Unit5.h"
#include "RandomVar.h"
#include "Sensor.h"
#include "ChooseITorST.h"
#include "setAllParaDialog.h"
#include "ReplayDialog.h"

#include "Person.h"
//#include "MMEngine.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm5 *Form5;
int sensorAmount;
// ��ĳ�G�b��榨����
std::vector<Sensor*> sensors;
sim::Master master;
void PlotTraceOne(int);
void PlotTraceAll(bool);

__fastcall TForm5::TForm5(TComponent* Owner)
	: TForm(Owner)
{
	this->runned = false;
	this->Dubug->Caption = "Run";
	this->DebugLabel->Caption = "";
	sensorAmount = 1;
	Form5->selectSensorComboBox->ItemIndex = 0;
	this->generatorButton->Visible = false;
	this->LogMode->ItemIndex = 2;
	this->plotButton->Caption = "reZoom";
	this->leftPanel->Caption = "";
	this->Label5->Visible = false;
	this->maxCharginSlotEdit->Visible = false;
	this->Label6->Visible = false;

	// for plot
		// ���b�GQueue
	Chart1->LeftAxis->Title->Caption   = "Queue size";
	SeriesCurrent->Title = "Q (current)";
	SeriesMean->Title    = "Q (mean)";
	SeriesCurrent->Stairs = true;  // ���説��ı
	SeriesMean->Stairs    = true;

	// �k�b�GEnergy
	Chart1->RightAxis->Visible = true;
	Chart1->RightAxis->Title->Caption = "Energy (EP)";
	SeriesEP->Title        = "EP";
	SeriesThreshold->Title = "r_tx";
	SeriesEP->VertAxis        = aRightAxis;
	SeriesThreshold->VertAxis = aRightAxis;
	SeriesEP->Stairs = true;
	SeriesThreshold->Stairs = true;

	//�]�i��^�ϨҦnŪ
	Chart1->Legend->Visible = true;

	this->SeriesThreshold->Active = false;
	this->SeriesEP->Active = false;
	this->chkQ->Checked = true;
	this->ckbMean->Checked = true;

	this->generatorButtonClick(NULL);
}

//---------------------------------------------------------------------------

void __fastcall TForm5::sensorAmountEditChange(TObject *Sender)
{
	if(Form5->sensorAmountEdit->Text != ""){
		sensorAmount = StrToInt(Form5->sensorAmountEdit->Text);
	}
	if(sensorAmount >= 4){
		master.logStateEachEvent = false;
	}
	this->generatorButtonClick(NULL);
}
//---------------------------------------------------------------------------

void __fastcall TForm5::generatorButtonClick(TObject *Sender)
{
		// �M���� sensor
	runned = false;
	for (size_t i=0;i<sensors.size();++i) delete sensors[i];
	sensors.clear();
	this->selectSensorComboBox->Clear();
	this->selectVisitComboBox->Clear();
	int n = StrToIntDef(sensorAmountEdit->Text, 1);
	for (int i=0;i<n;++i) {
		Sensor* s = new Sensor(i);
		s->setArrivalExp(0.4);  // �f_i�AUI �A��
		s->setServiceExp(1.5);  // �g_i�AUI �A��

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

	// for plot
		// ���b�GQueue
	Chart1->LeftAxis->Title->Caption   = "Queue size";
	SeriesCurrent->Title = "Q (current)";
	SeriesMean->Title    = "Q (mean)";
	SeriesCurrent->Stairs = true;  // ���説��ı
	SeriesMean->Stairs    = true;

	// �k�b�GEnergy
	Chart1->RightAxis->Visible = true;
	Chart1->RightAxis->Title->Caption = "Energy (EP)";
	SeriesEP->Title        = "EP";
	SeriesThreshold->Title = "r_tx";
	SeriesEP->VertAxis        = aRightAxis;
	SeriesThreshold->VertAxis = aRightAxis;
	SeriesEP->Stairs = true;
	SeriesThreshold->Stairs = true;

	//�]�i��^�ϨҦnŪ
	Chart1->Legend->Visible = true;

}

//---------------------------------------------------------------------------

// for setting as txt file
#include <System.Classes.hpp>  // TStringList

void SaveMsgToFile(const AnsiString& msg, const AnsiString& fileName)
{
	TStringList* sl = new TStringList();
	sl->Text = msg;
	sl->SaveToFile(fileName);   // �w�] ANSI�F�n UTF-8 �i��� TEncoding
	delete sl;
}

void __fastcall TForm5::DubugClick(TObject *Sender)
{
    this->Dubug->Caption = "Run";
    rv::reseed(12345);
    master.run();
    runned = true;

	// ø�ϡB����]�������r�^
    PlotTraceAll(true);
	AnsiString msg = master.reportAll();
    this->log->Caption = msg;

	AnsiString left = master.leftPanelSummary();
	this->leftPanel->Caption = left;

    // �g�ɡG���i / log�]�Y���ݭn�^
	SaveMsgToFile(master.dumpLogWithSummary(), "run_log.txt");

	// >>> �Y�A�b UI ��@�� CheckBox�GFreeAfterRun
   //	if (this->FreeAfterRunCheckBox->Checked) {


        // �p�G�A������ɮ� Replay�A�� sensors �]�i�@�_�R�C
		// �Y ReplayDialog �|����Ū CSV/HUMAN �ɡA���̿ଡ�� master�A�N�] false
		//master.purgeHeavyData(true); // �R�����
		// �]�i�Gmaster.purgeHeavyData(true); // �R����Ʀ��O�d sensors ���Х���
   //	}
					master.shrinkToPlotOnly(true);
    this->plotButtonClick(NULL);
}

//--------------------------------------------------------------------------


void __fastcall TForm5::selectSensorComboBoxChange(TObject *Sender)
{
    if (sensors.empty()) { this->log->Caption = "No sensors."; return; }
	int idx = Form5->selectSensorComboBox->ItemIndex;
    if (idx < 0 || idx >= (int)sensors.size()) { this->log->Caption = "Invalid sensor index."; return; }
    Sensor* s = sensors[idx];
    AnsiString msg = s->toString();
    Form5->log->Caption = msg;
}

//---------------------------------------------------------------------------

void __fastcall TForm5::selectVisitComboBoxChange(TObject *Sender)
{
    if (master.traceT.empty()) { this->log->Caption = "No trace."; return; }
    int sid = this->selectVisitComboBox->ItemIndex;
    if (sid == 0) {
        AnsiString msg = master.reportAll();
        this->log->Caption = msg;
        PlotTraceAll(true);
        return;
    }
    if (sid - 1 < 0 || sid - 1 >= (int)master.traceT.size()) { this->log->Caption = "Invalid trace index."; return; }
    PlotTraceOne(sid - 1);
    AnsiString msg = master.reportOne(sid - 1);
    this->log->Caption = msg;
}

//---------------------------------------------------------------------------



void PlotTraceAll(bool epAverage = true) {
	Form5->SeriesCurrent->Clear();
	Form5->SeriesMean->Clear();
	Form5->SeriesEP->Clear();
	Form5->SeriesThreshold->Clear(); // ����ɳq�`���Ϊ��e�u

	Form5->Chart1->Title->Caption = epAverage ? "All sensors (EP avg)" : "All sensors (EP sum)";

	const std::vector<double>& t  = master.traceT_all;
	const std::vector<double>& q  = master.traceQ_all;
	const std::vector<double>& m  = master.traceMeanQ_all;
	const std::vector<double>& e  = epAverage ? master.traceEavg_all : master.traceE_all;

	const int n = (int)t.size();
	for (int k = 0; k < n; ++k) {
		Form5->SeriesCurrent->AddXY(t[k], q[k], "", clRed);
		Form5->SeriesMean   ->AddXY(t[k], m[k], "", clBlue);
		Form5->SeriesEP     ->AddXY(t[k], e[k], "", clGreen);
	}
	//Form5->SeriesEP->Active = false;
	Form5->ckbEP->Checked = false;

	Form5->SeriesThreshold->Active = false;
	Form5->ckbRtx->Checked = false;

}

void PlotTraceOne(int sid) {
	if (sid < 0 || sid >= (int)master.traceT.size()) return;

	Form5->SeriesCurrent->Clear();
	Form5->SeriesMean->Clear();
	Form5->SeriesEP->Clear();
	Form5->SeriesThreshold->Clear();

	Form5->Chart1->Title->Caption = "Sensor " + IntToStr(sid+1);

	const std::vector<double>& t  = master.traceT[sid];
	const std::vector<double>& q  = master.traceQ[sid];
	const std::vector<double>& m  = master.traceMeanQ[sid];
	const std::vector<double>& e  = master.traceE[sid];      // �A��[�� EP trace
	const std::vector<double>& rt = master.traceRtx[sid];    // r_tx trace

	const int n = (int)t.size();
	for (int k = 0; k < n; ++k) {
		Form5->SeriesCurrent->AddXY(t[k], q[k],  "", clRed);
		Form5->SeriesMean   ->AddXY(t[k], m[k],  "", clBlue);
		Form5->SeriesEP     ->AddXY(t[k], e[k],  "", clGreen);
		Form5->SeriesThreshold->AddXY(t[k], rt[k], "", (TColor)0x00808080);
	}
}

void __fastcall TForm5::plotButtonClick(TObject *Sender)
{
		     // �@���h�짹���S���Y��
    while (Chart1->Zoomed) {
        Chart1->UndoZoom();
    }

    // �O�I�G��y�жb�]�^�۰ʡ]�קK����ʳ̤p/�̤j�ݯd�^
    Chart1->BottomAxis->Automatic = true;
    Chart1->LeftAxis->Automatic   = true;
    // �p�G���Υk�b/�W�b�]�@�ֳ]�w
    Chart1->RightAxis->Automatic  = true;
	Chart1->TopAxis->Automatic    = true;
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


void __fastcall TForm5::chkQClick(TObject *Sender)
{
	Form5->SeriesCurrent->Active = Form5->chkQ->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TForm5::ckbMeanClick(TObject *Sender)
{
	this->SeriesMean->Active = this->ckbMean->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TForm5::ckbRtxClick(TObject *Sender)
{
	this->SeriesThreshold->Active = this->ckbRtx->Checked;
	if(Form5->selectVisitComboBox->ItemIndex == 0){
	///Form5->SeriesEP->Active = false;
	//Form5->ckbEP->Checked = false;

	Form5->SeriesThreshold->Active = false;
	Form5->ckbRtx->Checked = false;}
}
//---------------------------------------------------------------------------

void __fastcall TForm5::ckbEPClick(TObject *Sender)
{
	this->SeriesEP->Active = this->ckbEP->Checked;
	/*if(Form5->selectVisitComboBox->ItemIndex == 0){
	Form5->SeriesEP->Active = false;
	Form5->ckbEP->Checked = false;

	Form5->SeriesThreshold->Active = false;
	Form5->ckbRtx->Checked = false;} */
}
//---------------------------------------------------------------------------
void __fastcall TForm5::setParaButtonClick(TObject *Sender)
{
	setAllPara->Show();
}
//---------------------------------------------------------------------------



void __fastcall TForm5::LogModeClick(TObject *Sender)
{
    switch (this->LogMode->ItemIndex) {
        case 0: master.logMode = sim::Master::LOG_CSV;   break;
        case 1: master.logMode = sim::Master::LOG_HUMAN; break;
		case 2: master.logMode = sim::Master::LOG_NONE;  break;
        default: master.logMode = sim::Master::LOG_CSV;  break;
    }

	// �Y��� NONE�A���F�A�֤@�I�A�i�����v�ƥ�ַ�
    if (master.logMode == sim::Master::LOG_NONE) {
        master.logStateEachEvent = false;
    } else {
        // �̧A���ݨD�۰ʶ}�ҩΫO�����
        master.logStateEachEvent = (sensorAmount < 4);  // �A�쥻������
    }
}




void __fastcall TForm5::replayButtonClick(TObject *Sender)
{
	Replay->Show();
}
//---------------------------------------------------------------------------



void __fastcall TForm5::FreeMemoryClick(TObject *Sender)
{
  master.shrinkToPlotOnly(true);
}
//---------------------------------------------------------------------------


