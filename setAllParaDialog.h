//---------------------------------------------------------------------------

#ifndef setAllParaDialogH
#define setAllParaDialogH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
//---------------------------------------------------------------------------
class TsetAllPara : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TLabel *Label2;
private:	// User declarations
public:		// User declarations
	__fastcall TsetAllPara(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TsetAllPara *setAllPara;
//---------------------------------------------------------------------------
#endif
