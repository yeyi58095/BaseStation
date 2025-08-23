object Form5: TForm5
  Left = 0
  Top = 0
  Caption = 'Form5'
  ClientHeight = 396
  ClientWidth = 791
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
  object DebugLabel: TLabel
    Left = 416
    Top = 24
    Width = 56
    Height = 13
    Caption = 'DebugLabel'
  end
  object Label2: TLabel
    Left = 32
    Top = 96
    Width = 51
    Height = 13
    Caption = 'Visit Which'
  end
  object Button1: TButton
    Left = 0
    Top = -7
    Width = 75
    Height = 25
    Caption = 'Button1'
    TabOrder = 0
  end
  object sensorAmountEdit: TEdit
    Left = 120
    Top = 21
    Width = 73
    Height = 21
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
    OnChange = selectSensorComboBoxChange
  end
  object Dubug: TButton
    Left = 303
    Top = 19
    Width = 75
    Height = 25
    Caption = 'Dubug'
    TabOrder = 4
    OnClick = DubugClick
  end
  object selectVisitComboBox: TComboBox
    Left = 89
    Top = 93
    Width = 145
    Height = 21
    TabOrder = 5
    Text = 'selectVisitComboBox'
    OnChange = selectVisitComboBoxChange
  end
  object Chart1: TChart
    Left = 303
    Top = 96
    Width = 400
    Height = 250
    Title.Text.Strings = (
      'TChart')
    TabOrder = 6
    DefaultCanvas = 'TGDIPlusCanvas'
    ColorPaletteIndex = 13
    object SeriesCurrent: TFastLineSeries
      Title = 'Current'
      LinePen.Color = 10708548
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object SeriesMean: TFastLineSeries
      Title = 'Mean'
      LinePen.Color = 3513587
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
  end
  object plotButton: TButton
    Left = 56
    Top = 160
    Width = 75
    Height = 25
    Caption = 'plot'
    TabOrder = 7
    OnClick = plotButtonClick
  end
end
