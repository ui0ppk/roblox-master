/**
 * MobileDevelopmentDeployer.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */
#include "stdafx.h"
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QHostInfo>
#include <QSignalMapper>
#include <qmath.h>

#include "StringConv.h"
#include "Script/script.h"
#include "MobileDevelopmentDeployer.h"
#include "AuthoringSettings.h"
#include "G3D/Random.h"

#define PAIR_CODE_SIZE 4
#define RBX_DEV_PORT 1313

MobileDevelopmentDeployer::MobileDevelopmentDeployer(QWidget* pParent)
	: QWidget(pParent),
    onPairBehavior(ON_PAIR_DO_NOTHING)
{
	tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
    timer = new QTimer();
}

MobileDevelopmentDeployer::~MobileDevelopmentDeployer()
{
    disconnectAllClients();
    
	if(tcpServer)
	{
		tcpServer->close();
        tcpServer->deleteLater();
	}
}

void MobileDevelopmentDeployer::disconnectAllClients()
{
    if (tcpClients.size() > 0)
    {
        for(std::vector<QTcpSocket*>::iterator iter = tcpClients.begin(); iter != tcpClients.end(); ++iter)
        {
            if(*iter)
            {
                (*iter)->write("!exit");
            }
        }
        tcpClients.clear();
    }
}

void MobileDevelopmentDeployer::disconnectClientsAndShutDown()
{
    disconnectAllClients();
    
	if (tcpServer)
	{
		tcpServer->close();
	}
    if (udpSocket)
    {
        udpSocket->close();
    }
    if (timer)
    {
        timer->stop();
    }
}

MobileDevelopmentDeployer& MobileDevelopmentDeployer::singleton()
{
	static MobileDevelopmentDeployer* developmentDeployer = new MobileDevelopmentDeployer();
	return *developmentDeployer;
}

int MobileDevelopmentDeployer::generateNewPairCode()
{
    int newPairCode = 0;

    // make current unix time the seed (should be random enough)
    G3D::Random generator(std::time(NULL));
    
    for ( int i = 0; i < forcedPairCodeSize(); ++i)
    {
        newPairCode += generator.integer(1, 9) * pow(10, i);
    }
    
    AuthoringSettings::singleton().setDeviceDeploymentPairingCode(newPairCode);
	RBX::GlobalAdvancedSettings::singleton()->saveState();
    return newPairCode;
}

int MobileDevelopmentDeployer::getSavedPairCode()
{
    return AuthoringSettings::singleton().deploymentPairingCode;
}

int MobileDevelopmentDeployer::getNumOfDigits(int numToCheck)
{
	if (numToCheck <= 0)
		return 0;
	return qFloor(log10(numToCheck)) + 1;
}

int MobileDevelopmentDeployer::forcedPairCodeSize()
{
	return PAIR_CODE_SIZE;
}

int MobileDevelopmentDeployer::getPairCode()
{
    int savedPairCode = getSavedPairCode();
    
    if ( getNumOfDigits(savedPairCode) <= 0 )
    {
        savedPairCode = generateNewPairCode();
    }
    
    RBXASSERT(getNumOfDigits(savedPairCode) == forcedPairCodeSize());
    
    return savedPairCode;
}

const QString MobileDevelopmentDeployer::getPairCodeString()
{
    int pairCodeInt = getPairCode();
    QString codeString = QString::number(pairCodeInt);
    
    if (codeString.size() == 3)
    {
        codeString.prepend('0');
    }
    
    RBXASSERT(codeString.size() == forcedPairCodeSize());
    RBXASSERT(pairCodeInt == codeString.toInt());
    
    return codeString;
}

bool MobileDevelopmentDeployer::isPairCode(const QString& codeString)
{
    if (codeString.isNull() || codeString.count() != forcedPairCodeSize())
        return false;
    
    int codeInt = codeString.toInt();
    int pairCodeInt = getPairCode();
    return ( codeInt == pairCodeInt );
}

QString MobileDevelopmentDeployer::getLocalIPAddress()
{
	QTcpSocket socket;
	socket.connectToHost("8.8.8.8", 53); // google DNS, or something else reliable
	if (socket.waitForConnected())
	{
		return socket.localAddress().toString();
	}

	return QString("");
}

void MobileDevelopmentDeployer::broadcastReadyDatagram()
{
    QString broadcastString("RbxDevPairServer readyToPair");
    broadcastDatagram(broadcastString.toAscii());
}

void MobileDevelopmentDeployer::broadcastDatagram(QByteArray dataToSend)
{
    sendToAllBroadcast(dataToSend);
}

void MobileDevelopmentDeployer::sendToAllBroadcast(QByteArray packet)
{
    // Get network interfaces list
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    
    // Interfaces iteration
    for (int i = 0; i < ifaces.size(); i++)
    {
        // Now get all IP addresses for the current interface
        QList<QNetworkAddressEntry> addrs = ifaces[i].addressEntries();
        
        // And for any IP address, if it is IPv4 and the interface is active, send the packet
        for (int j = 0; j < addrs.size(); j++)
        {
            if ((addrs[j].ip().protocol() == QAbstractSocket::IPv4Protocol) && (addrs[j].broadcast().toString() != ""))
            {
                udpSocket->writeDatagram(packet.data(), packet.length(), addrs[j].broadcast(), RBX_DEV_PORT);
            }
        }
    }
}

void MobileDevelopmentDeployer::pairWithRbxDevDevices()
{
    onPairBehavior = ON_PAIR_DO_NOTHING;
    listenForRbxDevDevices();
}

void MobileDevelopmentDeployer::startPlaySessionWithRbxDevDevices()
{
    onPairBehavior = ON_PAIR_START_SERVER;
    listenForRbxDevDevices();
}

void MobileDevelopmentDeployer::listenForRbxDevDevices()
{
	if(udpSocket && !udpSocket->isOpen())
	{
        if (!udpSocket->bind(QHostAddress::Any,RBX_DEV_PORT,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PairServer did not bind");
        
		connect(udpSocket, SIGNAL(readyRead()),this, SLOT(on_receivedUdpBroadcast()));
    
        timer->setInterval(500);
        connect(timer, SIGNAL(timeout()), this, SLOT(broadcastReadyDatagram()));
        timer->start();
	}
}

bool MobileDevelopmentDeployer::startTcpServer()
{
    if(tcpServer && !tcpServer->isListening())
    {
        if( tcpServer->listen(QHostAddress(getLocalIPAddress()),1315) )
         {
            connect(tcpServer, SIGNAL(newConnection()), this, SLOT(on_newConnection()));
            return true;
         }
    }
    return false;
}

void MobileDevelopmentDeployer::on_newConnection()
{
	QTcpSocket* tcpClient = tcpServer->nextPendingConnection();
	if (!tcpClient)
	{
		return;
	}

	if (tcpClient->state() == QTcpSocket::ConnectedState)
	{
		connect( tcpClient, SIGNAL(disconnected()), this, SLOT(on_disconnected()) );
		connect( tcpClient, SIGNAL(readyRead()), this, SLOT(on_readyRead()) );
        
        tcpClients.push_back(tcpClient);
	}
}

bool MobileDevelopmentDeployer::checkForClientPairCode(const QString& messageFromClient)
{
    if (messageFromClient.contains(QString::fromStdString("RbxDevClient pairCode")))
    {
        QStringList list = messageFromClient.split(QRegExp("\\s+"));
        if(list.count() == 3)
        {
            QString code = list.at(2);
            if ( isPairCode(code) )
            {
                if (udpSocket)
                {
                    QString broadcastString("RbxDevPairServer didPair true ip ");
                    broadcastString.append(getLocalIPAddress());
                    broadcastDatagram(broadcastString.toAscii());
                }
            }
            else
            {
                if (udpSocket)
                {
                    QString broadcastString("RbxDevPairServer didPair false ip ");
                    broadcastString.append("0.0.0.0");
                    broadcastDatagram(broadcastString.toAscii());
                }
            }
        }
        return true;
    }
    return false;
}

bool MobileDevelopmentDeployer::checkForClientPaired(const QString& messageFromClient, const QHostAddress& sender)
{
    if (messageFromClient.contains(QString::fromStdString("RbxDevClient didPair")))
    {
        QStringList list = messageFromClient.split(QRegExp("\\s+"));
        if (list.count() == 3)
        {
            QString didPair = list.at(2);
            if (didPair == "true")
            {
                if (onPairBehavior == ON_PAIR_START_SERVER)
                {
                    startTcpServer();
                    
                    QString broadcastString("RbxDevPairServer didStartServer");
                    broadcastDatagram(broadcastString.toAscii());
                }
                
                QHostInfo info = QHostInfo::fromName(sender.toString());
                Q_EMIT connectedToClient(info.hostName());
            }
        }
        return true;
    }
    return false;
}

void MobileDevelopmentDeployer::on_receivedUdpBroadcast()
{
    while (udpSocket && udpSocket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(udpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;
        
		udpSocket->readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort);
        
        QString message(datagram);
        
        if ( checkForClientPairCode(message) )
            return;
        if ( checkForClientPaired(message,sender) )
            return;
        if (message.contains(QString::fromStdString("RbxDevClient didConnectToServer")))
        {
            // todo: maybe do something here? confirmation message or something?
        }
	}
}

static bool isHidden(const boost::filesystem::path &p)
{
    boost::filesystem::path name = p.filename();
    if(name != ".." &&
       name != "."  &&
       name.c_str()[0] == '.')
    {
        return true;
    }
    
    return false;
}

void MobileDevelopmentDeployer::writeCoreScriptToFile(const std::string& filePath, const std::string& fileName, std::stringstream& stream)
{
     std::ifstream in(RBX::utf8_decode(filePath).c_str(), std::ios::in | std::ios::binary);
     
     if (in)
     {
         std::string relativeName = filePath.substr(RBX::BaseScript::adminScriptsPath.size() + 1);
         relativeName = relativeName.substr(0,relativeName.size() - 4);

         stream << RBX::format("RbxScriptName%sRbxEnd RbxScriptSource",relativeName.c_str());
         stream << in.rdbuf() << "RbxEnd ";
     }
}


bool MobileDevelopmentDeployer::writeCoreScriptsToStream(const boost::filesystem::path& dir_path, std::stringstream& stream)
{
    if (!boost::filesystem::exists(dir_path))
    {
        return false;
    }
  
    boost::filesystem::directory_iterator end_itr;
    for ( boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr )
    {
        bool isHiddenDir = ::isHidden(itr->path());
        if ( is_directory(itr->status()) && !isHiddenDir )
        {
            if ( writeCoreScriptsToStream( itr->path(), stream) )
                return true;
        }
        
        else if ( !isHiddenDir && is_regular_file(itr->status()) )
        {
            const std::string fileName = itr->path().filename().string();
            if (fileName.find(".lua") != std::string::npos)
            {
                const std::string filePath = itr->path().string();
                writeCoreScriptToFile(filePath, fileName, stream);
            }
        }
    }
    
    return false;
}

void MobileDevelopmentDeployer::on_readyRead()
{
	if (tcpClients.size() <= 0)
		return;
    
    QTcpSocket* tcpClient = qobject_cast<QTcpSocket*>(sender());
    if (!tcpClient)
        return;

	QByteArray ba = tcpClient->readLine();

	if(strcmp(ba.constData(), "!exit\n") == 0)
	{
		tcpClient->disconnectFromHost();
		return;
	}

	if(strcmp(ba.constData(), "RbxReadyForPlay") == 0)
	{
        QString address = getLocalIPAddress();
		tcpClient->write("RbxReadyForPlay 53640 " + address.toAscii());
        return;
	}
    
    if(strcmp(ba.constData(), "RbxReadyForCoreScripts") == 0)
    {
        if (RBX::BaseScript::hasCoreScriptReplacements())
        {
            tcpClient->write("RbxReadyForCoreScripts true");
        }
        else
        {
            tcpClient->write("RbxReadyForCoreScripts false");
        }
        return;
    }
    
    if(strcmp(ba.constData(), "RbxSendCoreScripts") == 0)
    {
        if (RBX::BaseScript::hasCoreScriptReplacements())
        {
            tcpClient->write("RbxSendCoreScripts ");
            
            std::stringstream ss;
            
            boost::filesystem::path rootPath = boost::filesystem::path(RBX::BaseScript::adminScriptsPath);
            writeCoreScriptsToStream(rootPath, ss);
            ss << "RbxCoreScriptEnd";

            std::string coreScriptsString = ss.str();
            tcpClient->write(coreScriptsString.c_str(), coreScriptsString.size());
        }
        return;
    }
    
    
    QString attemptToPairCorrectResponse = QString("RbxAttemptToPair ").append(getPairCodeString());
	if(strcmp(ba.constData(), attemptToPairCorrectResponse.toAscii()) == 0)
	{
		tcpClient->write("RbxAttemptToPair true");
	}
}

void MobileDevelopmentDeployer::on_disconnected()
{
    if (tcpClients.size() <= 0)
		return;
    
    QTcpSocket* tcpClient = qobject_cast<QTcpSocket*>(sender());
    if (!tcpClient)
        return;
    
	QObject::disconnect(tcpClient, SIGNAL(disconnected()));
	QObject::disconnect(tcpClient, SIGNAL(readyRead()));

	if ( tcpClient )
	{
        std::vector<QTcpSocket*>::iterator iter = std::find(tcpClients.begin(), tcpClients.end(), tcpClient);
        tcpClients.erase(iter);
        
		tcpClient->disconnectFromHost();
		tcpClient->deleteLater();
		tcpClient = NULL;
	}
}