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

#include <fcntl.h>

#include <QtContacts/QContact>
#include <QtContacts/QContactAddress>
#include <QtContacts/QContactEmailAddress>
#include <QtContacts/QContactGender>
#include <QtContacts/QContactId>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactName>
#include <QtContacts/QContactOrganization>
#include <QtContacts/QContactPhoneNumber>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTemporaryFile>
#include <QtVersit/QVersitContactExporter>
#include <QtVersit/QVersitContactImporter>
#include <QtVersit/QVersitReader>
#include <QtVersit/QVersitWriter>

extern "C" {
#include <blts_log.h>
#include <blts_reporting.h>
#include <blts_timing.h>
}

#include "blts-vcard-xport-perf.h"

#define goto_fail_if(EXPR)                                              \
    if (EXPR) {                                                         \
        BLTS_ERROR("%s: (" # EXPR ") failed\n", __FUNCTION__);          \
        goto fail;                                                      \
    }

#define goto_fail_if_MSG(EXPR, MSG)                                     \
    if (EXPR) {                                                         \
        BLTS_ERROR("%s: (" # EXPR ") failed: %s\n", __FUNCTION__, MSG); \
        goto fail;                                                      \
    }

using namespace QtContacts;
using namespace QtVersit;

bool BltsVCardXportPerf::test(const QString &managerName, const QString &tmpDirPrefix, int count)
{
    if (!managerName.isEmpty() && !QContactManager::availableManagers().contains(managerName)) {
        BLTS_ERROR("%s: manager not available: '%s'\n", __FUNCTION__, qPrintable(managerName));
        return false;
    }

    QList<QContactId> importedContactIds;

    // Prepare temporary directory

    QDir tmpDir(QDir(tmpDirPrefix).filePath("blts-vcard-xport-perf.tmp"));
    if (!tmpDir.exists() && !tmpDir.mkpath(".")) {
        BLTS_ERROR("%s: failed to create temporary directory '%s'\n", __FUNCTION__,
                qPrintable(tmpDir.absolutePath()));
        return false;
    }

    foreach (const QString &entry, tmpDir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot)) {
        if (!tmpDir.remove(entry)) {
            BLTS_ERROR("%s: failed to cleanup tmp directory: failed to remove entry '%s'\n",
                    __FUNCTION__, qPrintable(entry));
            return false;
        }
    }

    bool retv = false;

    // Test VCF file import
    {
        QContactManager manager(managerName);

        QList<QContact> contacts = generateContacts(count);

        QVersitContactExporter exporter;
        goto_fail_if (!exporter.exportContacts(contacts));

        QFile allContactsFile(tmpDir.filePath("all_contacts.vcf"));
        goto_fail_if (!allContactsFile.open(QIODevice::ReadWrite));

        // Prevent caching so the read test gives meaningful results for the generated-just-now file
        goto_fail_if (posix_fadvise(allContactsFile.handle(), 0, 0, POSIX_FADV_DONTNEED) != 0);

        QVersitWriter writer(&allContactsFile);
        goto_fail_if (!writer.startWriting(exporter.documents()));
        goto_fail_if (!writer.waitForFinished());

        goto_fail_if (!allContactsFile.seek(0));
        goto_fail_if (!allContactsFile.flush());
        goto_fail_if (posix_fadvise(allContactsFile.handle(), 0, 0, POSIX_FADV_NORMAL) != 0);

        timing_start();

        QVersitReader reader(&allContactsFile);
        goto_fail_if (!reader.startReading());
        goto_fail_if (!reader.waitForFinished());

        QVersitContactImporter importer;
        goto_fail_if (!importer.importDocuments(reader.results()));

        timing_stop();

        reportExtendedResult("import.read-from-file.elapsed", timing_elapsed(), "s");

        timing_start();

        QList<QContact> importedContacts = importer.contacts();

        goto_fail_if (!manager.saveContacts(&importedContacts));

        timing_stop();

        reportExtendedResult("import.save-to-backend.elapsed", timing_elapsed(), "s");

        importedContactIds = contactIds(importedContacts);
    }

    // Test export to separate VCF files
    {
        QContactManager manager(managerName);

        timing_start();

        QList<QContact> contacts = manager.contacts(importedContactIds);
        goto_fail_if (manager.error() != QContactManager::NoError);

        timing_stop();

        reportExtendedResult("export.load-from-backend.elapsed", timing_elapsed(), "s");

        timing_start();

        for (int i = 0; i < contacts.count(); ++i) {
            const QContact &contact = contacts.at(i);

            QVersitContactExporter exporter;
            goto_fail_if (!exporter.exportContacts(QList<QContact>() << contact));

            QFile file(tmpDir.filePath(QString("contact_%1.vcf").arg(i)));
            goto_fail_if (!file.open(QIODevice::ReadWrite));

            QVersitWriter writer(&file);
            goto_fail_if (!writer.startWriting(exporter.documents()));
            goto_fail_if (!writer.waitForFinished());
        }

        timing_stop();

        reportExtendedResult("export.write-to-files.elapsed", timing_elapsed(), "s");
    }

    // Test contacts deletion
    {
        QContactManager manager(managerName);

        timing_start();

        goto_fail_if (!manager.removeContacts(importedContactIds));

        timing_stop();

        reportExtendedResult("remove.elapsed", timing_elapsed(), "s");
    }

    retv = true;

fail:
    // For debugging purposes - do not cleanup tmp directory
    if (QProcessEnvironment::systemEnvironment().contains("BLTS_VCARD_XPORT_PERF_NO_CLEANUP")) {
        return retv;
    }

    foreach (const QString &entry, tmpDir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot)) {
        if (!tmpDir.remove(entry)) {
            BLTS_WARNING("%s: failed to cleanup tmp directory: failed to remove entry '%s'\n",
                    __FUNCTION__, qPrintable(entry));
        }
    }

    if (tmpDir.exists() && tmpDir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot).count() == 0
            && !tmpDir.rmdir(tmpDir.absolutePath())) {
        BLTS_WARNING("%s: failed to remove temporary directory '%s'\n", __FUNCTION__,
                qPrintable(tmpDir.absolutePath()));
    }

    return retv;
}

QList<QContact> BltsVCardXportPerf::generateContacts(int count)
{
    QList<QContact> contacts;
    contacts.reserve(count);

    for (int i = 0; i < count; ++i) {
        QContact contact;

        QContactName name;
        name.setFirstName(randomString(15));
        name.setLastName(randomString(15));
        contact.saveDetail(&name);

        QContactAddress address;
        address.setStreet(randomString(15));
        address.setLocality(randomString(20));
        contact.saveDetail(&address);

        QContactPhoneNumber landlinePhone;
        landlinePhone.setNumber(randomPhoneNumber());
        landlinePhone.setSubTypes(QList<int>() << QContactPhoneNumber::SubTypeLandline);
        contact.saveDetail(&landlinePhone);

        QContactPhoneNumber mobilePhone;
        mobilePhone.setNumber(randomPhoneNumber());
        mobilePhone.setSubTypes(QList<int>() << QContactPhoneNumber::SubTypeMobile);
        contact.saveDetail(&mobilePhone);

        QContactEmailAddress email;
        email.setEmailAddress(name.firstName() + "." + name.lastName() + "@example.com");
        contact.saveDetail(&email);

        static const QList<QContactGender::GenderField> genders =
            QList<QContactGender::GenderField>()
            << QContactGender::GenderUnspecified
            << QContactGender::GenderMale
            << QContactGender::GenderFemale;
        QContactGender gender;
        gender.setGender(genders[qrand() % genders.count()]);
        contact.saveDetail(&gender);

        QContactOrganization organization;
        organization.setName(randomString(15));
        organization.setDepartment(QStringList() << randomString(15));
        organization.setRole(randomString(15));
        contact.saveDetail(&organization);

        contacts.append(contact);
    }

    return contacts;
}

QString BltsVCardXportPerf::randomString(int len)
{
    QString retv;
    retv.reserve(len);

    for (int i = 0; i < len; ++i) {
        retv.append('a' + qrand() % ('z' - 'a'));
    }

    return retv;
}

QString BltsVCardXportPerf::randomPhoneNumber()
{
    QString retv;
    retv.reserve(13);

    retv.append("+420");

    for (int i = 0; i < 9; ++i) {
        retv.append('0' + qrand() % ('9' - '0'));
    }

    return retv;
}

QList<QtContacts::QContactId> BltsVCardXportPerf::contactIds(const QList<QContact> &contacts)
{
    QList<QContactId> contactIds;
    contactIds.reserve(contacts.count());

    foreach (const QContact &contact, contacts) {
        contactIds.append(contact.id());
    }

    return contactIds;
}

int BltsVCardXportPerf::reportExtendedResult(const QString &tag, double value, const QString &unit)
{
    return blts_report_extended_result(tag.toLocal8Bit().data(), value, unit.toLocal8Bit().data(),
            0);
}

/*
 * C API
 */

static int argc;
static char **argv;

int blts_vcard_xport_perf_init(int argc, char **argv)
{
    ::argc = argc;
    ::argv = argv;
    return 0;
}

int blts_vcard_xport_perf_test(const char *manager_name, const char *tmp_dir_prefix, int count)
{
    QCoreApplication app(argc, argv);

    qsrand(QDateTime::currentDateTime().toTime_t());

    return BltsVCardXportPerf().test(manager_name,tmp_dir_prefix, count)
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
