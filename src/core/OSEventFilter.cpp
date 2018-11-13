#include "OSEventFilter.h"

#include <QByteArray>

#include "autotype/AutoType.h"

OSEventFilter::OSEventFilter()
{
}

bool OSEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(result)

#if defined(Q_OS_UNIX)
    if (eventType == QByteArrayLiteral("xcb_generic_event_t")) {
#elif defined(Q_OS_WIN)
    if (eventType == QByteArrayLiteral("windows_generic_MSG")
        || eventType == QByteArrayLiteral("windows_dispatcher_MSG")) {
#endif
        int retCode = autoType()->callEventFilter(message);

        return retCode == 1;
    }

    return false;
}
