#ifndef ADASHEET_H
#define ADASHEET_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QRect>
#include <QVector>
#include <QUndoStack>

#include "adacell.h"

class AdaModel;
class AdaSheet : public QObject
{
    Q_OBJECT
public:
    explicit AdaSheet(AdaModel *model);

    QString name() const;
    void    setName(const QString &name);

    QString script() const;
    void    setScript(const QString &script);

    QUndoStack *undoStack();

    int rowCount() const;
    int columnCount() const;

    AdaCell *cell(int row, int column) const;
    AdaCell *ensureCell(int row, int column);
    void     removeCell(int row, int column);

    QRect spanAt(int row, int column) const;
    QVector<QRect> spans() const;
    QVector<QRect> spansIntersecting(const QRect &area) const;
    void setCellSpan(int row, int column, int rowSpan, int columnSpan);

    bool insertRows(int row, int count = 1);
    bool removeRows(int row, int count = 1);
    bool insertColumns(int column, int count = 1);
    bool removeColumns(int column, int count = 1);

signals:
    void nameChanged();
    void cellCreated(int row, int column, AdaCell *cell);
    void cellRemoved(int row, int column);
    void cellChanged(int row, int column);
    void cellSpanChanged(int row, int column, int oldRowSpan, int oldColumnSpan,
                         int newRowSpan, int newColumnSpan);

    void rowsAboutToBeInserted(int first, int last);
    void rowsInserted();
    void rowsAboutToBeRemoved(int first, int last);
    void rowsRemoved();
    void columnsAboutToBeInserted(int first, int last);
    void columnsInserted();
    void columnsAboutToBeRemoved(int first, int last);
    void columnsRemoved();

private:
    enum {
        DefaultRowCount = 100,
        DefaultColumnCount = 26
    };

    static quint64 cellKey(int row, int column);
    static int keyRow(quint64 key);
    static int keyColumn(quint64 key);
    void moveCells(int row, int rowDelta, int column, int columnDelta);

    AdaModel         *mModel;
    QString           mName;
    int               mColumnCount;
    int               mRowCount;
    QHash<quint64, AdaCell*> mCells;
    QHash<AdaCell*, quint64> mCellKeys;
    QString           mScript;
    QUndoStack        mUndoStack;
};

#endif // ADASHEET_H
