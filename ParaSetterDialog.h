//---------------------------------------------------------------------------

#ifndef ParaSetterDialogH
#define ParaSetterDialogH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TparameterSetter : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TEdit *Edit1;
	TLabel *Label2;
	TEdit *Edit2;
	TLabel *Label3;
	TButton *Button1;
	void __fastcall OnShow(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TparameterSetter(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TparameterSetter *parameterSetter;
//---------------------------------------------------------------------------
#endif
