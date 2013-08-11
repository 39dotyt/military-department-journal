#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QTabWidget>

#include "database.h"
#include "basewidget.h"
#include "universitygroupwidget.h"
#include "militaryprofessionwidget.h"
#include "teacherwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tabWidget = new QTabWidget(ui->centralWidget);
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onCurrentTabChanged(int)));
    tabWidget->hide();
    ui->centralWidget->layout()->addWidget(tabWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayError(QString message)
{
    ui->statusBar->showMessage(message, 2000);
}

void MainWindow::onCurrentTabChanged(int newTabIndex)
{
    if (newTabIndex == -1) {
        return;
    }
    BaseWidget *widget = static_cast<BaseWidget *>(tabWidget->widget(newTabIndex));
    ui->globalSearch->disconnect();
    connect(ui->globalSearch, SIGNAL(textChanged(QString)), widget, SIGNAL(queryChanged(QString)));
    disconnect(0, 0, this, SLOT(displayError(QString)));
    connect(widget, SIGNAL(error(QString)), this, SLOT(displayError(QString)));
}

void MainWindow::on_action_New_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Enter filename"), QDir::currentPath(),
                                                    tr("Journal (*.mdj)"));
    if (fileName.isEmpty()) {
        return;
    }
    QFile *file = new QFile(fileName);
    if (file->exists()) {
        file->remove();
    }
    if (!Database::init(fileName, true)) {
        displayError(tr("Cannot init database"));
    } else {
        initControls();
    }
}

void MainWindow::on_action_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Enter filename"), QDir::currentPath(),
                                                    tr("Journal (*.mdj)"));
    if (fileName.isEmpty()) {
        return;
    }
    if (!Database::open(fileName)) {
        displayError(tr("Cannot open database"));
    } else {
        initControls();
    }
}

void MainWindow::initControls()
{
    ui->centralWidget->setEnabled(true);
    ui->emblem->hide();
    tabWidget->show();
    tabWidget->addTab(new TeacherWidget(tabWidget), tr("Teacher"));
    tabWidget->addTab(new UniversityGroupWidget(tabWidget), tr("University Group"));
    tabWidget->addTab(new MilitaryProfessionWidget(tabWidget), tr("Military Profession"));
}
