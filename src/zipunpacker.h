// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

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