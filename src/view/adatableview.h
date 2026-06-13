#ifndef ADATABLEVIEW_H
#define ADATABLEVIEW_H

#include <QTableView>

class AdaSheet;
class AdaTableView : public QTableView
{
    Q_OBJECT
public:
    explicit AdaTableView(AdaSheet *sheet, QWidget *parent = 0);

public slots:
    void copySelection();
    void pasteClipboard();
    void mergeSelection();
    void demergeSelection();
    void refreshSpans();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    AdaSheet *mSheet;
};

#endif // ADATABLEVIEW_H
