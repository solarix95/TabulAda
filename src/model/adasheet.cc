#include "model/adasheet.h"
#include "model/adamodel.h"

//-------------------------------------------------------------------------------------------------
AdaSheet::AdaSheet(AdaModel *model)
    : QObject(model)
    , mModel(model)
    , mColumnCount(DefaultColumnCount)
    , mRowCount(DefaultRowCount)
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
QString AdaSheet::script() const
{
    return mScript;
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::setScript(const QString &script)
{
    mScript = script;
}

//-------------------------------------------------------------------------------------------------
QUndoStack *AdaSheet::undoStack()
{
    return &mUndoStack;
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::rowCount() const
{
    return mRowCount;
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::columnCount() const
{
    return mColumnCount;
}

//-------------------------------------------------------------------------------------------------
AdaCell *AdaSheet::cell(int row, int column) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());
    return mCells.value(cellKey(row, column), 0);
}

//-------------------------------------------------------------------------------------------------
AdaCell *AdaSheet::ensureCell(int row, int column)
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());

    const quint64 key = cellKey(row, column);
    AdaCell *cell = mCells.value(key, 0);
    if (!cell) {
        cell = new AdaCell(this);
        mCells.insert(key, cell);
        mCellKeys.insert(cell, key);
        connect(cell, &AdaCell::changed, this, [this, cell]() {
            if (!mCellKeys.contains(cell))
                return;
            const quint64 currentKey = mCellKeys.value(cell);
            emit cellChanged(keyRow(currentKey), keyColumn(currentKey));
        });
        emit cellCreated(row, column, cell);
    }
    return cell;
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::removeCell(int row, int column)
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());

    const quint64 key = cellKey(row, column);
    AdaCell *cell = mCells.take(key);
    if (!cell)
        return;

    mCellKeys.remove(cell);
    delete cell;
    emit cellRemoved(row, column);
}


//-------------------------------------------------------------------------------------------------
QRect AdaSheet::spanAt(int row, int column) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());

    for (auto it = mCells.constBegin(); it != mCells.constEnd(); ++it) {
        AdaCell *cell = it.value();
        if (cell->rowSpan() == 1 && cell->columnSpan() == 1)
            continue;
        const int anchorRow = keyRow(it.key());
        const int anchorColumn = keyColumn(it.key());
        const QRect span(anchorColumn, anchorRow,
                         cell->columnSpan(), cell->rowSpan());
        if (span.contains(column, row))
            return span;
    }
    return QRect();
}

//-------------------------------------------------------------------------------------------------
QVector<QRect> AdaSheet::spans() const
{
    QVector<QRect> result;
    for (auto it = mCells.constBegin(); it != mCells.constEnd(); ++it) {
        AdaCell *cell = it.value();
        if (cell->rowSpan() > 1 || cell->columnSpan() > 1)
            result << QRect(keyColumn(it.key()), keyRow(it.key()),
                            cell->columnSpan(), cell->rowSpan());
    }
    return result;
}

//-------------------------------------------------------------------------------------------------
QVector<QRect> AdaSheet::spansIntersecting(const QRect &area) const
{
    QVector<QRect> result;
    for (const QRect &span : spans()) {
        if (span.intersects(area))
            result << span;
    }
    return result;
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::setCellSpan(int row, int column, int rowSpan, int columnSpan)
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());
    Q_ASSERT(rowSpan >= 1 && row + rowSpan <= rowCount());
    Q_ASSERT(columnSpan >= 1 && column + columnSpan <= columnCount());

    AdaCell *cell = mCells.value(cellKey(row, column), 0);
    if (!cell && rowSpan == 1 && columnSpan == 1)
        return;
    if (!cell)
        cell = ensureCell(row, column);

    const int oldRowSpan = cell->rowSpan();
    const int oldColumnSpan = cell->columnSpan();
    if (oldRowSpan == rowSpan && oldColumnSpan == columnSpan)
        return;

    cell->setSpan(rowSpan, columnSpan);
    emit cellSpanChanged(row, column, oldRowSpan, oldColumnSpan,
                         rowSpan, columnSpan);
    if (!cell->hasInformation())
        removeCell(row, column);
}

//-------------------------------------------------------------------------------------------------
bool AdaSheet::insertRows(int row, int count)
{
    if (row < 0 || row > mRowCount || count <= 0)
        return false;

    emit rowsAboutToBeInserted(row, row + count - 1);
    moveCells(row, count, 0, 0);
    mRowCount += count;
    emit rowsInserted();
    return true;
}

//-------------------------------------------------------------------------------------------------
bool AdaSheet::removeRows(int row, int count)
{
    if (row < 0 || count <= 0 || row + count > mRowCount)
        return false;

    emit rowsAboutToBeRemoved(row, row + count - 1);
    const int last = row + count - 1;
    const QList<quint64> keys = mCells.keys();
    for (quint64 key : keys) {
        if (keyRow(key) < row || keyRow(key) > last)
            continue;
        AdaCell *cell = mCells.take(key);
        mCellKeys.remove(cell);
        delete cell;
    }
    moveCells(row + count, -count, 0, 0);
    mRowCount -= count;
    emit rowsRemoved();
    return true;
}

//-------------------------------------------------------------------------------------------------
bool AdaSheet::insertColumns(int column, int count)
{
    if (column < 0 || column > mColumnCount || count <= 0)
        return false;

    emit columnsAboutToBeInserted(column, column + count - 1);
    moveCells(0, 0, column, count);
    mColumnCount += count;
    emit columnsInserted();
    return true;
}

//-------------------------------------------------------------------------------------------------
bool AdaSheet::removeColumns(int column, int count)
{
    if (column < 0 || count <= 0 || column + count > mColumnCount)
        return false;

    emit columnsAboutToBeRemoved(column, column + count - 1);
    const int last = column + count - 1;
    const QList<quint64> keys = mCells.keys();
    for (quint64 key : keys) {
        if (keyColumn(key) < column || keyColumn(key) > last)
            continue;
        AdaCell *cell = mCells.take(key);
        mCellKeys.remove(cell);
        delete cell;
    }
    moveCells(0, 0, column + count, -count);
    mColumnCount -= count;
    emit columnsRemoved();
    return true;
}

//-------------------------------------------------------------------------------------------------
quint64 AdaSheet::cellKey(int row, int column)
{
    return (quint64(quint32(row)) << 32) | quint32(column);
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::keyRow(quint64 key)
{
    return int(key >> 32);
}

//-------------------------------------------------------------------------------------------------
int AdaSheet::keyColumn(quint64 key)
{
    return int(key & 0xffffffffu);
}

//-------------------------------------------------------------------------------------------------
void AdaSheet::moveCells(int row, int rowDelta, int column, int columnDelta)
{
    QHash<quint64, AdaCell*> movedCells;
    for (auto it = mCells.constBegin(); it != mCells.constEnd(); ++it) {
        int currentRow = keyRow(it.key());
        int currentColumn = keyColumn(it.key());
        if (rowDelta && currentRow >= row)
            currentRow += rowDelta;
        if (columnDelta && currentColumn >= column)
            currentColumn += columnDelta;

        const quint64 newKey = cellKey(currentRow, currentColumn);
        movedCells.insert(newKey, it.value());
        mCellKeys[it.value()] = newKey;
    }
    mCells.swap(movedCells);
}
