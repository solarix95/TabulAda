#ifndef ADATABLEPROXYMODEL_H
#define ADATABLEPROXYMODEL_H

#include <QSortFilterProxyModel>

class AdaSheet;
class AdaTableProxyModel : public QSortFilterProxyModel
{
public:
    AdaTableProxyModel(AdaSheet *sheet);

protected:
    virtual bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;

private:
    AdaSheet *mSheet;
};

#endif // ADATABLEPROXYMODEL_H
