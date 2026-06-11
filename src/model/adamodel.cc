#include "model/adamodel.h"

//-------------------------------------------------------------------------------------------------
AdaModel::AdaModel(QObject *parent)
    : QObject(parent)
{
}

//-------------------------------------------------------------------------------------------------
int AdaModel::count() const
{
    return mSheets.count();
}

//-------------------------------------------------------------------------------------------------
AdaSheet *AdaModel::sheet(int index)
{
    Q_ASSERT(index >= 0 && index < mSheets.count());
    Q_ASSERT(mSheets[index]);

    return mSheets[index];
}

//-------------------------------------------------------------------------------------------------
void AdaModel::appendNewSheet()
{
    AdaSheet *sheet = new AdaSheet(this);
    sheet->setName(tr("Tabelle %1").arg(mSheets.count() + 1));
    mSheets << sheet;
    emit sheetCreated(mSheets.count() - 1);
}

//-------------------------------------------------------------------------------------------------
void AdaModel::removeSheet(int index)
{
    Q_ASSERT(index >= 0 && index < mSheets.count());

    emit sheetAboutToBeRemoved(index);
    delete mSheets.takeAt(index);
}
