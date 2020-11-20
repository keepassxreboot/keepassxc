//
// Created by aetf on 11/21/20.
//

#include "DBusMgr.h"

#include "DBusObject.h"
#include "DBusTypes.h"
#include "fdosecrets/objects/Item.h"

#include "core/Global.h"

#include <QThread>

namespace FdoSecrets {
    QString pascalToCamel(const QString& pascal)
    {
        if (pascal.isEmpty()) {
            return pascal;
        }

        return pascal.at(0).toLower() + pascal.mid(1);
    }

    bool prepareInputParams(const QString& signature, const QVector<int>& inputTypes, const QDBusMessage& msg, QVarLengthArray<void*, 10> &params, QVariantList &auxParams)
    {
        // msg.signature() is verified by Qt to match msg.arguments(), but the content itself is untrusted,
        // so they need to be validated
        if (signature != msg.signature() || inputTypes.size() != msg.arguments().size()) {
            return false;
        }

        // prepare params
        for (int count = 0; count != inputTypes.size(); ++count) {
            const auto& id = inputTypes.at(count);
            const auto& arg = msg.arguments().at(count);

            if (arg.userType() == id) {
                // no conversion needed
                params.append(const_cast<void *>(arg.constData()));
            } else if (arg.canConvert(id)) {
                // make a copy to store the converted value
                auxParams.append(arg);
                auto& out = auxParams.last();
                if (!out.convert(id)) {
                    qDebug() << "Internal error: failed conversion from" << arg << "to type id" << id;
                    return false;
                }
                params.append(const_cast<void*>(out.constData()));
            } else {
                qDebug() << "Internal error: unhandled new parameter type id" << id << "and arg" << arg;
                return false;
            }
        }
        return true;
    }

    void DBusMgr::populateMethodCache(const QMetaObject& mo)
    {
        for (int i = 0; i != mo.methodOffset(); ++i) {
            auto mm = mo.method(i);

            // only register public Q_INVOKABLE methods
            if (mm.access() != QMetaMethod::Public || mm.methodType() != QMetaMethod::Method) {
                continue;
            }
            if (mm.returnType() != qMetaTypeId<DBusResult>()) {
                continue;
            }

            auto iface = mo.classInfo(mo.indexOfClassInfo("D-Bus Interface")).value();
            if (!iface) {
                continue;
            }
            auto cacheKey = QStringLiteral("%1.%2").arg(iface, mm.name());

            // skip if we already have it
            auto it = m_cachedMethods.find(cacheKey);
            if (it != m_cachedMethods.end()) {
                continue;
            }

            MethodData md;
            md.slotIdx = mm.methodIndex();

            bool valid = true;
            // assumes output params (reference parameter) all follows input params
            bool outputBegin = false;
            for (const auto& paramType : mm.parameterTypes()) {
                if (paramType.endsWith('&')) {
                    outputBegin = true;
                    auto id = QMetaType::type(paramType.chopped(1));
                    md.outputTypes.append(id);
                    auto paramData = typeToWireType(id);
                    if (paramData.signature.isEmpty()) {
                        qDebug() << "Internal error: unhandled new output type for dbus signature" << paramType;
                        valid = false;
                        break;
                    }
                    md.outputTargetTypes.append(paramData.dbusTypeId);
                    continue;
                }
                if (outputBegin) {
                    qDebug() << "Internal error: invalid method parameter order, no input parameter after output ones" << mm.name();
                    valid = false;
                    break;
                }
                auto id = QMetaType::type(paramType);
                auto sig = typeToWireType(id).signature;
                if (sig.isEmpty()) {
                    qDebug() << "Internal error: unhandled new parameter type for dbus signature" << paramType;
                    valid = false;
                    break;
                }
                md.inputTypes.append(id);
                md.signature += sig;
            }
            if (valid) {
                m_cachedMethods.insert(cacheKey, md);
            }
        }
    }

    bool DBusMgr::activateObject(const QString& path, const QString& interface, const QString& member, const QDBusMessage& msg)
    {
        auto obj = m_objects.value(path, nullptr);
        if (!obj) {
            qDebug() << "DBusMgr::handleMessage with unknown path";
            return false;
        }
        Q_ASSERT_X(QThread::currentThread() == obj->thread(),
                   "QDBusConnection: internal threading error",
                   "function called for an object that is in another thread!!");

        auto mo = obj->metaObject();
        // check if interface matches
        if (interface != mo->classInfo(mo->indexOfClassInfo("D-Bus Interface")).value()) {
            qDebug() << "DBusMgr::handleMessage with mismatch interface";
            return false;
        }

        // find the slot to call
        auto cacheKey = QStringLiteral("%1.%2").arg(interface, member);
        auto it = m_cachedMethods.find(cacheKey);
        if (it == m_cachedMethods.end()) {
            qDebug() << "DBusMgr::handleMessage with nonexisting method";
            return false;
        }

        QVarLengthArray<void*, 10> params;
        QVariantList auxParams;

        // the first one is for return type
        DBusResult ret;
        params.append(&ret);

        // prepare input
        if (!prepareInputParams(it->signature, it->inputTypes, msg, params, auxParams)) {
            return false;
        }

        // prepare output args
        QVariantList outputArgs;
        outputArgs.reserve(it->outputTypes.size());
        for (const auto& outputType : asConst(it->outputTypes)) {
            outputArgs.append(QVariant(outputType, nullptr));
            params.append(const_cast<void*>(outputArgs.last().constData()));
        }

        // call it
        bool fail = obj->qt_metacall(QMetaObject::InvokeMetaMethod, it->slotIdx, params.data()) >= 0;
        if (fail) {
            // generate internal error
            qWarning() << "Internal error: Failed to deliver message";
            return sendDBus(msg.createErrorReply(QDBusError::InternalError, QLatin1String("Failed to deliver message")));
        }

        if (!ret.ok()) {
            // error reply
            return sendDBus(msg.createErrorReply(ret, ""));
        }

        // output args need to be converted before they can be directly sent out:
        for (int i = 0; i != outputArgs.size(); ++i) {
            auto& outputArg = outputArgs[i];
            if (!outputArg.convert(it->outputTargetTypes.at(i))) {
                qWarning() << "Internal error: Failed to convert message output to type" << it->outputTargetTypes.at(i);
                return sendDBus(msg.createErrorReply(QDBusError::InternalError, QLatin1String("Failed to deliver message")));
            }
        }
        return sendDBus(msg.createReply(outputArgs));
    }

    bool DBusMgr::handleMessage(const QDBusMessage& message, const QDBusConnection&)
    {
        qDebug() << "DBusMgr::handleMessage: " << message;

        auto iface = message.interface();
        auto member = message.member();
        // introspect is handled by returning false
        // but we need to handle properties ourselves like regular functions
        if (iface == QLatin1String("org.freedesktop.DBus.Properties")) {
            iface = message.arguments().at(0).toString();
            // Set + Name
            if (member == QLatin1String("Set")) {
                member = member + message.arguments().at(1).toString();
            }
        }
        member = pascalToCamel(member);

        // from now on we may call into dbus objects, so setup client first
        // TODO: lookup client

        struct LocalInstanceSetter
        {
            explicit LocalInstanceSetter(DBusMgr* inst) {
                Q_ASSERT(ThreadLocalInstance == nullptr);
                DBusMgr::ThreadLocalInstance = inst;
            }
            ~LocalInstanceSetter() {
                DBusMgr::ThreadLocalInstance = nullptr; }
        };
        LocalInstanceSetter s(this);
        // get target object from path & interface
        auto msgSent = activateObject(message.path(), iface, member, message);
        if (!msgSent) {
            qDebug() << "DBusMgr::handleMessage with unknown interface or path";
            return false;
        }

        // TODO: cleanup client

        return false;
    }
} // namespace FdoSecrets
