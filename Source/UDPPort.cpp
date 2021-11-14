/*
	SuperColliderAU Copyright (c) 2006 Gerard Roma.
	SuperCollider   Copyright (c) 2002 James McCartney. All rights reserved.

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

#include "UDPPort.h"
#include "SC_OscUtils.hpp"

///// from SC_ComPort.cpp ///////////

bool ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket);

static void udp_reply_func(struct ReplyAddress *addr, char* msg, int size)
{
	using namespace boost::asio;

	ip::udp::socket * socket = reinterpret_cast<ip::udp::socket*>(addr->mReplyData);
	ip::udp::endpoint endpoint (addr->mAddress, addr->mPort);
    
	boost::system::error_code errc;
	socket->send_to( buffer(msg, size), endpoint, 0, errc);

	if (errc)
		printf("%s\n", errc.message().c_str());
}


static bool UnrollOSCPacket(World *inWorld, int inSize, char *inData, OSC_Packet *inPacket)
{
   if (inSize != 12)
       dumpOSC(2, inSize, inData);
    
	if (!strcmp(inData, "#bundle")) { // is a bundle
		char *data;
		char *dataEnd = inData + inSize;
		int len = 16;
		bool hasNestedBundle = false;

		// get len of nested messages only, without len of nested bundle(s)
		data = inData + 16; // skip bundle header
		while (data < dataEnd) {
			int32 msgSize = OSCint(data);
			data += sizeof(int32);
			if (strcmp(data, "#bundle")) // is a message
				len += sizeof(int32) + msgSize;
			else
				hasNestedBundle = true;
			data += msgSize;
		}

		if (hasNestedBundle) {
			if (len > 16) { // not an empty bundle
				// add nested messages to bundle buffer
				char *buf = (char*)malloc(len);
				inPacket->mSize = len;
				inPacket->mData = buf;

				memcpy(buf, inData, 16); // copy bundle header
				data = inData + 16; // skip bundle header
				while (data < dataEnd) {
					int32 msgSize = OSCint(data);
					data += sizeof(int32);
					if (strcmp(data, "#bundle")) { // is a message
						memcpy(buf, data-sizeof(int32), sizeof(int32) + msgSize);
						buf += msgSize;
					}
					data += msgSize;
				}

				// process this packet without its nested bundle(s)
				if(!ProcessOSCPacket(inWorld, inPacket)) {
					free(buf);
					return false;
				}
			}

			// process nested bundle(s)
			data = inData + 16; // skip bundle header
			while (data < dataEnd) {
				int32 msgSize = OSCint(data);
				data += sizeof(int32);
				if (!strcmp(data, "#bundle")) { // is a bundle
					OSC_Packet* packet = (OSC_Packet*)malloc(sizeof(OSC_Packet));
					memcpy(packet, inPacket, sizeof(OSC_Packet)); // clone inPacket

					if(!UnrollOSCPacket(inWorld, msgSize, data, packet)) {
						free(packet);
						return false;
					}
				}
				data += msgSize;
			}
		} else { // !hasNestedBundle
			char *buf = (char*)malloc(inSize);
			inPacket->mSize = inSize;
			inPacket->mData = buf;
			memcpy(buf, inData, inSize);

			if(!ProcessOSCPacket(inWorld, inPacket)) {
				free(buf);
				return false;
			}
		}
	} else { // is a message
		char *buf = (char*)malloc(inSize);
		inPacket->mSize = inSize;
		inPacket->mData = buf;
		memcpy(buf, inData, inSize);

		if(!ProcessOSCPacket(inWorld, inPacket)) {
			free(buf);
			return false;
		}
	}

	return true;
}

/////


void UDPPort::handleReceivedUDP(const boost::system::error_code& error,
	                       std::size_t bytes_transferred)
	{
		if (error == boost::asio::error::operation_aborted){
			return;    /* we're done */
		}

		if (error) {
			printf("SC_UdpInPort: received error - %s", error.message().c_str());
			startReceiveUDP();
			return;
		}
		//TODO

		OSC_Packet * packet = (OSC_Packet*)malloc(sizeof(OSC_Packet));
    
		packet->mReplyAddr.mProtocol = kUDP;
		packet->mReplyAddr.mAddress  = mRemoteEndpoint.address();
		packet->mReplyAddr.mPort     = mRemoteEndpoint.port();
		packet->mReplyAddr.mSocket   = mUdpSocket.native_handle();
		packet->mReplyAddr.mReplyFunc = udp_reply_func;
		packet->mReplyAddr.mReplyData = (void*)&mUdpSocket;

		packet->mSize = bytes_transferred;
    
		if (!UnrollOSCPacket(mWorld, bytes_transferred, mRecvBuffer.data(), packet))
			free(packet);

		startReceiveUDP();
	}

	void UDPPort::startReceiveUDP()
	{
		using namespace boost;
		mUdpSocket.async_receive_from(asio::buffer(mRecvBuffer), mRemoteEndpoint,
		                             boost::bind(&UDPPort::handleReceivedUDP, this,
		                                         asio::placeholders::error, asio::placeholders::bytes_transferred));
	}

	UDPPort::UDPPort(struct World * world, std::string bindTo, int inPortNum):
		mWorld(world), mPortNum(inPortNum), mbindTo(bindTo), mUdpSocket(mIoService)
	{
		using namespace boost::asio;
		startAsioThread();
		BOOST_AUTO(protocol, ip::udp::v4());
		mUdpSocket.open(protocol);

		mUdpSocket.bind(ip::udp::endpoint(boost::asio::ip::address::from_string(bindTo), inPortNum));

		boost::asio::socket_base::send_buffer_size option(65536);
		mUdpSocket.set_option(option);

#ifdef USE_RENDEZVOUS
		if (world->mRendezvous) {
			SC_Thread thread( boost::bind( PublishPortToRendezvous, kSCRendezvous_UDP, sc_htons(mPortNum) ) );
			mRendezvousThread = std::move(thread);
		}
#endif
		startReceiveUDP();
	}

	void UDPPort::startAsioThread()
	{
		SC_Thread asioThread ([this] {
			boost::asio::io_service::work work(mIoService);
			mIoService.run();
		});

		mAsioThread = std::move(asioThread);
	}

	void UDPPort::stopAsioThread()
	{
		if(mAsioThread.joinable()){
			mIoService.stop();
			mAsioThread.join();
		}
	}
