#include "robomongo/core/settings/ConnectionSettings.h"

#include <stdio.h>

#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    const unsigned port = 27017;
    const char *defaultServerHost = "localhost";
    const char *defaultNameConnection = "New Connection";

    const int maxLength = 300;
}

namespace Robomongo
{

    /**
     * Creates ConnectionSettings with default values
     */
    ConnectionSettings::ConnectionSettings() : QObject(),
        _connectionName(defaultNameConnection),
        _host(defaultServerHost),
        _port(port),
        _imported(false),
        _sshSettings(new SshSettings()),
        _sslSettings(new SslSettings()),
        _replicaSetSettings(new ReplicaSetSettings()),
        _isReplicaSet(false)
    { }

    void ConnectionSettings::fromVariant(const QVariantMap &map) {
        setConnectionName(QtUtils::toStdString(map.value("connectionName").toString()));
        setServerHost(QtUtils::toStdString(map.value("serverHost").toString().left(maxLength)));
        setServerPort(map.value("serverPort").toInt());
        setDefaultDatabase(QtUtils::toStdString(map.value("defaultDatabase").toString()));
        //// Temporarily disable Replica Setet - feature is under development
        //setReplicaSet(map.value("isReplicaSet").toBool());       
        
        QVariantList list = map.value("credentials").toList();
        for (QVariantList::const_iterator it = list.begin(); it != list.end(); ++it) {
            QVariant var = *it;
            CredentialSettings *credential = new CredentialSettings(var.toMap());
            addCredential(credential);
        }

        if (map.contains("ssh")) {
            _sshSettings->fromVariant(map.value("ssh").toMap());
        }

        if (map.contains("ssl")) {
            _sslSettings->fromVariant(map.value("ssl").toMap());
        }

        //// Temporarily disable Replica Setet - feature is under development
        //if (isReplicaSet()) {
        //    _replicaSetSettings->fromVariant(map.value("replicaSet").toMap());
        //}

//#ifdef MONGO_SSL
//      ,SSLInfo(map.value("sslEnabled").toBool(),QtUtils::toStdString(map.value("sslPemKeyFile").toString()))
//#endif
    }

    /**
     * Cleanup used resources
     */
    ConnectionSettings::~ConnectionSettings()
    {
        clearCredentials();
        delete _sshSettings;
        delete _sslSettings;
        delete _replicaSetSettings; // todo: unique_ptr
    }

    /**
     * Creates completely new ConnectionSettings by cloning this record.
     */
    ConnectionSettings *ConnectionSettings::clone() const
    {
        ConnectionSettings *record = new ConnectionSettings();
        record->apply(this);
        return record;
    }

    /**
     * Discards current state and applies state from 'source' ConnectionSettings.
     */
    void ConnectionSettings::apply(const ConnectionSettings *source)
    {
        setConnectionName(source->connectionName());
        setServerHost(source->serverHost());
        setServerPort(source->serverPort());
        setDefaultDatabase(source->defaultDatabase());
        setImported(source->imported());
        //// Temporarily disable Replica Setet - feature is under development
        //setReplicaSet(source->isReplicaSet());

        clearCredentials();
        QList<CredentialSettings *> cred = source->credentials();
        for (QList<CredentialSettings *>::iterator it = cred.begin(); it != cred.end(); ++it) {
            addCredential((*it)->clone());
        }

        _sshSettings = source->sshSettings()->clone();
        _sslSettings = source->sslSettings()->clone();
        _replicaSetSettings = source->_replicaSetSettings->clone(); // todo: leak candidate

//#ifdef MONGO_SSL
//        setSslInfo(source->sslInfo());
//#endif
    }

    /**
     * @brief Converts to QVariantMap
     */
    QVariant ConnectionSettings::toVariant() const
    {
        QVariantMap map;
        map.insert("connectionName", QtUtils::toQString(connectionName()));
        map.insert("serverHost", QtUtils::toQString(serverHost()));
        map.insert("serverPort", serverPort());
        map.insert("defaultDatabase", QtUtils::toQString(defaultDatabase()));
        //// Temporarily disable Replica Setet - feature is under development
        //map.insert("isReplicaSet", isReplicaSet());
        //if (isReplicaSet()) {
        //    map.insert("replicaSet", _replicaSetSettings->toVariant());
        //}

#ifdef MONGO_SSL
        SSLInfo infl = _info.sslInfo();
        map.insert("sslEnabled", infl._sslSupport);
        map.insert("sslPemKeyFile", QtUtils::toQString(infl._sslPEMKeyFile));
#endif
        QVariantList list;
        for (QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
            CredentialSettings *credential = *it;
            list.append(credential->toVariant());
        }
        map.insert("credentials", list);

        map.insert("ssh", _sshSettings->toVariant());
        map.insert("ssl", _sslSettings->toVariant());

        return map;
    }

     CredentialSettings *ConnectionSettings::findCredential(const std::string &databaseName) const
     {
         CredentialSettings *result = NULL;
         for (QList<CredentialSettings *>::const_iterator it = _credentials.begin(); it != _credentials.end(); ++it) {
             CredentialSettings *cred = *it;
             if (cred->databaseName() == databaseName) {
                 result = cred;
                 break;
             }
         }
         return result;
     }

    /**
     * @brief Adds credential to this connection
     */
    void ConnectionSettings::addCredential(CredentialSettings *credential)
    {
        if (!findCredential(credential->databaseName()))
            _credentials.append(credential);
    }

    /**
     * @brief Checks whether this connection has primary credential
     * which is also enabled.
     */
    bool ConnectionSettings::hasEnabledPrimaryCredential()
    {
        if (_credentials.count() == 0)
            return false;

        return primaryCredential()->enabled();
    }

    /**
     * @brief Returns primary credential
     */
    CredentialSettings *ConnectionSettings::primaryCredential()
    {
        if (_credentials.count() == 0) {
            _credentials.append(new CredentialSettings());
        }

        return _credentials.at(0);
    }

    /**
     * @brief Clears and releases memory occupied by credentials
     */
    void ConnectionSettings::clearCredentials()
    {
        qDeleteAll(_credentials);
        _credentials.clear();
    }

    std::string ConnectionSettings::getFullAddress() const
    {
        return info().toString();
    }



}
