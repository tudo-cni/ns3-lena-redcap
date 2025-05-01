/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_RRC_PROTOCOL_IDEAL_H
#define NR_RRC_PROTOCOL_IDEAL_H

#include <ns3/nr-rrc-sap.h>
#include <ns3/object.h>
#include <ns3/ptr.h>

#include <map>
#include <stdint.h>

namespace ns3
{

class NrUeRrcSapProvider;
class NrUeRrcSapUser;
class NrEnbRrcSapProvider;
class NrUeRrc;

/**
 * \ingroup ue
 * \ingroup gnb
 *
 * \brief RRC message passing from the UE to the GNB
 *
 * Models the transmission of RRC messages from the UE to the gNB in
 * an ideal fashion, without errors and without consuming any radio
 * resources.
 *
 */
class nrUeRrcProtocolIdeal : public Object
{
    friend class MemberNrUeRrcSapUser<nrUeRrcProtocolIdeal>;
    
  public:
    /**
     * \brief nrUeRrcProtocolIdeal constructor
     */
    nrUeRrcProtocolIdeal();
    /**
     * \brief ~nrUeRrcProtocolIdeal
     */
    ~nrUeRrcProtocolIdeal() override;

    // inherited from Object
    void DoDispose() override;
    /**
     * \brief GetTypeId
     * \return the type id of the object
     */
    static TypeId GetTypeId();

    /**
     * \brief SetNrUeRrcSapProvider
     * \param p
     */
    void SetNrUeRrcSapProvider(NrUeRrcSapProvider* p);
    /**
     * \brief GetNrUeRrcSapUser
     * \return
     */
    NrUeRrcSapUser* GetNrUeRrcSapUser();

    /**
     * \brief SetUeRrc
     * \param rrc
     */
    void SetUeRrc(Ptr<NrUeRrc> rrc);

  private:
    // methods forwarded from NrUeRrcSapUser
    void DoSetup(NrUeRrcSapUser::SetupParameters params);
    void DoSendRrcConnectionRequest(NrRrcSap::NrRrcConnectionRequest msg, uint16_t grantedBytes);
    void DoSendRrcResumeRequest(bool sdt, NrRrcSap::RrcResumeRequest msg, uint16_t grantedBytes);
    void DoSendRrcConnectionSetupCompleted(NrRrcSap::RrcConnectionSetupCompleted msg);
    void DoSendRrcResumeComplete(NrRrcSap::RrcResumeComplete msg);
    void DoSendRrcConnectionReconfigurationCompleted(
        NrRrcSap::RrcConnectionReconfigurationCompleted msg);
    void DoSendRrcConnectionReestablishmentRequest(
        NrRrcSap::RrcConnectionReestablishmentRequest msg);
    void DoSendRrcConnectionReestablishmentComplete(
        NrRrcSap::RrcConnectionReestablishmentComplete msg);
    void DoSendMeasurementReport(NrRrcSap::MeasurementReport msg);
    /**
     * \brief Send Ideal UE context remove request function
     *
     * Notify eNodeB to release UE context once radio link failure
     * or random access failure is detected. It is needed since no
     * RLF detection mechanism at eNodeB is implemented
     *
     * \param rnti the RNTI of the UE
     */
    void DoSendIdealUeContextRemoveRequest(uint16_t rnti);

    void DoUpdateBwp(uint16_t rnti, uint8_t bwpID);


    void SetGnbRrcSapProvider();

    Ptr<NrUeRrc> m_rrc;
    uint16_t m_rnti;
    NrUeRrcSapProvider* m_ueRrcSapProvider;
    NrUeRrcSapUser* m_ueRrcSapUser;
    NrGnbRrcSapProvider* m_enbRrcSapProvider;
};

/**
 * Models the transmission of RRC messages from the UE to the gNB in
 * an ideal fashion, without errors and without consuming any radio
 * resources.
 *
 */
class NrGnbRrcProtocolIdeal : public Object
{
    friend class MemberNrGnbRrcSapUser<NrGnbRrcProtocolIdeal>;

  public:
    NrGnbRrcProtocolIdeal();
    ~NrGnbRrcProtocolIdeal() override;

    // inherited from Object
    void DoDispose() override;
    static TypeId GetTypeId();

    void SetNrGnbRrcSapProvider(NrGnbRrcSapProvider* p);
    NrGnbRrcSapUser* GetNrGnbRrcSapUser();

    NrUeRrcSapProvider* GetUeRrcSapProvider(uint16_t rnti);
    void SetUeRrcSapProvider(uint16_t rnti, NrUeRrcSapProvider* p);

  private:
    // methods forwarded from NrGnbRrcSapUser
    void DoSetupUe(uint16_t rnti, NrGnbRrcSapUser::SetupUeParameters params);
    void DoRemoveUe(uint16_t rnti);
    void DoSendSystemInformation(uint16_t cellId, NrRrcSap::SystemInformation msg);
    void SendSystemInformation(uint16_t cellId, NrRrcSap::SystemInformation msg);
    void DoSendRrcSetup(uint16_t rnti, NrRrcSap::RrcSetup msg);
    void DoSendRrcResume(uint16_t rnti, NrRrcSap::RrcResume msg);
    void DoSendRrcReconfiguration(uint16_t rnti,
                                            NrRrcSap::RrcReconfiguration msg);
    void DoSendRrcConnectionReestablishment(uint16_t rnti,
                                            NrRrcSap::RrcConnectionReestablishment msg);
    void DoSendRrcConnectionReestablishmentReject(
        uint16_t rnti,
        NrRrcSap::RrcConnectionReestablishmentReject msg);
    void DoSendRrcRelease(uint16_t rnti, NrRrcSap::RrcRelease msg);
    void DoSendRrcConnectionReject(uint16_t rnti, NrRrcSap::RrcConnectionReject msg);
    Ptr<Packet> DoEncodeHandoverPreparationInformation(NrRrcSap::HandoverPreparationInfo msg);
    NrRrcSap::HandoverPreparationInfo DoDecodeHandoverPreparationInformation(Ptr<Packet> p);
    Ptr<Packet> DoEncodeHandoverCommand(NrRrcSap::RrcConnectionReconfiguration msg);
    NrRrcSap::RrcConnectionReconfiguration DoDecodeHandoverCommand(Ptr<Packet> p);

    uint16_t m_rnti;
    NrGnbRrcSapProvider* m_enbRrcSapProvider;
    NrGnbRrcSapUser* m_enbRrcSapUser;
    std::map<uint16_t, NrUeRrcSapProvider*> m_enbRrcSapProviderMap;
  
};

} // namespace ns3

#endif // NR_RRC_PROTOCOL_IDEAL_H
