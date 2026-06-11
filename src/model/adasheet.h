#ifndef ADASHEET_H
#define ADASHEET_H

#include <QObject>
#include <QString>
#include <QVector>

#include "adacell.h"

class AdaSheet : public QObject
{
    Q_OBJECT
public:
    explicit AdaSheet(QObject *parent = 0);

    QString name() const;
    void setName(const QString &name);

    QString script() const;
    void    setScript(const QString &script);

    int rowCount() const;
    int columnCount() const;

    AdaCell *cell(int row, int column) const;
    AdaCell *ensureCell(int row, int column);
    void removeCell(int row, int column);

signals:
    void nameChanged();
    void cellCreated(int row, int column, AdaCell *cell);
    void cellRemoved(int row, int column);

private:
    enum {
        DefaultRowCount = 100,
        DefaultColumnCount = 26
    };

    int cellIndex(int row, int column) const;

    QString           mName;
    QVector<AdaCell*> mCells;
    QString           mScript;
};

#endif // ADASHEET_H
