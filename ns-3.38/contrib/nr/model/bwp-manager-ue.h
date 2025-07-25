/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef BWPMANAGERUE_H
#define BWPMANAGERUE_H

#include <ns3/nr-phy-mac-common.h>
#include <ns3/simple-ue-component-carrier-manager.h>
#include <ns3/lte-ue-cmac-sap.h>


namespace ns3
{

class BwpManagerAlgorithm;
class NrControlMessage;

/**
 * \ingroup ue-bwp
 * \brief The BwpManagerUe class
 */
class BwpManagerUe : public SimpleUeComponentCarrierManager
{
  public:
    /**
     * \brief GetTypeId
     * \return the type id for the object
     */
    static TypeId GetTypeId();

    /**
     * \brief BwpManagerUe constructor
     */
    BwpManagerUe();
    /**
     * \brief ~BwpManagerUe
     */
    ~BwpManagerUe() override;

    /**
     * \brief Set the algorithm
     * \param algorithm pointer to the algorithm
     */
    void SetBwpManagerAlgorithm(const Ptr<BwpManagerAlgorithm>& algorithm);

    /**
     * \brief The UE received a HARQ feedback from spectrum. Where this feedback
     * should be forwarded?
     *
     * \param m the feedback
     * \return the BWP index in which the feedback can be transmitted to the gNB.
     */
    uint8_t RouteDlHarqFeedback(const DlHarqInfo& m) const;

    /**
     * \brief Decide the BWP for the control message received.
     * \param msg Message
     * \param sourceBwpId BWP Id from which this message come from.
     *
     * The routing is made following the bandwidth part reported in the message.
     *
     * \return the BWP Id to which this message should be routed to.
     */
    uint8_t RouteIngoingCtrlMsg(const Ptr<NrControlMessage>& msg, uint8_t sourceBwpId) const;

    /**
     * \brief Route the outgoing messages to the right BWP
     * \param msg the message
     * \param sourceBwpId the source bwp of the message
     *
     * The routing is made by following the mapping provided through the function
     * SetOutputLink. If no mapping has been installed, or if the sourceBwpId
     * provided is not in the mapping, then forward the message back to the
     * originating BWP.
     *
     * \see SetOutputLink
     *
     * \return the bwp to which the ctrl messages should be redirected
     */
    uint8_t RouteOutgoingCtrlMsg(const Ptr<NrControlMessage>& msg, uint8_t sourceBwpId) const;

    /**
     * \brief Set a mapping between two BWP.
     * \param sourceBwp The messages that comes from this value...
     * \param outputBwp ... will get routed in this bandwidth part.
     *
     * Call it for each mapping you want to install.
     */
    void SetOutputLink(uint32_t sourceBwp, uint32_t outputBwp);

  protected:
    void DoReportBufferStatus(LteMacSapProvider::ReportBufferStatusParameters params) override;
    void DoStartBwpSwitching(uint8_t bwpID);
    void SwitchBwp(const uint8_t bwpID);
    void SetBwpInactivityTimer(Time inactTimer);
    void DoRefreshBwpInactivityTimer();
    void BwpInactivityTimeout();

    std::vector<LteUeCcmRrcSapProvider::LcsConfig> DoAddLc(
        uint8_t lcId,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu) ;
    LteMacSapUser* DoConfigureSignalBearer(uint8_t lcId,
                                           LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
                                           LteMacSapUser* msu) ;

  private:
    Ptr<BwpManagerAlgorithm> m_algorithm;
    std::unordered_map<uint8_t, EpsBearer::Qci> m_lcToBearerMap; //!< Map from LCID to bearer ID

    std::unordered_map<uint32_t, uint32_t> m_outputLinks; //!< Mapping between BWP.

    uint8_t m_activeBwp{0}; // Bwp to be used.
    EventId m_bwpInactivityTimout;
    Time m_bwpInactivityTimer;
};

} // namespace ns3
#endif // BWPMANAGERUE_H
