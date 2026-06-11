#include "view/adatableview.h"

AdaTableView::AdaTableView(QWidget *parent)
    : QTableView(parent)
{
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
}
