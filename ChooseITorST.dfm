object Chooser: TChooser
  Left = 0
  Top = 0
  Caption = 'Chooser'
  ClientHeight = 127
  ClientWidth = 836
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnShow = OnShow
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 16
    Width = 31
    Height = 13
    Caption = 'Label1'
  end
  object Label2: TLabel
    Left = 312
    Top = 8
    Width = 13
    Height = 13
    Caption = 'DP'
  end
  object Label3: TLabel
    Left = 312
    Top = 27
    Width = 28
    Height = 13
    Caption = 'Qmax'
  end
  object Label4: TLabel
    Left = 399
    Top = 27
    Width = 56
    Height = 13
    Caption = '-1 for inifity'
  end
  object preLoad: TLabel
    Left = 301
    Top = 46
    Width = 39
    Height = 13
    Caption = 'preLoad'
  end
  object EP: TLabel
    Left = 558
    Top = 8
    Width = 12
    Height = 13
    Caption = 'EP'
  end
  object E_Cap: TLabel
    Left = 558
    Top = 27
    Width = 31
    Height = 13
    Caption = 'E_Cap'
  end
  object InitEnergy: TLabel
    Left = 558
    Top = 46
    Width = 50
    Height = 13
    Caption = 'InitEnergy'
  end
  object r_tx: TLabel
    Left = 558
    Top = 72
    Width = 20
    Height = 13
    Caption = 'r_tx'
  end
  object Label5: TLabel
    Left = 558
    Top = 106
    Width = 59
    Height = 13
    Caption = 'charge_rate'
  end
  object Button1: TButton
    Left = 48
    Top = 35
    Width = 75
    Height = 25
    Caption = 'IT'
    TabOrder = 0
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 208
    Top = 32
    Width = 75
    Height = 25
    Caption = 'ST'
    TabOrder = 1
    OnClick = Button2Click
  end
  object qmaxEdit: TEdit
    Left = 346
    Top = 24
    Width = 47
    Height = 21
    TabOrder = 2
    Text = '-1'
    OnChange = qmaxEditChange
  end
  object preloadEdit: TEdit
    Left = 346
    Top = 46
    Width = 121
    Height = 21
    TabOrder = 3
    Text = '0'
    OnChange = preloadEditChange
  end
  object ECapEdit: TEdit
    Left = 624
    Top = 24
    Width = 65
    Height = 21
    TabOrder = 4
    Text = '100'
    OnChange = ECapEditChange
  end
  object EnergyEdit: TEdit
    Left = 624
    Top = 46
    Width = 73
    Height = 21
    TabOrder = 5
    Text = '0'
    OnChange = EnergyEditChange
  end
  object rTxEdit: TEdit
    Left = 624
    Top = 73
    Width = 65
    Height = 21
    TabOrder = 6
    Text = '1'
    OnChange = rTxEditChange
  end
  object chargingRateEdit: TEdit
    Left = 623
    Top = 100
    Width = 72
    Height = 21
    TabOrder = 7
    Text = '1'
    OnChange = chargingRateEditChange
  end
  object Button3: TButton
    Left = 728
    Top = 41
    Width = 75
    Height = 25
    Caption = 'ok'
    TabOrder = 8
    OnClick = Button3Click
  end
end
