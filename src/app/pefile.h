#ifndef PEFILE_H
#define PEFILE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include <pe_bliss.h>

#include "entry.h"

using namespace pe_bliss;

class PeFile : public QObject
{
    Q_OBJECT

public:
    explicit PeFile(const QString &fileName, QObject* parent = nullptr);
    ~PeFile();

    bool apply(const QString &libraryName, const QString &libraryFile, QStringList libraryFunctions, QList<FunctionEntry> targetFunctions, const QString &sectionName) const;
    bool write() const;

private:
    const QString fileName; // TODO: add & reference here?
    pe_base* image = nullptr;

    bool read();
    const QList<FunctionEntry> buildAddressOfFunctions(const QString &libraryName) const;
    bool patchFunctions(const QString &libraryName, const QList<FunctionEntry> &functions, const QString &sectionName) const;
};

#endif // PEFILE_H
