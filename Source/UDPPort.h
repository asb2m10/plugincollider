/*
        SuperColliderAU Copyright (c) 2006 Gerard Roma.

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

// Adaptated from SC_UdpInPort in SC_ComPort.cpp

#pragma once

#include "OSC_Packet.h"
#include "SCProcess.h"
#include "SC_Lock.h"
#include "SC_WorldOptions.h"
#include "sc_msg_iter.h"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/typeof/typeof.hpp>

class SCProcess;

class UDPPort {
  private:
    SCProcess *mSCProcess;
    int mPortNum;
    std::string mbindTo;
    static const int kTextBufSize = 65536;
    boost::array<char, kTextBufSize> mRecvBuffer;
    boost::asio::ip::udp::endpoint mRemoteEndpoint;
    boost::asio::io_service mIoService;
    SC_Thread mAsioThread;
    void handleReceivedUDP(const boost::system::error_code &error,
                           std::size_t bytes_transferred);
    void startReceiveUDP();

  public:
    boost::asio::ip::udp::socket mUdpSocket;
    UDPPort(SCProcess *scprocess, std::string bindTo, int inPortNum);
    void startAsioThread();
    void stopAsioThread();
};
