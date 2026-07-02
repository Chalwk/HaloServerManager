#ifndef ZIPUNPACKER_H
#define ZIPUNPACKER_H

#include <QString>

class ZipUnpacker
{
public:
    ZipUnpacker() = default;
    bool extract(const QString &zipFilePath, const QString &destDir);
};

#endif