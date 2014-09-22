#include "file-cache-mgr.h"

#include "CppSQLite3.h"
#include <cstdio>
#include <QDir>
#include <QDebug>

#include "configurator.h"
#include "seafile-applet.h"

enum {
    FILE_CACHE_ID_MAX = 40,
    FILE_CACHE_PATH_MAX = 255,
};

FileCacheManager::FileCacheManager()
  : enabled_(true), closed_(true), db_(new CppSQLite3DB)
{
    qDebug("[file cache] sqlite version %s loaded", db_->SQLiteVersion());
    db_path_ = new QString(QDir(seafApplet->configurator()->seafileDir()).absoluteFilePath("file-cache.db"));
}

FileCacheManager::~FileCacheManager()
{
    try {
        close();
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
    }
}

void FileCacheManager::open()
{
    if (!closed_)
        return;
    try {
        db_->open(db_path_->toUtf8().constData());
        createTableIfNotExist();
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
        qWarning("[file cache] set to be disabled");
        enabled_ = false;
        return;
    }

}

void FileCacheManager::close()
{
    if (enabled_ && !closed_ )
        db_->close();
    delete db_;
    delete db_path_;
}

void FileCacheManager::createTableIfNotExist()
{
    if (!enabled_)
        return;
    static const char sql[] = "CREATE TABLE IF NOT EXISTS FileCache "
      " (oid VARCHAR(40), file_location VARCHAR(256), PRIMARY KEY(oid))";

    try {
        db_->execQuery(sql);
    } catch (CppSQLite3Exception &e) {
        qWarning("[file cache] %s", e.errorMessage());
        return;
    }
}

QString FileCacheManager::get(const QString &oid)
{
    if (!enabled_ || oid.size() > FILE_CACHE_ID_MAX)
        return "";

    static const char sql[] = "SELECT file_location FROM FileCache WHERE oid='%s'";

    char buf[64 + FILE_CACHE_ID_MAX];
    snprintf(buf, 63 + FILE_CACHE_ID_MAX, sql, oid.toAscii().constData());
    CppSQLite3Query q;
    try {
        q = db_->execQuery(buf);
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return "";
    }

    if (q.eof() || q.numFields() < 1) {
        return "";
    }

    QString file_location = QString::fromUtf8(q.getStringField(0));
    if (file_location.isEmpty() ||
        !QFileInfo(file_location).isFile()) {
        qDebug("[file cache] file %s does not exist", file_location.toUtf8().constData());
        remove(oid);
        return "";
    } else {
        qDebug("[file cache] file %s found", file_location.toUtf8().constData());
        return file_location;
    }
    return "";
}

void FileCacheManager::set(const QString &oid, const QString &file_location)
{
    if (!enabled_ || oid.size() > FILE_CACHE_ID_MAX)
        return;

    if (file_location.size() > FILE_CACHE_PATH_MAX) //too large to fit in
        return;

    if (file_location.isEmpty() ||
        !QFileInfo(file_location).isFile()) {
        qDebug("[file cache] file %s does not exist", file_location.toUtf8().constData());
        return remove(oid);
    }

    static const char sql[] = "INSERT OR REPLACE INTO FileCache VALUES ('%s', '%s') ";

    char buf[64 + FILE_CACHE_ID_MAX + FILE_CACHE_PATH_MAX];
    snprintf(buf, 63 + FILE_CACHE_ID_MAX + FILE_CACHE_PATH_MAX, sql,
             oid.toAscii().constData(),
             file_location.toUtf8().constData());
    try {
        CppSQLite3Query q = db_->execQuery(buf);
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return;
    }
}

void FileCacheManager::remove(const QString &oid)
{
    static const char sql[] = "DELETE FROM FileCache WHERE oid='%s'";
    char buf[64 + FILE_CACHE_ID_MAX];
    snprintf(buf, 63 + FILE_CACHE_ID_MAX, sql, oid.toAscii().constData());
    try {
        CppSQLite3Query q = db_->execQuery(buf);
    } catch (CppSQLite3Exception &e) {
        qDebug("[file cache] %s", e.errorMessage());
        return;
    }
}
