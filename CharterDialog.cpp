//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "CharterDialog.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TCharter *Charter;
//---------------------------------------------------------------------------
__fastcall TCharter::TCharter(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
