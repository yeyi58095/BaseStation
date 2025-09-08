//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("Unit5.cpp", Form5);
USEFORM("ChooseMethodDialog.cpp", methodChooser);
USEFORM("ChooseITorST.cpp", Chooser);
USEFORM("CharterDialog.cpp", Charter);
USEFORM("ParaSetterDialog.cpp", parameterSetter);
USEFORM("setAllParaDialog.cpp", setAllPara);
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TForm5), &Form5);
		Application->CreateForm(__classid(TCharter), &Charter);
		Application->CreateForm(__classid(TChooser), &Chooser);
		Application->CreateForm(__classid(TmethodChooser), &methodChooser);
		Application->CreateForm(__classid(TparameterSetter), &parameterSetter);
		Application->CreateForm(__classid(TsetAllPara), &setAllPara);
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
