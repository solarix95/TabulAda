#include <QVBoxLayout>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "view/adatableview.h"
#include "viewmodel/adatableviewmodel.h"

//-------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
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
    connect(&mModel, &AdaModel::sheetCreated, this, [this](int index) {
        AdaSheet *sheet = mModel.sheet(index);
        const int tabIndex = ui->tabWidget->addTab(createSheetView(index), sheet->name());

        connect(sheet, &AdaSheet::nameChanged, this, [this, sheet, tabIndex]() {
            ui->tabWidget->setTabText(tabIndex, sheet->name());
        });
    });

    mModel.appendNewSheet();
}

//-------------------------------------------------------------------------------------------------
QWidget *MainWindow::createSheetView(int index)
{
    auto *baseWidget = new QWidget(ui->tabWidget);
    auto *layout     = new QVBoxLayout(baseWidget);
    auto *tableView  = new AdaTableView(baseWidget);
    auto *viewModel  = new AdaTableViewModel(mModel.sheet(index), tableView);

    tableView->setModel(viewModel);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tableView);
    return baseWidget;
}
