/*
    PluginCollider Copyright (c) 2025 Pascal Gauthier.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OSC_Packet.h"
#include "SC_WorldOptions.h"
#include "sc_msg_iter.h"
#include "UDPPort.h"

static void udp_reply_func(struct ReplyAddress *addr, char *msg, int size) {
    UDPPort *udpPort = reinterpret_cast<UDPPort *>(addr->mReplyData);
    udpPort->processReply(addr, msg, size);
}

bool UDPPort::connectToPort(int portNumber) {
    if ( getListenPort() == portNumber )
        return true;

    connected = false;

    if (! disconnect())
        return false;

    socket.setOwned(new juce::DatagramSocket(false));
    socket->setEnablePortReuse(false);
    if (! socket->bindToPort(portNumber))
        return false;

    scprintf("Server listning to port %d\n", portNumber);
    startThread();
    connected = true;
    return true;
}

bool UDPPort::connectToNextFreePort(int startNum) {
    connected = false;

    if (! disconnect())
        return false;

    socket.setOwned(new juce::DatagramSocket(false));
    socket->setEnablePortReuse(false);

    for(; startNum < 12000 ; startNum++ ) {
        if ( socket->bindToPort(startNum) ) {
            scprintf("Server listning to port %d\n", startNum);
            startThread();
            connected = true;
            return true;
        }
    }

    return false;
}

bool UDPPort::disconnect() {
    if (socket != nullptr) {
        signalThreadShouldExit();

        if (socket.willDeleteObject())
            socket->shutdown();

        waitForThreadToExit(10000);
        socket.reset();
    }
    connected = false;
    return true;
}

void UDPPort::run() {
    const int bufferSize = 65535;
    juce::HeapBlock<char> oscBuffer(bufferSize);

    while (! threadShouldExit()) {
        juce::String senderAddr;
        int senderPort;

        jassert(socket != nullptr);
        auto ready = socket->waitUntilReady(true, 100);

        if (ready < 0 || threadShouldExit())
            return;

        if (ready == 0)
            continue;

        auto bytesRead = (size_t) socket->read(oscBuffer.getData(), bufferSize, false, senderAddr, senderPort);

        if (bytesRead >= 4) {
            if ( handleMessage == NULL )
                continue;

            OSC_Packet *packet = (OSC_Packet *)malloc(sizeof(OSC_Packet));
            packet->mReplyAddr.mProtocol = kUDP;
            packet->mReplyAddr.mAddress = boost::asio::ip::address::from_string(senderAddr.toRawUTF8());
            packet->mReplyAddr.mPort = senderPort;
            packet->mReplyAddr.mSocket = socket->getRawSocketHandle();
            packet->mReplyAddr.mReplyFunc = udp_reply_func;
            packet->mReplyAddr.mReplyData = (void *)this;
            packet->mSize = bytesRead;

            if ( !handleMessage(oscBuffer.getData(), bytesRead, packet) ) {
                free(packet);
            }
        }
    }

    connected = false;
}
