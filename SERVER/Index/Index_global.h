#ifndef INDEX_GLOBAL_H
#define INDEX_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(INDEX_LIBRARY)
#define INDEX_EXPORT Q_DECL_EXPORT
#else
#define INDEX_EXPORT Q_DECL_IMPORT
#endif

#endif // INDEX_GLOBAL_H
