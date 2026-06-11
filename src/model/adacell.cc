#include "adacell.h"

AdaCell::AdaCell(QObject *parent)
    : QObject(parent),
      mHasFont(false)
{
}

QVariant AdaCell::value() const
{
    return mValue;
}

void AdaCell::setValue(const QVariant &value)
{
    if (mValue == value)
        return;

    mValue = value;
    emit valueChanged(mValue);
    emit changed();
}

QString AdaCell::displayValue() const
{
    return mValue.toString();
}

bool AdaCell::hasFont() const
{
    return mHasFont;
}

QFont AdaCell::font() const
{
    return mFont;
}

void AdaCell::setFont(const QFont &font)
{
    if (mHasFont && mFont == font)
        return;

    mFont = font;
    mHasFont = true;
    emit changed();
}

void AdaCell::clearFont()
{
    if (!mHasFont)
        return;

    mFont = QFont();
    mHasFont = false;
    emit changed();
}

QColor AdaCell::bgColor() const
{
    return mBgColor;
}

void AdaCell::setBgColor(const QColor &color)
{
    if (mBgColor == color)
        return;

    mBgColor = color;
    emit changed();
}

QColor AdaCell::fgColor() const
{
    return mFgColor;
}

void AdaCell::setFgColor(const QColor &color)
{
    if (mFgColor == color)
        return;

    mFgColor = color;
    emit changed();
}

bool AdaCell::hasInformation() const
{
    return (mValue.isValid() && !mValue.toString().isEmpty())
            || mHasFont
            || mBgColor.isValid()
            || mFgColor.isValid();
}
