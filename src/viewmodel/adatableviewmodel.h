#ifndef ADATABLEVIEWMODEL_H
#define ADATABLEVIEWMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>

class AdaCell;
class AdaSheet;
class AdaTableViewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AdaTableViewModel(AdaSheet *sheet, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    void observeCell(int row, int column, AdaCell *cell);

    AdaSheet *mSheet;
};

#endif // ADATABLEVIEWMODEL_H
