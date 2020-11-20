//
// Created by aetf on 11/15/20.
//

#ifndef KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H
#define KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H

#define DBUS_SERVICE_SECRET "org.freedesktop.secrets"

#define DBUS_INTERFACE_SECRET_SERVICE "org.freedesktop.Secret.Service"
#define DBUS_INTERFACE_SECRET_SESSION "org.freedesktop.Secret.Session"
#define DBUS_INTERFACE_SECRET_COLLECTION "org.freedesktop.Secret.Collection"
#define DBUS_INTERFACE_SECRET_ITEM "org.freedesktop.Secret.Item"
#define DBUS_INTERFACE_SECRET_PROMPT "org.freedesktop.Secret.Prompt"

#define DBUS_ERROR_SECRET_NO_SESSION "org.freedesktop.Secret.Error.NoSession"
#define DBUS_ERROR_SECRET_NO_SUCH_OBJECT "org.freedesktop.Secret.Error.NoSuchObject"
#define DBUS_ERROR_SECRET_IS_LOCKED "org.freedesktop.Secret.Error.IsLocked"

#define DBUS_PATH_SECRETS "/org/freedesktop/secrets"

#define DBUS_PATH_TEMPLATE_ALIAS "%1/aliases/%2"
#define DBUS_PATH_TEMPLATE_SESSION "%1/session/%2"
#define DBUS_PATH_TEMPLATE_COLLECTION "%1/collection/%2"
#define DBUS_PATH_TEMPLATE_ITEM "%1/%2"
#define DBUS_PATH_TEMPLATE_PROMPT "%1/prompt/%2"

namespace FdoSecrets {
    static const auto IntrospectionService = R"xml(
<interface name="org.freedesktop.Secret.Service">
    <property name="Collections" type="ao" access="read"/>
    <signal name="CollectionCreated">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <signal name="CollectionDeleted">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <signal name="CollectionChanged">
        <arg name="collection" type="o" direction="out"/>
    </signal>
    <method name="OpenSession">
        <arg type="v" direction="out"/>
        <arg name="algorithm" type="s" direction="in"/>
        <arg name="input" type="v" direction="in"/>
        <arg name="result" type="o" direction="out"/>
    </method>
    <method name="CreateCollection">
        <arg type="o" direction="out"/>
        <arg name="properties" type="a{sv}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="QVariantMap"/>
        <arg name="alias" type="s" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="SearchItems">
        <arg type="ao" direction="out"/>
        <arg name="attributes" type="a{ss}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="StringStringMap"/>
        <arg name="locked" type="ao" direction="out"/>
    </method>
    <method name="Unlock">
        <arg type="ao" direction="out"/>
        <arg name="paths" type="ao" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="Lock">
        <arg type="ao" direction="out"/>
        <arg name="paths" type="ao" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
    <method name="GetSecrets">
        <arg type="a{o(oayays)}" direction="out"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="ObjectPathSecretMap"/>
        <arg name="items" type="ao" direction="in"/>
        <arg name="session" type="o" direction="in"/>
    </method>
    <method name="ReadAlias">
        <arg type="o" direction="out"/>
        <arg name="name" type="s" direction="in"/>
    </method>
    <method name="SetAlias">
        <arg name="name" type="s" direction="in"/>
        <arg name="collection" type="o" direction="in"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionCollection = R"xml(
<interface name="org.freedesktop.Secret.Collection">
    <property name="Items" type="ao" access="read"/>
    <property name="Label" type="s" access="readwrite"/>
    <property name="Locked" type="b" access="read"/>
    <property name="Created" type="t" access="read"/>
    <property name="Modified" type="t" access="read"/>
    <signal name="ItemCreated">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <signal name="ItemDeleted">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <signal name="ItemChanged">
        <arg name="item" type="o" direction="out"/>
    </signal>
    <method name="Delete">
        <arg type="o" direction="out"/>
    </method>
    <method name="SearchItems">
        <arg type="ao" direction="out"/>
        <arg name="attributes" type="a{ss}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="StringStringMap"/>
    </method>
    <method name="CreateItem">
        <arg type="o" direction="out"/>
        <arg name="properties" type="a{sv}" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="QVariantMap"/>
        <arg name="secret" type="(oayays)" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In1" value="FdoSecrets::wire::Secret"/>
        <arg name="replace" type="b" direction="in"/>
        <arg name="prompt" type="o" direction="out"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionItem = R"xml(
<interface name="org.freedesktop.Secret.Item">
    <property name="Locked" type="b" access="read"/>
    <property name="Attributes" type="a{ss}" access="readwrite">
        <annotation name="org.qtproject.QtDBus.QtTypeName" value="StringStringMap"/>
    </property>
    <property name="Label" type="s" access="readwrite"/>
    <property name="Created" type="t" access="read"/>
    <property name="Modified" type="t" access="read"/>
    <method name="Delete">
        <arg type="o" direction="out"/>
    </method>
    <method name="GetSecret">
        <arg type="(oayays)" direction="out"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="FdoSecrets::wire::Secret"/>
        <arg name="session" type="o" direction="in"/>
    </method>
    <method name="SetSecret">
        <arg name="secret" type="(oayays)" direction="in"/>
        <annotation name="org.qtproject.QtDBus.QtTypeName.In0" value="FdoSecrets::wire::Secret"/>
    </method>
</interface>
)xml";

    static const auto IntrospectionSession = R"xml(
<interface name="org.freedesktop.Secret.Session">
    <method name="Close">
    </method>
</interface>
)xml";

    static const auto IntrospectionPrompt = R"xml(
<interface name="org.freedesktop.Secret.Prompt">
    <signal name="Completed">
        <arg name="dismissed" type="b" direction="out"/>
        <arg name="result" type="v" direction="out"/>
    </signal>
    <method name="Prompt">
        <arg name="windowId" type="s" direction="in"/>
    </method>
    <method name="Dismiss">
    </method>
</interface>
)xml";

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSCONSTANTS_H
