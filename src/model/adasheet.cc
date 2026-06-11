#include "model/adasheet.h"

//-------------------------------------------------------------------------------------------------
AdaSheet::AdaSheet(QObject *parent)
    : QObject(parent),
      mCells(DefaultRowCount * DefaultColumnCount, 0)
{
}

//-------------------------------------------------------------------------------------------------
QString AdaSheet::name() const
{
    return mName;
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::setName(const QString &name)
{
    if (mName == name.trimmed())
        return;
    mName = name.trimmed();
    emit nameChanged();
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::rowCount() const
{
    return DefaultRowCount;
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::columnCount() const
{
    return DefaultColumnCount;
}

//-------------------------------------------------------------------------------------------------
AdaCell *AdaSheet::cell(int row, int column) const
{
    return mCells[cellIndex(row, column)];
}

//-------------------------------------------------------------------------------------------------
AdaCell *AdaSheet::ensureCell(int row, int column)
{
    const int index = cellIndex(row, column);
    if (!mCells[index]) {
        mCells[index] = new AdaCell(this);
        emit cellCreated(row, column, mCells[index]);
    }

    return mCells[index];
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::removeCell(int row, int column)
{
    const int index = cellIndex(row, column);
    if (!mCells[index])
        return;

    delete mCells[index];
    mCells[index] = 0;
    emit cellRemoved(row, column);
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::cellIndex(int row, int column) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());
    return row * columnCount() + column;
}
