object Replay: TReplay
  Left = 0
  Top = 0
  Caption = 'Replay'
  ClientHeight = 294
  ClientWidth = 889
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
  object lblInfo: TLabel
    Left = 456
    Top = 24
    Width = 30
    Height = 13
    Caption = 'lblInfo'
  end
  object ReplayChart: TChart
    Left = 32
    Top = 24
    Width = 385
    Height = 249
    Title.Text.Strings = (
      'TChart')
    Chart3DPercent = 37
    View3D = False
    TabOrder = 0
    DefaultCanvas = 'TGDIPlusCanvas'
    ColorPaletteIndex = 13
    object SeriesQ: TFastLineSeries
      LinePen.Color = 10708548
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object SeriesMeanQ: TFastLineSeries
      LinePen.Color = 3513587
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
    object SeriesEP: TFastLineSeries
      LinePen.Color = 1330417
      XValues.Name = 'X'
      XValues.Order = loAscending
      YValues.Name = 'Y'
      YValues.Order = loNone
    end
  end
  object btnStep: TButton
    Left = 456
    Top = 200
    Width = 75
    Height = 25
    Caption = 'Step by Step'
    TabOrder = 1
    OnClick = btnStepClick
  end
end
