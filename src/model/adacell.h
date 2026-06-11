#ifndef ADACELL_H
#define ADACELL_H

#include <QColor>
#include <QFont>
#include <QObject>
#include <QVariant>

class AdaCell : public QObject
{
    Q_OBJECT

public:
    explicit AdaCell(QObject *parent = 0);

    QVariant value() const;
    void setValue(const QVariant &value);

    QString displayValue() const;

    bool hasFont() const;
    QFont font() const;
    void setFont(const QFont &font);
    void clearFont();

    QColor bgColor() const;
    void setBgColor(const QColor &color);

    QColor fgColor() const;
    void setFgColor(const QColor &color);

    bool hasInformation() const;

signals:
    void changed();
    void valueChanged(const QVariant &value);

private:
    QVariant mValue;
    QFont mFont;
    QColor mBgColor;
    QColor mFgColor;
    bool mHasFont;
};

#endif // ADACELL_H
