object Form5: TForm5
  Left = 0
  Top = 0
  Caption = 'Form5'
  ClientHeight = 267
  ClientWidth = 462
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 32
    Top = 24
    Width = 73
    Height = 13
    Caption = 'Sensor Amount'
  end
  object Button1: TButton
    Left = 480
    Top = 261
    Width = 75
    Height = 25
    Caption = 'Button1'
    TabOrder = 0
    OnClick = Button1Click
  end
  object sensorAmountEdit: TEdit
    Left = 120
    Top = 21
    Width = 73
    Height = 16
    TabOrder = 1
    Text = '1'
    OnChange = sensorAmountEditChange
  end
  object generatorButton: TButton
    Left = 199
    Top = 23
    Width = 73
    Height = 17
    Caption = 'Generate'
    TabOrder = 2
    OnClick = generatorButtonClick
  end
  object selectSensorComboBox: TComboBox
    Left = 32
    Top = 43
    Width = 145
    Height = 21
    TabOrder = 3
    Text = 'selectSensorComboBox'
  end
end
