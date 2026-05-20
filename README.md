# Solar System Gazer

NASA JPL の軌道要素に基づいて太陽系の惑星配置をリアルタイムで可視化する Windows デスクトップアプリケーションです。

---

## 機能

### 基本表示
- 8 惑星の 3D 透視投影表示（軌道楕円・天体本体・土星の環）
- 任意の日時（1800〜2050 年）における惑星位置を計算
- ズーム・ドラッグによる自由な視点操作

### 表示オプション（右クリックメニュー）
- 惑星名ラベルの表示／非表示（ズームアウト時は自動間引き）
- 矮小惑星（ケレス・冥王星・ハウメア・マケマケ・エリス）の表示
- 衛星の表示（月・ガリレオ 4 衛星・タイタン）
- **方向固定モード**
  - 前面＝太陽：合・衝イベントの確認に最適
  - 背面＝太陽：夜空に見える惑星配置の確認に最適

### 時間制御
- 日時ピッカーによる任意の時刻設定
- アニメーション再生（1 時間〜10 年 /フレーム）
- コマ送りボタン（|◀ / ▶|、長押しで連続送り）
- 再生範囲末尾（2050 年）でループ

### 天文イベント
- 今日から 2 年間の合・衝・最大離角イベントを自動検出
- イベントリストのダブルクリックで日時・視点を自動設定
- 矮小惑星のイベントも含めることが可能（オプション）

### 情報パネル
天体をクリックすると詳細情報を表示します。
- 太陽からの距離・地球からの距離
- 公転周期・軌道速度
- 直径・密度・自転周期・衛星数・自転方向

### その他
- 中心天体の切り替え（太陽または任意の惑星）
- 視点プリセット（斜め・真上・黄道面）
- 距離スケールバー（AU / 万km 単位）
- ウィンドウ常時最前面オプション
- 各種設定の自動保存・復元

---

## 動作環境

- Windows 10 / 11（64 bit）
- Qt 6.x（MinGW 64bit）

---

## ビルド方法

### 必要なツール
- [Qt 6.9 以降](https://www.qt.io/download)（MinGW コンポーネントを含む）
- CMake 3.20 以降（Qt インストーラー同梱版で可）

### ビルド手順

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" \
    -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64" \
    -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingwXXXX_64/bin/g++.exe" \
    -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/mingwXXXX_64/bin/mingw32-make.exe"
cmake --build . --parallel
```

### 配布用バイナリの作成

```bash
windeployqt build/SolarSystemGazer.exe
```

---

## 操作方法

| 操作 | 動作 |
|---|---|
| ドラッグ | 視点回転 |
| ホイール | ズーム |
| 左クリック（天体） | 情報パネル表示 |
| 左クリック（空白） | 選択解除 |
| 右クリック | 表示設定メニュー |
| ⚙ ボタン | 視点プリセット・表示設定 |

---

## データ出典

| データ | 出典 |
|---|---|
| 8 惑星の軌道要素 | [NASA JPL Keplerian Elements for Approximate Positions of the Planets](https://ssd.jpl.nasa.gov/planets/approx_pos.html)（Table 1, 1800–2050 AD） |
| 冥王星の軌道要素 | 同上 |
| ケレス・その他矮小惑星 | MPC / JPL Horizons 近似値 |
| 月の軌道要素 | USNO 標準値（J2000.0） |
| ガリレオ衛星・タイタン | JPL / MPC 近似値 |
| 惑星物理データ | [NASA Planetary Fact Sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/) |
| 惑星北極方向 | IAU Working Group on Cartographic Coordinates（J2000.0 黄道座標に変換） |
| 衛星数 | 2023–2024 年確定値 |

軌道計算の有効期間は **1800〜2050 年**です。範囲外では精度が低下します。

---

## ライセンス

MIT License
