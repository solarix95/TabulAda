#include <QActionGroup>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "view/adatableview.h"
#include "view/iconfactory.h"
#include "viewmodel/adatableviewmodel.h"
#include "viewmodel/adatableproxymodel.h"

//-------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mUndoGroup(this)
{
    ui->setupUi(this);
}

//-------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

//-------------------------------------------------------------------------------------------------
void MainWindow::init()
{
    setupMenu();
    setupToolbar();

    connect(&mModel, &AdaModel::sheetCreated, this, [this](int index) {
        AdaSheet *sheet = mModel.sheet(index);
        mUndoGroup.addStack(sheet->undoStack());
        QWidget *sheetView = createSheetView(sheet);
        const int tabIndex = ui->tabWidget->addTab(sheetView, sheet->name());
        ui->tabWidget->setCurrentIndex(tabIndex);

        connect(sheet, &AdaSheet::nameChanged, this, [this, sheet, sheetView]() {
            const int currentTabIndex = ui->tabWidget->indexOf(sheetView);
            if (currentTabIndex >= 0)
                ui->tabWidget->setTabText(currentTabIndex, sheet->name());
        });
    });

    connect(&mModel, &AdaModel::sheetAboutToBeRemoved, this, [this](int index) {
        QWidget *sheetView = ui->tabWidget->widget(index);
        ui->tabWidget->removeTab(index);
        delete sheetView;
    });

    connect(&mModel, &AdaModel::sheetRemoved, this, [this](int) {
        if (mModel.count() == 0)
            mModel.appendNewSheet();
    });

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested,
            &mModel, &AdaModel::removeSheet);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        mUndoGroup.setActiveStack(index >= 0 && index < mModel.count()
                                  ? mModel.sheet(index)->undoStack() : nullptr);
    });

    auto *newSheetButton = new QToolButton(ui->tabWidget);
    newSheetButton->setText(QStringLiteral("+"));
    newSheetButton->setToolTip(tr("Neue Tabelle"));
    ui->tabWidget->setCornerWidget(newSheetButton, Qt::TopRightCorner);
    connect(newSheetButton, &QToolButton::clicked,
            &mModel, &AdaModel::appendNewSheet);

    mModel.appendNewSheet();
}

//-------------------------------------------------------------------------------------------------
QWidget *MainWindow::createSheetView(AdaSheet *sheet)
{
    auto *baseWidget = new QWidget(ui->tabWidget);
    auto *layout     = new QVBoxLayout(baseWidget);
    auto *tableView  = new AdaTableView(sheet, baseWidget);
    auto *viewModel  = new AdaTableViewModel(sheet, tableView);
    auto *proxyModel = new AdaTableProxyModel(sheet);

    proxyModel->setSourceModel(viewModel);
    tableView->setModel(proxyModel);
    tableView->refreshSpans();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tableView);
    return baseWidget;
}

//-------------------------------------------------------------------------------------------------
void MainWindow::applyToSelection(const QVariant &value, int role)
{
    QWidget *page = ui->tabWidget->currentWidget();
    auto *tableView = page ? page->findChild<AdaTableView *>() : nullptr;
    if (!tableView || !tableView->model() || !tableView->selectionModel())
        return;

    QModelIndexList indexes = tableView->selectionModel()->selectedIndexes();
    if (indexes.isEmpty() && tableView->currentIndex().isValid())
        indexes << tableView->currentIndex();

    const int sheetIndex = ui->tabWidget->currentIndex();
    QUndoStack *undoStack = sheetIndex >= 0 && sheetIndex < mModel.count()
            ? mModel.sheet(sheetIndex)->undoStack() : nullptr;
    if (undoStack)
        undoStack->beginMacro(tr("Formatierung ändern"));
    for (const QModelIndex &index : indexes)
        tableView->model()->setData(index, value, role);
    if (undoStack)
        undoStack->endMacro();
}

//-------------------------------------------------------------------------------------------------
void MainWindow::setupMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu(tr("&Bearbeiten"));
    QAction *undoAction = mUndoGroup.createUndoAction(editMenu, tr("&Rückgängig"));
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);
    QAction *redoAction = mUndoGroup.createRedoAction(editMenu, tr("&Wiederholen"));
    redoAction->setShortcuts(QKeySequence::Redo);
    editMenu->addAction(redoAction);
}

//-------------------------------------------------------------------------------------------------
void MainWindow::setupToolbar()
{
    QToolBar *toolbar = addToolBar(tr("Formatierung"));
    toolbar->setObjectName(QStringLiteral("formatToolbar"));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(18, 18));

    auto *fontBox = new QFontComboBox(toolbar);
    fontBox->setToolTip(tr("Schriftart"));
    toolbar->addWidget(fontBox);
    connect(fontBox, &QFontComboBox::currentFontChanged, this,
            [this](const QFont &selectedFont) {
        QWidget *page = ui->tabWidget->currentWidget();
        auto *tableView = page ? page->findChild<AdaTableView *>() : nullptr;
        if (!tableView || !tableView->currentIndex().isValid())
            return;

        QVariant fontData = tableView->currentIndex().data(Qt::FontRole);
        QFont cellFont = fontData.isValid() ? qvariant_cast<QFont>(fontData)
                                            : tableView->font();
        cellFont.setFamily(selectedFont.family());
        applyToSelection(cellFont, Qt::FontRole);
    });

    auto *fontSizeBox = new QComboBox(toolbar);
    fontSizeBox->setEditable(true);
    fontSizeBox->setInsertPolicy(QComboBox::NoInsert);
    fontSizeBox->setToolTip(tr("Schriftgröße"));
    for (int size : QFontDatabase::standardSizes())
        fontSizeBox->addItem(QString::number(size));
    fontSizeBox->setCurrentText(QString::number(font().pointSize()));
    toolbar->addWidget(fontSizeBox);
    connect(fontSizeBox, &QComboBox::currentTextChanged, this,
            [this](const QString &text) {
        bool valid = false;
        const int pointSize = text.toInt(&valid);
        QWidget *page = ui->tabWidget->currentWidget();
        auto *tableView = page ? page->findChild<AdaTableView *>() : nullptr;
        if (!valid || pointSize <= 0 || !tableView
                || !tableView->currentIndex().isValid())
            return;

        QVariant fontData = tableView->currentIndex().data(Qt::FontRole);
        QFont cellFont = fontData.isValid() ? qvariant_cast<QFont>(fontData)
                                            : tableView->font();
        cellFont.setPointSize(pointSize);
        applyToSelection(cellFont, Qt::FontRole);
    });

    toolbar->addSeparator();
    QAction *foregroundAction = toolbar->addAction(
            IconFactory::create(IconFactory::TextColor), tr("Textfarbe"));
    connect(foregroundAction, &QAction::triggered, this, [this]() {
        const QColor color = QColorDialog::getColor(Qt::black, this, tr("Textfarbe"));
        if (color.isValid())
            applyToSelection(color, Qt::ForegroundRole);
    });
    QAction *backgroundAction = toolbar->addAction(
            IconFactory::create(IconFactory::BackgroundColor), tr("Hintergrund"));
    connect(backgroundAction, &QAction::triggered, this, [this]() {
        const QColor color = QColorDialog::getColor(Qt::white, this, tr("Hintergrundfarbe"));
        if (color.isValid())
            applyToSelection(color, Qt::BackgroundRole);
    });

    toolbar->addSeparator();
    auto *alignmentGroup = new QActionGroup(toolbar);
    alignmentGroup->setExclusive(true);
    const struct {
        QString text;
        Qt::Alignment alignment;
        IconFactory::Icon icon;
    } alignments[] = {
        { tr("Links"), Qt::AlignLeft | Qt::AlignVCenter, IconFactory::AlignLeft },
        { tr("Zentriert"), Qt::AlignHCenter | Qt::AlignVCenter, IconFactory::AlignCenter },
        { tr("Rechts"), Qt::AlignRight | Qt::AlignVCenter, IconFactory::AlignRight }
    };
    for (const auto &item : alignments) {
        QAction *action = toolbar->addAction(IconFactory::create(item.icon), item.text);
        action->setCheckable(true);
        alignmentGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, item]() {
            applyToSelection(int(item.alignment), Qt::TextAlignmentRole);
        });
    }
    toolbar->addSeparator();
    QAction *mergeAction = toolbar->addAction(
            IconFactory::create(IconFactory::MergeCells), tr("Zellen verbinden"));
    connect(mergeAction, &QAction::triggered, this, [this]() {
        QWidget *page = ui->tabWidget->currentWidget();
        auto *tableView = page ? page->findChild<AdaTableView *>() : nullptr;
        if (tableView)
            tableView->mergeSelection();
    });
    QAction *demergeAction = toolbar->addAction(
            IconFactory::create(IconFactory::SplitCells), tr("Zellverbund aufheben"));
    connect(demergeAction, &QAction::triggered, this, [this]() {
        QWidget *page = ui->tabWidget->currentWidget();
        auto *tableView = page ? page->findChild<AdaTableView *>() : nullptr;
        if (tableView)
            tableView->demergeSelection();
    });
}
