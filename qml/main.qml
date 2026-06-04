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

    // セーフエリア余白
    readonly property real safeTopMargin: {
        try {
            return typeof SafeArea !== 'undefined' && SafeArea.margins ? SafeArea.margins.top : 0;
        } catch (e) { return 0; }
    }
    readonly property real safeBottomMargin: {
        try {
            return typeof SafeArea !== 'undefined' && SafeArea.margins ? SafeArea.margins.bottom : 0;
        } catch (e) { return 0; }
    }

    // ─── 設定の永続化 ──────────────────────────────────────────────────
    Settings {
        id: appSettings
        property alias showLabels:       solarView.showLabels
        property alias showDwarfPlanets: solarView.showDwarfPlanets
        property alias showSatellites:   solarView.showSatellites
        property alias dirLock:          solarView.dirLock
        property alias centerBody:       solarView.centerBody
        property alias speedIndex:       speedCombo.currentIndex
        property real camLon:  30.0
        property real camLat:  30.0
        property real camDist: 50.0
    }

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

    Timer {
        id: eventRefreshTimer
        interval: 500
        repeat: false
        onTriggered: if (eventSheet.opened) eventList.refresh()
    }

    Component.onCompleted: {
        var savedDist = appSettings.camDist
        solarView.setCamera(appSettings.camLon, appSettings.camLat)
        solarView.setCameraDistance(savedDist)
    }

    // ─── ボタンアイコン描画関数 ───────────────────────────────────────
    // デスクトップ版 button_icons.h と同じ形状定義

    function icnPlay(ctx, w, h, c) {
        ctx.fillStyle = c
        ctx.beginPath()
        ctx.moveTo(w*0.25, h*0.13); ctx.lineTo(w*0.82, h*0.50); ctx.lineTo(w*0.25, h*0.87)
        ctx.closePath(); ctx.fill()
    }

    function icnPause(ctx, w, h, c) {
        ctx.fillStyle = c
        ctx.fillRect(w*0.22, h*0.13, w*0.22, h*0.74)
        ctx.fillRect(w*0.56, h*0.13, w*0.22, h*0.74)
    }

    function icnStepBack(ctx, w, h, c) {
        ctx.fillStyle = c
        ctx.fillRect(w*0.10, h*0.15, w*0.14, h*0.70)
        ctx.beginPath()
        ctx.moveTo(w*0.88, h*0.15); ctx.lineTo(w*0.30, h*0.50); ctx.lineTo(w*0.88, h*0.85)
        ctx.closePath(); ctx.fill()
    }

    function icnStepFwd(ctx, w, h, c) {
        ctx.fillStyle = c
        ctx.beginPath()
        ctx.moveTo(w*0.12, h*0.15); ctx.lineTo(w*0.70, h*0.50); ctx.lineTo(w*0.12, h*0.85)
        ctx.closePath(); ctx.fill()
        ctx.fillRect(w*0.76, h*0.15, w*0.14, h*0.70)
    }

    function icnClock(ctx, w, h, c) {
        var cx = w/2, cy = h/2, r = w*0.42
        ctx.strokeStyle = c; ctx.lineWidth = w*0.09; ctx.lineCap = "butt"
        ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI*2); ctx.stroke()
        ctx.lineCap = "round"
        var ha = -60 * Math.PI / 180
        ctx.lineWidth = w*0.11
        ctx.beginPath(); ctx.moveTo(cx, cy)
        ctx.lineTo(cx + r*0.45*Math.cos(ha), cy + r*0.45*Math.sin(ha)); ctx.stroke()
        var ma = -90 * Math.PI / 180
        ctx.lineWidth = w*0.09
        ctx.beginPath(); ctx.moveTo(cx, cy)
        ctx.lineTo(cx + r*0.65*Math.cos(ma), cy + r*0.65*Math.sin(ma)); ctx.stroke()
        ctx.fillStyle = c
        ctx.beginPath(); ctx.arc(cx, cy, w*0.07, 0, Math.PI*2); ctx.fill()
    }

    function icnCalendar(ctx, w, h, c) {
        var m = w*0.08, bx = m, by = m + h*0.12
        var bw = w - m*2, bh = h*0.80 - m, hdr = bh*0.32
        ctx.strokeStyle = c; ctx.lineWidth = w*0.07; ctx.lineJoin = "round"; ctx.lineCap = "square"
        ctx.strokeRect(bx, by, bw, bh)
        ctx.beginPath(); ctx.moveTo(bx, by+hdr); ctx.lineTo(bx+bw, by+hdr); ctx.stroke()
        ctx.lineWidth = w*0.09; ctx.lineCap = "round"
        ctx.beginPath(); ctx.moveTo(bx+bw*0.28, by-h*0.10); ctx.lineTo(bx+bw*0.28, by+h*0.06); ctx.stroke()
        ctx.beginPath(); ctx.moveTo(bx+bw*0.72, by-h*0.10); ctx.lineTo(bx+bw*0.72, by+h*0.06); ctx.stroke()
        ctx.fillStyle = c
        var dr = w*0.07, gx0 = bx+bw*0.18, gy0 = by+hdr+(bh-hdr)*0.28
        var gsx = bw*0.32, gsy = (bh-hdr)*0.45
        for (var col = 0; col < 3; col++)
            for (var row = 0; row < 2; row++) {
                ctx.beginPath(); ctx.arc(gx0+col*gsx, gy0+row*gsy, dr, 0, Math.PI*2); ctx.fill()
            }
    }

    function icnGear(ctx, w, h, c) {
        var cx = w/2, cy = h/2, Ro = w*0.46, Ri = w*0.34, Rh = w*0.17, N = 8, dA = Math.PI/N
        ctx.fillStyle = c; ctx.beginPath()
        for (var i = 0; i < N; i++) {
            var a0 = 2*Math.PI*i/N - dA*0.45, a1 = 2*Math.PI*i/N - dA*0.20
            var a2 = 2*Math.PI*i/N + dA*0.20, a3 = 2*Math.PI*i/N + dA*0.45
            if (i === 0) ctx.moveTo(cx + Ri*Math.cos(a0), cy + Ri*Math.sin(a0))
            else         ctx.lineTo(cx + Ri*Math.cos(a0), cy + Ri*Math.sin(a0))
            ctx.lineTo(cx + Ro*Math.cos(a1), cy + Ro*Math.sin(a1))
            ctx.lineTo(cx + Ro*Math.cos(a2), cy + Ro*Math.sin(a2))
            ctx.lineTo(cx + Ri*Math.cos(a3), cy + Ri*Math.sin(a3))
        }
        ctx.closePath(); ctx.fill()
        ctx.globalCompositeOperation = "destination-out"
        ctx.beginPath(); ctx.arc(cx, cy, Rh, 0, Math.PI*2); ctx.fill()
        ctx.globalCompositeOperation = "source-over"
    }

    function icnConjunction(ctx, w, h, c) {
        ctx.strokeStyle = c; ctx.lineWidth = w*0.08; ctx.lineCap = "round"
        ctx.beginPath(); ctx.moveTo(w*0.05, h*0.50); ctx.lineTo(w*0.95, h*0.50); ctx.stroke()
        ctx.fillStyle = c
        ctx.beginPath(); ctx.arc(w*0.23, h*0.50, w*0.14, 0, Math.PI*2); ctx.fill()
        ctx.strokeStyle = c; ctx.lineWidth = w*0.08; ctx.lineCap = "butt"
        ctx.beginPath(); ctx.arc(w*0.72, h*0.50, w*0.20, 0, Math.PI*2); ctx.stroke()
    }

    // ─── 再利用可能アイコンキャンバス ────────────────────────────────
    // contentItem として ToolButton に渡す。
    // parent.pressed / parent.enabled の変化を iconColor 経由で検知して再描画する。
    component IconCanvas: Canvas {
        property color iconColor: parent.pressed ? "#ffffff" : (parent.enabled ? "#cccccc" : "#666666")
        property real iconScale: 0.78
        onIconColorChanged: requestPaint()
        Component.onCompleted: requestPaint()

        function beginScaled(ctx) {
            var s = iconScale
            ctx.save()
            ctx.translate(width * (1 - s) * 0.5, height * (1 - s) * 0.5)
            ctx.scale(s, s)
        }
        function endScaled(ctx) { ctx.restore() }
    }

    // ─── 太陽系ビュー (フルスクリーン) ─────────────────────────────────
    SolarItem {
        id: solarView
        anchors.fill: parent

        DragHandler {
            id: drag
            target: null
            minimumPointCount: 1
            maximumPointCount: 1
            property point lastPos
            onActiveChanged: { if (active) lastPos = centroid.position }
            onCentroidChanged: {
                if (active) {
                    solarView.rotateDelta(centroid.position.x - lastPos.x,
                                         centroid.position.y - lastPos.y)
                    lastPos = centroid.position
                }
            }
        }

        PinchHandler {
            id: pinch
            target: null
            property real lastScale: 1.0
            onActiveChanged: { if (active) lastScale = activeScale }
            onActiveScaleChanged: {
                solarView.pinchZoom(activeScale / lastScale)
                lastScale = activeScale
            }
        }

        TapHandler {
            onTapped: (eventPoint) => solarView.tap(eventPoint.position.x,
                                                     eventPoint.position.y)
            onLongPressed: if (root.isPortrait) contextMenu.open()
        }
    }

    // ─── 上部バー ──────────────────────────────────────────────────────
    Rectangle {
        id: topBar
        z: 100
        visible: root.isPortrait
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 52 + root.safeTopMargin
        color: "#d01e1e2e"

        RowLayout {
            anchors { fill: parent; topMargin: root.safeTopMargin; leftMargin: 12; rightMargin: 12 }
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
                implicitWidth: 44
                implicitHeight: 36
                onClicked: eventSheet.open()
                ToolTip.text: "合・衝・最大離角イベント"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnConjunction(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }

            // 設定ボタン
            ToolButton {
                implicitWidth: 44
                implicitHeight: 36
                onClicked: settingsDrawer.open()
                ToolTip.text: "設定"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnGear(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }
        }
    }

    // ─── 下部コントロールバー ──────────────────────────────────────────
    Rectangle {
        id: bottomBar
        z: 100
        visible: root.isPortrait
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 62 + root.safeBottomMargin
        color: "#d01e1e2e"

        RowLayout {
            anchors { fill: parent; leftMargin: 8; rightMargin: 8; bottomMargin: root.safeBottomMargin }
            spacing: 4

            // 日時ピッカー呼び出しボタン
            ToolButton {
                implicitWidth: 44
                implicitHeight: 44
                onClicked: datePickerDialog.open()
                ToolTip.text: "日時を指定"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnCalendar(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }

            // 現在時刻
            ToolButton {
                implicitWidth: 44
                implicitHeight: 44
                onClicked: {
                    solarView.dateTime = (new Date())
                    if (playBtn.playing) solarView.setTrailActive(true)
                    else solarView.clearTrails()
                }
                ToolTip.text: "現在時刻に戻す"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnClock(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }

            // コマ戻し
            ToolButton {
                implicitWidth: 44
                implicitHeight: 44
                enabled: !playBtn.playing
                autoRepeat: true
                autoRepeatDelay: 400
                autoRepeatInterval: 80
                onClicked: stepBy(-1)
                ToolTip.text: "1コマ戻る"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnStepBack(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }

            // 再生 / 停止
            ToolButton {
                id: playBtn
                property bool playing: false
                implicitWidth: 50
                implicitHeight: 44
                highlighted: playing
                onClicked: {
                    playing = !playing
                    solarView.setTrailActive(playing)
                    if (playing) playTimer.start()
                    else         playTimer.stop()
                }
                ToolTip.text: playing ? "停止" : "再生"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    id: playCanvas
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        if (playBtn.playing) root.icnPause(ctx, width, height, iconColor)
                        else                 root.icnPlay(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                    Connections {
                        target: playBtn
                        function onPlayingChanged() { playCanvas.requestPaint() }
                    }
                }
            }

            // コマ進め
            ToolButton {
                implicitWidth: 44
                implicitHeight: 44
                enabled: !playBtn.playing
                autoRepeat: true
                autoRepeatDelay: 400
                autoRepeatInterval: 80
                onClicked: stepBy(1)
                ToolTip.text: "1コマ進む"
                ToolTip.visible: hovered
                contentItem: IconCanvas {
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        beginScaled(ctx)
                        root.icnStepFwd(ctx, width, height, iconColor)
                        endScaled(ctx)
                    }
                }
            }

            // 速度コンボ
            ComboBox {
                id: speedCombo
                model: ["1時間", "6時間", "12時間", "1日", "1週", "1ヶ月", "3ヶ月", "1年", "5年", "10年"]
                currentIndex: 3
                implicitWidth: 90
                implicitHeight: 44
                font.pixelSize: 12

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

        readonly property int selYear:  1800 + yearTumbler.currentIndex
        readonly property int selMonth: monthTumbler.currentIndex + 1
        readonly property int daysInMonth:
            new Date(Date.UTC(selYear, selMonth, 0)).getUTCDate()

        function pad2(n) { return (n < 10 ? "0" : "") + n }

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
            property var labelOf: (v) => v
        }

        contentItem: RowLayout {
            spacing: 2
            DateTumbler {
                id: yearTumbler
                Layout.preferredWidth: 78
                model: 251
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
            var mo = monthTumbler.currentIndex
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
                        solarView.dirLock = 1
                    } else if (eventType.indexOf("衝") >= 0) {
                        solarView.dirLock = 2
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
