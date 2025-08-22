object methodChooser: TmethodChooser
  Left = 0
  Top = 0
  Caption = 'methodChooser'
  ClientHeight = 191
  ClientWidth = 423
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 56
    Top = 40
    Width = 31
    Height = 13
    Caption = 'Label1'
  end
  object ComboBox1: TComboBox
    Left = 104
    Top = 72
    Width = 145
    Height = 21
    TabOrder = 0
    Text = 'Choose R.V.'
    OnChange = ComboBox1Change
    Items.Strings = (
      'normal'
      'exponential'
      'uniform')
  end
end
