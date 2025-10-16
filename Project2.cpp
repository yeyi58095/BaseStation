//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include <stdio.h>
#include <fstream>
#include <string.h>   // strstr
#include <time.h>     // time for default seed
#include <System.SysUtils.hpp> // ParamCount / ParamStr / StrToIntDef / StrToFloatDef

//---------------------------------------------------------------------------
USEFORM("ReplayDialog.cpp", Replay);
USEFORM("setAllParaDialog.cpp", setAllPara);
USEFORM("Unit5.cpp", Form5);
USEFORM("ChooseMethodDialog.cpp", methodChooser);
USEFORM("CharterDialog.cpp", Charter);
USEFORM("ChooseITorST.cpp", Chooser);
USEFORM("ParaSetterDialog.cpp", parameterSetter);
//---------------------------------------------------------------------------
extern "C" int RunHeadlessEngine(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    const char* outPath
);

// 如果你尚未實作引擎，先提供一個弱化的「偵錯訊息」版本，避免你誤會有結果就是對的。
// 實際要跑正確數字：請刪除下面這個假的實作，並在其他 .cpp 完成真的 RunHeadlessEngine。
static int RunHeadlessEngine_Fallback(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    const char* outPath
) {
    std::ofstream flog("headless_log.txt", std::ios::app);
    flog << "[WARN] RunHeadlessEngine() 未連接真實引擎，請在你的 Engine.cpp 實作它。\n";
    flog << "       當前參數：mu=" << mu << ", e=" << e << ", C=" << C
         << ", lambda=" << lambda << ", T=" << T << ", seed=" << seed << "\n";
    flog.close();
    // 謹慎起見，不寫任何假數字到 outPath，避免混淆。
    return -1;
}

// 工具：讀取命令列參數 --key=value
static bool GetArgD(const UnicodeString &key, double &val) {
    int i;
    for (i = 1; i <= ParamCount(); ++i) {
        UnicodeString s = ParamStr(i);
        UnicodeString prefix = L"--" + key + L"=";
        if (s.Pos(prefix) == 1) {
            UnicodeString num = s.SubString(prefix.Length()+1, s.Length()-prefix.Length());
            try {
                val = StrToFloat(num);
                return true;
            } catch (...) {
                return false;
            }
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
            try {
                val = StrToInt(num);
                return true;
            } catch (...) {
                return false;
            }
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
            } catch (...) {
                return false;
            }
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

    // 預設參數（可被命令列覆蓋）
    double mu      = 3.0;
    double e       = 2.4;
    int    C       = 5;
    double lambda  = 1.2;
    int    T       = 10000;
    unsigned int seed = (unsigned int)time(NULL);
    AnsiString outPath = "result.txt";

    // 讀取命令列
    double tmpd;
    int tmpi;
    unsigned int tmpu;
    AnsiString tmps;

    if (GetArgD(L"mu", tmpd)) mu = tmpd;
    if (GetArgD(L"e", tmpd))  e  = tmpd;
    if (GetArgI(L"C", tmpi))  C  = tmpi;
    if (GetArgD(L"lambda", tmpd)) lambda = tmpd;
    if (GetArgI(L"T", tmpi))  T = tmpi;
    if (GetArgU(L"seed", tmpu)) seed = tmpu;
    if (GetArgS(L"out", tmps)) outPath = tmps;

    flog << "params: mu="<<mu<<", e="<<e<<", C="<<C
         <<", lambda="<<lambda<<", T="<<T<<", seed="<<seed
         <<", out="<<outPath.c_str()<<"\n";

    // 嘗試呼叫「你真實的模擬引擎」
    int rc = -9999;

    // 若你已在別處實作 RunHeadlessEngine，下面這行就會成功連結並回傳 0。
    // 若尚未實作，會由 fallback 寫出警告到 headless_log.txt。
    rc = RunHeadlessEngine(mu, e, C, lambda, T, seed, outPath.c_str());

    if (rc != 0) {
        // 呼叫失敗，使用 fallback 告知你還沒接上引擎（不寫假結果）
        rc = RunHeadlessEngine_Fallback(mu, e, C, lambda, T, seed, outPath.c_str());
    }

    flog << "[Headless] done, rc=" << rc << "\n";
    flog.close();
    return rc;
}

//---------------------------------------------------------------------------

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    try
    {
        // 無頭旗標
        if (HasFlag(L"headless")) {
            return DoHeadless();
        }

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
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    catch (...)
    {
        try { throw Exception(""); }
        catch (Exception &exception) { Application->ShowException(&exception); }
    }
    return 0;
}
//---------------------------------------------------------------------------

