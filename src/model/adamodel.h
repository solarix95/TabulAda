#ifndef ADAMODEL_H
#define ADAMODEL_H

#include <QList>
#include <QObject>

#include "adasheet.h"

class AdaModel : public QObject
{
    Q_OBJECT

public:
    explicit AdaModel(QObject *parent = 0);

    int count() const;
    AdaSheet *sheet(int index);

    void appendNewSheet();
    void removeSheet(int index);

signals:
    void sheetCreated(int index);
    void sheetAboutToBeRemoved(int index);
    void sheetRemoved(int index);

private:
    QList<AdaSheet*> mSheets;
};

#endif // ADAMODEL_H
