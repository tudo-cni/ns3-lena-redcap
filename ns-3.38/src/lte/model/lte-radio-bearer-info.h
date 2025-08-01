/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_RADIO_BEARER_INFO_H
#define LTE_RADIO_BEARER_INFO_H

#include <ns3/eps-bearer.h>
#include <ns3/ipv4-address.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/object.h>
#include <ns3/pointer.h>

namespace ns3
{

class LteRlc;
class LtePdcp;

/**
 * store information on active radio bearer instance
 *
 */
class LteRadioBearerInfo : public Object
{
  public:
    LteRadioBearerInfo();
    ~LteRadioBearerInfo() override;
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    Ptr<LteRlc> m_rlc;   ///< RLC
    Ptr<LtePdcp> m_pdcp; ///< PDCP
};

/**
 * store information on active signaling radio bearer instance
 *
 */
class LteSignalingRadioBearerInfo : public LteRadioBearerInfo
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    uint8_t m_srbIdentity;                                  ///< SRB identity
    LteRrcSap::LogicalChannelConfig m_logicalChannelConfig; ///< logical channel config
};

/**
 * store information on active data radio bearer instance
 *
 */
class LteDataRadioBearerInfo : public LteRadioBearerInfo
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    EpsBearer m_epsBearer;                                  ///< EPS bearer
    uint8_t m_epsBearerIdentity;                            ///< EPS bearer identity
    uint8_t m_drbIdentity;                                  ///< DRB identity
    LteRrcSap::RlcConfig m_rlcConfig;                       ///< RLC config
    uint8_t m_logicalChannelIdentity;                       ///< logical channel identity
    LteRrcSap::LogicalChannelConfig m_logicalChannelConfig; ///< logical channel config
    uint32_t m_gtpTeid; /**< S1-bearer GTP tunnel endpoint identifier, see 36.423 9.2.1 */
    Ipv4Address m_transportLayerAddress; /**< IP Address of the SGW, see 36.423 9.2.1 */
    bool sdtConfigured{false}; 
};

} // namespace ns3

#endif // LTE_RADIO_BEARER_INFO_H
