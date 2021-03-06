#ifndef STUDENTWIDGET_H
#define STUDENTWIDGET_H

#include "basewidget.h"
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QPushButton>
#include "qt4table-steroids/lineeditdelegate.h"
#include "qt4table-steroids/spinboxdelegate.h"
#include "qt4table-steroids/checkboxdelegate.h"
#include "qt4table-steroids/relationalcombobox.h"
#include "namemorpher.h"

class StudentWidget : public BaseWidget {
  Q_OBJECT
public:
  explicit StudentWidget(QWidget *parent = 0);
  ~StudentWidget();
private:
  QLayout *createControlsLayout();
  QLineEdit *firstnameEdit;
  QLineEdit *middlenameEdit;
  QLineEdit *lastnameEdit;
  QLineEdit *firstnameDatumEdit;
  QLineEdit *middlenameDatumEdit;
  QLineEdit *lastnameDatumEdit;
  QLineEdit *firstnameAccusativeEdit;
  QLineEdit *middlenameAccusativeEdit;
  QLineEdit *lastnameAccusativeEdit;
  QDateEdit *dobEdit;
//    QCalendarWidget *dobCalendar;
  TableSteroids::RelationalComboBox *universityGroupEdit;
  QLineEdit *decreeEnrollmentNumberEdit;
//    QLineEdit *decreeExpulsionNumberEdit;
//    QComboBox *expulsionReasonEdit;
//    QComboBox *expulsedFromEdit;
//  QComboBox *troopEdit;
  QPushButton *addStudentButton;
  NameMorpher *nameMorpher;
private slots:
  void addStudent();
  void morphName();
};

#endif // STUDENTWIDGET_H
