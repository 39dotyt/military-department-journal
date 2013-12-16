#include "subjectwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include "models/subjectmodel.h"

SubjectWidget::SubjectWidget(QWidget *parent)
    : BaseWidget(parent) {
  model = new SubjectModel(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  view->setModel(model);
  mainLayout->addWidget(view);
  QHBoxLayout *controlsLayout = new QHBoxLayout();
  subjectNameEdit = new QLineEdit(this);
  addSubjectButton = new QPushButton(tr("Add subject"), this);
  controlsLayout->addWidget(subjectNameEdit);
  controlsLayout->addWidget(addSubjectButton);
  connect(addSubjectButton, SIGNAL(clicked()), this, SLOT(addSubject()));
  mainLayout->addLayout(controlsLayout);
}

void SubjectWidget::addSubject() {
  QSqlQuery query;
  query.prepare("INSERT INTO subject (name) VALUES (?)");
  query.addBindValue(subjectNameEdit->text());
  subjectNameEdit->clear();
  if (!query.exec()) {
    throw query.lastError(); // TODO: replace with signal emitting
  } else {
    model->select();
  }
}
