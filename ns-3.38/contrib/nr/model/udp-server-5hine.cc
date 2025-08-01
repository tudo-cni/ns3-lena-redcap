/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "ns3/udp-server-5hine.h"

#include "ns3/packet-loss-counter.h"
#include "ns3/seq-ts-header.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpServer5hine");

NS_OBJECT_ENSURE_REGISTERED(UdpServer5hine);

TypeId
UdpServer5hine::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpServer5hine")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<UdpServer5hine>()
            .AddAttribute("Port",
                          "Port on which we listen for incoming packets.",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UdpServer5hine::m_port),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("PacketWindowSize",
                          "The size of the window used to compute the packet loss. This value "
                          "should be a multiple of 8.",
                          UintegerValue(32),
                          MakeUintegerAccessor(&UdpServer5hine::GetPacketWindowSize,
                                               &UdpServer5hine::SetPacketWindowSize),
                          MakeUintegerChecker<uint16_t>(8, 256))
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&UdpServer5hine::m_rxTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&UdpServer5hine::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

UdpServer5hine::UdpServer5hine()
    : m_lossCounter(0)
{
    NS_LOG_FUNCTION(this);
    m_received = 0;
}

UdpServer5hine::~UdpServer5hine()
{
    NS_LOG_FUNCTION(this);
}

uint16_t
UdpServer5hine::GetPacketWindowSize() const
{
    NS_LOG_FUNCTION(this);
    return m_lossCounter.GetBitMapSize();
}

void
UdpServer5hine::SetPacketWindowSize(uint16_t size)
{
    NS_LOG_FUNCTION(this << size);
    m_lossCounter.SetBitMapSize(size);
}

uint32_t
UdpServer5hine::GetLost() const
{
    NS_LOG_FUNCTION(this);
    return m_lossCounter.GetLost();
}

uint64_t
UdpServer5hine::GetReceived() const
{
    NS_LOG_FUNCTION(this);
    return m_received;
}

void
UdpServer5hine::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
UdpServer5hine::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&UdpServer5hine::HandleRead, this));

    if (!m_socket6)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket6 = Socket::CreateSocket(GetNode(), tid);
        Inet6SocketAddress local = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if (m_socket6->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    m_socket6->SetRecvCallback(MakeCallback(&UdpServer5hine::HandleRead, this));
}

void
UdpServer5hine::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
UdpServer5hine::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        m_rxTrace(packet);
        m_rxTraceWithAddresses(packet, from, localAddress);
        if (packet->GetSize() > 0)
        {
            uint32_t receivedSize = packet->GetSize();
            SeqTsHeader seqTs;
            packet->RemoveHeader(seqTs);
            uint32_t currentSequenceNumber = seqTs.GetSeq();
            if (InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("TraceDelay: RX " << receivedSize << " bytes from "
                                              << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                              << " Sequence Number: " << currentSequenceNumber
                                              << " Uid: " << packet->GetUid() << " TXtime: "
                                              << seqTs.GetTs() << " RXtime: " << Simulator::Now()
                                              << " Delay: " << Simulator::Now() - seqTs.GetTs());
            }
            else if (Inet6SocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("TraceDelay: RX " << receivedSize << " bytes from "
                                              << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                                              << " Sequence Number: " << currentSequenceNumber
                                              << " Uid: " << packet->GetUid() << " TXtime: "
                                              << seqTs.GetTs() << " RXtime: " << Simulator::Now()
                                              << " Delay: " << Simulator::Now() - seqTs.GetTs());
            }

            m_lossCounter.NotifyReceived(currentSequenceNumber);
            m_received++;
        }
    }
}

} // Namespace ns3
