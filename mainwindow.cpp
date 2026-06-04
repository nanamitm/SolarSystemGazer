#include "mainwindow.h"
#include "solarwidget.h"
#include "planetdata.h"
#include "eventdetector.h"
#include "orbitalcalc.h"

#include <QSettings>
#include <QCloseEvent>
#include <QTimeZone>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QMenu>

#include <QDateTimeEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include "button_icons.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Solar System Gazer");
    setMinimumSize(720, 680);
    buildUi();
    loadSettings();
}

void MainWindow::buildUi()
{
    // ----- 描画ウィジェット -----
    m_solarWidget = new SolarWidget(this);

    // ----- 日時コントロール -----
    m_dtEdit = new QDateTimeEdit(this);
    m_dtEdit->setDisplayFormat("yyyy-MM-dd  hh:mm");
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_dtEdit->setTimeZone(QTimeZone(Qt::UTC));
#endif
    m_dtEdit->setMinimumDateTime(QDateTime(QDate(1800,  1,  1), QTime(0,  0), QTimeZone(Qt::UTC)));
    m_dtEdit->setMaximumDateTime(QDateTime(QDate(2050, 12, 31), QTime(23, 59), QTimeZone(Qt::UTC)));
    m_dtEdit->setDateTime(QDateTime::currentDateTimeUtc());
    m_dtEdit->setCalendarPopup(true);
    m_dtEdit->setFixedWidth(200);

    m_nowButton = new QPushButton(this);
    m_nowButton->setIcon(ButtonIcons::clockNow());
    m_nowButton->setIconSize({20, 20});
    m_nowButton->setFixedSize(36, 36);
    m_nowButton->setToolTip("現在時刻に戻す");

    m_playButton = new QPushButton(this);
    m_playButton->setCheckable(true);
    m_playButton->setIcon(ButtonIcons::play());
    m_playButton->setIconSize({20, 20});
    m_playButton->setFixedSize(44, 36);
    m_playButton->setToolTip("再生 / 停止");

    m_speedCombo = new QComboBox(this);
    // (表示ラベル, 1フレームの秒数) のペアで登録
    const QList<QPair<QString, qint64>> speeds = {
        { "1時間",    3600LL        },
        { "6時間",    21600LL       },
        { "12時間",   43200LL       },
        { "1日",      86400LL       },
        { "1週",      604800LL      },
        { "1ヶ月",    2592000LL     },
        { "3ヶ月",    7776000LL     },
        { "1年",      31557600LL    },
        { "5年",      157788000LL   },
        { "10年",     315576000LL   },
    };
    for (const auto &s : speeds)
        m_speedCombo->addItem(s.first, s.second);
    m_speedCombo->setCurrentIndex(3);   // デフォルト: 1日
    m_speedCombo->setFixedWidth(90);

    // ----- コマ送りボタン -----
    m_stepBkBtn = new QPushButton(this);
    m_stepBkBtn->setIcon(ButtonIcons::stepBack());
    m_stepBkBtn->setIconSize({20, 20});
    m_stepBkBtn->setFixedSize(36, 36);
    m_stepBkBtn->setToolTip("速度コンボの単位で1コマ戻る（長押しで連続）");
    m_stepBkBtn->setAutoRepeat(true);
    m_stepBkBtn->setAutoRepeatDelay(400);
    m_stepBkBtn->setAutoRepeatInterval(80);

    m_stepFwBtn = new QPushButton(this);
    m_stepFwBtn->setIcon(ButtonIcons::stepForward());
    m_stepFwBtn->setIconSize({20, 20});
    m_stepFwBtn->setFixedSize(36, 36);
    m_stepFwBtn->setToolTip("速度コンボの単位で1コマ進む（長押しで連続）");
    m_stepFwBtn->setAutoRepeat(true);
    m_stepFwBtn->setAutoRepeatDelay(400);
    m_stepFwBtn->setAutoRepeatInterval(80);

    // ----- 天文イベントボタン -----
    m_eventBtn = new QPushButton(this);
    m_eventBtn->setIcon(ButtonIcons::conjunction());
    m_eventBtn->setIconSize({20, 20});
    m_eventBtn->setFixedSize(40, 36);
    m_eventBtn->setToolTip("今日から2年間の合・衝・最大離角イベントを表示");

    // ----- 設定ボタン（歯車） -----
    m_settingsBtn = new QPushButton(this);
    m_settingsBtn->setIcon(ButtonIcons::gear());
    m_settingsBtn->setIconSize({20, 20});
    m_settingsBtn->setFixedSize(36, 36);
    m_settingsBtn->setToolTip("表示設定・視点プリセット");

    // ----- 中心天体コンボ -----
    m_centerCombo = new QComboBox(this);
    m_centerCombo->addItem("太陽", -1);
    const auto &planets = getPlanets();
    for (int i = 0; i < planets.size(); ++i)
        m_centerCombo->addItem(planets[i].name, i);
    m_centerCombo->setCurrentIndex(0);
    m_centerCombo->setFixedWidth(90);

    // ----- タイマー (自動更新 / アニメーション) -----
    m_timer = new QTimer(this);
    m_timer->setInterval(50);   // 50ms ≒ 20fps
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onAutoUpdate);

    // ----- イベントリスト デバウンスタイマー -----
    m_eventRefreshTimer = new QTimer(this);
    m_eventRefreshTimer->setSingleShot(true);
    connect(m_eventRefreshTimer, &QTimer::timeout, this, &MainWindow::onEventRefresh);

    // ----- シグナル接続 -----
    connect(m_dtEdit,    &QDateTimeEdit::dateTimeChanged,
            this, &MainWindow::onDateTimeChanged);
    connect(m_nowButton,  &QPushButton::clicked,
            this, &MainWindow::onNowClicked);
    connect(m_stepBkBtn,  &QPushButton::clicked,
            this, &MainWindow::onStepBack);
    connect(m_stepFwBtn,  &QPushButton::clicked,
            this, &MainWindow::onStepForward);
    connect(m_playButton, &QPushButton::toggled,
            this, &MainWindow::onPlayToggled);
    connect(m_eventBtn,    &QPushButton::clicked,
            this, &MainWindow::onShowEvents);
    connect(m_settingsBtn, &QPushButton::clicked,
            this, &MainWindow::onSettingsClicked);
    connect(m_centerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCenterChanged);

    // ----- レイアウト -----
    QHBoxLayout *toolbar = new QHBoxLayout;
    toolbar->addWidget(new QLabel("日時 (UTC):"));
    toolbar->addWidget(m_dtEdit);
    toolbar->addSpacing(8);
    toolbar->addWidget(m_nowButton);
    toolbar->addWidget(m_stepBkBtn);
    toolbar->addWidget(m_playButton);
    toolbar->addWidget(m_stepFwBtn);
    toolbar->addSpacing(12);
    toolbar->addWidget(new QLabel("ステップ:"));
    toolbar->addWidget(m_speedCombo);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_eventBtn);
    toolbar->addSpacing(12);
    toolbar->addWidget(new QLabel("中心:"));
    toolbar->addWidget(m_centerCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_settingsBtn);

    QVBoxLayout *main = new QVBoxLayout;
    main->setContentsMargins(6, 6, 6, 6);
    main->setSpacing(6);
    main->addLayout(toolbar);
    main->addWidget(m_solarWidget, 1);

    QWidget *central = new QWidget(this);
    central->setLayout(main);
    setCentralWidget(central);

    // ダークスタイル
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #1e1e2e; color: #ddd; }
        QDateTimeEdit, QPushButton, QComboBox {
            background: #2a2a3e; color: #ddd;
            border: 1px solid #444; border-radius: 4px;
            padding: 3px 6px;
        }
        QLabel {
            background: transparent; border: none;
            color: #ddd; padding: 3px 4px;
        }
        QPushButton:hover   { background: #3a3a5e; }
        QPushButton:checked { background: #4a3a7e; color: #fff; }
        QMenu {
            background: #2a2a3e; color: #ddd;
            border: 1px solid #555; padding: 3px 0;
        }
        QMenu::item { padding: 5px 24px 5px 28px; }
        QMenu::item:selected { background: #3a3a5e; }
        QMenu::item:checked  { color: #88aaff; }
        QMenu::indicator { width: 14px; height: 14px; left: 7px; }
        QMenu::separator { background: #444; height: 1px; margin: 3px 6px; }
    )");

    statusBar()->setStyleSheet("background:#1e1e2e; color:#888;");
    statusBar()->showMessage("ドラッグで視点回転 / ホイールでズーム");

    // 初期描画
    onDateTimeChanged();
}

void MainWindow::onDateTimeChanged()
{
    if (m_playbackActive) return;          // 再生中の自動更新は無視
    if (!m_timer->isActive())              // 停止中のみ軌跡をリセット
        m_solarWidget->clearTrails();
    m_solarWidget->setDateTime(m_dtEdit->dateTime());

    // イベントリストが開いていれば 500ms デバウンス後に再計算
    // ただしダブルクリック起因の変更は抑制（選択行を維持するため）
    if (m_eventDlg && m_eventTable && !m_suppressEventRefresh)
        m_eventRefreshTimer->start(500);
}

void MainWindow::onNowClicked()
{
    m_dtEdit->setDateTime(QDateTime::currentDateTimeUtc());
    if (m_timer->isActive())
        m_solarWidget->setTrailActive(true);  // 軌跡の起点を新しい時刻にリセット
    else
        m_solarWidget->clearTrails();
}

void MainWindow::onPlayToggled(bool checked)
{
    if (checked) {
        m_playButton->setIcon(ButtonIcons::pause());
        m_stepBkBtn->setEnabled(false);
        m_stepFwBtn->setEnabled(false);
        m_solarWidget->setTrailActive(true);
        m_timer->start();
    } else {
        m_playButton->setIcon(ButtonIcons::play());
        m_timer->stop();
        m_solarWidget->setTrailActive(false);
        m_solarWidget->clearTrails();
        m_stepBkBtn->setEnabled(true);
        m_stepFwBtn->setEnabled(true);
        // 停止後にもイベントリストを更新（停止直後なので少し短いディレイ）
        if (m_eventDlg && m_eventTable)
            m_eventRefreshTimer->start(300);
    }
}

void MainWindow::onStepBack()
{
    const qint64 secs = m_speedCombo->currentData().toLongLong();
    m_dtEdit->setDateTime(m_dtEdit->dateTime().addSecs(-secs));
}

void MainWindow::onStepForward()
{
    const qint64 secs = m_speedCombo->currentData().toLongLong();
    m_dtEdit->setDateTime(m_dtEdit->dateTime().addSecs(secs));
}

void MainWindow::onAutoUpdate()
{
    const qint64 secsPerFrame = m_speedCombo->currentData().toLongLong();
    QDateTime dt = m_dtEdit->dateTime().addSecs(secsPerFrame);

    // 範囲末尾を超えたら先頭にループ
    const QDateTime &minDt = m_dtEdit->minimumDateTime();
    const QDateTime &maxDt = m_dtEdit->maximumDateTime();
    if (dt > maxDt) {
        const qint64 range   = minDt.secsTo(maxDt);
        const qint64 elapsed = minDt.secsTo(dt);
        dt = minDt.addSecs(elapsed % range);
    }
    m_playbackActive = true;
    m_dtEdit->setDateTime(dt);
    m_playbackActive = false;
    m_solarWidget->setDateTime(dt);
}

void MainWindow::onCenterChanged(int)
{
    int bodyIdx = m_centerCombo->currentData().toInt();
    m_solarWidget->setCenterBody(bodyIdx);
}

void MainWindow::onSettingsClicked()
{
    QMenu menu(this);

    // ── 視点プリセット ──────────────────────────────────────────────
    struct Preset { const char *name; double lon, lat; };
    static const Preset presets[] = {
        {"斜め（デフォルト）", 30.0, 30.0},
        {"真上",              0.0, 89.0},
        {"黄道面",            0.0,  3.0},
    };
    auto *viewMenu = menu.addMenu("視点プリセット");
    for (const auto &pr : presets) {
        connect(viewMenu->addAction(pr.name), &QAction::triggered,
                this, [this, lon = pr.lon, lat = pr.lat]() {
            m_solarWidget->setCamera(lon, lat);
        });
    }

    menu.addSeparator();

    // ── 表示設定 ────────────────────────────────────────────────────
    auto addToggle = [&](const QString &text, bool checked,
                         void (SolarWidget::*setter)(bool)) {
        auto *act = menu.addAction(text);
        act->setCheckable(true);
        act->setChecked(checked);
        connect(act, &QAction::triggered,
                this, [this, setter](bool c) { (m_solarWidget->*setter)(c); });
    };
    addToggle("惑星名を表示",           m_solarWidget->showLabels(),
              &SolarWidget::setShowLabels);
    addToggle("冥王星・矮小惑星を表示", m_solarWidget->showDwarfPlanets(),
              &SolarWidget::setShowDwarfPlanets);
    addToggle("衛星を表示",             m_solarWidget->showSatellites(),
              &SolarWidget::setShowSatellites);
    menu.addSeparator();
    // 方向固定（3択サブメニュー）
    auto *lockMenu  = menu.addMenu("方向固定");
    auto *noneAct2  = lockMenu->addAction("固定なし");
    auto *frontAct2 = lockMenu->addAction("前面=太陽（合・衝確認）");
    auto *backAct2  = lockMenu->addAction("背面=太陽（夜空方向）");
    noneAct2->setCheckable(true);  noneAct2->setChecked(m_solarWidget->dirLock() == SolarWidget::DirLock::None);
    frontAct2->setCheckable(true); frontAct2->setChecked(m_solarWidget->dirLock() == SolarWidget::DirLock::SunFront);
    backAct2->setCheckable(true);  backAct2->setChecked(m_solarWidget->dirLock() == SolarWidget::DirLock::SunBack);
    connect(noneAct2,  &QAction::triggered, this, [this](){ m_solarWidget->setDirLock(SolarWidget::DirLock::None); });
    connect(frontAct2, &QAction::triggered, this, [this](){ m_solarWidget->setDirLock(SolarWidget::DirLock::SunFront); });
    connect(backAct2,  &QAction::triggered, this, [this](){ m_solarWidget->setDirLock(SolarWidget::DirLock::SunBack); });

    menu.addSeparator();
    auto *onTopAct = menu.addAction("常に最前面に表示");
    onTopAct->setCheckable(true);
    onTopAct->setChecked(windowFlags() & Qt::WindowStaysOnTopHint);
    connect(onTopAct, &QAction::triggered, this, [this](bool checked) {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        QTimer::singleShot(0, this, &QWidget::show);
    });

    menu.exec(m_settingsBtn->mapToGlobal(
        QPoint(0, m_settingsBtn->height())));
}

void MainWindow::onShowEvents()
{
    if (m_eventDlg) { m_eventDlg->raise(); m_eventDlg->activateWindow(); return; }

    auto *dlg = new QDialog(this);
    m_eventDlg = dlg;
    dlg->setWindowTitle("天文イベント");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(530, 500);
    dlg->setStyleSheet(R"(
        QWidget { background:#1e1e2e; color:#ddd; }
        QTableWidget { background:#1a1a2a; gridline-color:#333;
                       alternate-background-color:#222238; }
        QTableWidget::item { color:#ddd; }
        QTableWidget::item:selected { background:#3a3a5e; color:#fff; }
        QHeaderView::section { background:#2a2a3e; color:#aaa;
                                border:1px solid #444; padding:4px; }
        QScrollBar:vertical { background:#1a1a2a; width:10px; }
        QScrollBar::handle:vertical { background:#444; border-radius:4px; }
    )");
    connect(dlg, &QDialog::destroyed, this, [this]() {
        m_eventDlg           = nullptr;
        m_eventTable         = nullptr;
        m_includeDwarfsCheck = nullptr;
        m_eventRefreshTimer->stop();
    });

    // テーブル（空で作成、データは onEventRefresh で投入）
    auto *table = new QTableWidget(0, 4, dlg);
    table->setHorizontalHeaderLabels({"日時 (UTC)", "天体", "イベント", "離角"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_eventTable = table;

    // ─── フィルタ ─────────────────────────────────────────────────────
    m_includeDwarfsCheck = new QCheckBox("矮小惑星を含む", dlg);
    m_includeDwarfsCheck->setChecked(false);
    m_includeDwarfsCheck->setStyleSheet("QCheckBox{color:#bbb;font-size:8pt;}");
    connect(m_includeDwarfsCheck, &QCheckBox::toggled,
            this, &MainWindow::onEventRefresh);

    // ─── ダブルクリック時の動作設定 ───────────────────────────────────
    auto *actGroup = new QGroupBox("ダブルクリック時の動作", dlg);
    actGroup->setStyleSheet(R"(
        QGroupBox { color:#aaa; border:1px solid #444; border-radius:4px;
                    margin-top:6px; padding-top:6px; font-size:8pt; }
        QGroupBox::title { subcontrol-origin:margin; left:8px; padding:0 4px; }
        QCheckBox { color:#ccc; spacing:6px; }
        QCheckBox::indicator { width:14px; height:14px; border:1px solid #666;
                               border-radius:3px; background:#1a1a2a; }
        QCheckBox::indicator:checked { background:#5a5a9e; border-color:#7a7abe; }
    )");
    auto *actVLayout = new QVBoxLayout(actGroup);
    actVLayout->setSpacing(4);

    // 1行目: 既存の動作チェックボックス
    auto *actRow1 = new QHBoxLayout();
    auto *centerSunCb = new QCheckBox("中心を太陽に戻す", actGroup);
    centerSunCb->setChecked(true);
    auto *zoomCb = new QCheckBox("ズームを自動調整", actGroup);
    zoomCb->setChecked(true);
    auto *topViewCb = new QCheckBox("真上視点に切り替える", actGroup);
    topViewCb->setChecked(true);
    auto *sunLockCb = new QCheckBox("太陽方向を固定", actGroup);
    sunLockCb->setChecked(true);
    connect(centerSunCb, &QCheckBox::toggled, zoomCb, &QCheckBox::setEnabled);
    actRow1->addWidget(centerSunCb);
    actRow1->addSpacing(12);
    actRow1->addWidget(zoomCb);
    actRow1->addSpacing(12);
    actRow1->addWidget(topViewCb);
    actRow1->addSpacing(12);
    actRow1->addWidget(sunLockCb);
    actRow1->addStretch();
    actVLayout->addLayout(actRow1);

    // 2行目: 矮小惑星イベント専用
    auto *actRow2 = new QHBoxLayout();
    auto *showDwarfsCb = new QCheckBox("矮小惑星の表示を有効にする", actGroup);
    showDwarfsCb->setChecked(true);
    showDwarfsCb->setEnabled(false);  // 矮小惑星フィルタOFF時は無効
    connect(m_includeDwarfsCheck, &QCheckBox::toggled,
            showDwarfsCb, &QCheckBox::setEnabled);
    actRow2->addWidget(showDwarfsCb);
    actRow2->addStretch();
    actVLayout->addLayout(actRow2);

    // ダブルクリック: JD と天体インデックスをテーブルの UserRole から取得
    connect(table, &QTableWidget::cellDoubleClicked,
            this, [this, centerSunCb, zoomCb, topViewCb, sunLockCb,
                        showDwarfsCb](int row, int) {
        if (!m_eventTable || row < 0 || row >= m_eventTable->rowCount()) return;
        auto *dtItem = m_eventTable->item(row, 0);
        if (!dtItem) return;
        const double jd      = dtItem->data(Qt::UserRole).toDouble();
        const int    bodyIdx = dtItem->data(Qt::UserRole + 1).toInt();
        const int    pCnt    = getPlanets().size();
        const bool   isDwarf = (bodyIdx >= pCnt);

        if (centerSunCb->isChecked())
            m_centerCombo->setCurrentIndex(0);

        // ダブルクリック起因の時間変更はイベントリストを更新しない
        m_suppressEventRefresh = true;
        m_dtEdit->setDateTime(QDateTime::fromMSecsSinceEpoch(
            qint64((jd - 2440587.5) * 86400000.0), QTimeZone(Qt::UTC)));
        m_suppressEventRefresh = false;

        if (zoomCb->isChecked()) {
            const double a0 = isDwarf
                ? getDwarfPlanets()[bodyIdx - pCnt].elem.a0
                : getPlanets()[bodyIdx].elem.a0;
            m_solarWidget->setCameraDistance(std::max(2.0, a0 * 1.6));
        }

        if (topViewCb->isChecked())
            m_solarWidget->setCamera(m_solarWidget->camLon(), 89.0);

        if (sunLockCb->isChecked())
            m_solarWidget->setDirLock(SolarWidget::DirLock::SunFront);

        // 矮小惑星イベントのとき表示を有効化
        if (isDwarf && showDwarfsCb->isChecked())
            m_solarWidget->setShowDwarfPlanets(true);
    });

    auto *note = new QLabel("ダブルクリックで日時を移動  ／  時間変更で自動更新", dlg);
    note->setAlignment(Qt::AlignCenter);
    note->setStyleSheet("color:#555; font-size:8pt; padding:1px;");

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(6, 6, 6, 4);
    layout->setSpacing(4);
    layout->addWidget(m_includeDwarfsCheck);
    layout->addWidget(table);
    layout->addWidget(actGroup);
    layout->addWidget(note);

    // 初回データ投入
    onEventRefresh();

    dlg->show();
}

void MainWindow::onEventRefresh()
{
    if (!m_eventDlg || !m_eventTable) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const double startJD    = dateTimeToJD(m_dtEdit->dateTime());
    const double maxJD      = dateTimeToJD(m_dtEdit->maximumDateTime());
    const double endJD      = std::min(startJD + 365.25 * 2.0, maxJD);
    const bool   inclDwarfs = m_includeDwarfsCheck && m_includeDwarfsCheck->isChecked();
    const QVector<AstroEvent> events = detectEvents(startJD, endJD, inclDwarfs);
    QApplication::restoreOverrideCursor();

    m_eventDlg->setWindowTitle(QString("天文イベント  %1 から2年間")
        .arg(m_dtEdit->dateTime().toString("yyyy-MM-dd")));

    m_eventTable->clearSelection();
    const auto &planets = getPlanets();
    const auto &dwarfs  = getDwarfPlanets();
    const int   pCnt    = planets.size();

    auto bodyName = [&](int idx) -> QString {
        return idx < pCnt ? planets[idx].name
                          : dwarfs[idx - pCnt].name;
    };

    m_eventTable->setRowCount(events.size());
    for (int r = 0; r < events.size(); ++r) {
        const auto &ev = events[r];
        const QDateTime dt = QDateTime::fromMSecsSinceEpoch(
            qint64((ev.jd - 2440587.5) * 86400000.0), QTimeZone(Qt::UTC));

        auto mk = [](const QString &s) {
            auto *item = new QTableWidgetItem(s);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        auto *dtItem = mk(dt.toString("yyyy-MM-dd  hh:mm"));
        dtItem->setData(Qt::UserRole,     ev.jd);
        dtItem->setData(Qt::UserRole + 1, ev.planetIdx);
        m_eventTable->setItem(r, 0, dtItem);
        m_eventTable->setItem(r, 1, mk(bodyName(ev.planetIdx)));
        m_eventTable->setItem(r, 2, mk(eventTypeName(ev.type)));
        const QString elongStr =
            (ev.type == EventType::GreatestElongEast ||
             ev.type == EventType::GreatestElongWest)
            ? QString("%1°").arg(ev.elongDeg, 0, 'f', 1) : "";
        m_eventTable->setItem(r, 3, mk(elongStr));
    }
}

void MainWindow::saveSettings()
{
    QSettings s("Qt6 Demo", "SolarSystemGazer");
    s.setValue("showLabels",       m_solarWidget->showLabels());
    s.setValue("showDwarfPlanets", m_solarWidget->showDwarfPlanets());
    s.setValue("showSatellites",   m_solarWidget->showSatellites());
    s.setValue("dirLock",          (int)m_solarWidget->dirLock());
    s.setValue("geometry",         saveGeometry());
    s.setValue("alwaysOnTop",      bool(windowFlags() & Qt::WindowStaysOnTopHint));
    s.setValue("camLon",           m_solarWidget->camLon());
    s.setValue("camLat",           m_solarWidget->camLat());
    s.setValue("camDist",          m_solarWidget->camDist());
    s.setValue("centerBody",       m_solarWidget->centerBody());
    s.setValue("speedSecs",        m_speedCombo->currentData().toLongLong());
}

void MainWindow::loadSettings()
{
    QSettings s("Qt6 Demo", "SolarSystemGazer");

    // 再生速度（秒数で保存→コンボで一致するインデックスを選択）
    const qint64 savedSecs = s.value("speedSecs", 86400LL).toLongLong();
    for (int i = 0; i < m_speedCombo->count(); ++i) {
        if (m_speedCombo->itemData(i).toLongLong() == savedSecs) {
            m_speedCombo->setCurrentIndex(i);
            break;
        }
    }

    // 中心天体（コンボ経由で SolarWidget に反映。setCenterBody が camDist を自動設定する）
    const int cb = s.value("centerBody", -1).toInt();
    for (int i = 0; i < m_centerCombo->count(); ++i) {
        if (m_centerCombo->itemData(i).toInt() == cb) {
            m_centerCombo->setCurrentIndex(i);
            break;
        }
    }

    // カメラ（centerBody の自動 camDist より後に設定して上書き）
    m_solarWidget->setCamera(
        s.value("camLon",  30.0).toDouble(),
        s.value("camLat",  30.0).toDouble());
    m_solarWidget->setCameraDistance(s.value("camDist", 50.0).toDouble());

    // 表示設定
    m_solarWidget->setShowLabels(s.value("showLabels", true).toBool());
    m_solarWidget->setShowDwarfPlanets(s.value("showDwarfPlanets", false).toBool());
    m_solarWidget->setShowSatellites(s.value("showSatellites", false).toBool());
    m_solarWidget->setDirLock(static_cast<SolarWidget::DirLock>(
        s.value("dirLock", 0).toInt()));
    const QByteArray geo = s.value("geometry").toByteArray();
    if (!geo.isEmpty()) restoreGeometry(geo);
    if (s.value("alwaysOnTop", false).toBool())
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    QMainWindow::closeEvent(e);
}
