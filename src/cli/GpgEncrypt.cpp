#include "GpgEncrypt.h"

#include <cstdlib>
#include <stdio.h>

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/PasswordCacheService.h"

#include <QLocale>
#include <QProcess>
#include <gpgme.h>
#include <unistd.h>
#include <utility>

const QCommandLineOption GpgEncrypt::RecoveryKeyOption =
    QCommandLineOption(QStringList() << "r"
                                     << "rkeyid",
                       QObject::tr("Id of the key pair to be used for recovery.\n"));
GpgEncrypt::GpgEncrypt()
{
    name = QString("gpgencrypt");
    description = QObject::tr("Encrypt the specified path");
    options.append(GpgEncrypt::RecoveryKeyOption);
    positionalArguments.append({QString("key"),
                                QObject::tr("Id of the key pair to be used for encryption and decryption.\n"
                                            "use gpg --card-status to list available keys\n"
                                            "if you give an invalid keys it will list available ones"),
                                QString("[key]")});
    optionalArguments.append({QString("path"), QObject::tr("Path to enable encryption for. Default is /."), QString("[group]")});
}
#include <stdio.h>
// this leaks some resources, but if there was failure the program will be terminated soon anyway
#define callNcheck(foo)                                                                                                \
    error = foo;                                                                                                       \
    if (gpgme_err_code(error) != GPG_ERR_NO_ERROR)                                                                     \
        do {                                                                                                           \
            Utils::STDERR << gpgme_err_code(error) << " " << gpgme_strsource(error) << " " << gpgme_strerror(error)    \
                          << endl;                                                                                     \
            return EXIT_FAILURE;                                                                                       \
    } while (0)

// We just quit right after start but that causes the card to be initialised
static gpgme_error_t gpgme_interact_cb_t_impl(void* /*handle*/, const char* status, const char* /*args*/, int fd)
{
    if (strcmp(status, "GET_LINE") == 0) {
        char rr[] = "quit\n\x00";
        write(fd, rr, sizeof(rr));
    }
    return 0;
}

bool detectAndInitCard()
{
    gpgme_error_t error = 0;
    const char* ver = gpgme_check_version(0);
    Utils::STDERR << "GPGme version: " << ver << endl;
    gpgme_ctx_t ctx;
    callNcheck(gpgme_new(&ctx));

    // this seems to work as the key is not touched, but the documentation doesn't state much
    // "GPG_ERR_INV_VALUE if ctx or key is not a valid pointer."
    gpgme_key_t empty_key = nullptr;
    gpgme_data_t data_fun;
    callNcheck(gpgme_data_new(&data_fun));
    gpgme_op_interact(ctx, empty_key, GPGME_INTERACT_CARD, gpgme_interact_cb_t_impl, NULL, data_fun);
    gpgme_data_seek(data_fun, 0, SEEK_SET);

    QString output;
    output.reserve(1024);
    int s = 0;
    char buff[30];
    while ((s = gpgme_data_read(data_fun, buff, sizeof(buff))) > 0) {
        output.append(QString::fromUtf8(buff, s));
    }
    gpgme_data_release(data_fun);
    gpgme_release(ctx);
    if (output.contains("Reader:")) {
        int start = output.indexOf("Reader:");
        int end = output.indexOf("\n", start);
        if (end != -1) {
            Utils::STDERR << "Found Card: " << output.mid(start, end - start) << endl;
            return true;
        }
    }
    return false;
}
void encryptRecursively(Group * startGroup){
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;
    err << QObject::tr(" Encrypting group %1.").arg(startGroup->name()) << endl;
    for(auto entry:startGroup->entries()){
        err << QObject::tr("  Encrypting entry %1.").arg(entry->title()) << endl;
        entry->beginUpdate();
        entry->updateGpgData(true);
        entry->endUpdate();
    }
    for(auto group:startGroup->children()){
        encryptRecursively(group);
    }
} 

int GpgEncrypt::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();

    const QString& keyId = args.at(1);
    const QString& path = args.size()==3? args.at(2) : "/";
    
    QStringList rKey = parser->values(GpgEncrypt::RecoveryKeyOption);

    Group* group = database->rootGroup()->findGroupByPath(path);
    if(!group){
        err << QObject::tr("Cannot find group %1.").arg(path) << endl;
        return EXIT_FAILURE;
    }

    err << QObject::tr("Detecting and initialising the OpenPGP card") << endl;
    if (!detectAndInitCard()) {
        err << QObject::tr("Couldn't find a smart card. Ensure the card is connected and retry.") << endl;
        return EXIT_FAILURE;
    }
    err << QObject::tr("Testing key %1 ").arg(keyId) << endl;
    gpgme_error_t error = 0;
    gpgme_ctx_t ctx;
    callNcheck(gpgme_new(&ctx));
    gpgme_key_t key;
    error = gpgme_get_key(ctx, keyId.toLatin1().data(), &key, 0);
    if (gpgme_err_code(error) != GPG_ERR_NO_ERROR) {
        err << gpgme_err_code(error) << gpgme_strsource(error) << gpgme_strerror(error) << endl;
        err << QObject::tr("Could not find the specified key") << endl;
        err << QObject::tr("GPG recognises the following keys:") << endl;
        auto error = gpgme_op_keylist_start(ctx, NULL, 0);
        while (!error) {
            error = gpgme_op_keylist_next(ctx, &key);
            if (error)
                break;
            err << "\t" << key->subkeys->keyid << ": " << ((key->uids && key->uids->name) ? key->uids->name : "") << " "
                << ((key->uids && key->uids->email) ? key->uids->email : "") << endl;
        }
        return EXIT_FAILURE;
    }
    gpgme_set_armor(ctx, 1);
    QString password = "This is a test passphrase!";
    gpgme_data_t data;
    gpgme_data_t target;
    gpgme_key_t keys[2] = {key, NULL};
    err << "key was found, testing encryption" << endl;
    callNcheck(gpgme_data_new_from_mem(&data, password.toUtf8().data(), password.toUtf8().length(), 1));
    callNcheck(gpgme_data_new(&target));
    callNcheck(gpgme_op_encrypt(
        ctx, keys, (gpgme_encrypt_flags_t)(GPGME_ENCRYPT_ALWAYS_TRUST | GPGME_ENCRYPT_NO_ENCRYPT_TO), data, target));
    ssize_t s;
    char buff[30];
    gpgme_data_seek(target, 0, SEEK_SET);
    QString output;
    output.reserve(1024);
    while ((s = gpgme_data_read(target, buff, sizeof(buff))) > 0) {
        output.append(QString::fromUtf8(buff, s));
    }
    gpgme_key_release(key);
    gpgme_data_release(data);
    err << QObject::tr("Testing decryption\nIf it is a first call you will be asked for a PIN.\nIf you configured Youbikey "
           "correctly it will start blinking. Touch it to continiue.\n"
           "If your Youbikey didn't need to be touched please reconfigure it by running 'ykman openpgp set-touch enc on'")
        << endl;
    callNcheck(gpgme_data_new(&data));
    gpgme_data_seek(target, 0, SEEK_SET);
    callNcheck(gpgme_op_decrypt(ctx, target, data));
    gpgme_data_seek(data, 0, SEEK_SET);
    output="";
    output.reserve(1024);
    while ((s = gpgme_data_read(data, buff, sizeof(buff))) > 0) {
        output.append(QString::fromUtf8(buff, s));
    }
    if(output!=password){
        err<<QObject::tr("Decryption error: \n\tExpected: %1 Got %2").arg(password,output)<<endl;
        return EXIT_FAILURE;
    }
    err<<"\nAll tests PASSED!\n\nBegining encryption of path \""<<path<<"\""<<endl;
    group->customData()->set("GPG-Key",keyId);
    encryptRecursively(group);
    QString errorMessage;
    if (!database->save(&errorMessage, true, false)) {
        err << QObject::tr("Writing the database failed: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }
    err << QObject::tr("Database saved.") << endl;
    return EXIT_SUCCESS;
}