object setAllPara: TsetAllPara
  Left = 0
  Top = 0
  Caption = 'setAllPara'
  ClientHeight = 229
  ClientWidth = 921
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
    Left = 48
    Top = 59
    Width = 56
    Height = 13
    Caption = 'arrival Rate'
  end
  object Label2: TLabel
    Left = 48
    Top = 16
    Width = 13
    Height = 13
    Caption = 'DP'
  end
  object ITpara1: TLabel
    Left = 128
    Top = 91
    Width = 38
    Height = 14
    Caption = 'ITpara1'
  end
  object ITpara2: TLabel
    Left = 128
    Top = 111
    Width = 38
    Height = 13
    Caption = 'ITpara2'
  end
  object Label3: TLabel
    Left = 48
    Top = 152
    Width = 61
    Height = 13
    Caption = 'Service Rate'
  end
  object STpara1: TLabel
    Left = 128
    Top = 176
    Width = 40
    Height = 13
    Caption = 'STpara1'
  end
  object STpara2: TLabel
    Left = 128
    Top = 195
    Width = 40
    Height = 13
    Caption = 'STpara2'
  end
  object BufferSize: TLabel
    Left = 48
    Top = 35
    Width = 49
    Height = 13
    Caption = 'BufferSize'
  end
  object ComboBox1: TComboBox
    Left = 128
    Top = 56
    Width = 105
    Height = 21
    TabOrder = 0
    Text = 'ComboBox1'
    Items.Strings = (
      'normal'
      'exponential'
      'uniform')
  end
  object ITpara1Edit: TEdit
    Left = 172
    Top = 88
    Width = 65
    Height = 17
    TabOrder = 1
    Text = '0'
  end
  object ITpara2Edit: TEdit
    Left = 172
    Top = 111
    Width = 61
    Height = 18
    TabOrder = 2
    Text = '0'
  end
  object ComboBox2: TComboBox
    Left = 128
    Top = 149
    Width = 105
    Height = 21
    TabOrder = 3
    Text = 'ComboBox2'
    Items.Strings = (
      'normal'
      'exponential'
      'uniform')
  end
  object STpara1Edit: TEdit
    Left = 174
    Top = 176
    Width = 59
    Height = 16
    TabOrder = 4
    Text = '0'
  end
  object STpara2Edit: TEdit
    Left = 174
    Top = 198
    Width = 59
    Height = 19
    TabOrder = 5
    Text = '0'
  end
  object BufferSizeEdit: TEdit
    Left = 128
    Top = 29
    Width = 105
    Height = 21
    TabOrder = 6
    Text = '10'
  end
end
