#ifndef OSEVENTFILTER_H
#define OSEVENTFILTER_H
#include <QAbstractNativeEventFilter>

class QByteArray;

class OSEventFilter : public QAbstractNativeEventFilter
{
public:
    OSEventFilter();
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    Q_DISABLE_COPY(OSEventFilter)
};

#endif // OSEVENTFILTER_H
