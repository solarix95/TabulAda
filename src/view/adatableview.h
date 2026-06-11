#ifndef ADATABLEVIEW_H
#define ADATABLEVIEW_H

#include <QTableView>

class AdaTableView : public QTableView
{
    Q_OBJECT
public:
    explicit AdaTableView(QWidget *parent = 0);
};

#endif // ADATABLEVIEW_H
