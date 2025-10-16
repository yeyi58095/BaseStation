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

// �����D�禡�]�d�ҡ^
void RunHeadlessSimulation()
{
    printf("[Headless Mode] Simulation started...\n");

    // �����p�� (�o�̥u�O�ܽd)
    double mu = 3.0;
    double e = 2.4;
    double C = 5.0;
    double lambda = 1.2;
    int T = 10000;

    // �������G�]�i�אּ�A�u���������޿�^
    double result = mu * e * lambda / (C + T * 0.0001);

    // �g�X���G��
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
        // �ഫ�R�O�C�ѼƬ� C ����r��
        char cmd[1024];
        wcstombs(cmd, lpCmdLine, sizeof(cmd));

        // �P�_�O�_�� headless �Ҧ�
        if (strstr(cmd, "--headless") != NULL)
        {
            RunHeadlessSimulation();
            return 0; // ���i�J VCL ���ε{���D�j��
        }

        // ���` GUI �Ҧ�
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

