#pragma once
#include <QString>
#include <QVariant>

// 設定の保存先。
//
// デスクトップ／Android: QSettings（従来どおりレジストリ / ini）
// WebAssembly: window.localStorage
//   QSettings が書き込む Emscripten のファイルシステムはメモリ上にしかなく、
//   リロードで消えるため。

#ifdef Q_OS_WASM
#include <emscripten.h>

class AppSettings {
public:
    QVariant value(const QString& key, const QVariant& defaultValue = {}) const
    {
        char* raw = (char*)EM_ASM_PTR({
            var v = null;
            try { v = window.localStorage.getItem('SolarSystemGazer/' + UTF8ToString($0)); }
            catch (e) { v = null; }
            return v === null ? 0 : stringToNewUTF8(v);
        }, key.toUtf8().constData());

        if (!raw) return defaultValue;
        QString s = QString::fromUtf8(raw);
        free(raw);
        return QVariant(s);
    }

    void setValue(const QString& key, const QVariant& v)
    {
        EM_ASM({
            try {
                window.localStorage.setItem('SolarSystemGazer/' + UTF8ToString($0),
                                            UTF8ToString($1));
            } catch (e) { /* ストレージ無効時は保存しないだけ */ }
        }, key.toUtf8().constData(), v.toString().toUtf8().constData());
    }
};

#else
#include <QSettings>

class AppSettings {
public:
    AppSettings() : m_settings("Qt6 Demo", "SolarSystemGazer") {}

    QVariant value(const QString& key, const QVariant& defaultValue = {}) const
    {
        return m_settings.value(key, defaultValue);
    }
    void setValue(const QString& key, const QVariant& v) { m_settings.setValue(key, v); }

private:
    mutable QSettings m_settings;
};
#endif
