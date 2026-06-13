#ifndef ADATABLEVIEWMODEL_H
#define ADATABLEVIEWMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QPoint>
#include <QVector>

class AdaCell;
class AdaSheet;
class QUndoStack;
class AdaTableViewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AdaTableViewModel(AdaSheet *sheet, QObject *parent = 0);

    QUndoStack *undoStack() const;

    bool mergeCells(int row, int column, int rowSpan, int columnSpan);
    bool demergeCells(const QVector<QPoint> &cells);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool insertRows(int row, int count = 1,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count = 1,
                    const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count = 1,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count = 1,
                       const QModelIndex &parent = QModelIndex()) override;

private:
    AdaSheet *mSheet;
};

#endif // ADATABLEVIEWMODEL_H
