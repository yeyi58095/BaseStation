//---------------------------------------------------------------------------

#ifndef ChooseMethodDialogH
#define ChooseMethodDialogH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TmethodChooser : public TForm
{
__published:	// IDE-managed Components
	TComboBox *ComboBox1;
	TLabel *Label1;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall ComboBox1Change(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TmethodChooser(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TmethodChooser *methodChooser;
//---------------------------------------------------------------------------
#endif
