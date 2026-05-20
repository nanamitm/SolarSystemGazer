#pragma once
#include <QMainWindow>

class SolarWidget;
class QDateTimeEdit;
class QPushButton;
class QComboBox;
class QTimer;
class QLabel;
class QDialog;
class QTableWidget;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onDateTimeChanged();
    void onNowClicked();
    void onAutoUpdate();
    void onPlayToggled(bool checked);
    void onStepBack();
    void onStepForward();
    void onCenterChanged(int comboIdx);
    void onSettingsClicked();
    void onShowEvents();
    void onEventRefresh();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    void saveSettings();
    void loadSettings();

private:
    SolarWidget    *m_solarWidget;
    QDateTimeEdit  *m_dtEdit;
    QPushButton    *m_nowButton;
    QPushButton    *m_playButton;
    QComboBox      *m_speedCombo;   // 1フレームあたりの秒数
    QPushButton    *m_stepBkBtn;    // 1コマ戻る
    QPushButton    *m_stepFwBtn;    // 1コマ進む
    QPushButton    *m_eventBtn;     // 天文イベント一覧
    QDialog        *m_eventDlg          = nullptr;
    QTableWidget   *m_eventTable         = nullptr;
    QCheckBox      *m_includeDwarfsCheck = nullptr;
    QTimer         *m_eventRefreshTimer    = nullptr;
    bool            m_suppressEventRefresh = false;
    QPushButton    *m_settingsBtn;  // 設定メニュー（歯車）
    QComboBox      *m_centerCombo;  // 中心天体選択
    QTimer         *m_timer;
    bool            m_playbackActive = false;

    void buildUi();
};
