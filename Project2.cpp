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

// �p�G�A�|����@�����A�����Ѥ@�Ӯz�ƪ��u�����T���v�����A�קK�A�~�|�����G�N�O�諸�C
// ��ڭn�]���T�Ʀr�G�ЧR���U���o�Ӱ�����@�A�æb��L .cpp �����u�� RunHeadlessEngine�C
static int RunHeadlessEngine_Fallback(
    double mu, double e, int C, double lambda, int T, unsigned int seed,
    const char* outPath
) {
    std::ofstream flog("headless_log.txt", std::ios::app);
    flog << "[WARN] RunHeadlessEngine() ���s���u������A�Цb�A�� Engine.cpp ��@���C\n";
    flog << "       ��e�ѼơGmu=" << mu << ", e=" << e << ", C=" << C
         << ", lambda=" << lambda << ", T=" << T << ", seed=" << seed << "\n";
    flog.close();
    // �ԷV�_���A���g���󰲼Ʀr�� outPath�A�קK�V�c�C
    return -1;
}

// �u��GŪ���R�O�C�Ѽ� --key=value
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

// �L�Y����y�{
static int DoHeadless() {
    std::ofstream flog("headless_log.txt");
    flog << "[Headless] start\n";

    // �w�]�Ѽơ]�i�Q�R�O�C�л\�^
    double mu      = 3.0;
    double e       = 2.4;
    int    C       = 5;
    double lambda  = 1.2;
    int    T       = 10000;
    unsigned int seed = (unsigned int)time(NULL);
    AnsiString outPath = "result.txt";

    // Ū���R�O�C
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

    // ���թI�s�u�A�u�ꪺ���������v
    int rc = -9999;

    // �Y�A�w�b�O�B��@ RunHeadlessEngine�A�U���o��N�|���\�s���æ^�� 0�C
    // �Y�|����@�A�|�� fallback �g�Xĵ�i�� headless_log.txt�C
    rc = RunHeadlessEngine(mu, e, C, lambda, T, seed, outPath.c_str());

    if (rc != 0) {
        // �I�s���ѡA�ϥ� fallback �i���A�٨S���W�����]���g�����G�^
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
        // �L�Y�X��
        if (HasFlag(L"headless")) {
            return DoHeadless();
        }

        // GUI �Ҧ�
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

