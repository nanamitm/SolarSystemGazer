#pragma once
#include <QLayout>
#include <QList>
#include <QWidget>

// 横に並べ、幅が足りなくなったら次の行に折り返すレイアウト。
// WebAssembly 版のツールバーで使う（ブラウザの表示領域はツールバー 1 行分の
// 幅を下回ることがあるため）。
class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 6)
        : QLayout(parent), m_space(spacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() override
    {
        while (QLayoutItem* item = takeAt(0)) delete item;
    }

    void addItem(QLayoutItem* item) override { m_items.append(item); }
    int  count() const override { return m_items.size(); }
    QLayoutItem* itemAt(int i) const override { return m_items.value(i); }
    QLayoutItem* takeAt(int i) override
    {
        return (i >= 0 && i < m_items.size()) ? m_items.takeAt(i) : nullptr;
    }

    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int width) const override
    {
        return doLayout(QRect(0, 0, width, 0), true);
    }

    void setGeometry(const QRect& rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    QSize sizeHint() const override { return minimumSize(); }

    QSize minimumSize() const override
    {
        QSize size;
        for (QLayoutItem* item : m_items) size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        return size + QSize(m.left() + m.right(), m.top() + m.bottom());
    }

private:
    // Places the items; with testOnly it only measures the resulting height.
    int doLayout(const QRect& rect, bool testOnly) const
    {
        const QMargins m = contentsMargins();
        const QRect area = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
        int x = area.x(), y = area.y(), lineHeight = 0;

        for (QLayoutItem* item : m_items) {
            if (item->isEmpty()) continue;
            const QSize hint = item->sizeHint();
            int next = x + hint.width();
            if (next - 1 > area.right() && lineHeight > 0) {   // wrap
                x = area.x();
                y += lineHeight + m_space;
                next = x + hint.width();
                lineHeight = 0;
            }
            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), hint));
            x = next + m_space;
            lineHeight = qMax(lineHeight, hint.height());
        }
        return y + lineHeight - rect.y() + m.bottom();
    }

    QList<QLayoutItem*> m_items;
    int m_space;
};
