#include <QFile>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QSettings>

#include "patcher.h"
#include "global.h"
#include "fileutils.h"
#include "pefile.h"

bool Patcher::isPatched(const QDir &dir)
{
    int count = 0;

    for (const FileEntry &file : files) {
        for (const TargetEntry &target : file.getTargets()) {
            // Check if target file is patched.
            if (FileUtils::isValid(dir, file, target, true)) {
                count++;

                break;
            }
        }
    }
    qDebug() << QString("%1 av %2").arg(count).arg(files.length());

    return count == files.length();
}

void Patcher::copyFiles(const QDir &dir)
{
    bool success = false;

    for (const QString &fileName : patch_library_runtime_dependencies) {
        QFile sourceFile = QFile(fileName);
        QFile destinationFile = dir.filePath(fileName);

        if (!destinationFile.exists() || destinationFile.remove()) {
            success = sourceFile.copy(destinationFile.fileName());
        }
    }

    if (success) {
        qDebug() << QT_TR_NOOP("Copying runtime dependencies.");
    } else {
        qDebug() << QT_TR_NOOP("Error: Could not copy runtime dependencies, missing from application directory.");
    }
}

bool Patcher::patchFile(const QDir &dir, const FileEntry &fileEntry, const TargetEntry &target)
{
    QFile file = dir.filePath(fileEntry.getName());

    qDebug() << QT_TR_NOOP(QString("Patching: %1").arg(file.fileName()));

    // Create PeFile instance for this particular target.
    PeFile* peFile = new PeFile(file);

    // Apply PE and binary patches.
    peFile->apply(patch_library_name, patch_library_file, patch_library_functions, target.getAddresses(), patch_pe_section);

    // Write PE to file.
    peFile->write();

    delete peFile;

    return FileUtils::isValid(dir, fileEntry, target, true);
}

bool Patcher::patch(QWidget* parent, const QDir &dir)
{
    int count = 0;

    // Scanning for valid files to start patching.
    for (const FileEntry &file : files) {
        for (const TargetEntry &target : file.getTargets()) {
            // Validate target file against stored checksum.
            if (FileUtils::isValid(dir, file, target, false)) {
                // Backup original file.
                FileUtils::backup(dir, file);

                // Patch target file.
                if (!patchFile(dir, file, target)) {
                    QMessageBox::warning(parent, "Warning", QT_TR_NOOP(QString("Invalid checksum for patched file %1, aborting!").arg(file.getName())));
                    undoPatch(dir);

                    return false;
                }

                count++;
            }
        }
    }

    // Something is not right, reverting back to backup files.
    if (count < files.length()) {
        QMessageBox::warning(parent, "Warning", QT_TR_NOOP("Not all files where patched, files have been restored, please try patching again."));
        undoPatch(dir);

        return false;
    }

    // Copy needed libraries.
    copyFiles(dir);

    return true;
}

void Patcher::undoPatch(const QDir &dir) {
    // Restore patched files.
    for (const FileEntry &fileEntry : files) {
        FileUtils::restore(dir, fileEntry);
    }

    // Delete backed up game files.
    for (const FileEntry &fileEntry : files) {
        QFile::remove(dir.filePath(FileUtils::appendToName(dir, fileEntry, game_backup_file_suffix)));
    }

    // Delete patch library file.
    QFile::remove(dir.filePath(patch_library_file));

    // Remove patch runtime dependencies.
    for (const QString &fileName : patch_library_runtime_dependencies) {
        QFile::remove(dir.filePath(fileName));
    }

    // Remove network configuration file.
    QFile::remove(dir.filePath(patch_network_configuration_file));
}

void Patcher::generateNetworkConfigFile(const QDir &dir, const QNetworkAddressEntry &address)
{
    QFile file = dir.filePath(patch_network_configuration_file);
    QSettings settings(file.fileName(), QSettings::IniFormat);

    settings.beginGroup(patch_library_name);
        settings.setValue(patch_network_configuration_address, address.ip().toString());
        settings.setValue(patch_network_configuration_broadcast, address.broadcast().toString());
        settings.setValue(patch_network_configuration_netmask, address.netmask().toString());
    settings.endGroup();

    qDebug() << QT_TR_NOOP(QString("Generated network configuration, saved to: %1").arg(file.fileName()));
}
