#include "database.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

Database::Database(QObject *parent) :
    QObject(parent)
{
}

bool Database::init(const QString fileName, const bool test)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fileName);
    if (!db.open()) {
        return false;
    }
    QString initScript =
            "CREATE TABLE university_faculty ("
            "  id INTEGER PRIMARY KEY,"
            "  archived BOOLEAN DEFAULT 0 NOT NULL,"
            "  name TEXT NOT NULL UNIQUE"
            ");"
            "CREATE INDEX university_faculty_archived ON university_faculty (archived);"
            "CREATE TABLE military_profession ("
            "  id INTEGER PRIMARY KEY,"
            "  archived BOOLEAN DEFAULT 0 NOT NULL,"
            "  code TEXT NOT NULL UNIQUE,"
            "  name TEXT NOT NULL UNIQUE"
            ");"
            "CREATE INDEX military_profession_archived ON military_profession (archived);"
            "CREATE TABLE teacher ("
            "  id INTEGER PRIMARY KEY,"
            "  dismissed BOOLEAN DEFAULT 0 NOT NULL,"
            "  name TEXT NOT NULL"
            ");"
            "CREATE INDEX teacher_dismissed ON teacher (dismissed);"
            "CREATE TABLE troop ("
            "  id INTEGER PRIMARY KEY,"
            "  name TEXT NOT NULL UNIQUE,"
//            "  year_of_training INTEGER DEFAULT 1,"
            "  entered_at_military_department_date DATE,"
            "  graduated_from_military_department_date DATE,"
//            "  graduated BOOLEAN DEFAULT 0 NOT NULL,"
            "  curator_id INTEGER REFERENCES teacher (id),"
            "  military_profession_id INTEGER REFERENCES military_profession (id) NOT NULL"
            ");"
            "CREATE TABLE university_group ("
            "  id INTEGER PRIMARY KEY,"
            "  name TEXT NOT NULL UNIQUE,"
            "  graduated_from_university_date DATE,"
            "  faculty_id INTEGER REFERENCES university_faculty (id) NOT NULL,"
            "  troop_id INTEGER REFERENCES troop (id) NOT NULL"
            ");"
            // every univeristy group is linked with troop. If troop contains multiple groups, when we generate
            // exam lists for troop, we shoul generate separate list for every group
            "CREATE TABLE expulsion_reason (" // we should create that because SQLite does not support ENUMs
            "  id INTEGER PRIMARY KEY,"
            "  reason TEXT NOT NULL UNIQUE"
            ");"
            "CREATE TABLE expulsed_from (" // we should create that because SQLite does not support ENUMs
            "  id INTEGER PRIMARY KEY,"
            "  unit TEXT NOT NULL UNIQUE"
            ");"
            "CREATE TABLE student ("
            "  id INTEGER PRIMARY KEY,"
            "  lastname TEXT NOT NULL,"
            "  firstname TEXT NOT NULL,"
            "  middlename TEXT NOT NULL,"
            "  lastname_datum TEXT,"
            "  firstname_datum TEXT,"
            "  middlename_datum TEXT,"
            "  lastname_accusative TEXT,"
            "  firstname_accusative TEXT,"
            "  middlename_accusative TEXT,"
            "  dob DATE,"
            "  university_group_id INTEGER REFERENCES university_group (id) NOT NULL,"
            "  decree_enrollment_number TEXT," // TODO: discucss about
            "  decree_expulsion_number TEXT," // TODO: discuss about
            "  expulsion_reason_id INTEGER REFERENCES expulsion_reason (id),"
            "  expulsed_from_id INTEGER REFERENCES expulsed_from (id)"
            ");"
            "CREATE TABLE subject ("
            "  id INTEGER PRIMARY KEY,"
            "  archived BOOLEAN DEFAULT 0 NOT NULL,"
            "  name TEXT NOT NULL UNIQUE"
            ");"
            "CREATE INDEX subject_archived ON subject (archived);"
            "CREATE TABLE subject_duration ("
            "  subject_id INTEGER REFERENCES subject (id) NOT NULL,"
            "  military_profession_id INTEGER REFERENCES military_profession (id) NOT NULL,"
            "  duration INTEGER NOT NULL"
            ");"
            "CREATE UNIQUE INDEX subject_duration_unique ON subject_duration (subject_id, military_profession_id);"
            "CREATE TABLE control_type (" // we should create that because SQLite does not support ENUMs
            "  id INTEGER PRIMARY KEY,"
            "  type TEXT NOT NULL UNIQUE"
            ");"
            "CREATE TABLE evaluation ("
            "  id INTEGER PRIMARY KEY,"
            "  subject_id INTEGER REFERENCES subject (id) NOT NULL,"
            "  control_type_id INTEGER REFERENCES control_type (id) NOT NULL,"
            "  teacher_id INTEGER REFERENCES teacher (id) NOT NULL,"
            "  troop_id INTEGER REFERENCES troop (id) NOT NULL,"
            "  date DATE NOT NULL"
            ");"
            "CREATE TABLE mark ("
            "  evaluation_id INTEGER REFERENCES evaluation (id) NOT NULL,"
            "  student_id INTEGER REFERENCES student (id) NOT NULL,"
            "  value INTEGER NOT NULL,"
            "  teacher_id INTEGER REFERENCES teacher (id)," // filled only if (mark.teacher_id != evaluation.teacher_id)
            "  date DATE" // filled only if (marked.date != evaluation.date)
            ");";
    QSqlQuery query;
    for (QString queryString : initScript.split(';')) {
        if (!queryString.isEmpty()) {
            execQueryAndReturnId(&query, queryString);
        }
    }
    QStringList baseQueries = {
        "INSERT INTO control_type (type) VALUES ('Exam'), ('Credit')",
        "INSERT INTO expulsion_reason (reason) VALUES ('Poor progress'), ('Non-attendance'), "
        "('Violation of the statute')",
        "INSERT INTO expulsed_from (unit) VALUES ('University'), ('Military Department')"
    };
    for (QString queryString : baseQueries) {
        execQueryAndReturnId(&query, queryString);
    }
    if (test) {
        QVariant universityFacultyId;
        QVariant militaryProfessionId;
        QVariant troopId;
        QVariant universityGroupId;
        QVariantList studentIds;
        QVariantList subjectIds;
        universityFacultyId = execQueryAndReturnId(&query, "INSERT INTO university_faculty (name) VALUES ('AI')");
        militaryProfessionId = execQueryAndReturnId(&query, "INSERT INTO military_profession (code, name) "
                                                          "VALUES ('VUS-260100', 'Units management')");
        query.prepare("INSERT INTO troop (name, military_profession_id) VALUES ('AI41', ?)");
        query.addBindValue(militaryProfessionId);
        troopId = execQueryAndReturnId(&query);
        query.prepare("INSERT INTO university_group (name, faculty_id, troop_id) VALUES ('AI-0904', ?, ?)");
        query.addBindValue(universityFacultyId);
        query.addBindValue(troopId);
        universityGroupId = execQueryAndReturnId(&query);
        query.prepare("INSERT INTO student (lastname, firstname, middlename, lastname_datum, firstname_datum, "
                      "middlename_datum, university_group_id) VALUES "
                      "(:lastname, :firstname, :middlename, :lastname_d, :firstname_d, :middlename_d, "
                      ":university_group_id)");
        query.bindValue(":university_group_id", universityGroupId);
        for (short i = 0; i < 3; ++i) {
            query.bindValue(":lastname", QString::number(i) + "Lastname");
            query.bindValue(":firstname", QString::number(i) + "Firstname");
            query.bindValue(":middlename", QString::number(i) + "Middlename");
            query.bindValue(":lastname_d", QString::number(i) + "LastnameD");
            query.bindValue(":firstname_d", QString::number(i) + "FirstnameD");
            query.bindValue(":middlename_d", QString::number(i) + "MiddlenameD");
            studentIds.append(execQueryAndReturnId(&query));
        }
        query.prepare("INSERT INTO subject (name) VALUES (?)");
        for (QString subjectName : { "Wizard training", "Sword training", "Bebop training" }) {
            query.addBindValue(subjectName);
            subjectIds.append(execQueryAndReturnId(&query));
        }
    }
    return true;
}

bool Database::open(const QString fileName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fileName);
    if (!db.open()) {
        return false;
    }
    return true;
}

QVariant Database::execQueryAndReturnId(QSqlQuery *query)
{
    if (!query->exec()) {
        throw query->lastError();
    }
    return query->lastInsertId();
}

QVariant Database::execQueryAndReturnId(QSqlQuery *query, const QString queryString)
{
    if (!query->exec(queryString)) {
        throw query->lastError();
    }
    return query->lastInsertId();
}
