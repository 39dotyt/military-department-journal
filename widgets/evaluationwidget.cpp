#include "evaluationwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QDir>
#include <QSqlRecord>
#include <QDebug>
#include <QSqlRelationalDelegate>
#include <QLabel>
#include <QFileDialog>
#include "qt4table-steroids/datedelegate.h"
#include "models/evaluationmodel.h"
#include "documentgenerator.h"
#include "trooprelationaldelegate.h"

EvaluationWidget::EvaluationWidget(QWidget *parent)
    : BaseWidget(parent),
      subjectEdit(new TableSteroids::RelationalComboBox("subject", "name")),
      controlTypeEdit(
        new TableSteroids::RelationalComboBox("control_type", "type")),
      teacherEdit(new TableSteroids::RelationalComboBox("teacher", "name")),
      addEvaluationButton(new QPushButton(tr("Добавить испытание"), this)),
      generateEvaluationListButton(new QPushButton(tr("Создать лист"), this)),
      marksEditorLayout(new QVBoxLayout()),
      containerLayout(new QSplitter()),
      marksEditor(new QTableWidget(this)) {
  model = new EvaluationModel(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  view->setModel(model);
  view->setItemDelegate(new QSqlRelationalDelegate(view));
  containerLayout->addWidget(view);
  QWidget *w = new QWidget(this);
  w->setLayout(marksEditorLayout);
  containerLayout->addWidget(w);
  mainLayout->addWidget(containerLayout);
  view->setMinimumWidth(550);
  QVBoxLayout *controlsLayout = new QVBoxLayout();
  subjectEdit->setMinimumWidth(0);
  teacherEdit->setMinimumWidth(0);
  DateDelegate *dateDelegate = new DateDelegate(QDate::currentDate(), this);
  dateEdit = static_cast<QDateEdit *>(dateDelegate->createEditor(this));
  view->setItemDelegateForColumn(1, dateDelegate);
  QHBoxLayout *layoutOne = new QHBoxLayout();
  layoutOne->addWidget(subjectEdit);
  layoutOne->addWidget(controlTypeEdit);
  QHBoxLayout *layoutTwo = new QHBoxLayout();
  TroopRelationalDelegate *troopDelegate = new TroopRelationalDelegate(this);
  troopEdit = static_cast<TableSteroids::RelationalComboBox *>(
        troopDelegate->createEditor(this));
  view->setItemDelegateForColumn(3, troopDelegate);
  layoutTwo->addWidget(teacherEdit);
  layoutTwo->addWidget(troopEdit);
  layoutTwo->addWidget(dateEdit);
  QHBoxLayout *layoutThree = new QHBoxLayout();
  layoutThree->addWidget(addEvaluationButton);
  layoutThree->addWidget(generateEvaluationListButton);
  controlsLayout->addLayout(layoutOne);
  controlsLayout->addLayout(layoutTwo);
  controlsLayout->addLayout(layoutThree);
//  generateEvaluationListButton->setDisabled(true);
  connect(addEvaluationButton, SIGNAL(clicked()), this, SLOT(addEvaluation()));
  connect(generateEvaluationListButton, SIGNAL(clicked()),
          this, SLOT(generateEvaluationList()));
  mainLayout->addLayout(controlsLayout);
  view->setColumnWidth(1, 100);
  view->setColumnWidth(2, 230);
  view->setColumnWidth(3, 130);
  view->setColumnWidth(4, 80);
  view->setColumnWidth(5, 170);
  marksEditorLayout->addWidget(new QLabel(tr("Оценки")));
  marksEditor->setDisabled(true);
  marksEditor->setColumnCount(6);
  // student_id
  // value (int)
  // student name
  // value (text)
  // teacher name
  // date
  marksEditor->hideColumn(0);
  marksEditor->hideColumn(1);
  marksEditor->setColumnWidth(2, 100);
  marksEditor->setColumnWidth(3, 70);
  marksEditor->setColumnWidth(4, 70);
  marksEditor->setColumnWidth(5, 60);
  marksEditor->setHorizontalHeaderItem(
        2, new QTableWidgetItem(tr("Студент")));
  marksEditor->setHorizontalHeaderItem(
        3, new QTableWidgetItem(tr("Оценка")));
  marksEditor->setHorizontalHeaderItem(
        4, new QTableWidgetItem(tr("Преподаватель")));
  marksEditor->setHorizontalHeaderItem(
        5, new QTableWidgetItem(tr("Дата")));
  marksEditor->setSelectionBehavior(QAbstractItemView::SelectRows);
  marksEditor->setItemDelegateForColumn(3, new ValueDelegate(this));
  marksEditorLayout->addWidget(marksEditor);
  connect(view->selectionModel(),
          SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
          this, SLOT(onEvaluationSelected(QModelIndex)));
}

void EvaluationWidget::addEvaluation() {
  int troopId = troopEdit->currentId();
  QVariantMap record = {
    {"subject_id", subjectEdit->currentId()},
    {"control_type_id", controlTypeEdit->currentId()},
    {"teacher_id", teacherEdit->currentId()},
    {"troop_id", troopId},
    {"date", dateEdit->date()}
  };
  int newEvaluationId = insertRecord(record);
  if (newEvaluationId == -1) {
    return;
  }
  QSqlQuery query;
  query.prepare("SELECT s.id AS id FROM student AS s, university_group AS ug"
                " WHERE s.university_group_id = ug.id AND ug.troop_id = ?");
  query.addBindValue(troopId);
  if (!execQuery(query)) {
    return;
  }
  QVariantList studentIds;
  while (query.next()) {
    studentIds.append(query.record().value("id"));
  }
  query.prepare("INSERT INTO mark (evaluation_id, student_id, value)"
                " VALUES (?, ?, 0)");
  for (QVariant studentId : studentIds) {
    query.addBindValue(newEvaluationId);
    query.addBindValue(studentId);
    if (!execQuery(query, tr("Ошибка при вставке дефолт оценок студентов"))) {
      return;
    }
  }
}

void EvaluationWidget::generateEvaluationList() {
  //TODO: do not generate lists for dates after group expulsion date
  //TODO: fix group name, if the exam date is another
  QString directoryToSaveLists = QFileDialog::getExistingDirectory(
        this, tr("Укажите директорию для сохранения ведомостей"),
        QDir::homePath());
  if (directoryToSaveLists.isEmpty()) {
    return;
  }
  QSqlQuery query;
  query.prepare(
        "SELECT uf.name AS faculty_name, ug.id AS group_id,"
        " ug.name AS group_name"
        " FROM university_faculty AS uf, university_group AS ug"
        " WHERE ug.faculty_id = uf.id AND ug.troop_id = ?");
  int troopId = troopEdit->currentId();
  query.addBindValue(troopId);
  if (!execQuery(query)) {
    return;
  }
  QStringList facultyNames;
  QVariantList groupIds;
  QStringList groupNames;
  while (query.next()) {
    facultyNames.append(query.record().value("faculty_name").toString());
    groupIds.append(query.record().value("group_id"));
    groupNames.append(query.record().value("group_name").toString());
  }
  if (groupIds.isEmpty()) {
    emit error(tr("Во взводу нет ни одной группы"));
    return;
  }
  query.prepare("SELECT mp.code AS code FROM military_profession AS mp,"
                " troop AS t"
                " WHERE mp.id = t.military_profession_id AND t.id = ?");
  query.addBindValue(troopId);
  if (!execAndNextQuery(query)) {
    return;
  }
  QString militaryProfession = query.record().value("code").toString();
  QList<QStringList> students;
  query.prepare("SELECT lastname, firstname, middlename FROM student"
                " WHERE university_group_id = ?");
  for (QVariant groupId : groupIds) {
    query.addBindValue(groupId);
    if (!execQuery(query)) {
      return;
    }
    QStringList studentsList;
    while (query.next()) {
      QString student = query.record().value("lastname").toString() +
          " " + QString(query.record().value("firstname").toString().at(0)) +
          ".";
      QString middlename = query.record().value("middlename").toString();
      if (!middlename.isEmpty()) {
        student += QString(middlename.at(0)) + ".";
      }
      studentsList.append(student);
    }
    students.append(studentsList);
  }
  QDate date = dateEdit->date();
  QString semester;
  QString year;
  if (date.month() > 2) {
    semester = "II";
    year = QString::number(date.year() - 1) + "/" +
        QString::number(date.year());
  } else {
    semester = "I";
    year = QString::number(date.year()) + "/" +
        QString::number(date.year() + 1);
  }
  QString subjectName = subjectEdit->currentText();
  QString troopName = troopEdit->currentText();
  for (int i = 0; i < groupIds.length(); ++i) {
    QString grade = groupNames.at(i).split("-").at(1).split(" ").at(0);
    if (grade.length() == 4) {
      grade = grade.left(2);
      grade = QString::number(
            QDate::currentDate().year() - QString("20" + grade).toInt());
    } else {
      grade = QString(grade.at(0));
    }
    if (!DocumentGenerator::generateExamList(
          QApplication::applicationDirPath() + "/vedomost.docx",
          directoryToSaveLists + "/vedomost_" + troopName + "_" + subjectName +
          "_" + groupIds.at(i).toString() + ".docx", semester, year,
          facultyNames.at(i), troopName, grade, militaryProfession,
          subjectName, teacherEdit->currentText(),
          date, students.at(i))) {
      emit error(tr("При генерации ведомости возникла ошибка"));
      return;
    }
  }
}

void EvaluationWidget::onEvaluationSelected(QModelIndex selection) {
  disconnect(marksEditor, SIGNAL(cellChanged(int, int)),
             this, SLOT(onMarkChanged(int, int)));
  marksEditor->setEnabled(true);
  marksEditor->setRowCount(0);
  currentEvaluationId = model->record(selection.row()).value("id");
  QSqlQuery query;
  query.prepare(
        "SELECT s.id AS student_id, s.firstname AS student_firstname,"
        " s.middlename AS student_middlename, s.lastname AS student_lastname,"
        " mv.id AS value_id, mv.name AS value_text, t.name AS teacher_name,"
        " m.date AS date"
        " FROM mark AS m, student AS s, mark_value AS mv LEFT JOIN teacher AS t"
        " ON m.teacher_id = t.id"
        " WHERE m.student_id = s.id AND m.value = mv.id"
        " AND m.evaluation_id = ? ORDER BY s.lastname ASC, m.date DESC");
  query.addBindValue(currentEvaluationId);
  if (!execQuery(query)) {
    return;
  }
  int curRow = 0;
  QSqlRecord record;
  QTableWidgetItem *item = nullptr;
  QVariantList insertedStudentIds;
  while (query.next()) {
    record = query.record();
    QVariant newStudentId = record.value("student_id");
    if (insertedStudentIds.contains(newStudentId)) {
      continue;
    } else {
      insertedStudentIds.append(newStudentId);
    }
    marksEditor->insertRow(marksEditor->rowCount());
    item = new QTableWidgetItem();
    item->setData(Qt::EditRole, newStudentId);
    marksEditor->setItem(curRow, 0, item);
    item = new QTableWidgetItem();
    item->setData(Qt::EditRole, record.value("value_id"));
    marksEditor->setItem(curRow, 1, item);
    QString studentName = record.value("student_lastname").toString();
    studentName += " " +
        QString(record.value("student_firstname").toString().at(0)) + ".";
    QString studentMiddlename = record.value("student_middlename").toString();
    if (!studentMiddlename.isEmpty()) {
      studentName += QString(studentMiddlename.at(0)) + ".";
    }
    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, studentName);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    marksEditor->setItem(curRow, 2, item);
    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole | Qt::EditRole, record.value("value_text"));
    marksEditor->setItem(curRow, 3, item);
    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, record.value("teacher_name"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    marksEditor->setItem(curRow, 4, item);
    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, record.value("date"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    marksEditor->setItem(curRow, 5, item);
    ++curRow;
  }
  connect(marksEditor, SIGNAL(cellChanged(int, int)),
          this, SLOT(onMarkChanged(int, int)));
}

void EvaluationWidget::onMarkChanged(int row, int column) {
  if (column != 3) {
    return;
  }
  QString queryString = "UPDATE mark SET value = ? WHERE evaluation_id = ?"
      " AND student_id = ?";
  QVariant date;
  QString dateString = marksEditor->item(row, 5)->data(Qt::EditRole).toString();
  if (!dateString.isEmpty()) {
    date = QDate::fromString(dateString, "yyyy-MM-dd");
    queryString += " AND date = ?";
  } else {
    queryString += " AND date IS NULL";
  }
  QSqlQuery query;
  query.prepare(queryString);
  query.addBindValue(marksEditor->item(row, 1)->data(Qt::EditRole));
  query.addBindValue(currentEvaluationId);
  query.addBindValue(marksEditor->item(row, 0)->data(Qt::EditRole));
  if (!date.isNull()) {
    query.addBindValue(date);
  }
  execQuery(query);
}


ValueDelegate::ValueDelegate(QObject *parent)
  : QStyledItemDelegate(parent) {
}

QWidget *ValueDelegate::createEditor(
    QWidget *parent, const QStyleOptionViewItem &option,
    const QModelIndex &index) const {
  return new TableSteroids::RelationalComboBox(
        "mark_value", "name", {{"order", "id"}},
        QSqlDatabase::defaultConnection, parent);
}

void ValueDelegate::setEditorData(
    QWidget *editor, const QModelIndex &index) const {
  TableSteroids::RelationalComboBox *cb =
      static_cast<TableSteroids::RelationalComboBox *>(editor);
  cb->setEditText(index.data(Qt::EditRole).toString());
}

void ValueDelegate::setModelData(
    QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const {
  TableSteroids::RelationalComboBox *cb =
      static_cast<TableSteroids::RelationalComboBox *>(editor);
  model->setData(model->index(index.row(), 1), cb->currentId());
  model->setData(index, cb->currentText());
}
