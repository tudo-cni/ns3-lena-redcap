/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 *
 */

#ifndef UDP_CLIENT_5HINE_H
#define UDP_CLIENT_5HINE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"

namespace ns3
{

class Socket;
class Packet;

/**
 * \ingroup UdpClient5hineserver5hine
 *
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class UdpClient5hine : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UdpClient5hine();

    ~UdpClient5hine() override;

    /**
     * \brief set the remote address and port
     * \param ip remote IP address
     * \param port remote port
     */
    void SetRemote(Address ip, uint16_t port);
    /**
     * \brief set the remote address
     * \param addr remote address
     */
    void SetRemote(Address addr);

    /**
     * \return the total bytes sent by this app
     */
    uint64_t GetTotalTx() const;

    /**
     * \brief Schedule the next packet transmissions
     * \param dt timestamps for packets.
     */
    void ScheduleTransmit(std::vector<TimeValue> dt);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief Send a packet
     */
    void Send();

    /**
     * \brief Send a packet instantly
     */
    void SendOnce();

    uint32_t m_count; //!< Maximum number of packets the application will send
    Time m_interval;  //!< Packet inter-send time
    std::vector<TimeValue> m_transmitTimestamps;
    uint32_t m_size;  //!< Size of the sent packet (including the SeqTsHeader)

    uint32_t m_sent;       //!< Counter for sent packets
    uint64_t m_totalTx;    //!< Total bytes sent
    Ptr<Socket> m_socket;  //!< Socket
    Address m_peerAddress; //!< Remote peer address
    uint16_t m_peerPort;   //!< Remote peer port
    EventId m_sendEvent;   //!< Event to send the next packet
    bool appStopped {false};
#ifdef NS3_LOG_ENABLE
    std::string m_peerAddressString; //!< Remote peer address string
#endif                               // NS3_LOG_ENABLE
};

} // namespace ns3

#endif /* UDP_CLIENT_5HINE_H */
