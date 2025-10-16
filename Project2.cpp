//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include <stdio.h>      // for printf
#include <string.h>     // for strstr, strcmp
#include <fstream>      // for file output

//---------------------------------------------------------------------------
USEFORM("setAllParaDialog.cpp", setAllPara);
USEFORM("Unit5.cpp", Form5);
USEFORM("ChooseMethodDialog.cpp", methodChooser);
USEFORM("CharterDialog.cpp", Charter);
USEFORM("ChooseITorST.cpp", Chooser);
USEFORM("ParaSetterDialog.cpp", parameterSetter);
USEFORM("ReplayDialog.cpp", Replay);
//---------------------------------------------------------------------------

// 模擬主函式（範例）
void RunHeadlessSimulation()
{
    printf("[Headless Mode] Simulation started...\n");

    // 模擬計算 (這裡只是示範)
    double mu = 3.0;
    double e = 2.4;
    double C = 5.0;
    double lambda = 1.2;
    int T = 10000;

    // 模擬結果（可改為你真正的模擬邏輯）
    double result = mu * e * lambda / (C + T * 0.0001);

    // 寫出結果檔
    std::ofstream fout("fig4_c5.txt");
    if (fout.is_open()) {
        fout << "# mu=" << mu << ", e=" << e << ", C=" << C << "\n";
        fout << "# lambda average_packet_delay(ms)\n";
        fout << lambda << " " << result << "\n";
        fout.close();
        printf("Result written to fig4_c5.txt\n");
    } else {
        printf("Error: cannot open output file.\n");
    }

    printf("[Headless Mode] Simulation finished.\n");
}

//---------------------------------------------------------------------------

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR lpCmdLine, int)
{
    try
    {
        // 轉換命令列參數為 C 風格字串
        char cmd[1024];
        wcstombs(cmd, lpCmdLine, sizeof(cmd));

        // 判斷是否為 headless 模式
        if (strstr(cmd, "--headless") != NULL)
        {
            RunHeadlessSimulation();
            return 0; // 不進入 VCL 應用程式主迴圈
        }

        // 正常 GUI 模式
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
        try
        {
            throw Exception("");
        }
        catch (Exception &exception)
        {
            Application->ShowException(&exception);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------

