//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include <stdio.h>
#include <fstream>
#include <string.h>   // strstr
#include <time.h>     // time for default seed
#include <System.SysUtils.hpp> // ParamCount / ParamStr / StrToIntDef / StrToFloatDef

#include "HeadlessBridge.h" // <— 確保先包含（我們會用 HB_*）
//---------------------------------------------------------------------------
USEFORM("ReplayDialog.cpp", Replay);
USEFORM("setAllParaDialog.cpp", setAllPara);
USEFORM("Unit5.cpp", Form5);
USEFORM("ChooseMethodDialog.cpp", methodChooser);
USEFORM("CharterDialog.cpp", Charter);
USEFORM("ChooseITorST.cpp", Chooser);
USEFORM("ParaSetterDialog.cpp", parameterSetter);
//---------------------------------------------------------------------------

// 舊 signature：維持不變（距離策略由 HB_* 設）
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    int N, int r_tx, int slots, int alwaysChargeFlag,
    const char* outPath, const char* versionStr
);

// ---- CLI 讀取工具 ----
static bool GetArgD(const UnicodeString &key, double &val) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
        UnicodeString s = ParamStr(i);
        UnicodeString prefix = L"--" + key + L"=";
        if (s.Pos(prefix) == 1) {
            UnicodeString num = s.SubString(prefix.Length()+1, s.Length()-prefix.Length());
            try { val = StrToFloat(num); return true; } catch (...) { return false; }
        }
    }
    return false;
}
static bool GetArgI(const UnicodeString &key, int &val) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
        UnicodeString s = ParamStr(i);
        UnicodeString prefix = L"--" + key + L"=";
        if (s.Pos(prefix) == 1) {
            UnicodeString num = s.SubString(prefix.Length()+1, s.Length()-prefix.Length());
            try { val = StrToInt(num); return true; } catch (...) { return false; }
        }
    }
    return false;
}
static bool GetArgU(const UnicodeString &key, unsigned int &val) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
		UnicodeString s = ParamStr(i);
		UnicodeString prefix = L"--" + key + L"=";
		if (s.Pos(prefix) == 1) {
            UnicodeString num = s.SubString(prefix.Length()+1, s.Length()-prefix.Length());
            try {
                int tmp = StrToInt(num);
				if (tmp < 0) tmp = -tmp;
                val = (unsigned int)tmp;
                return true;
            } catch (...) { return false; }
        }
    }
    return false;
}
static bool GetArgS(const UnicodeString &key, AnsiString &val) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
        UnicodeString s = ParamStr(i);
        UnicodeString prefix = L"--" + key + L"=";
        if (s.Pos(prefix) == 1) {
            UnicodeString rest = s.SubString(prefix.Length()+1, s.Length()-prefix.Length());
            val = AnsiString(rest);
            return true;
        }
    }
    return false;
}
static bool HasFlag(const UnicodeString &flag) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
        if (ParamStr(i) == L"--" + flag) return true;
    }
    return false;
}

// 無頭執行流程
static int DoHeadless() {
	std::ofstream flog("headless_log.txt");
	flog << "[Headless] start\n";

	// 既有預設
	double mu = 3.0, e = 2.4, lambda = 1.2;
	int    C  = 5, T = 10000;
	unsigned int seed = (unsigned int)time(NULL);
	AnsiString outPath = "result.csv";

	int N = 1;
	int r_tx = 1;                  // 啟動門檻（維持舊參數）
	int slots = 0;
	int alwaysChargeFlag = 1;      // GUI 預設 true
	AnsiString versionStr = "BaseStation";

	int Qmax = 100000;  // 預設（舊相容）

	double tau  = 0.0;
	// 距離／rBase 旗標（全可省略＝完全舊行為）
	int useD = 0;                 // 1=啟用距離策略
	AnsiString dDist = "";        // "uni|uniform|norm|normal|logn|lognormal|exp|exponential"
	double dP1 = 0.0, dP2 = 0.0;  // 分佈參數（語意由 dDist 決定）
	unsigned int dseed = 0;
	double pt = 1.0, alpha = 1.0; // charge_rate = pt / d^alpha（若你要用）
	int rBase = -1;               // 非距離模式時固定 txCostBase；-1=未指定

	// 讀取 CLI
	double tmpd; int tmpi; unsigned int tmpu; AnsiString tmps;
	if (GetArgD(L"mu", tmpd)) mu = tmpd;
    if (GetArgD(L"e", tmpd))  e  = tmpd;
    if (GetArgI(L"C", tmpi))  C  = tmpi;
    if (GetArgD(L"lambda", tmpd)) lambda = tmpd;
    if (GetArgI(L"T", tmpi))  T = tmpi;
    if (GetArgU(L"seed", tmpu)) seed = tmpu;
    if (GetArgS(L"out", tmps)) outPath = tmps;
	if (GetArgI(L"N", tmpi)) N = tmpi;
    if (GetArgI(L"rtx", tmpi)) r_tx = tmpi;
    if (GetArgI(L"slots", tmpi)) slots = tmpi;
    if (HasFlag(L"alwaysCharge")) alwaysChargeFlag = 1; else alwaysChargeFlag = 0;
	if (GetArgS(L"version", tmps)) versionStr = tmps;
	if (GetArgI(L"Qmax", tmpi)) Qmax = tmpi;
	if (GetArgI(L"Q", tmpi)) Qmax = tmpi;
	if (GetArgD(L"tau",  tmpd)) tau  = tmpd;

    if (GetArgI(L"useD", tmpi)) useD = tmpi;
    if (GetArgS(L"dDist", tmps)) dDist = tmps;
	if (GetArgD(L"dP1", tmpd)) dP1 = tmpd;
    if (GetArgD(L"dP2", tmpd)) dP2 = tmpd;
    if (GetArgU(L"dseed", tmpu)) dseed = tmpu;
    if (GetArgD(L"pt", tmpd)) pt = tmpd;
    if (GetArgD(L"alpha", tmpd)) alpha = tmpd;
    if (GetArgI(L"rBase", tmpi)) rBase = tmpi;

    // 將距離策略設定塞給 HB_*（RunHeadlessEngine 內部會讀）
	HB_ResetDistancePolicy();

	if (!useD && rBase >= 0) {
		HB_SetFixedRBase(rBase);   // <— 這一行是關鍵
	}

	// 正規化 dDist
    AnsiString modeNorm = dDist.LowerCase();
    if (modeNorm == "uni") modeNorm = "uniform";
	else if (modeNorm == "norm") modeNorm = "normal";
	else if (modeNorm == "logn") modeNorm = "lognormal";
	else if (modeNorm == "exp") modeNorm = "exponential";

    if (useD) {
        // 依分佈設定
		if (modeNorm == "" || modeNorm == "uniform") {
            // 預設用 uniform；dP1=dmin, dP2=dmax（容錯：若兩者皆<=0 → 關閉距離）
            if (dP2 > dP1 && dP2 > 0.0) {
                HB_SetRandomDistance("uniform", dP1, dP2, 0.0, 0.0, dseed, pt, alpha);
            } else {
                useD = 0; // 參數不合理，退回舊模式
			}
        } else if (modeNorm == "normal") {
            HB_SetRandomDistance("normal", 0.0, 0.0, dP1, dP2, dseed, pt, alpha);
        } else if (modeNorm == "lognormal") {
            HB_SetRandomDistance("lognormal", 0.0, 0.0, dP1, dP2, dseed, pt, alpha);
        } else if (modeNorm == "exponential") {
			HB_SetRandomDistance("exponential", 0.0, 0.0, dP1, 0.0, dseed, pt, alpha);
        } else {
			useD = 0; // 未知模式 → 關閉距離
		}
	}

	// 如果不用距離、但 rBase 有給 → 固定 txCostBase
	if (!useD && rBase >= 0) {
		HB_SetPowerLaw(1.0, 0.0); // 讓 policy 內部知道不靠距離縮放
		// 其實 rBase 會在 Engine 端套用，不需要 HB 記，但記一下也無妨
	}
					HB_SetQueueMax(Qmax);
					HB_SetSwitchOver(tau);      // <— 新增
	// log
	std::ofstream &L = flog;
	L << "params: mu="<<mu<<", e="<<e<<", C="<<C
	  <<", lambda="<<lambda<<", T="<<T<<", seed="<<seed
	  <<", N="<<N<<", rtx="<<r_tx<<", slots="<<slots
	  <<", alwaysCharge="<<alwaysChargeFlag
	  <<", Q="<<Qmax<<", tau="<<tau
	  <<", out="<<outPath.c_str()
	  <<", version="<<versionStr.c_str()<<"\n";

	L << "distance_policy: useD="<<useD
	  <<", dDist="<<modeNorm.c_str()
	  <<", dP1="<<dP1<<", dP2="<<dP2
	  <<", dseed="<<dseed<<", pt="<<pt<<", alpha="<<alpha
	  <<", rBase="<<rBase << "\n";

	int rc = RunHeadlessEngine(mu, e, C, lambda, T, seed,
							   N, r_tx, slots, alwaysChargeFlag,
							   outPath.c_str(), versionStr.c_str());

	L << "[Headless] done, rc=" << rc << "\n";
	flog.close();
	return rc;
}

//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    try {
        if (HasFlag(L"headless")) return DoHeadless();

        // GUI 模式
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->CreateForm(__classid(TForm5), &Form5);
        Application->CreateForm(__classid(TCharter), &Charter);
        Application->CreateForm(__classid(TChooser), &Chooser);
        Application->CreateForm(__classid(TmethodChooser), &methodChooser);
        Application->CreateForm(__classid(TparameterSetter), &parameterSetter);
        Application->CreateForm(__classid(TsetAllPara), &setAllPara);
        Application->CreateForm(__classid(TReplay), &Replay);
        Application->Run();
    }
    catch (Exception &exception) { Application->ShowException(&exception); }
    catch (...) { try { throw Exception(""); } catch (Exception &exception) { Application->ShowException(&exception); } }
    return 0;
}
//---------------------------------------------------------------------------

