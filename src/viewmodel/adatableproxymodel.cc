#include "adatableproxymodel.h"

AdaTableProxyModel::AdaTableProxyModel(AdaSheet *sheet)
    : mSheet(sheet)
{
    Q_ASSERT(mSheet);
}


bool AdaTableProxyModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    return true;
}
