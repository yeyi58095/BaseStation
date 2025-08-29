object Form5: TForm5
  Left = 0
  Top = 0
  Caption = 's'
  ClientHeight = 388
  ClientWidth = 1161
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
    Left = 31
    Top = 304
    Width = 51
    Height = 13
    Caption = 'Visit Which'
  end
  object Label3: TLabel
    Left = 65
    Top = 70
    Width = 40
    Height = 13
    Caption = 'endTime'
  end
  object Label4: TLabel
    Left = 51
    Top = 97
    Width = 54
    Height = 13
    Caption = 'switchOver'
  end
  object Label5: TLabel
    Left = 16
    Top = 124
    Width = 89
    Height = 13
    Caption = 'max charging slots'
  end
  object Label6: TLabel
    Left = 162
    Top = 124
    Width = 52
    Height = 13
    Caption = '0 for inifity'
  end
  object log: TLabel
    Left = 782
    Top = 24
    Width = 35
    Height = 18
  end
  object sensorAmountEdit: TEdit
    Left = 120
    Top = 21
    Width = 73
    Height = 21
    TabOrder = 0
    Text = '1'
    OnChange = sensorAmountEditChange
  end
  object generatorButton: TButton
    Left = 199
    Top = 23
    Width = 73
    Height = 17
    Caption = 'Generate'
    TabOrder = 1
    OnClick = generatorButtonClick
  end
  object selectSensorComboBox: TComboBox
    Left = 32
    Top = 43
    Width = 145
    Height = 21
    TabOrder = 2
    Text = 'selectSensorComboBox'
    OnChange = selectSensorComboBoxChange
  end
  object Dubug: TButton
    Left = 303
    Top = 19
    Width = 75
    Height = 25
    Caption = 'Dubug'
    TabOrder = 3
    OnClick = DubugClick
  end
  object selectVisitComboBox: TComboBox
    Left = 96
    Top = 301
    Width = 145
    Height = 21
    TabOrder = 4
    Text = 'selectVisitComboBox'
    OnChange = selectVisitComboBoxChange
  end
  object Chart1: TChart
    Left = 303
    Top = 97
    Width = 400
    Height = 250
    Title.Text.Strings = (
      'TChart')
    Chart3DPercent = 1
    View3D = False
    Zoom.Pen.Width = 0
    TabOrder = 5
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
    object SeriesThreshold: TFastLineSeries
      Title = 'Threshold'
      LinePen.Color = 1330417
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object SeriesEP: TFastLineSeries
      Title = 'EP'
      LinePen.Color = 11048782
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
  end
  object plotButton: TButton
    Left = 64
    Top = 336
    Width = 75
    Height = 25
    Caption = 'plot'
    TabOrder = 6
    OnClick = plotButtonClick
  end
  object endTimeEdit: TEdit
    Left = 111
    Top = 70
    Width = 121
    Height = 21
    TabOrder = 7
    Text = '10000'
    OnChange = endTimeEditChange
  end
  object switchOverEdit: TEdit
    Left = 111
    Top = 97
    Width = 121
    Height = 21
    TabOrder = 8
    Text = '0'
    OnChange = switchOverEditChange
  end
  object maxCharginSlotEdit: TEdit
    Left = 111
    Top = 124
    Width = 28
    Height = 21
    TabOrder = 9
    Text = '0'
    OnChange = maxCharginSlotEditChange
  end
  object chkQ: TCheckBox
    Left = 709
    Top = 98
    Width = 60
    Height = 14
    Caption = 'chkQ'
    TabOrder = 10
    OnClick = chkQClick
  end
  object ckbMean: TCheckBox
    Left = 709
    Top = 118
    Width = 60
    Height = 19
    Caption = 'ckbMean'
    TabOrder = 11
    OnClick = ckbMeanClick
  end
  object ckbRtx: TCheckBox
    Left = 709
    Top = 143
    Width = 60
    Height = 17
    Caption = 'ckbRtx'
    TabOrder = 12
    OnClick = ckbRtxClick
  end
  object ckbEP: TCheckBox
    Left = 709
    Top = 166
    Width = 60
    Height = 17
    Caption = 'ckbEP'
    TabOrder = 13
    OnClick = ckbEPClick
  end
end
