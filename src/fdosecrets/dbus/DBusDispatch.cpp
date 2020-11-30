//
// Created by aetf on 11/21/20.
//

#include "DBusMgr.h"

#include "DBusObject.h"
#include "DBusTypes.h"
#include "fdosecrets/objects/Item.h"

#include "core/Global.h"

#include <QThread>

namespace FdoSecrets
{
    QString camelToPascal(const QString& camel)
    {
        if (camel.isEmpty()) {
            return camel;
        }
        return camel.at(0).toUpper() + camel.mid(1);
    }

    bool prepareInputParams(const QVector<int>& inputTypes,
                            const QVariantList& args,
                            QVarLengthArray<void*, 10>& params,
                            QVariantList& auxParams)
    {
        // prepare params
        for (int count = 0; count != inputTypes.size(); ++count) {
            const auto& id = inputTypes.at(count);
            const auto& arg = msg.arguments().at(count);

            if (arg.userType() == id) {
                // no conversion needed
                params.append(const_cast<void*>(arg.constData()));
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

            // map from function name to dbus name
            auto member = camelToPascal(mm.name());
            // also "remove" => "Delete" due to c++ keyword restriction
            if (member == "Remove") {
                member = QStringLiteral("Delete");
            }
            auto cacheKey = QStringLiteral("%1.%2").arg(iface, member);

            // skip if we already have it
            auto it = m_cachedMethods.find(cacheKey);
            if (it != m_cachedMethods.end()) {
                continue;
            }

            MethodData md;
            md.isProperty = mm.tag() && mm.tag() == QStringLiteral("DBUS_PROPERTY");
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
                    qDebug() << "Internal error: invalid method parameter order, no input parameter after output ones"
                             << mm.name();
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

    bool DBusMgr::activateObject(const QString& path,
                                 const QString& interface,
                                 const QString& member,
                                 const QDBusMessage& msg)
    {
//        qDebug() << "DBusMgr::handleMessage: " << message;

        // save a mutable copy of the message, as we may modify it to unify property access
        // and method call
        RequestedMethod req{
            message.interface(),
            message.member(),
            message.signature(),
            message.arguments(),
            false,
        };

        if (req.interface == QLatin1String("org.freedesktop.DBus.Introspectable")) {
            // introspection can be handled by Qt, just return false
            return false;
        } else if (req.interface == QLatin1String("org.freedesktop.DBus.Properties")) {
            // but we need to handle properties ourselves like regular functions
            if (!rewriteRequestForProperty(req)) {
                // invalid message
                qDebug() << "Invalid message" << message;
                return false;
            }
        }

        // from now on we may call into dbus objects, so setup client first
        auto client = findClient(message.service());
        if (!client) {
            // the client already died
            return false;
        }

        struct ContextSetter
        {
            explicit ContextSetter(DBusClientPtr client)
            {
                if (Context) {
                    // override by unit test
                    return;
                }
                own = true;
                Context = std::move(client);
            }
            ~ContextSetter()
            {
                if (own) {
                    Context.reset();
                }
            }
            bool own{false};
        };
        ContextSetter s(std::move(client));

        // activate the target object
        return activateObject(message.path(), req, message);
    }

    bool DBusMgr::rewriteRequestForProperty(RequestedMethod& req)
    {
        if (req.member == "Set" && req.signature == "ssv") {
            // convert to normal method call: SetName
            req.interface = req.args.at(0).toString();
            req.member = req.member + req.args.at(1).toString();
            // unwrap the QDBusVariant and expose the inner signature
            auto arg = req.args.last().value<QDBusVariant>().variant();
            req.args = {arg};
            if (arg.userType() == qMetaTypeId<QDBusArgument>()) {
                req.signature = arg.value<QDBusArgument>().currentSignature();
            } else if (arg.userType() == QMetaType::QString){
                req.signature = "s";
            } else {
                qDebug() << "Unhandled SetProperty value type" << QMetaType::typeName(arg.userType()) << arg.userType();
                return false;
            }
        } else if (req.member == "Get" && req.signature == "ss") {
            // convert to normal method call: Name
            req.interface = req.args.at(0).toString();
            req.member = req.args.at(1).toString();
            req.signature = "";
            req.args = {};
        } else if (req.member == "GetAll" && req.signature == "s"){
            // special handled in activateObject
            req.interface = req.args.at(0).toString();
            req.member = "";
            req.signature = "";
            req.args = {};
            req.isGetAll = true;
        } else {
            return false;
        }
        return true;
    }

    bool DBusMgr::activateObject(const QString& path, const RequestedMethod& req, const QDBusMessage& msg)
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
        // either interface matches, or interface is empty if req is property get all
        QString interface = mo->classInfo(mo->indexOfClassInfo("D-Bus Interface")).value();
        if (req.interface != interface
            && !(req.isGetAll && req.interface.isEmpty())) {
            qDebug() << "DBusMgr::handleMessage with mismatch interface" << msg;
            return false;
        }

        // special handle of property getall
        if (req.isGetAll) {
            return objectPropertyGetAll(obj, interface, msg);
        }

        // find the slot to call
        auto cacheKey = QStringLiteral("%1.%2").arg(interface, member);
        auto it = m_cachedMethods.find(cacheKey);
        if (it == m_cachedMethods.end()) {
            qDebug() << "DBusMgr::handleMessage with nonexisting method";
            return false;
        }

        // requested signature is verified by Qt to match the content of arguments,
        // but this list of arguments itself is untrusted
        if (it->signature != req.signature || it->inputTypes.size() != req.args.size()) {
            qDebug() << "Message signature does not match, expected" << it->signature << it->inputTypes.size() << "got" << req.signature << req.args.size();
            return false;
        }

        DBusResult ret;
        QVariantList outputArgs;
        if (!deliverMethod(obj, *it, req.args, ret, outputArgs)) {
            qDebug() << "Failed to deliver method" << msg;
            return sendDBus(
                msg.createErrorReply(QDBusError::InternalError, QStringLiteral("Failed to deliver message")));
        }

        if (!ret.ok()) {
            return sendDBus(msg.createErrorReply(ret, ""));
        }
        return sendDBus(msg.createReply(outputArgs));
    }

    bool DBusMgr::objectPropertyGetAll(DBusObject* obj, const QString& interface, const QDBusMessage& msg)
    {
        QVariantMap result;

        // prefix match the cacheKey
        auto prefix = interface + ".";
        for (auto it = m_cachedMethods.constBegin(); it != m_cachedMethods.constEnd(); ++it) {
            if (!it.key().startsWith(prefix)) {
                continue;
            }
            if (!it.value().isProperty) {
                continue;
            }
            auto name = it.key().mid(prefix.size());

            DBusResult ret;
            QVariantList outputArgs;
            if (!deliverMethod(obj, it.value(), {}, ret, outputArgs)) {
                // ignore any error per spec
                continue;
            }
            if (ret.err()) {
                // ignore any error per spec
                continue;
            }
            Q_ASSERT(outputArgs.size() == 1);

            result.insert(name, outputArgs.first());
        }

        return sendDBus(msg.createReply(QVariantList{result}));
    }

    bool DBusMgr::deliverMethod(DBusObject* obj, const MethodData& method, const QVariantList& args, DBusResult& ret, QVariantList& outputArgs)
    {
        QVarLengthArray<void*, 10> params;
        QVariantList auxParams;

        // the first one is for return type
        params.append(&ret);

        // prepare input
        if (!prepareInputParams(method.inputTypes, args, params, auxParams)) {
            qDebug() << "Failed to prepare input params";
            return false;
        }

        // prepare output args
        outputArgs.reserve(outputArgs.size() + method.outputTypes.size());
        for (const auto& outputType : asConst(method.outputTypes)) {
            outputArgs.append(QVariant(outputType, nullptr));
            params.append(const_cast<void*>(outputArgs.last().constData()));
        }

        // call it
        bool fail = obj->qt_metacall(QMetaObject::InvokeMetaMethod, method.slotIdx, params.data()) >= 0;
        if (fail) {
            // generate internal error
            qWarning() << "Internal error: Failed to deliver message";
            return false;
        }

        if (!ret.ok()) {
            // error reply
            return true;
        }

        // output args need to be converted before they can be directly sent out:
        for (int i = 0; i != outputArgs.size(); ++i) {
            auto& outputArg = outputArgs[i];
            if (!outputArg.convert(method.outputTargetTypes.at(i))) {
                qWarning() << "Internal error: Failed to convert message output to type" << method.outputTargetTypes.at(i);
                return false;
            }
        }

        return true;
    }

    bool DBusMgr::handleMessage(const QDBusMessage& message, const QDBusConnection&)
    {
        qDebug() << "DBusMgr::handleMessage: " << message;

        auto iface = message.interface();
        auto member = message.member();
        // introspection is handled by returning false
        // but we need to handle properties ourselves like regular functions
        if (iface == QLatin1String("org.freedesktop.DBus.Properties")) {
            iface = message.arguments().at(0).toString();
            // Set + Name
            if (member == QLatin1String("Set")) {
                member = member + message.arguments().at(1).toString();
            }
        }
        // mapping from dbus member name to function names on the object
        member = pascalToCamel(member);
        // also "delete" => "remove" due to c++ keyword
        if (member == QLatin1Literal("delete")) {
            member = QLatin1Literal("remove");
        }

        // from now on we may call into dbus objects, so setup client first
        auto client = findClient(message.service());
        if (!client) {
            // the client already died
            return false;
        }

        struct ContextSetter
        {
            explicit ContextSetter(DBusClientPtr client)
            {
                Q_ASSERT(Context == nullptr);
                Context = std::move(client);
            }
            ~ContextSetter()
            {
                Context = nullptr;
            }
        };
        ContextSetter s(std::move(client));

        // activate the target object
        return activateObject(message.path(), iface, member, message);
    }
} // namespace FdoSecrets
