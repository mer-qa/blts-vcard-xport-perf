/*
 * blts-vcard-xport-perf - vcard storage performance test suite
 *
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Martin Kampas <martin.kampas@jollamobile.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BLTS_VCARD_XPORT_PERF_H
#define BLTS_VCARD_XPORT_PERF_H

#ifdef __cplusplus

#include <QtCore/QObject>
#include <QtCore/QString>

namespace QtContacts {
    class QContact;
    class QContactId;
    class QContactManager;
}

class BltsVCardXportPerf : public QObject
{
    Q_OBJECT

public:
    bool test(const QString &managerName, const QString &tmpDirPrefix, int count);

private:
    static QList<QtContacts::QContact> generateContacts(int count);
    static QString randomString(int len);
    static QString randomPhoneNumber();
    static QList<QtContacts::QContactId> contactIds(const QList<QtContacts::QContact> &contacts);
    static int reportExtendedResult(const QString &tag, double value, const QString &unit);
};

#endif // __cplusplus

/*
 * C API
 */

#ifdef __cplusplus
extern "C" {
#endif

int blts_vcard_xport_perf_init(int argc, char **argv);
int blts_vcard_xport_perf_test(const char *manager_name, const char *tmp_dir_prefix, int count);

#ifdef __cplusplus
}
#endif

#endif // BLTS_VCARD_XPORT_PERF_H
