#include "viewmodel/adatableviewmodel.h"
#include "model/adasheet.h"

#include <QUndoCommand>
#include <QUndoStack>

namespace {

struct CellSnapshot
{
    int row;
    int column;
    QVariant value;
    bool hasFont;
    QFont font;
    QColor bgColor;
    QColor fgColor;
    bool hasAlignment;
    Qt::Alignment alignment;
    int rowSpan;
    int columnSpan;
};

bool applyCellData(AdaSheet *sheet, int row, int column,
                   const QVariant &value, int role)
{
    AdaCell *cell = sheet->cell(row, column);
    const bool hasNewInformation = role == Qt::EditRole
            ? value.isValid() && !value.toString().isEmpty()
            : value.isValid();
    if (!cell && !hasNewInformation)
        return false;
    if (!cell)
        cell = sheet->ensureCell(row, column);

    switch (role) {
    case Qt::EditRole:
        cell->setValue(value);
        break;
    case Qt::FontRole:
        if (value.isValid())
            cell->setFont(qvariant_cast<QFont>(value));
        else
            cell->clearFont();
        break;
    case Qt::BackgroundRole:
        cell->setBgColor(value.isValid() ? qvariant_cast<QColor>(value) : QColor());
        break;
    case Qt::ForegroundRole:
        cell->setFgColor(value.isValid() ? qvariant_cast<QColor>(value) : QColor());
        break;
    case Qt::TextAlignmentRole:
        if (value.isValid())
            cell->setAlignment(Qt::Alignment(value.toInt()));
        else
            cell->clearAlignment();
        break;
    default:
        return false;
    }

    if (!cell->hasInformation())
        sheet->removeCell(row, column);
    return true;
}

QVector<CellSnapshot> captureCells(AdaSheet *sheet, int firstRow, int lastRow,
                                   int firstColumn, int lastColumn)
{
    QVector<CellSnapshot> cells;
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            AdaCell *cell = sheet->cell(row, column);
            if (!cell)
                continue;
            CellSnapshot snapshot;
            snapshot.row = row;
            snapshot.column = column;
            snapshot.value = cell->value();
            snapshot.hasFont = cell->hasFont();
            snapshot.font = cell->font();
            snapshot.bgColor = cell->bgColor();
            snapshot.fgColor = cell->fgColor();
            snapshot.hasAlignment = cell->hasAlignment();
            snapshot.alignment = cell->alignment();
            snapshot.rowSpan = cell->rowSpan();
            snapshot.columnSpan = cell->columnSpan();
            cells << snapshot;
        }
    }
    return cells;
}

void restoreCells(AdaSheet *sheet, const QVector<CellSnapshot> &cells)
{
    for (const CellSnapshot &snapshot : cells) {
        AdaCell *cell = sheet->ensureCell(snapshot.row, snapshot.column);
        cell->setValue(snapshot.value);
        if (snapshot.hasFont)
            cell->setFont(snapshot.font);
        else
            cell->clearFont();
        cell->setBgColor(snapshot.bgColor);
        cell->setFgColor(snapshot.fgColor);
        if (snapshot.hasAlignment)
            cell->setAlignment(snapshot.alignment);
        else
            cell->clearAlignment();
        sheet->setCellSpan(snapshot.row, snapshot.column,
                           snapshot.rowSpan, snapshot.columnSpan);
    }
}

QString commandTextForRole(int role)
{
    switch (role) {
    case Qt::EditRole: return QObject::tr("Zellinhalt aendern");
    case Qt::FontRole: return QObject::tr("Schrift aendern");
    case Qt::BackgroundRole: return QObject::tr("Hintergrundfarbe aendern");
    case Qt::ForegroundRole: return QObject::tr("Textfarbe aendern");
    case Qt::TextAlignmentRole: return QObject::tr("Ausrichtung aendern");
    }
    return QObject::tr("Zelle aendern");
}

class SetCellDataCommand : public QUndoCommand
{
public:
    SetCellDataCommand(AdaSheet *sheet, int row, int column,
                       const QVariant &oldValue, const QVariant &newValue, int role)
        : QUndoCommand(commandTextForRole(role))
        , mSheet(sheet), mRow(row), mColumn(column)
        , mOldValue(oldValue), mNewValue(newValue), mRole(role)
    {
    }

    void undo() override { applyCellData(mSheet, mRow, mColumn, mOldValue, mRole); }
    void redo() override { applyCellData(mSheet, mRow, mColumn, mNewValue, mRole); }

private:
    AdaSheet *mSheet;
    int mRow;
    int mColumn;
    QVariant mOldValue;
    QVariant mNewValue;
    int mRole;
};

class InsertRowsCommand : public QUndoCommand
{
public:
    InsertRowsCommand(AdaSheet *sheet, int row, int count)
        : QUndoCommand(QObject::tr("Zeilen einfuegen"))
        , mSheet(sheet), mRow(row), mCount(count) {}
    void undo() override { mSheet->removeRows(mRow, mCount); }
    void redo() override { mSheet->insertRows(mRow, mCount); }
private:
    AdaSheet *mSheet;
    int mRow;
    int mCount;
};

class RemoveRowsCommand : public QUndoCommand
{
public:
    RemoveRowsCommand(AdaSheet *sheet, int row, int count)
        : QUndoCommand(QObject::tr("Zeilen entfernen"))
        , mSheet(sheet), mRow(row), mCount(count)
        , mCells(captureCells(sheet, row, row + count - 1,
                              0, sheet->columnCount() - 1)) {}
    void undo() override
    {
        mSheet->insertRows(mRow, mCount);
        restoreCells(mSheet, mCells);
    }
    void redo() override { mSheet->removeRows(mRow, mCount); }
private:
    AdaSheet *mSheet;
    int mRow;
    int mCount;
    QVector<CellSnapshot> mCells;
};

class InsertColumnsCommand : public QUndoCommand
{
public:
    InsertColumnsCommand(AdaSheet *sheet, int column, int count)
        : QUndoCommand(QObject::tr("Spalten einfuegen"))
        , mSheet(sheet), mColumn(column), mCount(count) {}
    void undo() override { mSheet->removeColumns(mColumn, mCount); }
    void redo() override { mSheet->insertColumns(mColumn, mCount); }
private:
    AdaSheet *mSheet;
    int mColumn;
    int mCount;
};

class RemoveColumnsCommand : public QUndoCommand
{
public:
    RemoveColumnsCommand(AdaSheet *sheet, int column, int count)
        : QUndoCommand(QObject::tr("Spalten entfernen"))
        , mSheet(sheet), mColumn(column), mCount(count)
        , mCells(captureCells(sheet, 0, sheet->rowCount() - 1,
                              column, column + count - 1)) {}
    void undo() override
    {
        mSheet->insertColumns(mColumn, mCount);
        restoreCells(mSheet, mCells);
    }
    void redo() override { mSheet->removeColumns(mColumn, mCount); }
private:
    AdaSheet *mSheet;
    int mColumn;
    int mCount;
    QVector<CellSnapshot> mCells;
};


class SetSpansCommand : public QUndoCommand
{
public:
    SetSpansCommand(AdaSheet *sheet, const QVector<QRect> &oldSpans,
                    const QVector<QRect> &newSpans, const QString &text)
        : QUndoCommand(text)
        , mSheet(sheet)
        , mOldSpans(oldSpans)
        , mNewSpans(newSpans)
    {
    }

    void undo() override
    {
        apply(mNewSpans, mOldSpans);
    }

    void redo() override
    {
        apply(mOldSpans, mNewSpans);
    }

private:
    void apply(const QVector<QRect> &removeSpans, const QVector<QRect> &addSpans)
    {
        for (const QRect &span : removeSpans)
            mSheet->setCellSpan(span.y(), span.x(), 1, 1);
        for (const QRect &span : addSpans)
            mSheet->setCellSpan(span.y(), span.x(), span.height(), span.width());
    }

    AdaSheet *mSheet;
    QVector<QRect> mOldSpans;
    QVector<QRect> mNewSpans;
};

} // namespace

//-------------------------------------------------------------------------------------------------
AdaTableViewModel::AdaTableViewModel(AdaSheet *sheet, QObject *parent)
    : QAbstractTableModel(parent)
    , mSheet(sheet)
{
    Q_ASSERT(mSheet);

    connect(mSheet, &AdaSheet::cellChanged, this, [this](int row, int column) {
        const QModelIndex changedIndex = index(row, column);
        emit dataChanged(changedIndex, changedIndex,
                         QVector<int>() << Qt::DisplayRole << Qt::EditRole
                                        << Qt::FontRole << Qt::BackgroundRole
                                        << Qt::ForegroundRole
                                        << Qt::TextAlignmentRole);
    });
    connect(mSheet, &AdaSheet::cellRemoved, this, [this](int row, int column) {
        const QModelIndex changedIndex = index(row, column);
        emit dataChanged(changedIndex, changedIndex);
    });
    connect(mSheet, &AdaSheet::rowsAboutToBeInserted, this,
            [this](int first, int last) { beginInsertRows(QModelIndex(), first, last); });
    connect(mSheet, &AdaSheet::rowsInserted, this, [this]() { endInsertRows(); });
    connect(mSheet, &AdaSheet::rowsAboutToBeRemoved, this,
            [this](int first, int last) { beginRemoveRows(QModelIndex(), first, last); });
    connect(mSheet, &AdaSheet::rowsRemoved, this, [this]() { endRemoveRows(); });
    connect(mSheet, &AdaSheet::columnsAboutToBeInserted, this,
            [this](int first, int last) { beginInsertColumns(QModelIndex(), first, last); });
    connect(mSheet, &AdaSheet::columnsInserted, this, [this]() { endInsertColumns(); });
    connect(mSheet, &AdaSheet::columnsAboutToBeRemoved, this,
            [this](int first, int last) { beginRemoveColumns(QModelIndex(), first, last); });
    connect(mSheet, &AdaSheet::columnsRemoved, this, [this]() { endRemoveColumns(); });
}

QUndoStack *AdaTableViewModel::undoStack() const
{
    return mSheet->undoStack();
}

int AdaTableViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mSheet->rowCount();
}

int AdaTableViewModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mSheet->columnCount();
}

Qt::ItemFlags AdaTableViewModel::flags(const QModelIndex &index) const
{
    return index.isValid()
            ? QAbstractTableModel::flags(index) | Qt::ItemIsEditable
            : Qt::NoItemFlags;
}

QVariant AdaTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    AdaCell *cell = mSheet->cell(index.row(), index.column());
    if (!cell)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole: return cell->displayValue();
    case Qt::EditRole: return cell->value();
    case Qt::FontRole: return cell->hasFont() ? QVariant(cell->font()) : QVariant();
    case Qt::BackgroundRole:
        return cell->bgColor().isValid() ? QVariant(cell->bgColor()) : QVariant();
    case Qt::ForegroundRole:
        return cell->fgColor().isValid() ? QVariant(cell->fgColor()) : QVariant();
    case Qt::TextAlignmentRole:
        return cell->hasAlignment() ? QVariant(int(cell->alignment())) : QVariant();
    }
    return QVariant();
}

bool AdaTableViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole && role != Qt::FontRole
            && role != Qt::BackgroundRole && role != Qt::ForegroundRole
            && role != Qt::TextAlignmentRole)
        return false;

    const QVariant oldValue = data(index, role);
    const bool hasNewInformation = role == Qt::EditRole
            ? value.isValid() && !value.toString().isEmpty()
            : value.isValid();
    if ((!oldValue.isValid() && !hasNewInformation) || oldValue == value)
        return false;

    mSheet->undoStack()->push(new SetCellDataCommand(
            mSheet, index.row(), index.column(), oldValue, value, role));
    return true;
}

QVariant AdaTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Vertical)
        return section + 1;

    QString label;
    int column = section;
    do {
        label.prepend(QChar('A' + column % 26));
        column = column / 26 - 1;
    } while (column >= 0);
    return label;
}

bool AdaTableViewModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || row < 0 || row > rowCount() || count <= 0)
        return false;
    mSheet->undoStack()->push(new InsertRowsCommand(mSheet, row, count));
    return true;
}

bool AdaTableViewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || row < 0 || count <= 0 || row + count > rowCount())
        return false;
    mSheet->undoStack()->push(new RemoveRowsCommand(mSheet, row, count));
    return true;
}

bool AdaTableViewModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    if (parent.isValid() || column < 0 || column > columnCount() || count <= 0)
        return false;
    mSheet->undoStack()->push(new InsertColumnsCommand(mSheet, column, count));
    return true;
}

bool AdaTableViewModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (parent.isValid() || column < 0 || count <= 0
            || column + count > columnCount())
        return false;
    mSheet->undoStack()->push(new RemoveColumnsCommand(mSheet, column, count));
    return true;
}

//-------------------------------------------------------------------------------------------------
bool AdaTableViewModel::mergeCells(int row, int column,
                                   int rowSpan, int columnSpan)
{
    if (row < 0 || column < 0 || rowSpan < 1 || columnSpan < 1
            || row + rowSpan > rowCount()
            || column + columnSpan > columnCount()
            || (rowSpan == 1 && columnSpan == 1))
        return false;

    const QRect mergedArea(column, row, columnSpan, rowSpan);
    const QVector<QRect> oldSpans = mSheet->spansIntersecting(mergedArea);
    const QVector<QRect> newSpans(1, mergedArea);
    mSheet->undoStack()->push(new SetSpansCommand(
            mSheet, oldSpans, newSpans, tr("Zellen verbinden")));
    return true;
}

//-------------------------------------------------------------------------------------------------
bool AdaTableViewModel::demergeCells(const QVector<QPoint> &cells)
{
    QVector<QRect> oldSpans;
    for (const QPoint &cell : cells) {
        if (cell.y() < 0 || cell.y() >= rowCount()
                || cell.x() < 0 || cell.x() >= columnCount())
            continue;
        const QRect span = mSheet->spanAt(cell.y(), cell.x());
        if (!span.isEmpty() && !oldSpans.contains(span))
            oldSpans << span;
    }
    if (oldSpans.isEmpty())
        return false;

    mSheet->undoStack()->push(new SetSpansCommand(
            mSheet, oldSpans, QVector<QRect>(), tr("Zellverbund aufheben")));
    return true;
}
