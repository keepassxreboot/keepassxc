#include "PasswordCacheService.h"
#include <QDir>
#include <QRegularExpression>
#include <QProcess>
#include <utility>
#include <QtDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include "gui/MessageBox.h"
#include <gpgme.h>

#include <iostream>
#include <unistd.h>

gpgme_error_t gpgme_interact_cb_t_impl(void* /*handle*/, const char* status, const char* /*args*/, int fd)
{
    if (strcmp(status, "GET_LINE") == 0) {
        char rr[] = "quit\n\x00";
        write(fd, rr, sizeof(rr));
    }
    return 0;
}
bool initGPG()
{
    gpgme_error_t error = 0;
    const char* ver = gpgme_check_version(0);
    //Utils::STDERR << "GPGme version: " << ver << endl;
    gpgme_ctx_t ctx;
    gpgme_new(&ctx);

    // this seems to work as the key is not touched, but the documentation doesn't state much
    // "GPG_ERR_INV_VALUE if ctx or key is not a valid pointer."
    gpgme_key_t empty_key = nullptr;
    gpgme_data_t data_fun;
    gpgme_data_new(&data_fun);
    gpgme_op_interact(ctx, empty_key, GPGME_INTERACT_CARD, gpgme_interact_cb_t_impl, NULL, data_fun);
    gpgme_data_seek(data_fun, 0, SEEK_SET);

    QString output;
    output.reserve(1024);
    int s = 0;
    char buff[30];
    while ((s = gpgme_data_read(data_fun, buff, sizeof(buff))) > 0) {
        // fwrite(buff, 1, s, stdout);
        output.append(QString::fromUtf8(buff, s));
    }
    // Utils::STDERR  << output<<endl;
    gpgme_data_release(data_fun);
    gpgme_release(ctx);
    if (output.contains("Reader:")) {
        int start = output.indexOf("Reader:");
        int end = output.indexOf("\n", start);
        if (end != -1) {
            //Utils::STDERR << "Found Card: " << output.mid(start, end - start) << endl;
            return true;
        }
    }
    return false;
}

void GpgDecryptionThread::setEncryptedPassword(const QString &password){
    m_encryptedPassword=password;
}
#define callNcheck(foo) error=foo;if(gpgme_err_code(error)!=GPG_ERR_NO_ERROR)do{qInfo()<<"GPGme error: "<<gpgme_err_code(error)<< " "<<gpgme_strsource (error)<<" " <<gpgme_strerror(error);emit decryptionError(2,QString()+gpgme_err_code(error)+" "+gpgme_strsource (error)+" "+gpgme_strerror(error));}while(0)

void GpgDecryptionThread::run(){
    bool init=initGPG();
    if(!init){
        emit decryptionError(0,"init");
        return;
    }
    QByteArray data=QByteArray::fromBase64(m_encryptedPassword.toLatin1());
    gpgme_error_t error=0;
    const  char * ver=gpgme_check_version(0);
    //TODO: clean memory leaks
    gpgme_ctx_t ctx;
    gpgme_data_t g_data;
    gpgme_data_t target;
    callNcheck(gpgme_new(&ctx));
    callNcheck(gpgme_data_new_from_mem(&g_data,data.constData(),data.length(),1));
    callNcheck(gpgme_data_new(&target));
    callNcheck(gpgme_op_decrypt(ctx,g_data,target));
    ssize_t s;
    char buff[30];
    QByteArray out_arr;
    gpgme_data_seek(target,0,SEEK_SET);
    while( (s=gpgme_data_read(target,buff,sizeof(buff)))>0){
        out_arr.append(buff,s);
    }
    QString ret = QString::fromLatin1(out_arr);
    gpgme_data_release(g_data);
    gpgme_data_release(target);
    gpgme_release(ctx);
    emit passwordDecrypted(ret);
}
void PasswordCacheService::passwordDecrypted(const QString &password){
    m_error=-1;
    m_returned=password;
    m_messageBox->close();
}
void PasswordCacheService::passwordDecriptionFailed(int error,const QString &extError){
    m_error=error;
    m_returned=extError;
    m_messageBox->close();
}
QString PasswordCacheService::GpgDecryptPassword(const QString &username,const QString &title,const QString & gpgPassword){
    GpgDecryptionThread *thread=new GpgDecryptionThread(this);

    QMessageBox &msgBox=*new QMessageBox();
    msgBox.setWindowTitle(tr("GPG password decrypt"));
    msgBox.setText(tr("Title: %1\nUsername: %2\n\n Please touch youbikey to proceed").arg(title,username));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    m_messageBox=&msgBox;

    connect(thread, &GpgDecryptionThread::passwordDecrypted, this, &PasswordCacheService::passwordDecrypted);
    connect(thread, &GpgDecryptionThread::decryptionError, this, &PasswordCacheService::passwordDecriptionFailed);
    thread->setEncryptedPassword(gpgPassword);
    thread->start();
    msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
    msgBox.activateWindow();
    msgBox.raise();
    auto status=msgBox.exec();
    
    //gpg returned error
    if(m_error!=-1){
        //indicate to try init on the next try
        initialized=false;
        auto result = MessageBox::question(nullptr,
                                           tr("GPG process failed!"),
                                           tr("Can't decrypt your password.\n"
                                           "Check if the yubikey is connected and click Retry.\n\n"
                                           "The process failed with the following error message:\n")+m_returned,
                                           MessageBox::Retry | MessageBox::Abort);
        if(result==MessageBox::Retry){
            return GpgDecryptPassword(username,title,gpgPassword);
        }else{
            return "";
        }
    } 
 
    return m_returned;
}
QString PasswordCacheService::GpgDecryptPassword(const Entry& entry,const QString & gpgPassword){
    return GpgDecryptPassword(entry.username(),entry.title(),gpgPassword);
}


PasswordCacheService * PasswordCacheService::s_instance=nullptr;
PasswordCacheService::PasswordCacheService(QObject* parent)
    :QObject(parent),initialized(0){

}
QString PasswordCacheService::value(const Entry& entry)const{
    return value(entry.uuid());
}
QString PasswordCacheService::value(const QUuid& key)const{
    //qInfo()<< " returning value from cache for "<<key<< " : "<<m_passwordMap.value(key); 
    return m_passwordMap.value(key);
}
QString PasswordCacheService::decodeAndStore(const Entry& entry,const QString& encryptedPassword){
    //qInfo()<< "Getting password for uuid "<<entry.uuid(); 
    QString pwd=GpgDecryptPassword(entry,encryptedPassword);
    //Disable cache as it is not needed after refactoring - no more repeated asking for unlocking.
    //It may become handy in the future so the PasswordCacheService stays for now.
    /* if(pwd!="")
        m_passwordMap[entry.uuid()]=pwd;*/
    return pwd;
}
void PasswordCacheService::remove(const QUuid& key){
    m_passwordMap.remove(key);
}
bool PasswordCacheService::contains(const QUuid& key) const{
    //qInfo()<< " checking if we have password for "<<key<< " : "<<m_passwordMap.contains(key); 
    return m_passwordMap.contains(key);
}
void PasswordCacheService::clear(){
    m_passwordMap.clear();
}
PasswordCacheService& PasswordCacheService::instance(){
    if(!s_instance){
        s_instance=new PasswordCacheService(nullptr);
    }
    return *s_instance;
}

void PasswordCacheService::expirationTimerExpired(){

}
void PasswordCacheService::touchTimerExpired(){

}
