#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QVariant>
#include "../../client/NetworkManager.h"
#include "../../common/Packet.h"

namespace wizz {

class NetworkManagerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!qApp) {
            static int argc = 1;
            static char* argv[] = {(char*)"test"};
            new QCoreApplication(argc, argv);
        }
    }

    void SetUp() override {
        qRegisterMetaType<wizz::Packet>("wizz::Packet");
        
        NetworkManager& nm = NetworkManager::instance();
        
        // If the manager isn't ready, wait for the initialized signal
        // We use a small timeout to avoid hanging if it's already initialized
        QSignalSpy initSpy(&nm, &NetworkManager::initialized);
        if (initSpy.count() == 0) {
            initSpy.wait(1000); 
        }
    }

    void processEvents(int timeoutMs = 100) {
        QEventLoop loop;
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }
};

TEST_F(NetworkManagerTest, ParseDirectMessage) {
    NetworkManager& nm = NetworkManager::instance();
    QSignalSpy spy(&nm, &NetworkManager::messageReceived);

    Packet msgPkt(PacketType::DirectMessage);
    msgPkt.writeString("System");
    msgPkt.writeString("Hello Test");

    nm.processPacket(msgPkt);

    // Give the event loop a moment to deliver the signal (since nm is on another thread)
    if (spy.count() == 0) {
        spy.wait(500);
    }

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toString(), "System");
    EXPECT_EQ(arguments.at(1).toString(), "Hello Test");
}

TEST_F(NetworkManagerTest, ParseContactList) {
    NetworkManager& nm = NetworkManager::instance();
    QSignalSpy spy(&nm, &NetworkManager::contactListReceived);

    Packet listPkt(PacketType::ContactList);
    listPkt.writeInt(2);
    
    listPkt.writeString("Alice");
    listPkt.writeInt(0);
    listPkt.writeString("Wonderland");

    listPkt.writeString("Bob");
    listPkt.writeInt(3);
    listPkt.writeString("Builder");

    nm.processPacket(listPkt);

    if (spy.count() == 0) {
        spy.wait(500);
    }

    ASSERT_EQ(spy.count(), 1);
    auto contacts = nm.getContacts();
    ASSERT_EQ(contacts.size(), 2);
    EXPECT_EQ(std::get<0>(contacts[0]), "Alice");
    EXPECT_EQ(std::get<2>(contacts[0]), "Wonderland");
}

} // namespace wizz
