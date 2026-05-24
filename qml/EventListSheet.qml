import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SolarSystemGazer

Item {
    id: root

    required property var solarViewRef
    required property var speedComboRef

    signal jumpRequested(real jd, string eventType, real a0, bool isDwarf)

    // ─── ジャンプ時の動作オプション（デスクトップ版と同等） ──────────
    property bool optCenterSun:  true
    property bool optAutoZoom:   true
    property bool optAutoView:   true
    property bool optEnableDwarf: true

    // ─── インラインコンポーネント（ルート内に定義） ──────────────────
    component TableHeaderCell: Item {
        required property string text
        required property real columnWidth
        width: columnWidth
        height: 32
        Label {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 6 }
            text: parent.text
            color: "#aaa"
            font.pixelSize: 11
            font.bold: true
        }
    }

    component EventCell: Item {
        required property string text
        required property real cellWidth
        property alias color: lbl.color
        width: cellWidth
        height: 44
        Label {
            id: lbl
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left; leftMargin: 6
                right: parent.right; rightMargin: 2
            }
            text: parent.text
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    // ─── イベント検索ブリッジ ──────────────────────────────────────
    EventBridge {
        id: bridge
    }

    property bool includeDwarfs: false
    property var events: []

    function refresh() {
        var startJd = bridge.dateToJd(solarViewRef.dateTime)
        events = bridge.computeEvents(startJd, startJd + 365.25 * 2, includeDwarfs)
    }

    Component.onCompleted: refresh()
    onSolarViewRefChanged: refresh()

    Column {
        anchors.fill: parent
        spacing: 0

        // ─── ヘッダー ───────────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 48
            color: "#252535"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 8 }

                Label {
                    text: "天文イベント"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#ddd"
                    Layout.fillWidth: true
                }

                CheckBox {
                    text: "矮小惑星を含む"
                    checked: root.includeDwarfs
                    font.pixelSize: 12
                    onToggled: {
                        root.includeDwarfs = checked
                        root.refresh()
                    }
                }

                ToolButton {
                    text: "更新"
                    font.pixelSize: 12
                    onClicked: root.refresh()
                }
            }
        }

        // ─── ジャンプ時の動作オプション ─────────────────────────────
        Rectangle {
            width: parent.width
            height: 84
            color: "#1f1f30"

            Flow {
                anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 2 }
                spacing: 2

                Label {
                    text: "タップ時:"
                    color: "#888"
                    font.pixelSize: 11
                    height: 38
                    verticalAlignment: Text.AlignVCenter
                }
                CheckBox {
                    text: "中心を太陽"; checked: root.optCenterSun; font.pixelSize: 11
                    onToggled: root.optCenterSun = checked
                }
                CheckBox {
                    text: "ズーム自動"; checked: root.optAutoZoom; font.pixelSize: 11
                    onToggled: root.optAutoZoom = checked
                }
                CheckBox {
                    text: "視点自動"; checked: root.optAutoView; font.pixelSize: 11
                    onToggled: root.optAutoView = checked
                }
                CheckBox {
                    text: "矮小惑星表示"; checked: root.optEnableDwarf; font.pixelSize: 11
                    onToggled: root.optEnableDwarf = checked
                }
            }
        }

        // ─── テーブルヘッダー ────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 32
            color: "#2a2a3e"

            Row {
                anchors.fill: parent

                TableHeaderCell { text: "日時 (UTC)"; columnWidth: root.width * 0.38 }
                TableHeaderCell { text: "天体";        columnWidth: root.width * 0.18 }
                TableHeaderCell { text: "イベント";    columnWidth: root.width * 0.30 }
                TableHeaderCell { text: "離角";        columnWidth: root.width * 0.14 }
            }
        }

        // ─── イベントリスト ──────────────────────────────────────────
        ListView {
            id: listView
            width: parent.width
            height: root.height - 48 - 84 - 32
            clip: true
            model: root.events

            delegate: Rectangle {
                width: listView.width
                height: 44
                color: index % 2 === 0 ? "#1a1a2a" : "#222238"

                Row {
                    anchors { fill: parent; leftMargin: 4 }

                    EventCell {
                        text: modelData.dateStr
                        cellWidth: listView.width * 0.38
                        color: "#ccc"
                    }
                    EventCell {
                        text: modelData.planet
                        cellWidth: listView.width * 0.18
                        color: "#aaccff"
                    }
                    EventCell {
                        text: modelData.eventType
                        cellWidth: listView.width * 0.30
                        color: "#ddd"
                    }
                    EventCell {
                        text: modelData.elong
                        cellWidth: listView.width * 0.14
                        color: "#ccc"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.jumpRequested(modelData.jd, modelData.eventType,
                                                  modelData.a0, modelData.isDwarf)
                }
            }

            Label {
                anchors.centerIn: parent
                text: "イベントなし"
                color: "#555"
                font.pixelSize: 14
                visible: root.events.length === 0
            }
        }
    }
}
