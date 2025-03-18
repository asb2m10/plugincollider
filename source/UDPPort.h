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

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class UDPPort : juce::Thread {
public:
    std::function<bool(char *msg, int size, OSC_Packet *packet)> handleMessage;
    bool connectToPort(int portNumber);
    bool connectToNextFreePort(int startPortNumber);
    bool disconnect();

    UDPPort() : juce::Thread("UDP Port Receiver") {
    }

    ~UDPPort() {
        disconnect();
    }

    int getListenPort() {
        if ( socket != nullptr ) {
            return socket->getBoundPort();
        }
        return -1;
    }

    void processReply(struct ReplyAddress *addr, char *msg, int size) {
        jassert(socket);
        if ( socket == nullptr )  {
            scprintf("Cannot reply since server socket closed\n");
            return;

        }
        if ( ! socket->write(juce::String(addr->mAddress.to_string()), addr->mPort, msg, size) ) {
            scprintf("Unable to send reply OSC message\n");
        }
    }

    bool isConnected() {
        return connected;
    }

private:
    juce::OptionalScopedPointer<juce::DatagramSocket> socket;
    void run() override;
    bool connected = false;
};
