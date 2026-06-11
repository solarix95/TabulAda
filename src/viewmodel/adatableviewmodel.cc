#include "viewmodel/adatableviewmodel.h"
#include "model/adasheet.h"

AdaTableViewModel::AdaTableViewModel(AdaSheet *sheet, QObject *parent)
    : QAbstractTableModel(parent),
      mSheet(sheet)
{
    Q_ASSERT(mSheet);

    connect(mSheet, &AdaSheet::cellCreated, this,
            [this](int row, int column, AdaCell *cell) {
        observeCell(row, column, cell);
    });
    connect(mSheet, &AdaSheet::cellRemoved, this, [this](int row, int column) {
        const QModelIndex changedIndex = index(row, column);
        emit dataChanged(changedIndex, changedIndex);
    });
}

int AdaTableViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mSheet->rowCount();
}

int AdaTableViewModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mSheet->columnCount();
}

Qt::ItemFlags AdaTableViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QVariant AdaTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    AdaCell *cell = mSheet->cell(index.row(), index.column());
    if (!cell)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        return cell->displayValue();
    case Qt::EditRole:
        return cell->value();
    case Qt::FontRole:
        return cell->hasFont() ? QVariant(cell->font()) : QVariant();
    case Qt::BackgroundRole:
        return cell->bgColor().isValid() ? QVariant(cell->bgColor()) : QVariant();
    case Qt::ForegroundRole:
        return cell->fgColor().isValid() ? QVariant(cell->fgColor()) : QVariant();
    }

    return QVariant();
}

bool AdaTableViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole && role != Qt::FontRole
            && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
        return false;

    const bool hasNewInformation =
            role == Qt::EditRole
            ? value.isValid() && !value.toString().isEmpty()
            : value.isValid();

    AdaCell *cell = mSheet->cell(index.row(), index.column());
    if (!cell && !hasNewInformation)
        return false;
    if (!cell)
        cell = mSheet->ensureCell(index.row(), index.column());

    switch (role) {
    case Qt::EditRole:
        if (cell->value() == value)
            return false;
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
    }

    if (!cell->hasInformation())
        mSheet->removeCell(index.row(), index.column());
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

void AdaTableViewModel::observeCell(int row, int column, AdaCell *cell)
{
    connect(cell, &AdaCell::changed, this, [this, row, column]() {
        const QModelIndex changedIndex = index(row, column);
        emit dataChanged(changedIndex, changedIndex,
                         QVector<int>() << Qt::DisplayRole << Qt::EditRole
                                        << Qt::FontRole << Qt::BackgroundRole
                                        << Qt::ForegroundRole);
    });
}
