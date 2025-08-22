//---------------------------------------------------------------------------

#ifndef ChooseITorSTH
#define ChooseITorSTH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TChooser : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TButton *Button1;
	TButton *Button2;
	void __fastcall OnShow(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TChooser(TComponent* Owner);
	int state; //
};
//---------------------------------------------------------------------------
extern PACKAGE TChooser *Chooser;
//---------------------------------------------------------------------------
#endif
