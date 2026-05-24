import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore
import SolarSystemGazer

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 760
    title: "Solar System Gazer"

    Material.theme: Material.Dark
    Material.accent: Material.Blue

    // 横画面では太陽系ビューのみ表示し、操作バーは縦画面だけに出す
    readonly property bool isPortrait: height >= width

    // 横画面に切り替わったら開いているメニュー類を閉じる
    onIsPortraitChanged: {
        if (!isPortrait) {
            settingsDrawer.close()
            eventSheet.close()
            contextMenu.close()
            datePickerDialog.close()
        }
    }

    // 軌道モデル(NASA JPL近似)の有効範囲。UTC基準で 1800–2050 年。
    readonly property double minDateMs: Date.UTC(1800,  0,  1,  0,  0)
    readonly property double maxDateMs: Date.UTC(2050, 11, 31, 23, 59)
    function clampDateMs(ms) { return Math.max(minDateMs, Math.min(maxDateMs, ms)) }

    // 速度ステップ [秒]（speedCombo の並びに対応）— 単一の定義元
    readonly property var speedSecs: [3600, 21600, 43200, 86400, 604800,
                                       2592000, 7776000, 31557600, 157788000, 315576000]

    // ─── 設定の永続化 ──────────────────────────────────────────────────
    // 書き込み可能なプロパティは alias で自動保存/復元
    Settings {
        id: appSettings
        property alias showLabels:       solarView.showLabels
        property alias showDwarfPlanets: solarView.showDwarfPlanets
        property alias showSatellites:   solarView.showSatellites
        property alias dirLock:          solarView.dirLock
        property alias centerBody:       solarView.centerBody
        property alias speedIndex:       speedCombo.currentIndex
        // カメラは読み取り専用プロパティのため手動で保存/復元
        property real camLon:  30.0
        property real camLat:  30.0
        property real camDist: 50.0
    }

    // カメラ変更を設定へ保存 / 時刻変更でイベントリストを自動更新
    Connections {
        target: solarView
        function onCameraChanged() {
            appSettings.camLon  = solarView.camLon
            appSettings.camLat  = solarView.camLat
            appSettings.camDist = solarView.camDist
        }
        function onDateTimeChanged() {
            if (eventSheet.opened)
                eventRefreshTimer.restart()
        }
    }

    // イベントリスト再計算のデバウンス（デスクトップ版と同じ500ms）
    Timer {
        id: eventRefreshTimer
        interval: 500
        repeat: false
        onTriggered: if (eventSheet.opened) eventList.refresh()
    }

    // 起動時にカメラを復元（centerBody 復元による camDist 上書き後に適用）
    // setCamera が onCameraChanged 経由で appSettings.camDist を書き換えるため、
    // 復元値は先にローカルへ退避してから適用する
    Component.onCompleted: {
        var savedDist = appSettings.camDist
        solarView.setCamera(appSettings.camLon, appSettings.camLat)
        solarView.setCameraDistance(savedDist)
    }

    // ─── 太陽系ビュー (フルスクリーン) ─────────────────────────────────
    SolarItem {
        id: solarView
        anchors.fill: parent

        // 1本指ドラッグ → 視点回転
        DragHandler {
            id: drag
            target: null
            minimumPointCount: 1
            maximumPointCount: 1
            property point lastPos

            onActiveChanged: {
                if (active) lastPos = centroid.position
            }
            onCentroidChanged: {
                if (active) {
                    solarView.rotateDelta(centroid.position.x - lastPos.x,
                                         centroid.position.y - lastPos.y)
                    lastPos = centroid.position
                }
            }
        }

        // 2本指ピンチ → ズーム
        PinchHandler {
            id: pinch
            target: null
            property real lastScale: 1.0

            onActiveChanged: {
                if (active) lastScale = activeScale
            }
            onActiveScaleChanged: {
                solarView.pinchZoom(activeScale / lastScale)
                lastScale = activeScale
            }
        }

        // タップ → 天体選択 / 長押し → 表示設定メニュー
        TapHandler {
            onTapped: (eventPoint) => solarView.tap(eventPoint.position.x,
                                                     eventPoint.position.y)
            onLongPressed: if (root.isPortrait) contextMenu.open()
        }
    }

    // ─── 上部バー ──────────────────────────────────────────────────────
    Rectangle {
        id: topBar
        visible: root.isPortrait
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 52 + SafeArea.margins.top
        color: "#d01e1e2e"   // 半透明ダーク

        RowLayout {
            anchors { fill: parent; topMargin: SafeArea.margins.top; leftMargin: 12; rightMargin: 12 }
            spacing: 8

            Label {
                text: Qt.formatDateTime(solarView.dateTime, "yyyy-MM-dd  hh:mm")
                color: "#dddddd"
                font.pixelSize: 13
                font.family: "monospace"
            }

            Item { Layout.fillWidth: true }

            // 合・衝イベントボタン
            ToolButton {
                text: "合・衝"
                font.pixelSize: 12
                onClicked: eventSheet.open()
                implicitWidth: 64
                implicitHeight: 36
            }

            // 設定ボタン
            ToolButton {
                icon.name: "settings"
                text: "⚙"
                font.pixelSize: 18
                onClicked: settingsDrawer.open()
                implicitWidth: 44
                implicitHeight: 36
            }
        }
    }

    // ─── 下部コントロールバー ──────────────────────────────────────────
    Rectangle {
        id: bottomBar
        visible: root.isPortrait
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 62 + SafeArea.margins.bottom
        color: "#d01e1e2e"

        RowLayout {
            anchors { fill: parent; leftMargin: 8; rightMargin: 8; bottomMargin: SafeArea.margins.bottom }
            spacing: 4

            // 日時ピッカー呼び出しボタン
            ToolButton {
                text: "📅"
                font.pixelSize: 16
                implicitWidth: 44
                implicitHeight: 44
                onClicked: datePickerDialog.open()
                ToolTip.text: "日時を指定"
                ToolTip.visible: hovered
            }

            // 現在時刻
            ToolButton {
                text: "現在"
                font.pixelSize: 12
                implicitWidth: 52
                implicitHeight: 44
                onClicked: {
                    solarView.dateTime = (new Date())
                    if (playBtn.playing) solarView.setTrailActive(true)
                    else solarView.clearTrails()
                }
            }

            // コマ戻し
            ToolButton {
                text: "|◀"
                font.pixelSize: 14
                implicitWidth: 44
                implicitHeight: 44
                enabled: !playBtn.playing
                autoRepeat: true
                autoRepeatDelay: 400
                autoRepeatInterval: 80
                onClicked: stepBy(-1)
            }

            // 再生 / 停止
            ToolButton {
                id: playBtn
                property bool playing: false
                text: playing ? "⏸" : "▶"
                font.pixelSize: 18
                implicitWidth: 52
                implicitHeight: 44
                highlighted: playing
                onClicked: {
                    playing = !playing
                    solarView.setTrailActive(playing)
                    if (playing) playTimer.start()
                    else         playTimer.stop()
                }
            }

            // コマ進め
            ToolButton {
                text: "▶|"
                font.pixelSize: 14
                implicitWidth: 44
                implicitHeight: 44
                enabled: !playBtn.playing
                autoRepeat: true
                autoRepeatDelay: 400
                autoRepeatInterval: 80
                onClicked: stepBy(1)
            }

            // 速度コンボ
            ComboBox {
                id: speedCombo
                model: ["1時間", "6時間", "12時間", "1日", "1週", "1ヶ月", "3ヶ月", "1年", "5年", "10年"]
                currentIndex: 3
                implicitWidth: 90
                implicitHeight: 44
                font.pixelSize: 12

                // 横画面など画面高さが低いとき、画面内に収めてスクロール可能にする。
                // 下部バーにあるので上方向に開く。高さは項目数ベースで決め、
                // contentHeight への依存を避ける（相互依存でスクロール不能になるのを防ぐ）。
                popup: Popup {
                    y: -height
                    width: speedCombo.width
                    implicitHeight: Math.min(speedCombo.count * 48 + 2, root.height - 100)
                    padding: 1
                    background: Rectangle { color: "#2a2a3e"; radius: 4; border.color: "#555" }
                    contentItem: ListView {
                        clip: true
                        model: speedCombo.popup.visible ? speedCombo.delegateModel : null
                        currentIndex: speedCombo.highlightedIndex
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }
    }

    // ─── アニメーションタイマー ──────────────────────────────────────
    Timer {
        id: playTimer
        interval: 50
        repeat: true
        onTriggered: {
            var secs  = root.speedSecs[speedCombo.currentIndex]
            var newMs = solarView.dateTime.getTime() + secs * 1000
            // 末尾を超えたら先頭へループ（連続再生のため）
            if (newMs > root.maxDateMs) {
                var range = root.maxDateMs - root.minDateMs
                newMs = root.minDateMs + ((newMs - root.minDateMs) % range)
            }
            solarView.dateTime = (new Date(newMs))
        }
    }

    // ─── コマ送りヘルパー ────────────────────────────────────────────
    function stepBy(dir) {
        var secs  = root.speedSecs[speedCombo.currentIndex] * dir
        var newMs = clampDateMs(solarView.dateTime.getTime() + secs * 1000)
        solarView.dateTime = (new Date(newMs))
        solarView.clearTrails()
    }

    // ─── 日時ピッカーダイアログ（ドラム式） ──────────────────────────
    Dialog {
        id: datePickerDialog
        title: "日時を設定 (UTC)"
        modal: true
        anchors.centerIn: parent

        // 選択中の年・月から当月の日数を算出（日ドラムの範囲に使用）
        readonly property int selYear:  1800 + yearTumbler.currentIndex
        readonly property int selMonth: monthTumbler.currentIndex + 1
        readonly property int daysInMonth:
            new Date(Date.UTC(selYear, selMonth, 0)).getUTCDate()

        function pad2(n) { return (n < 10 ? "0" : "") + n }

        // 開くたびに現在時刻へ各ドラムを同期（UTC基準）
        onAboutToShow: {
            var d = solarView.dateTime
            yearTumbler.currentIndex   = d.getUTCFullYear() - 1800
            monthTumbler.currentIndex  = d.getUTCMonth()
            dayTumbler.currentIndex    = d.getUTCDate() - 1
            hourTumbler.currentIndex   = d.getUTCHours()
            minuteTumbler.currentIndex = d.getUTCMinutes()
        }

        component DateTumbler: Tumbler {
            implicitHeight: 150
            visibleItemCount: 5
            delegate: Text {
                text: Tumbler.tumbler.labelOf(modelData)
                color: "#ddd"
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: 1.0 - Math.abs(Tumbler.displacement) / 2.0
            }
            // 各ドラムが表示テキストを決める関数を持つ
            property var labelOf: (v) => v
        }

        contentItem: RowLayout {
            spacing: 2

            DateTumbler {
                id: yearTumbler
                Layout.preferredWidth: 78
                model: 251                       // 1800〜2050
                labelOf: (v) => 1800 + v
            }
            DateTumbler {
                id: monthTumbler
                Layout.preferredWidth: 46
                model: 12
                labelOf: (v) => datePickerDialog.pad2(v + 1)
            }
            DateTumbler {
                id: dayTumbler
                Layout.preferredWidth: 46
                model: datePickerDialog.daysInMonth
                labelOf: (v) => datePickerDialog.pad2(v + 1)
            }
            DateTumbler {
                id: hourTumbler
                Layout.preferredWidth: 46
                model: 24
                labelOf: (v) => datePickerDialog.pad2(v)
            }
            DateTumbler {
                id: minuteTumbler
                Layout.preferredWidth: 46
                model: 60
                labelOf: (v) => datePickerDialog.pad2(v)
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            var y  = 1800 + yearTumbler.currentIndex
            var mo = monthTumbler.currentIndex                       // 0始まり
            var da = Math.min(dayTumbler.currentIndex + 1, daysInMonth)
            var hh = hourTumbler.currentIndex
            var mi = minuteTumbler.currentIndex
            var ms = clampDateMs(Date.UTC(y, mo, da, hh, mi))
            solarView.dateTime = (new Date(ms))
            solarView.clearTrails()
        }
    }

    // ─── 表示設定コンテキストメニュー（長押し） ──────────────────────
    Menu {
        id: contextMenu
        title: "表示設定"

        MenuItem {
            text: (solarView.showLabels ? "✓ " : "　") + "惑星名を表示"
            onTriggered: solarView.showLabels = !solarView.showLabels
        }
        MenuItem {
            text: (solarView.showDwarfPlanets ? "✓ " : "　") + "矮小惑星を表示"
            onTriggered: solarView.showDwarfPlanets = !solarView.showDwarfPlanets
        }
        MenuItem {
            text: (solarView.showSatellites ? "✓ " : "　") + "衛星を表示"
            onTriggered: solarView.showSatellites = !solarView.showSatellites
        }
        MenuSeparator {}
        Menu {
            title: "方向固定"
            MenuItem {
                text: (solarView.dirLock === 0 ? "✓ " : "　") + "固定なし"
                onTriggered: solarView.dirLock = 0
            }
            MenuItem {
                text: (solarView.dirLock === 1 ? "✓ " : "　") + "前面=太陽（合・衝確認）"
                onTriggered: solarView.dirLock = 1
            }
            MenuItem {
                text: (solarView.dirLock === 2 ? "✓ " : "　") + "背面=太陽（夜空方向）"
                onTriggered: solarView.dirLock = 2
            }
        }
    }

    // ─── 設定 Drawer（右から） ───────────────────────────────────────
    Drawer {
        id: settingsDrawer
        width: Math.min(parent.width * 0.82, 300)
        height: parent.height
        edge: Qt.RightEdge

        background: Rectangle { color: "#1e1e2e" }

        ScrollView {
            anchors { fill: parent; topMargin: 8 }
            contentWidth: availableWidth

            Column {
                width: parent.width
                spacing: 4
                leftPadding: 16
                rightPadding: 16

                Label {
                    text: "設定"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#ddd"
                    bottomPadding: 8
                }

                // ── 表示設定 ───────────────────────────────────────
                Label { text: "表示"; color: "#aaa"; font.pixelSize: 12; topPadding: 4 }

                SwitchDelegate {
                    width: parent.width - 32
                    text: "惑星名"
                    checked: solarView.showLabels
                    onToggled: solarView.showLabels = checked
                }
                SwitchDelegate {
                    width: parent.width - 32
                    text: "矮小惑星"
                    checked: solarView.showDwarfPlanets
                    onToggled: solarView.showDwarfPlanets = checked
                }
                SwitchDelegate {
                    width: parent.width - 32
                    text: "衛星"
                    checked: solarView.showSatellites
                    onToggled: solarView.showSatellites = checked
                }

                MenuSeparator { width: parent.width - 32 }

                // ── 中心天体 ───────────────────────────────────────
                Label { text: "中心天体"; color: "#aaa"; font.pixelSize: 12 }
                ComboBox {
                    id: centerCombo
                    width: parent.width - 32
                    model: ["太陽", "水星", "金星", "地球", "火星",
                            "木星", "土星", "天王星", "海王星"]
                    currentIndex: solarView.centerBody + 1
                    onActivated: (index) => solarView.centerBody = index - 1
                    font.pixelSize: 13
                }

                MenuSeparator { width: parent.width - 32 }

                // ── 方向固定 ───────────────────────────────────────
                Label { text: "方向固定"; color: "#aaa"; font.pixelSize: 12 }
                ButtonGroup { id: lockGroup }
                RadioDelegate {
                    width: parent.width - 32
                    text: "固定なし"
                    checked: solarView.dirLock === 0
                    onToggled: if (checked) solarView.dirLock = 0
                    ButtonGroup.group: lockGroup
                }
                RadioDelegate {
                    width: parent.width - 32
                    text: "前面=太陽（合・衝確認）"
                    checked: solarView.dirLock === 1
                    onToggled: if (checked) solarView.dirLock = 1
                    ButtonGroup.group: lockGroup
                }
                RadioDelegate {
                    width: parent.width - 32
                    text: "背面=太陽（夜空方向）"
                    checked: solarView.dirLock === 2
                    onToggled: if (checked) solarView.dirLock = 2
                    ButtonGroup.group: lockGroup
                }

                MenuSeparator { width: parent.width - 32 }

                // ── 視点プリセット ─────────────────────────────────
                Label { text: "視点プリセット"; color: "#aaa"; font.pixelSize: 12 }
                Button {
                    width: parent.width - 32
                    text: "斜め（デフォルト）"
                    font.pixelSize: 13
                    onClicked: { solarView.setCamera(30, 30); settingsDrawer.close() }
                }
                Button {
                    width: parent.width - 32
                    text: "真上"
                    font.pixelSize: 13
                    onClicked: { solarView.setCamera(0, 89); settingsDrawer.close() }
                }
                Button {
                    width: parent.width - 32
                    text: "黄道面"
                    font.pixelSize: 13
                    onClicked: { solarView.setCamera(0, 3); settingsDrawer.close() }
                }

                Item { height: 24 }
            }
        }
    }

    // ─── 天文イベント BottomSheet ─────────────────────────────────────
    Drawer {
        id: eventSheet
        width: parent.width
        height: parent.height * 0.72
        edge: Qt.BottomEdge

        background: Rectangle { color: "#1e1e2e" }

        EventListSheet {
            id: eventList
            anchors.fill: parent
            solarViewRef: solarView
            speedComboRef: speedCombo
            onJumpRequested: (jd, eventType, a0, isDwarf) => {
                var ms = (jd - 2440587.5) * 86400000.0
                if (optCenterSun)
                    solarView.centerBody = -1
                solarView.dateTime = (new Date(ms))
                solarView.clearTrails()
                if (optEnableDwarf && isDwarf)
                    solarView.showDwarfPlanets = true
                if (optAutoView) {
                    if (eventType.indexOf("最大離角") >= 0) {
                        solarView.setCamera(solarView.camLon, 89.0)
                        solarView.dirLock = 0
                    } else if (eventType.indexOf("合") >= 0) {
                        solarView.dirLock = 1   // SunFront
                    } else if (eventType.indexOf("衝") >= 0) {
                        solarView.dirLock = 2   // SunBack
                    } else {
                        solarView.dirLock = 0
                    }
                }
                if (optAutoZoom)
                    solarView.setCameraDistance(Math.max(2.0, a0 * 1.6))
                eventSheet.close()
            }
        }
    }
}
