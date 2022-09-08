/**
 * MobileDevelopmentDeployer.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>
#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>

#include <boost/filesystem.hpp>

class MobileDevelopmentDeployer : public QWidget
{
	Q_OBJECT
	
public:
	MobileDevelopmentDeployer(QWidget *pParent=0);
	~MobileDevelopmentDeployer();
	static MobileDevelopmentDeployer& singleton();

    void pairWithRbxDevDevices();
    void startPlaySessionWithRbxDevDevices();
    
	void disconnectClientsAndShutDown();
    
    const QString getPairCodeString();
    
    static int getNumOfDigits(int numToCheck);
	static int forcedPairCodeSize();
    
Q_SIGNALS:
    void connectedToClient(const QString& clientName);

private:
	QTcpServer *tcpServer;
    std::vector<QTcpSocket*> tcpClients;
    
	QUdpSocket* udpSocket; // used to establish connection with server
    
    QTimer* timer;

	QString getLocalIPAddress();
    
    typedef enum
    {
        ON_PAIR_DO_NOTHING,
        ON_PAIR_START_SERVER
    } OnPairBehavior;
    
    OnPairBehavior onPairBehavior;
    
	void listenForRbxDevDevices();
    
    int generateNewPairCode();
    int getSavedPairCode();
    int getPairCode();
    
    void broadcastDatagram(QByteArray dataToSend);
    void sendToAllBroadcast(QByteArray packet);
    
    bool startTcpServer();
    
    bool isPairCode(const QString& codeString);
    
    bool checkForClientPairCode(const QString& messageFromClient);
    bool checkForClientPaired(const QString& messageFromClient, const QHostAddress& sender);
    
    void disconnectAllClients();
    
    bool writeCoreScriptsToStream(const boost::filesystem::path& dir_path, std::stringstream& stream);
    void writeCoreScriptToFile(const std::string& filePath, const std::string& fileName, std::stringstream& stream);
    
private Q_SLOTS:
	void on_newConnection();
	void on_readyRead();
	void on_disconnected();
    
    void on_receivedUdpBroadcast();
    void broadcastReadyDatagram();
};
