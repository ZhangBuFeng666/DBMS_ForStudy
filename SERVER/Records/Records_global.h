#ifndef RECORDS_GLOBAL_H
#define RECORDS_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(RECORDS_LIBRARY)
#define RECORDS_EXPORT Q_DECL_EXPORT
#else
#define RECORDS_EXPORT Q_DECL_IMPORT
#endif

#endif // RECORDS_GLOBAL_H
