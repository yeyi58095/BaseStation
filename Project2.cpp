//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include <stdio.h>
#include <fstream>
#include <string>     // std::string
#include <string.h>   // strstr
#include <time.h>     // time for default seed
#include <System.SysUtils.hpp> // ParamCount / ParamStr / StrToIntDef / StrToFloatDef

#include "HeadlessBridge.h"
//---------------------------------------------------------------------------
USEFORM("ReplayDialog.cpp", Replay);
USEFORM("setAllParaDialog.cpp", setAllPara);
USEFORM("Unit5.cpp", Form5);
USEFORM("ChooseMethodDialog.cpp", methodChooser);
USEFORM("CharterDialog.cpp", Charter);
USEFORM("ChooseITorST.cpp", Chooser);
USEFORM("ParaSetterDialog.cpp", parameterSetter);
//---------------------------------------------------------------------------

// ---- CLI 讀取工具 ----
static bool GetArgD(const UnicodeString &key, double &val) {
    for (int i = 1; i <= ParamCount(); ++i) {
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
    for (int i = 1; i <= ParamCount(); ++i) {
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
    for (int i = 1; i <= ParamCount(); ++i) {
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
    for (int i = 1; i <= ParamCount(); ++i) {
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
    for (int i = 1; i <= ParamCount(); ++i) {
        if (ParamStr(i) == L"--" + flag) return true;
    }
    return false;
}

// =============================
// Headless 執行流程
// =============================
static int DoHeadless() {
    // 既有預設
    double mu = 3.0, e = 2.4, lambda = 1.2;
    int    C  = 5, T = 10000;
    unsigned int seed = (unsigned int)time(NULL);
    AnsiString outPath = "result.csv";

    AnsiString policy = "rr";

    int N = 1;
    int r_tx = 1;                  // 啟動門檻（維持舊參數）
    int slots = 0;
    int alwaysChargeFlag = 1;      // GUI 預設 true
    AnsiString versionStr = "BaseStation";

    int Qmax = 100000;  // 預設（舊相容）

    double tau  = 0.0;

    // 距離／rBase 旗標（全可省略＝完全舊行為）
    int useD = 0;
    AnsiString dDist = "";
    double dP1 = 0.0, dP2 = 0.0;
    unsigned int dseed = 0;
    double pt = 1.0, alpha = 1.0;
    int rBase = -1;

    // ================================
    // 讀取 CLI 參數
    // ================================
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
    // alwaysCharge: 有 flag → 1，沒 flag → 0
    if (HasFlag(L"alwaysCharge")) alwaysChargeFlag = 1; else alwaysChargeFlag = 0;

    if (GetArgS(L"version", tmps)) versionStr = tmps;
    if (GetArgI(L"Qmax", tmpi)) Qmax = tmpi;
    if (GetArgI(L"Q", tmpi)) Qmax = tmpi;
    if (GetArgD(L"tau",  tmpd)) tau  = tmpd;
    if (GetArgS(L"policy", tmps)) policy = tmps;

    // 距離相關
    if (GetArgI(L"useD", tmpi)) useD = tmpi;
    if (GetArgS(L"dDist", tmps)) dDist = tmps;
    if (GetArgD(L"dP1", tmpd)) dP1 = tmpd;
    if (GetArgD(L"dP2", tmpd)) dP2 = tmpd;
    if (GetArgU(L"dseed", tmpu)) dseed = tmpu;
    if (GetArgD(L"pt", tmpd)) pt = tmpd;
    if (GetArgD(L"alpha", tmpd)) alpha = tmpd;
    if (GetArgI(L"rBase", tmpi)) rBase = tmpi;

    // ================================
    // Log 模式解析：--log=none|csv|human
    // ================================
    AnsiString logModeStr = "";
    if (GetArgS(L"log", logModeStr)) {
        AnsiString mode = logModeStr.LowerCase();
        if (mode == "none") {
            HB_LogNone();
        } else if (mode == "csv") {
            HB_LogCSV();
        } else if (mode == "human") {
            HB_LogHuman();
        } else {
            HB_LogNone();
        }
    } else {
        HB_LogNone(); // 預設不記 log（跑圖時用）
    }

    // ================================
    // 距離 / rBase 策略設定
    // ================================
    HB_ResetDistancePolicy();

    if (!useD && rBase >= 0) {
        HB_SetFixedRBase(rBase);
    }

    AnsiString modeNorm = dDist.LowerCase();
    if (modeNorm == "uni") modeNorm = "uniform";
    else if (modeNorm == "norm") modeNorm = "normal";
    else if (modeNorm == "logn") modeNorm = "lognormal";
    else if (modeNorm == "exp")  modeNorm = "exponential";

    if (useD) {
        if (modeNorm == "" || modeNorm == "uniform") {
            // dP1,dP2 = min,max
            HB_SetRandomDistance("uniform", dP1, dP2, 0.0, 0.0, dseed, pt, alpha);
        } else if (modeNorm == "normal") {
            HB_SetRandomDistance("normal", 0.0, 0.0, dP1, dP2, dseed, pt, alpha);
        } else if (modeNorm == "lognormal") {
            HB_SetRandomDistance("lognormal", 0.0, 0.0, dP1, dP2, dseed, pt, alpha);
        } else if (modeNorm == "exponential") {
            HB_SetRandomDistance("exponential", 0.0, 0.0, dP1, 0.0, dseed, pt, alpha);
        }
    }

    if (!useD && rBase >= 0) {
        // 距離模式關掉時，用 power-law 的預設（這裡用 R=1）
        HB_SetPowerLaw(1.0, 0.0);
    }

    // Queue / tau / policy
    HB_SetQueueMax(Qmax);
    HB_SetSwitchOver(tau);
    HB_SetPolicy(policy.c_str());

    // ================================
    // Run
    // ================================
    int rc = RunHeadlessEngine(mu, e, C, lambda, T, seed,
                               N, r_tx, slots, alwaysChargeFlag,
                               outPath.c_str(), versionStr.c_str());

    // headless_log.txt 由 HeadlessBridge 裡的 RunSimulationCore() 在 log=human 時自動產生
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
    catch (...) {
        try { throw Exception(""); }
        catch (Exception &exception) { Application->ShowException(&exception); }
    }
    return 0;
}
//---------------------------------------------------------------------------

