#ifndef SECURITY_GLOBAL_H
#define SECURITY_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SECURITY_LIBRARY)
#define SECURITY_EXPORT Q_DECL_EXPORT
#else
#define SECURITY_EXPORT Q_DECL_IMPORT
#endif

#endif // SECURITY_GLOBAL_H
