/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_UE_PHY_H
#define NR_UE_PHY_H

#include "nr-amc.h"
#include "nr-harq-phy.h"
#include "nr-phy.h"
#include "nr-ue-cphy-sap.h"
#include "nr-phy-sap.h"

//#include <ns3/lte-ue-phy-sap.h>
#include <ns3/traced-callback.h>
//#include <ns3/nr-energy-model.h>
#include "nr-energy-model.h"

namespace ns3
{

class NrChAccessManager;
class BeamManager;
class BeamId;
class NrUePowerControl;

/**
 * \ingroup ue-phy
 * \brief The UE PHY class
 *
 * This class represents the PHY in the User Equipment. Much of the processing
 * and scheduling is done inside the gNb, so the user is a mere "executor"
 * of the decision of the base station.
 *
 * The slot processing is the same as the gnb phy, working as a state machine
 * in which the processing is done at the beginning of the slot.
 *
 * \section ue_phy_conf Configuration
 *
 * The attributes of this class (described in the section Attributes) can be
 * configured through a direct call to `SetAttribute` or, before the PHY creation,
 * with the helper method NrHelper::SetUePhyAttribute().
 *
 * \section ue_phy_attach Attachment to a GNB
 *
 * In theory, much of the configuration should pass through RRC, and through
 * messages that come from the gNb. However, we still are not at this level,
 * and we have to rely on direct calls to configure the same values between
 * the gnb and the ue. At this moment, the call that the helper has to perform
 * are in NrHelper::AttachToEnb().
 *
 * To initialize the class, you must call also SetSpectrumPhy() and StartEventLoop().
 * Usually, this is taken care inside the helper.
 *
 * \see NrPhy::SetSpectrumPhy()
 * \see NrPhy::StartEventLoop()
 */
class NrUePhy : public NrPhy
{
    friend class UeMemberNrUePhySapProvider;
    friend class MemberNrUecphySapProvider<NrUePhy>;

  public:
    /**
     * \brief Get the object TypeId
     * \return the object type id
     */
    static TypeId GetTypeId();

    /**
     * \brief NrUePhy default constructor.
     */
    NrUePhy();

    /**
     * \brief ~NrUePhy
     */
    ~NrUePhy() override;

    /**
     * \brief Retrieve the pointer for the C PHY SAP provider (AKA the PHY interface towards the
     * RRC) \return the C PHY SAP pointer
     */
    NrUecphySapProvider* GetUeCphySapProvider() __attribute__((warn_unused_result));

    /**
     * \brief Install ue C PHY SAP user (AKA the PHY interface towards the RRC)
     * \param s the C PHY SAP user pointer to install
     */
    void SetUeCphySapUser(NrUeCphySapUser* s);

    /**
     * \brief Install the PHY sap user (AKA the UE MAC)
     *
     * \param ptr the PHY SAP user pointer to install
     */
    void SetPhySapUser(NrUePhySapUser* ptr);

    /*
     * \brief Enable or disable uplink power control
     * \param enable parameter that enables or disables power control
     */
    void SetEnableUplinkPowerControl(bool enable);

    /**
     * \brief Set the transmission power for the UE
     *
     * Please note that there is also an attribute ("NrUePhy::TxPower")
     * \param pow power
     */
    void SetTxPower(double pow);

    /**
     * \brief Retrieve the TX power of the UE
     *
     * Please note that there is also an attribute ("NrGnbPhy::TxPower")
     * \return the TX power of the UE
     */
    double GetTxPower() const override;

    /**
     * \brief Returns the latest measured RSRP value
     * Called by NrUePowerControl.
     */
    double GetRsrp() const;

    /**
     * \brief Get LTE uplink power control entity
     *
     * \return ptr pointer to LTE uplink power control entity
     */
    Ptr<NrUePowerControl> GetUplinkPowerControl() const;

    /**
     * \brief Allow configuration of uplink power control algorithm.
     * E.g. necessary in FDD, when measurements are received in
     * downlink BWP, but they are used in uplink BWP
     * NOTE: This way of configuring is a temporal solution until
     * BWP manager has this function implemented for UL PC, FFR,
     * algorithm and simillar algorithms, in which is needed to have
     * a pair of DL and UL BWPs. In future this function will be called
     * only by a friend class.
     * \param pc Pointer to NrUePowerControl
     */
    void SetUplinkPowerControl(Ptr<NrUePowerControl> pc);

    /**
     * \brief Register the UE to a certain Enb
     *
     * Install the configuration parameters in the UE.
     *
     * \param bwpId the bwp id to which this PHY is attaching to
     *
     *
     */
    void RegisterToEnb(uint16_t bwpId);

    /**
     * \brief Set the AMC pointer from the GNB
     * \param amc The DL AMC of the GNB. This will be used to create the DL CQI
     * that will be sent to the GNB.
     *
     * This function will be soon deprecated, hopefully with some values that
     * comes from RRC. For the moment, it is called by the helper at the
     * registration time.
     *
     */
    void SetDlAmc(const Ptr<const NrAmc>& amc);

    /**
     * \brief Set the number of UL CTRL symbols
     * \param ulCtrlSyms value
     *
     * This function will be soon deprecated, hopefully with a value that
     * comes from RRC. For the moment, it is called by the helper at the
     * registration time.
     */
    void SetUlCtrlSyms(uint8_t ulCtrlSyms);

    /**
     * \brief Set the number of DL CTRL symbols
     * \param dlCtrlSyms value
     *
     * This function will be soon deprecated, hopefully with a value that
     * comes from RRC. For the moment, it is called by the helper at the
     * registration time.
     */
    void SetDlCtrlSyms(uint8_t dlCtrlSyms);

    /**
     * \brief Function that sets the number of RBs per RBG.
     * This function will be soon deprecated, as soon as all the functions at
     * gNb PHY, MAC and UE PHY that work with DCI bitmask start
     * to work on level of RBs instead of RBGs.
     * This function is configured by helper
     *
     * \param numRbPerRbg Number of RBs per RBG
     */
    void SetNumRbPerRbg(uint32_t numRbPerRbg);

    /**
     * \brief Set the UE pattern.
     *
     * Temporary.
     * \param pattern The UE pattern
     */
    void SetPattern(const std::string& pattern);

    /**
     * \brief Receive a list of CTRL messages
     *
     * Connected by the helper to a callback of the spectrum.
     *
     * \param msg Message
     */
    void PhyCtrlMessagesReceived(const Ptr<NrControlMessage>& msg);

    /**
     * \brief Receive a PHY data packet
     *
     * Connected by the helper to a callback of the spectrum.
     *
     * \param p Received packet
     */
    void PhyDataPacketReceived(const Ptr<Packet>& p);

    /**
     * \brief Generate a DL CQI report
     *
     * Connected by the helper to a callback in corresponding ChunkProcessor
     *
     * \param sinr the SINR
     * \param streamIndex the index of the stream for which is reported this SINR
     */
    void GenerateDlCqiReport(const SpectrumValue& sinr, uint8_t streamIndex);

    /**
     * \brief Get the current RNTI of the user
     *
     * \return the current RNTI of the user
     */
    uint16_t GetRnti() __attribute__((warn_unused_result));

    /**
     * \brief Get the IMSI of the user
     *
     * \return the IMSI of the user
     */
    uint64_t GetImsi();


    /**
     * \brief Get the HARQ feedback (on the transmission) from
     * NrSpectrumPhy and send it through ideal PUCCH to gNB
     *
     * Connected by the helper to a spectrum phy callback
     *
     * \param m the HARQ feedback
     */
    void EnqueueDlHarqFeedback(const DlHarqInfo& m);

    /**
     * \brief Set the rank indicator value.
     *
     * The is value will be used when a UE is configured
     * to use a fixed rank indicator value instead of the
     * adaptive rank indicator value.
     *
     * \param ri The rank indicator value.
     */
    void SetFixedRankIndicator(uint8_t ri);

    /**
     * \brief Get the value of fixed rank indicator.
     *
     * \return The rank indicator value.
     */
    uint8_t GetFixedRankIndicator() const;

    /**
     * \brief Use the fixed value of rank indicator
     *
     * \param useFixedRi Flag to indicate if a UE should use a fixed or adaptive
     *        rank indicator value.
     */
    void UseFixedRankIndicator(bool useFixedRi);

    /**
     * \brief Set SINR threshold in dB that is used to adaptively choose the rank indicator value.
     *
     * \param sinrThreshold The SINR threshold in dB.
     */
    void SetRiSinrThreshold1(double sinrThreshold);

    /**
     * \brief Get the SINR threshold that is used to adaptively choose the rank indicator value.
     *
     * \return The SINR threshold value in dB.
     */
    double GetRiSinrThreshold1() const;

    /**
     * \brief Set SINR threshold in dB that is used to adaptively choose the rank indicator value.
     *
     * \param sinrThreshold The SINR threshold in dB.
     */
    void SetRiSinrThreshold2(double sinrThreshold);

    /**
     * \brief Get the SINR threshold that is used to adaptively choose the rank indicator value.
     *
     * \return The SINR threshold value in dB.
     */
    double GetRiSinrThreshold2() const;

    /**
     *  TracedCallback signature for DL CTRL SINR trace callback
     *
     * \param [in] cellId
     * \param [in] rnti
     * \param [in] sinr
     * \param [in] bwpId
     * \param [in] streamId
     */
    typedef void (*DlCtrlSinrTracedCallback)(uint16_t, uint16_t, double, uint16_t, uint8_t);

    /**
     *  TracedCallback signature for DL DATA SINR trace callback
     *
     * \param [in] cellId
     * \param [in] rnti
     * \param [in] sinr
     * \param [in] bwpId
     * \param [in] streamId
     */
    typedef void (*DlDataSinrTracedCallback)(uint16_t, uint16_t, double, uint16_t, uint8_t);

    /**
     *  TracedCallback signature for Ue Phy Received Control Messages.
     *
     * \param [in] frame Frame number.
     * \param [in] subframe Subframe number.
     * \param [in] slot number.
     * \param [in] VarTti
     * \param [in] nodeId
     * \param [in] rnti
     * \param [in] bwpId
     * \param [in] pointer to msg to get the msg type
     */
    typedef void (*RxedUePhyCtrlMsgsTracedCallback)(const SfnSf sfnSf,
                                                    const uint16_t nodeId,
                                                    const uint16_t rnti,
                                                    const uint8_t bwpId,
                                                    Ptr<NrControlMessage>);

    /**
     *  TracedCallback signature for Ue Phy Transmitted Control Messages.
     *
     * \param [in] frame Frame number.
     * \param [in] subframe Subframe number.
     * \param [in] slot number.
     * \param [in] VarTti
     * \param [in] nodeId
     * \param [in] rnti
     * \param [in] bwpId
     * \param [in] pointer to msg to get the msg type
     */
    typedef void (*TxedUePhyCtrlMsgsTracedCallback)(const SfnSf sfnSf,
                                                    const uint16_t nodeId,
                                                    const uint16_t rnti,
                                                    const uint8_t bwpId,
                                                    Ptr<NrControlMessage>);

    /**
     *  TracedCallback signature for Ue Phy DL DCI reception.
     *
     * \param [in] frame Frame number.
     * \param [in] subframe Subframe number.
     * \param [in] slot number.
     * \param [in] VarTti
     * \param [in] nodeId
     * \param [in] rnti
     * \param [in] bwpId
     * \param [in] harq ID
     * \param [in] K1 Delay
     */
    typedef void (*RxedUePhyDlDciTracedCallback)(const SfnSf sfnSf,
                                                 const uint16_t nodeId,
                                                 const uint16_t rnti,
                                                 const uint8_t bwpId,
                                                 uint8_t harqId,
                                                 uint32_t K1Delay);

    /**
     *  TracedCallback signature for Ue Phy DL HARQ Feedback transmission.
     *
     * \param [in] frame Frame number.
     * \param [in] subframe Subframe number.
     * \param [in] slot number.
     * \param [in] VarTti
     * \param [in] nodeId
     * \param [in] rnti
     * \param [in] bwpId
     * \param [in] harq ID
     * \param [in] K1 Delay
     */
    typedef void (*TxedUePhyHarqFeedbackTracedCallback)(const SfnSf sfnSf,
                                                        const uint16_t nodeId,
                                                        const uint16_t rnti,
                                                        const uint8_t bwpId,
                                                        uint8_t harqId,
                                                        uint32_t K1Delay);

    /**
     * This callback method type is used by the NrUePhy to notify
     * the status of a DL HARQ feedback
     */
    typedef Callback<void, const DlHarqInfo&> NrPhyDlHarqFeedbackCallback;

    /**
     * \brief Sets the callback to be called when DL HARQ feedback is generated
     */
    void SetPhyDlHarqFeedbackCallback(const NrPhyDlHarqFeedbackCallback& c);

    /**
     * \brief Function called by NrSpectrumPhy to report the
     * DL HARQ feedback from that NrSpectrumPhy instance
     * \param streamId The streamId of the NrSpectrumPhy to which it belongs this HARQ process
     * \param harqFeedback the feedback for the corresponding NrSpectrumPhy from which is called
     * this function \param harqProcessId the HARQ process ID that was saved in the information of
     * the expected TB \param rv the redundancy version that was provided in the information of the
     * expected TB
     */
    void NotifyDlHarqFeedback(uint8_t streamId,
                              DlHarqInfo::HarqStatus harqFeedback,
                              uint8_t harqProcessId,
                              uint8_t rv);

    /**
     * \brief Set the channel access manager interface for this instance of the PHY
     * \param cam the pointer to the interface
     */
    void SetCam(const Ptr<NrChAccessManager>& cam);

    const SfnSf& GetCurrentSfnSf() const override;

    // From nr phy. Not used in the UE
    BeamConfId GetBeamConfId(uint16_t rnti) const override;

    /**
     * \brief Start the ue Event Loop
     *
     * As parameters, there are the initial values for some variables.
     *
     * \param nodeId the UE nodeId
     * \param frame Frame
     * \param subframe SubF.
     * \param slot Slot
     */
    void ScheduleStartEventLoop(uint32_t nodeId,
                                uint16_t frame,
                                uint8_t subframe,
                                uint16_t slot) override;

    /**
     * \brief Called when rsReceivedPower is fired
     * \param power the power received
     * \param streamIndex the index of the stream from which is called this function
     */
    void ReportRsReceivedPower(const SpectrumValue& power, uint8_t streamIndex);

    /**
     * \brief Called when DlCtrlSinr is fired
     * \param sinr the sinr PSD
     * \param streamId the stream ID
     */
    void ReportDlCtrlSinr(const SpectrumValue& sinr, uint8_t streamId);

    /**
     * \brief Compute the CQI based on the SINR
     *
     * The function was implemented to assist mainly the NrSpectrumPhy class
     * to include the CQI in RxPacketTraceUe trace.
     *
     * \param sinr the sinr PSD
     * \return The CQI
     */
    uint8_t ComputeCqi(const SpectrumValue& sinr);

    /**
     * \brief Receive PSS and calculate RSRQ in dBm
     *
     * \param cellId the cell ID
     * \param p PSS list
     */
    void ReceivePss(uint16_t cellId, const Ptr<SpectrumValue>& p);

    /**
     * \brief TracedCallback signature for power trace source
     *
     * It depends on the TxPower configured attribute, and the number of
     * RB allocated.
     *
     * \param [in] sfnSf Slot number
     * \param [in] v Power Spectral Density
     * \param [in] time Time for which this power is set
     * \param [in] rnti RNTI of the UE
     * \param [in] imsi IMSI of the UE
     * \param [in] bwpId Reference BWP ID
     * \param [in] cellId Reference Cell ID
     */
    typedef void (*PowerSpectralDensityTracedCallback)(const SfnSf& sfnSf,
                                                       Ptr<const SpectrumValue> v,
                                                       const Time& time,
                                                       uint16_t rnti,
                                                       uint64_t imsi,
                                                       uint16_t bwpId,
                                                       uint16_t cellId);

  void SetActiveBwpStatus (bool state);
  bool GetActiveBwpStatus() const;

  NrPhySapProvider::PrachConfig GetPrachOccasionsFromIndex(uint8_t prachIndex);

  protected:
    /**
     * \brief DoDispose method inherited from Object
     */
    void DoDispose() override;
    uint32_t GetNumRbPerRbg() const override;

  private:
    /**
     * \brief Layer-1 filtering of RSRP measurements and reporting to the RRC entity.
     * Fo the moment we don't report to RRC but the function is prepared to be
     * extended once RRC is ported.
     *
     * Initially executed at +0.200s, and then repeatedly executed with
     * periodicity as indicated by the *UeMeasFilterPeriod* attribute.
     */
    void ReportUeMeasurements();

    /**
     * \brief Compute the AvgSinr (copied from LteUePhy)
     * \param sinr the SINR
     * \return the average on all the RB
     */
    static double ComputeAvgSinr(const SpectrumValue& sinr);

    void StartEventLoop(uint16_t frame, uint8_t subframe, uint16_t slot);

    /**
     * \brief Channel access granted, invoked after the LBT
     *
     * \param time Time of the grant
     */
    void ChannelAccessGranted(const Time& time);

    /**
     * \brief Channel access denied
     */
    void ChannelAccessDenied();

    /**
     * \brief RequestAccess
     */
    void RequestAccess();

    /**
     * \brief Forward the received RAR to the MAC
     * \param rarMsg RAR message
     */
    void DoReceiveRar(Ptr<NrRarMessage> rarMsg);

    /**
     * \brief Forward the received Paging to the MAC
     * \param pagMsg PAGING message
     */
    void DoReceivePaging(Ptr<NrPagingMessage> pagMsg);
    /**
     * \brief Create a DlCqiFeedback message
     * \param dlCqi the structure that contains DL CQI feedback values per stream
     * \return a CTRL message with the DL CQI feedback
     */
    Ptr<NrDlCqiMessage> CreateDlCqiFeedbackMessage(const DlCqiInfo& dlcqi)
        __attribute__((warn_unused_result));
    /**
     * \brief Receive DL CTRL and return the duration of the transmission
     * \param dci the current DCI
     * \return the duration of the DL CTRL interval
     */
    Time DlCtrl(const std::shared_ptr<DciInfoElementTdma>& dci) __attribute__((warn_unused_result));
    /**
     * \brief Transmit UL CTRL and return the duration of the transmission
     * \param dci the current DCI
     * \return the duration of the UL CTRL interval
     */
    Time UlCtrl(const std::shared_ptr<DciInfoElementTdma>& dci) __attribute__((warn_unused_result));
    /**
     * \brief Transmit UL SRS and return the duration of the transmission
     * \param dci the current DCI
     * \return the duration of the UL SRS interval
     */
    Time UlSrs(const std::shared_ptr<DciInfoElementTdma>& dci);
    /**
     * \brief Receive DL data and return the duration of the transmission
     * \param dci the current DCI
     * \return the duration of the DL data interval
     */
    Time DlData(const std::shared_ptr<DciInfoElementTdma>& dci) __attribute__((warn_unused_result));

    /**
     * \brief Transmit UL data and return the duration of the transmission
     * \param dci the current DCI
     * \return the duration of the UL data interval
     */
    Time UlData(const std::shared_ptr<DciInfoElementTdma>& dci) __attribute__((warn_unused_result));

    Time TransmitMsg3(const std::shared_ptr<DciInfoElementTdma>& dci) __attribute__((warn_unused_result));

    /**
     * \brief Try to perform an lbt before UL CTRL
     *
     * This function should be called after we receive the DL_DCI for the slot,
     * and then checks if we can re-use the channel through shared MCOT. Otherwise,
     * schedule an LBT before the transmission of the UL CTRL.
     */
    void TryToPerformLbt();

    /**
     * \brief Start the slot processing
     * \param s the slot number
     */
    void StartSlot(const SfnSf& s);

    /**
     * \brief Start the processing of a variable TTI
     * \param dci the DCI of the variable TTI
     *
     * This time can be a DL CTRL, a DL data, a UL data, or UL CTRL, with
     * any number of symbols (limited to the number of symbols per slot).
     *
     * At the end of processing, schedule the method EndVarTtti that will finish
     * the processing of the variable tti allocation.
     *
     * \see DlCtrl
     * \see UlCtrl
     * \see DlData
     * \see UlData
     */
    void StartVarTti(const std::shared_ptr<DciInfoElementTdma>& dci, const bool isMsg3);

    /**
     * \brief End the processing of a variable tti
     * \param lastDci the DCI of the variable TTI that has just passed
     *
     * The end of the variable tti indicates that the allocation has been
     * transmitted/received. Depending on the variable tti left, the method
     * will schedule another var tti (StartVarTti()) or will wait until the
     * end of the slot (EndSlot()).
     *
     * \see StartVarTti
     * \see EndSlot
     */
    void EndVarTti(const std::shared_ptr<DciInfoElementTdma>& dci);

    /**
     * \brief Set the Tx power spectral density based on the RB index vector
     * \param mask vector of the index of the RB (in SpectrumValue array)
     * in which there is a transmission
     * \param numSym number of symbols of the transmission
     * \param activeStreams the number of active streams for the transmission
     */
    void SetSubChannelsForTransmission(const std::vector<int>& mask,
                                       uint32_t numSym,
                                       uint8_t activeStreams);
    /**
     * \brief Send ctrl msgs considering L1L2CtrlLatency
     * \param msg The ctrl msg to be sent
     */
    void DoSendControlMessage(Ptr<NrControlMessage> msg);
    /**
     * \brief Send ctrl msgs without considering L1L2CtrlLatency
     * \param msg The ctrl msg to be sent
     */
    void DoSendControlMessageNow(Ptr<NrControlMessage> msg);

    /**
     * \brief Process a received data Dci
     * \param ulSfnSf The slot to which the Dci refers to
     * \param dciInfoElem the DCI
     */
    void ProcessDataDci(const SfnSf& ulSfnSf,
                        const std::shared_ptr<DciInfoElementTdma>& dciInfoElem);

    /**
     * \brief Process a received SRS Dci
     * \param ulSfnSf The slot to which the Dci refers to
     * \param dciInfoElem the DCI
     */
    void ProcessSrsDci(const SfnSf& ulSfnSf,
                       const std::shared_ptr<DciInfoElementTdma>& dciInfoElem);

    /**
     * \brief Transmit to the spectrum phy the data stored in pb
     *
     * \param pb Data to transmit
     * \param duration period of transmission
     * \param ctrlMsg Control messages
     */
    void SendDataChannels(const Ptr<PacketBurst>& pb,
                          const std::list<Ptr<NrControlMessage>>& ctrlMsg,
                          const Time& duration);
    /**
     * \brief Transmit the control channel
     *
     * \param duration the duration of transmission
     *
     * Call the NrSpectrumPhy class, indicating the control message to transmit.
     */
    void SendCtrlChannels(Time duration);

    // SAP methods
    void DoReset();
    void DoStartCellSearch(uint16_t dlEarfcn);
    void DoSynchronizeWithGnb(uint16_t cellId);
    void DoSynchronizeWithGnb(uint16_t cellId, uint16_t dlEarfcn);
    void DoSetPa(double pa);
    /**
     * \param rsrpFilterCoefficient value. Determines the strength of
     * smoothing effect induced by layer 3 filtering of RSRP
     * used for uplink power control in all attached UE.
     * If equals to 0, no layer 3 filtering is applicable.
     */
    void DoSetRsrpFilterCoefficient(uint8_t rsrpFilterCoefficient);

    /**
     * \brief It is called to set an initial bandwidth
     * that will be used until bandwidth is being configured
     */
    void DoSetInitialBandwidth();
    /**
     *
     * Get cell ID
     * \returns cell ID
     */
    uint16_t DoGetCellId();
    /**
     * Get DL EARFCN
     * \returns DL EARFCN
     */
    uint32_t DoGetDlEarfcn();
    /**
     * \brief Function that is called by RRC SAP.
     * TODO This function and its name can be updated once NR RRC SAP is implemented
     */
    void DoSetDlBandwidth(uint16_t ulBandwidth);
    /**
     * \brief Function that is called by RRC SAP.
     * TODO This function and its name can be updated once NR RRC SAP is implemented
     */
    void DoConfigureUplink(uint16_t ulEarfcn, uint8_t ulBandwidth);
    void DoConfigureReferenceSignalPower(int8_t referenceSignalPower);
    void DoSetRnti(uint16_t rnti);
    void DoSetTransmissionMode(uint8_t txMode);
    void DoSetSrsConfigurationIndex(uint16_t srcCi);
    /**
     * \brief Reset Phy after radio link failure function
     *
     * It resets the physical layer parameters of the
     * UE after RLF.
     *
     */
    void DoResetPhyAfterRlf();
    /**
     * \brief Reset radio link failure parameters
     *
     * Upon receiving N311 in Sync indications from the UE
     * PHY, the UE RRC instructs the UE PHY to reset the
     * RLF parameters so, it can start RLF detection again.
     *
     */
    void DoResetRlfParams();

    /**
     * \brief Start in Snyc detection function
     *
     * When T310 timer is started, it indicates that physical layer
     * problems are detected at the UE and the recovery process is
     * started by checking if the radio frames are in-sync for N311
     * consecutive times.
     *
     */
    void DoStartInSnycDetection();

    /**
     * \brief Set IMSI
     *
     * \param imsi the IMSI of the UE
     */
    void DoSetImsi(uint64_t imsi);
    void DoActivateBwp(LteNrTddSlotType type);
    void DoDeactivateBwp(LteNrTddSlotType type);


    /**
     * \brief Push proper DL CTRL/UL CTRL symbols in the current slot allocation
     * \param currentSfnSf The current sfnSf
     *
     * The symbols are inserted based on the current TDD pattern; if no pattern
     * is known (e.g., we are in the first slot, and the SIB has not reached
     * yet the UE) it is automatically inserted a DL CTRL symbol.
     */
    void PushCtrlAllocations(const SfnSf currentSfnSf);

    /**
     * \brief Inserts the received DCI for the current slot allocation
     * \param dci The DCI description of the current slot
     */
    void InsertAllocation(const std::shared_ptr<DciInfoElementTdma>& dci);

    /**
     * \brief Inserts the received DCI for a future slot allocation
     * \param dci The DCI description of the future slot
     */
    void InsertFutureAllocation(const SfnSf& sfnSf, const std::shared_ptr<DciInfoElementTdma>& dci);


    void DoInsertMsg3Allocation(const SfnSf sfnSf, const std::shared_ptr<DciInfoElementTdma>& dci);

    /**
     * \brief Select the rank indicator to be reported to gNB
     *
     * If the UE is configured to report a fixed RI value, this method
     * will return the configured fixed RI value. Otherwise, RI is
     * selected adaptively based on the average SINR of the streams.
     * <b>Note: This method is designed to handle only 2 streams</b>
     *
     * \return The rank indicator
     */
    uint8_t SelectRi(const std::vector<double>& avrgSinr);


    NrUePhySapUser* m_phySapUser;              //!< SAP pointer
    NrUecphySapProvider* m_ueCphySapProvider; //!< SAP pointer
    NrUeCphySapUser* m_ueCphySapUser;         //!< SAP pointer

    bool m_enableUplinkPowerControl{
        false};                           //!< Flag that indicates whether power control is enabled
    Ptr<NrUePowerControl> m_powerControl; //!< UE power control entity

    Ptr<const NrAmc> m_amc; //!< AMC model used to compute the CQI feedback

    Time m_wbCqiLast;
    Time m_lastSlotStart; //!< Time of the last slot start

    bool m_ulConfigured{false};     //!< Flag to indicate if RRC configured the UL
    bool m_receptionEnabled{false}; //!< Flag to indicate if we are currently receiveing data
    
    uint16_t m_rnti{0};             //!< Current RNTI of the user
    uint32_t m_currTbs{0}; //!< Current TBS of the receiveing DL data (used to compute the feedback)
    uint64_t m_imsi{0};    ///< The IMSI of the UE
    std::unordered_map<uint8_t, uint32_t>
        m_harqIdToK1Map; //!< Map that holds the K1 delay for each Harq process id

    int64_t m_numRbPerRbg{
        -1}; //!< number of resource blocks within the channel bandwidth, this parameter is
             //!< configured by MAC through phy SAP provider interface

    SfnSf m_currentSlot;

    /**
     * \brief Status of the channel for the PHY
     */
    enum ChannelStatus
    {
        NONE,      //!< The PHY doesn't know the channel status
        REQUESTED, //!< The PHY requested channel access
        GRANTED    //!< The PHY has the channel, it can transmit
    };

    ChannelStatus m_channelStatus{NONE}; //!< The channel status
    Ptr<NrChAccessManager> m_cam;        //!< Channel Access Manager
    Time m_lbtThresholdForCtrl;          //!< Threshold for LBT before the UL CTRL
    bool m_tryToPerformLbt{false};       //!< Boolean value set in DlCtrl() method
    EventId m_lbtEvent;
    uint8_t m_dlCtrlSyms{1}; //!< Number of CTRL symbols in DL
    uint8_t m_ulCtrlSyms{1}; //!< Number of CTRL symbols in UL

    double m_rsrp{0}; //!< The latest measured RSRP value

    /// Summary results of measuring a specific cell. Used for layer-1 filtering.
    struct UeMeasurementsElement
    {
        double rsrpSum;  //!< Sum of RSRP sample values in linear unit.
        uint8_t rsrpNum; //!< Number of RSRP samples.
        // For the moment rsrq is not supported so set to 0
        double rsrqSum;  //!< Sum of RSRQ sample values in linear unit.
        uint8_t rsrqNum; //!< Number of RSRQ samples.
    };

    /**
     * Store measurement results during the last layer-1 filtering period.
     * Indexed by the physical cell ID where the measurements come from.
     */
    std::map<uint16_t, UeMeasurementsElement> m_ueMeasurementsMap;
    /**
     * The `UeMeasurementsFilterPeriod` attribute. Time period for reporting UE
     * measurements, i.e., the length of layer-1 filtering (default 200 ms).
     */
    Time m_ueMeasurementsFilterPeriod;

    /**
     * The `DlDataSinr` trace source. Trace information regarding
     * average SINR (see TS 36.214). Exporting cell ID, RNTI, SINR, BWP id, and stream id.
     */
    TracedCallback<uint16_t, uint16_t, double, uint16_t, uint8_t> m_dlDataSinrTrace;
    /**
     * The `DlCtrlSinrTracedCallback` trace source. Trace information regarding
     * average SINR (see TS 36.214). Exporting cell ID, RNTI, SINR, BWP id, and stream id.
     */
    TracedCallback<uint16_t, uint16_t, double, uint16_t, uint8_t> m_dlCtrlSinrTrace;
    TracedCallback<uint64_t, uint64_t> m_reportUlTbSize; //!< Report the UL TBS
    TracedCallback<uint64_t, uint64_t> m_reportDlTbSize; //!< Report the DL TBS
    TracedCallback<const SfnSf&,
                   Ptr<const SpectrumValue>,
                   const Time&,
                   uint16_t,
                   uint64_t,
                   uint16_t,
                   uint16_t>
        m_reportPowerSpectralDensity; //!< Report the Tx power

    /**
     * Trace information regarding RSRP
     * Exporting cell ID, IMSI, RNTI, RSRP and BWP id
     */
    TracedCallback<uint16_t, uint16_t, uint16_t, double, uint8_t> m_reportRsrpTrace;

    /**
     * Trace information regarding Ue PHY Received Control Messages
     * Frame number, Subframe number, slot, VarTtti, nodeId, rnti, bwpId,
     * pointer to message in order to get the msg type
     */
    TracedCallback<SfnSf, uint16_t, uint16_t, uint8_t, Ptr<const NrControlMessage>>
        m_phyRxedCtrlMsgsTrace;

    /**
     * Trace information regarding Ue PHY Transmitted Control Messages
     * Frame number, Subframe number, slot, VarTtti, nodeId, rnti, bwpId,
     * pointer to message in order to get the msg type
     */
    TracedCallback<SfnSf, uint16_t, uint16_t, uint8_t, Ptr<const NrControlMessage>>
        m_phyTxedCtrlMsgsTrace;

    /**
     * Trace information regarding Ue PHY Rxed DL DCI Messages
     * Frame number, Subframe number, slot, VarTtti, nodeId,
     * rnti, bwpId, Harq ID, K1 delay
     */
    TracedCallback<SfnSf, uint16_t, uint16_t, uint8_t, uint8_t, uint32_t> m_phyUeRxedDlDciTrace;

    /**
     * Trace information regarding Ue PHY Txed Harq Feedback
     * Frame number, Subframe number, slot, VarTtti, nodeId,
     * rnti, bwpId, Harq ID, K1 delay
     */
    TracedCallback<SfnSf, uint16_t, uint16_t, uint8_t, uint8_t, uint32_t>
        m_phyUeTxedHarqFeedbackTrace;

    NrPhyDlHarqFeedbackCallback
        m_phyDlHarqFeedbackCallback; //!< callback that is notified when the DL HARQ feedback is
                                     //!< being generated

    std::unordered_map<uint8_t, DlHarqInfo>
        m_dlHarqInfo; //!< The attribute list used to merge HARQ infos from different NrSpectrumPhy
                      //!< instances belonging to this NrUePhy, i.e., if there are two streams,
                      //!< should be cleaned after triggering m_phyDlHarqFeedbackCallback

    uint8_t m_activeDlDataStreams; //!< The value is updated each time DlData function is called,
                                   //!< first it is reset to 0, and then it is incremented each time
                                   //!< is called AddExpectedTb
    std::unordered_map<uint8_t, uint8_t>
        m_activeDlDataStreamsPerHarqId; // active streams per HARQ process ID

    std::vector<uint8_t> m_prevDlWbCqi; //!< Vector to cache the CQI values reported by this UE PHY
    uint8_t m_dlCqiFeedbackCounter{0};  /**< Counter to count the number of DL CQI
                                             report(s) this UE PHY prepares upon
                                             receiving SINR from underlying one or
                                             multiple SpectrumPhy instances
                                             */
    uint8_t m_fixedRi{0};               //!< The rank indicator
    bool m_useFixedRi{false};           /**< If true, UE will use a fixed RI, otherwise,
                                             an adaptive one. It is set using the
                                             attribute UseFixedRi.
                                             */
    double m_riSinrThreshold1{UINT32_MAX}; /**< SINR threshold in dB that is used to
                                        adaptively choose the rank indicator value
                                        */

    double m_riSinrThreshold2{UINT32_MAX}; /**< SINR threshold in dB that is used to
                                        adaptively choose the rank indicator value
                                        */

    bool m_reportedRi2{false}; /**< Flag to keep track of an event when a UE
                                    first time reports RI equal to 2.
                                    */
  
  std::shared_ptr<DciInfoElementTdma> m_ulDci;

  bool m_activeBwpUL = false;
  bool m_activeBwpDL = false;

  

};

}

#endif /* NR_UE_PHY_H */
