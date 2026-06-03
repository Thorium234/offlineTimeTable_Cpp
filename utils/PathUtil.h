#pragma once

#include <QString>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

namespace PathUtil {
    inline QString resolvePath(const QString &relativePath) {
        if (QDir::isAbsolutePath(relativePath)) {
            return relativePath;
        }
        
        // If there's no QCoreApplication instance, fallback to current working directory
        QString appDir;
        if (QCoreApplication::instance()) {
            appDir = QCoreApplication::applicationDirPath();
        } else {
            appDir = QDir::currentPath();
        }
        
        // 1. Check if the file exists directly under appDir
        QString path1 = appDir + "/" + relativePath;
        if (QFileInfo::exists(path1)) {
            return path1;
        }
        
        // 2. Check if the file exists under appDir/..
        QString path2 = appDir + "/../" + relativePath;
        if (QFileInfo::exists(path2)) {
            return path2;
        }
        
        // 3. If the file doesn't exist, check if the parent directory of path2 exists
        QFileInfo info2(path2);
        if (info2.dir().exists()) {
            return path2;
        }
        
        // 4. Default fallback: path1 (and create directory if needed)
        return path1;
    }
}
