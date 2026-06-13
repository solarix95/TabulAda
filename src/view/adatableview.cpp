#include "view/adatableview.h"
#include "model/adasheet.h"
#include "viewmodel/adatableviewmodel.h"

#include <QAbstractProxyModel>

#include <QApplication>
#include <QClipboard>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMimeData>
#include <QStringList>
#include <QUndoStack>
#include <QVector>

namespace {

QString encodeClipboardCell(const QString &value)
{
    if (!value.contains(QLatin1Char('\t'))
            && !value.contains(QLatin1Char('\n'))
            && !value.contains(QLatin1Char('\r'))
            && !value.contains(QLatin1Char('"')))
        return value;

    QString escaped = value;
    escaped.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

QVector<QStringList> decodeClipboardText(const QString &text)
{
    QVector<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false;

    for (int i = 0; i < text.size(); ++i) {
        const QChar character = text.at(i);
        if (quoted) {
            if (character == QLatin1Char('"')) {
                if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                field += character;
            }
            continue;
        }

        if (character == QLatin1Char('"') && field.isEmpty()) {
            quoted = true;
        } else if (character == QLatin1Char('\t')) {
            row << field;
            field.clear();
        } else if (character == QLatin1Char('\n') || character == QLatin1Char('\r')) {
            if (character == QLatin1Char('\r')
                    && i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n'))
                ++i;
            row << field;
            rows << row;
            row.clear();
            field.clear();
        } else {
            field += character;
        }
    }

    row << field;
    rows << row;
    if (rows.size() > 1 && rows.last().size() == 1 && rows.last().first().isEmpty()
            && (text.endsWith(QLatin1Char('\n')) || text.endsWith(QLatin1Char('\r'))))
        rows.removeLast();
    return rows;
}

QUndoStack *undoStackForModel(QAbstractItemModel *model)
{
    while (auto *proxy = qobject_cast<QAbstractProxyModel *>(model))
        model = proxy->sourceModel();
    auto *viewModel = qobject_cast<AdaTableViewModel *>(model);
    return viewModel ? viewModel->undoStack() : nullptr;
}

} // namespace

//-------------------------------------------------------------------------------------------------
AdaTableView::AdaTableView(AdaSheet *sheet, QWidget *parent)
    : QTableView(parent)
    , mSheet(sheet)
{
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    Q_ASSERT(mSheet);
    connect(mSheet, &AdaSheet::cellSpanChanged, this, [this](int, int, int, int, int, int) { refreshSpans(); });
    connect(mSheet, &AdaSheet::rowsInserted, this, &AdaTableView::refreshSpans);
    connect(mSheet, &AdaSheet::rowsRemoved, this, &AdaTableView::refreshSpans);
    connect(mSheet, &AdaSheet::columnsInserted, this, &AdaTableView::refreshSpans);
    connect(mSheet, &AdaSheet::columnsRemoved, this, &AdaTableView::refreshSpans);
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::copySelection()
{
    if (!model() || !selectionModel())
        return;

    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty() && currentIndex().isValid())
        indexes << currentIndex();
    if (indexes.isEmpty())
        return;

    int firstRow = indexes.first().row();
    int lastRow = firstRow;
    int firstColumn = indexes.first().column();
    int lastColumn = firstColumn;
    for (const QModelIndex &index : indexes) {
        firstRow = qMin(firstRow, index.row());
        lastRow = qMax(lastRow, index.row());
        firstColumn = qMin(firstColumn, index.column());
        lastColumn = qMax(lastColumn, index.column());
    }

    QStringList lines;
    for (int row = firstRow; row <= lastRow; ++row) {
        QStringList fields;
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const QModelIndex index = model()->index(row, column);
            const QString value = selectionModel()->isSelected(index)
                    || (indexes.size() == 1 && index == indexes.first())
                    ? model()->data(index, Qt::EditRole).toString()
                    : QString();
            fields << encodeClipboardCell(value);
        }
        lines << fields.join(QLatin1Char('\t'));
    }

    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::pasteClipboard()
{
    if (!model() || !selectionModel())
        return;

    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData || !mimeData->hasText())
        return;

    const QVector<QStringList> rows = decodeClipboardText(mimeData->text());
    if (rows.isEmpty())
        return;

    QModelIndex start = currentIndex();
    const QModelIndexList selected = selectionModel()->selectedIndexes();
    if (!selected.isEmpty()) {
        start = selected.first();
        for (const QModelIndex &index : selected) {
            if (index.row() < start.row()
                    || (index.row() == start.row() && index.column() < start.column()))
                start = index;
        }
    }
    if (!start.isValid())
        start = model()->index(0, 0);
    if (!start.isValid())
        return;

    int columnCount = 0;
    for (const QStringList &row : rows)
        columnCount = qMax(columnCount, row.size());

    const int requiredRows = start.row() + rows.size();
    const int requiredColumns = start.column() + columnCount;
    QUndoStack *undoStack = undoStackForModel(model());
    if (undoStack)
        undoStack->beginMacro(tr("Einfügen"));
    if (requiredRows > model()->rowCount()
            && !model()->insertRows(model()->rowCount(),
                                    requiredRows - model()->rowCount())) {
        if (undoStack)
            undoStack->endMacro();
        return;
    }
    if (requiredColumns > model()->columnCount()
            && !model()->insertColumns(model()->columnCount(),
                                       requiredColumns - model()->columnCount())) {
        if (undoStack)
            undoStack->endMacro();
        return;
    }

    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &fields = rows.at(row);
        for (int column = 0; column < fields.size(); ++column) {
            model()->setData(model()->index(start.row() + row,
                                            start.column() + column),
                             fields.at(column), Qt::EditRole);
        }
    }

    if (undoStack)
        undoStack->endMacro();

    const QModelIndex end = model()->index(requiredRows - 1, requiredColumns - 1);
    selectionModel()->select(QItemSelection(start, end),
                             QItemSelectionModel::ClearAndSelect);
    setCurrentIndex(start);
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        copySelection();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        pasteClipboard();
        event->accept();
        return;
    }
    QTableView::keyPressEvent(event);
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::mergeSelection()
{
    if (!model() || !selectionModel())
        return;

    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty() && currentIndex().isValid())
        indexes << currentIndex();
    if (indexes.size() < 2)
        return;

    QAbstractItemModel *sourceModel = model();
    while (auto *proxy = qobject_cast<QAbstractProxyModel *>(sourceModel))
        sourceModel = proxy->sourceModel();
    auto *viewModel = qobject_cast<AdaTableViewModel *>(sourceModel);
    if (!viewModel)
        return;

    int firstRow = viewModel->rowCount();
    int lastRow = -1;
    int firstColumn = viewModel->columnCount();
    int lastColumn = -1;
    for (QModelIndex index : indexes) {
        QAbstractItemModel *indexModel =
                const_cast<QAbstractItemModel *>(index.model());
        while (auto *proxy = qobject_cast<QAbstractProxyModel *>(indexModel)) {
            index = proxy->mapToSource(index);
            indexModel = proxy->sourceModel();
        }
        firstRow = qMin(firstRow, index.row());
        lastRow = qMax(lastRow, index.row());
        firstColumn = qMin(firstColumn, index.column());
        lastColumn = qMax(lastColumn, index.column());
    }

    viewModel->mergeCells(firstRow, firstColumn,
                          lastRow - firstRow + 1,
                          lastColumn - firstColumn + 1);
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::demergeSelection()
{
    if (!model() || !selectionModel())
        return;

    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty() && currentIndex().isValid())
        indexes << currentIndex();

    QAbstractItemModel *sourceModel = model();
    while (auto *proxy = qobject_cast<QAbstractProxyModel *>(sourceModel))
        sourceModel = proxy->sourceModel();
    auto *viewModel = qobject_cast<AdaTableViewModel *>(sourceModel);
    if (!viewModel)
        return;

    QVector<QPoint> cells;
    for (QModelIndex index : indexes) {
        QAbstractItemModel *indexModel =
                const_cast<QAbstractItemModel *>(index.model());
        while (auto *proxy = qobject_cast<QAbstractProxyModel *>(indexModel)) {
            index = proxy->mapToSource(index);
            indexModel = proxy->sourceModel();
        }
        cells << QPoint(index.column(), index.row());
    }
    viewModel->demergeCells(cells);
}

//-------------------------------------------------------------------------------------------------
void AdaTableView::refreshSpans()
{
    clearSpans();
    for (const QRect &span : mSheet->spans())
        setSpan(span.y(), span.x(), span.height(), span.width());
}
