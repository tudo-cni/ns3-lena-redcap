/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
// Copyright (c) 2015 NYU WIRELESS, Tandon School of Engineering, New York University
// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#define NS_LOG_APPEND_CONTEXT                                                                      \
    do                                                                                             \
    {                                                                                              \
        std::clog << " [ CellId " << GetCellId() << ", bwpId " << GetBwpId() << "] ";              \
    } while (false);

#include "nr-ue-phy.h"

#include "beam-manager.h"
#include "nr-ch-access-manager.h"
#include "nr-ue-net-device.h"
#include "nr-ue-power-control.h"

#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/lte-radio-bearer-tag.h>
#include <ns3/node.h>
#include <ns3/object-vector.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>

#include <algorithm>
#include <cfloat>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrUePhy");
NS_OBJECT_ENSURE_REGISTERED(NrUePhy);

NrUePhy::NrUePhy()
{
    NS_LOG_FUNCTION(this);
    m_wbCqiLast = Simulator::Now();
    m_ueCphySapProvider = new MemberNrUecphySapProvider<NrUePhy>(this);
    m_powerControl = CreateObject<NrUePowerControl>(this);

    Simulator::Schedule(m_ueMeasurementsFilterPeriod, &NrUePhy::ReportUeMeasurements, this);
}

NrUePhy::~NrUePhy()
{
    NS_LOG_FUNCTION(this);
}

void
NrUePhy::DoDispose()
{
    NS_LOG_FUNCTION(this);
    delete m_ueCphySapProvider;
    m_phyDlHarqFeedbackCallback = MakeNullCallback<void, const DlHarqInfo&>();
    NrPhy::DoDispose();
}

TypeId
NrUePhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrUePhy")
            .SetParent<NrPhy>()
            .AddConstructor<NrUePhy>()
            .AddAttribute("TxPower",
                          "Transmission power in dBm",
                          DoubleValue(23.0),
                          MakeDoubleAccessor(&NrUePhy::m_txPower),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "NoiseFigure",
                "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                "\"the difference in decibels (dB) between"
                " the noise output of the actual receiver to the noise output of an "
                " ideal receiver with the same overall gain and bandwidth when the receivers "
                " are connected to sources at the standard noise temperature T0.\" "
                "In this model, we consider T0 = 290K.",
                DoubleValue(5.0), // nr code from NYU and UniPd assumed in the code the value of
                                  // 5dB, thats why we configure the default value to that
                MakeDoubleAccessor(&NrPhy::SetNoiseFigure, &NrPhy::GetNoiseFigure),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "PowerAllocationType",
                "Defines the type of the power allocation. Currently are supported "
                "two types: \"UniformPowerAllocBw\", which is a uniform power allocation over all "
                "bandwidth (over all RBs), and \"UniformPowerAllocBw\", which is a uniform "
                "power allocation over used (active) RBs. By default is set a uniform power "
                "allocation over used RBs .",
                EnumValue(NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_USED),
                MakeEnumAccessor(&NrPhy::SetPowerAllocationType, &NrPhy::GetPowerAllocationType),
                MakeEnumChecker(NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_BW,
                                "UniformPowerAllocBw",
                                NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_USED,
                                "UniformPowerAllocUsed"))
            .AddAttribute("LBTThresholdForCtrl",
                          "After a DL/UL transmission, if we have less than this value to send the "
                          "UL CTRL, we consider the channel as granted",
                          TimeValue(MicroSeconds(25)),
                          MakeTimeAccessor(&NrUePhy::m_lbtThresholdForCtrl),
                          MakeTimeChecker())
            .AddAttribute("TbDecodeLatency",
                          "Transport block decode latency",
                          TimeValue(MicroSeconds(100)),
                          MakeTimeAccessor(&NrPhy::SetTbDecodeLatency, &NrPhy::GetTbDecodeLatency),
                          MakeTimeChecker())
            .AddAttribute("EnableUplinkPowerControl",
                          "If true, Uplink Power Control will be enabled.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&NrUePhy::SetEnableUplinkPowerControl),
                          MakeBooleanChecker())
            .AddAttribute("FixedRankIndicator",
                          "The rank indicator",
                          UintegerValue(1),
                          MakeUintegerAccessor(&NrUePhy::SetFixedRankIndicator,
                                               &NrUePhy::GetFixedRankIndicator),
                          MakeUintegerChecker<uint8_t>(1, 2))
            .AddAttribute("UseFixedRi",
                          "If true, UE will use a fixed configured RI value; otherwise,"
                          "it will use an adaptive RI value based on the SINR of the"
                          "streams",
                          BooleanValue(true),
                          MakeBooleanAccessor(&NrUePhy::UseFixedRankIndicator),
                          MakeBooleanChecker())
            .AddAttribute(
                "RiSinrThreshold1",
                "The SINR threshold 1 in dB. It is used to adaptively choose"
                "the rank indicator value when a UE is trying to switch from"
                "one stream to two. The UE will report RI = 2 if the average"
                "SINR of the measured stream is above this threshold; otherwise,"
                "it will report RI = 1. The initial threshold value of 10 dB"
                "is selected according to: https://ieeexplore.ieee.org/abstract/document/6364098 "
                "Figure 2",
                DoubleValue(10.0),
                MakeDoubleAccessor(&NrUePhy::SetRiSinrThreshold1, &NrUePhy::GetRiSinrThreshold1),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "RiSinrThreshold2",
                "The SINR threshold 2 in dB. It is used to adaptively choose"
                "the rank indicator value once a UE has already switched to"
                "two streams, i.e., it has already received the data on the"
                "second stream and has measured its average SINR. The UE will"
                "report RI = 2 if the average SINR of both the stream is"
                "above this threshold; otherwise, it will report RI = 1."
                "The initial threshold value of 10 dB is selected according to: "
                "https://ieeexplore.ieee.org/abstract/document/6364098 Figure 2",
                DoubleValue(10.0),
                MakeDoubleAccessor(&NrUePhy::SetRiSinrThreshold2, &NrUePhy::GetRiSinrThreshold2),
                MakeDoubleChecker<double>())
            .AddTraceSource("DlDataSinr",
                            "DL DATA SINR statistics.",
                            MakeTraceSourceAccessor(&NrUePhy::m_dlDataSinrTrace),
                            "ns3::NrUePhy::DlDataSinrTracedCallback")
            .AddTraceSource("DlCtrlSinr",
                            "Report the SINR computed for DL CTRL",
                            MakeTraceSourceAccessor(&NrUePhy::m_dlCtrlSinrTrace),
                            "ns3::NrUePhy::DlCtrlSinrTracedCallback")
            .AddAttribute("UeMeasurementsFilterPeriod",
                          "Time period for reporting UE measurements, i.e., the"
                          "length of layer-1 filtering.",
                          TimeValue(MilliSeconds(200)),
                          MakeTimeAccessor(&NrUePhy::m_ueMeasurementsFilterPeriod),
                          MakeTimeChecker())
            .AddAttribute("NrSpectrumPhyList",
                          "List of all SpectrumPhy instances of this NrUePhy.",
                          ObjectVectorValue(),
                          MakeObjectVectorAccessor(&NrUePhy::m_spectrumPhys),
                          MakeObjectVectorChecker<NrSpectrumPhy>())
            .AddTraceSource("ReportUplinkTbSize",
                            "Report allocated uplink TB size for trace.",
                            MakeTraceSourceAccessor(&NrUePhy::m_reportUlTbSize),
                            "ns3::UlTbSize::TracedCallback")
            .AddTraceSource("ReportDownlinkTbSize",
                            "Report allocated downlink TB size for trace.",
                            MakeTraceSourceAccessor(&NrUePhy::m_reportDlTbSize),
                            "ns3::DlTbSize::TracedCallback")
            .AddTraceSource("ReportRsrp",
                            "RSRP statistics.",
                            MakeTraceSourceAccessor(&NrUePhy::m_reportRsrpTrace),
                            "ns3::CurrentRsrp::TracedCallback")
            .AddTraceSource("UePhyRxedCtrlMsgsTrace",
                            "Ue PHY Control Messages Traces.",
                            MakeTraceSourceAccessor(&NrUePhy::m_phyRxedCtrlMsgsTrace),
                            "ns3::NrPhyRxTrace::RxedUePhyCtrlMsgsTracedCallback")
            .AddTraceSource("UePhyTxedCtrlMsgsTrace",
                            "Ue PHY Control Messages Traces.",
                            MakeTraceSourceAccessor(&NrUePhy::m_phyTxedCtrlMsgsTrace),
                            "ns3::NrPhyRxTrace::TxedUePhyCtrlMsgsTracedCallback")
            .AddTraceSource("UePhyRxedDlDciTrace",
                            "Ue PHY DL DCI Traces.",
                            MakeTraceSourceAccessor(&NrUePhy::m_phyUeRxedDlDciTrace),
                            "ns3::NrPhyRxTrace::RxedUePhyDlDciTracedCallback")
            .AddTraceSource("UePhyTxedHarqFeedbackTrace",
                            "Ue PHY DL HARQ Feedback Traces.",
                            MakeTraceSourceAccessor(&NrUePhy::m_phyUeTxedHarqFeedbackTrace),
                            "ns3::NrPhyRxTrace::TxedUePhyHarqFeedbackTracedCallback")
            .AddTraceSource("ReportPowerSpectralDensity",
                            "Power Spectral Density data.",
                            MakeTraceSourceAccessor(&NrUePhy::m_reportPowerSpectralDensity),
                            "ns3::NrUePhy::PowerSpectralDensityTracedCallback")
            .AddAttribute ("ActiveBwp",
                            "Only an active Bwp can be used for transmission/Reception",
                            BooleanValue (false),
                            MakeBooleanAccessor (&NrUePhy::SetActiveBwpStatus,
                                                    &NrUePhy::GetActiveBwpStatus),
                            MakeBooleanChecker ());
    return tid;
}

void
NrUePhy::ChannelAccessGranted([[maybe_unused]] const Time& time)
{
    NS_LOG_FUNCTION(this);
    // That will be granted only till the end of the slot
    m_channelStatus = GRANTED;
}

void
NrUePhy::ChannelAccessDenied()
{
    NS_LOG_FUNCTION(this);
    m_channelStatus = NONE;
}

void
NrUePhy::SetUeCphySapUser(NrUeCphySapUser* s)
{
    NS_LOG_FUNCTION(this);
    m_ueCphySapUser = s;
}

void
NrUePhy::DoInsertMsg3Allocation(const SfnSf sfnSf,const std::shared_ptr<DciInfoElementTdma>& dci )
{
    NS_LOG_FUNCTION(this);

    VarTtiAllocInfo varTtiInfo(dci);
    varTtiInfo.m_isMsg3 = true;
    if (SlotAllocInfoExists(sfnSf))
    {
        auto& ulSlot = PeekSlotAllocInfo(sfnSf);
        ulSlot.m_varTtiAllocInfo.push_back(varTtiInfo);
        std::sort(ulSlot.m_varTtiAllocInfo.begin(), ulSlot.m_varTtiAllocInfo.end());
    }
    else
    {
        SlotAllocInfo slotAllocInfo = SlotAllocInfo(sfnSf);
        slotAllocInfo.m_varTtiAllocInfo.push_back(varTtiInfo);
        PushBackSlotAllocInfo(slotAllocInfo);

      NS_LOG_INFO ("Slot InsertMsg3Allocation:" << sfnSf );
    }
}

NrUecphySapProvider*
NrUePhy::GetUeCphySapProvider()
{
    NS_LOG_FUNCTION(this);
    return (m_ueCphySapProvider);
}

void
NrUePhy::SetEnableUplinkPowerControl(bool enable)
{
    m_enableUplinkPowerControl = enable;
}

void
NrUePhy::SetTxPower(double pow)
{
    m_txPower = pow;
    m_powerControl->SetTxPower(pow);
}

double
NrUePhy::GetTxPower() const
{
    return m_txPower;
}

double
NrUePhy::GetRsrp() const
{
    return m_rsrp;
}

Ptr<NrUePowerControl>
NrUePhy::GetUplinkPowerControl() const
{
    NS_LOG_FUNCTION(this);
    return m_powerControl;
}

void
NrUePhy::SetUplinkPowerControl(Ptr<NrUePowerControl> pc)
{
    m_powerControl = pc;
}

void
NrUePhy::SetDlAmc(const Ptr<const NrAmc>& amc)
{
    m_amc = amc;
}

void
NrUePhy::SetSubChannelsForTransmission(const std::vector<int>& mask,
                                       uint32_t numSym,
                                       uint8_t activeStreams)
{
    // in uplink we currently support maximum 1 stream for DATA and CTRL, only SRS will be sent
    // using more than 1 stream
    Ptr<SpectrumValue> txPsd = GetTxPowerSpectralDensity(mask, activeStreams);
    NS_ASSERT(txPsd);

    m_reportPowerSpectralDensity(m_currentSlot,
                                 txPsd,
                                 numSym * GetSymbolPeriod(),
                                 m_rnti,
                                 m_imsi,
                                 GetBwpId(),
                                 GetCellId());
    for (std::size_t streamIndex = 0; streamIndex < m_spectrumPhys.size(); streamIndex++)
    {
        m_spectrumPhys.at(streamIndex)->SetTxPowerSpectralDensity(txPsd);
    }
}

void
NrUePhy::DoSendControlMessage(Ptr<NrControlMessage> msg)
{
    NS_LOG_FUNCTION(this << msg);
    EnqueueCtrlMessage(msg);
}

void
NrUePhy::DoSendControlMessageNow(Ptr<NrControlMessage> msg)
{
    NS_LOG_FUNCTION(this << msg);
    EnqueueCtrlMsgNow(msg);
}

void
NrUePhy::ProcessDataDci(const SfnSf& ulSfnSf,
                        const std::shared_ptr<DciInfoElementTdma>& dciInfoElem)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("UE" << m_rnti << " UL-DCI received for slot " << ulSfnSf << " symStart "
                      << static_cast<uint32_t>(dciInfoElem->m_symStart) << " numSym "
                      << static_cast<uint32_t>(dciInfoElem->m_numSym) << " tbs "
                      << dciInfoElem->m_tbSize.at(0) << " harqId "
                      << static_cast<uint32_t>(dciInfoElem->m_harqProcess));

    if (ulSfnSf == m_currentSlot)
    {
        InsertAllocation(dciInfoElem);
    }
    else
    {
        InsertFutureAllocation(ulSfnSf, dciInfoElem);
    }
}

void
NrUePhy::ProcessSrsDci([[maybe_unused]] const SfnSf& ulSfnSf,
                       [[maybe_unused]] const std::shared_ptr<DciInfoElementTdma>& dciInfoElem)
{
    NS_LOG_FUNCTION(this);
    // Instruct PHY for transmitting the SRS
    if (ulSfnSf == m_currentSlot)
    {
        InsertAllocation(dciInfoElem);
    }
    else
    {
        InsertFutureAllocation(ulSfnSf, dciInfoElem);
    }
}

void
NrUePhy::RegisterToEnb(uint16_t bwpId)
{
    NS_LOG_FUNCTION(this);

    InitializeMessageList();
    DoSetCellId(bwpId);
}

void
NrUePhy::SetUlCtrlSyms(uint8_t ulCtrlSyms)
{
    m_ulCtrlSyms = ulCtrlSyms;
}

void
NrUePhy::SetDlCtrlSyms(uint8_t dlCtrlSyms)
{
    m_dlCtrlSyms = dlCtrlSyms;
}

void
NrUePhy::SetNumRbPerRbg(uint32_t numRbPerRbg)
{
    m_numRbPerRbg = numRbPerRbg;
}

void
NrUePhy::SetPattern(const std::string& pattern)
{
    NS_LOG_FUNCTION(this);

    static std::unordered_map<std::string, LteNrTddSlotType> lookupTable = {
        {"DL", LteNrTddSlotType::DL},
        {"UL", LteNrTddSlotType::UL},
        {"S", LteNrTddSlotType::S},
        {"F", LteNrTddSlotType::F},
    };

    std::vector<LteNrTddSlotType> vector;
    std::stringstream ss(pattern);
    std::string token;
    std::vector<std::string> extracted;

    while (std::getline(ss, token, '|'))
    {
        extracted.push_back(token);
    }

    vector.reserve(extracted.size());
    for (const auto& v : extracted)
    {
        vector.push_back(lookupTable[v]);
    }

    m_tddPattern = vector;
}

uint32_t
NrUePhy::GetNumRbPerRbg() const
{
    return m_numRbPerRbg;
}

double
NrUePhy::ComputeAvgSinr(const SpectrumValue& sinr)
{
    // averaged SINR among RBs
    double sum = 0.0;
    uint8_t rbNum = 0;
    Values::const_iterator it;

    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        sum += (*it);
        rbNum++;
    }

    double avrgSinr = (rbNum > 0) ? (sum / rbNum) : DBL_MAX;

    return avrgSinr;
}

void
NrUePhy::InsertAllocation(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    VarTtiAllocInfo varTtiInfo(dci);
    m_currSlotAllocInfo.m_varTtiAllocInfo.push_back(varTtiInfo);
    std::sort(m_currSlotAllocInfo.m_varTtiAllocInfo.begin(),
              m_currSlotAllocInfo.m_varTtiAllocInfo.end());
}

void
NrUePhy::InsertFutureAllocation(const SfnSf& sfnSf, const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    VarTtiAllocInfo varTtiInfo(dci);
    if (SlotAllocInfoExists(sfnSf))
    {
        auto& ulSlot = PeekSlotAllocInfo(sfnSf);
        ulSlot.m_varTtiAllocInfo.push_back(varTtiInfo);
        std::sort(ulSlot.m_varTtiAllocInfo.begin(), ulSlot.m_varTtiAllocInfo.end());
    }
    else
    {
        SlotAllocInfo slotAllocInfo = SlotAllocInfo(sfnSf);
        slotAllocInfo.m_varTtiAllocInfo.push_back(varTtiInfo);
        PushBackSlotAllocInfo(slotAllocInfo);

      NS_LOG_INFO ("Slot InsertFutureAllocation:" << sfnSf );
    }
}

void
NrUePhy::PhyCtrlMessagesReceived(const Ptr<NrControlMessage>& msg)
{
    NS_LOG_FUNCTION(this);
    if(!m_activeBwpDL&&!m_activeBwpUL)
    {
        return;
    }
    if (msg->GetMessageType() == NrControlMessage::DL_DCI)
    {
        auto dciMsg = DynamicCast<NrDlDciMessage>(msg);
        auto dciInfoElem = dciMsg->GetDciInfoElement();
        m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), msg);

        if (dciInfoElem->m_rnti != 0 && dciInfoElem->m_rnti != m_rnti)
        {
            std::cout<< "not for me "<<  m_currentSlot<<std::endl;
            return; // DCI not for me
        }
        SfnSf dciSfn = m_currentSlot;
        uint32_t k0Delay = dciMsg->GetKDelay();
        dciSfn.Add(k0Delay);
        
        for (std::size_t stream = 0; stream < dciInfoElem->m_tbSize.size(); stream++)
        {
            NS_LOG_DEBUG("UE" << m_rnti << " stream " << stream << " DL-DCI received for slot "
                              << dciSfn << " symStart "
                              << static_cast<uint32_t>(dciInfoElem->m_symStart) << " numSym "
                              << static_cast<uint32_t>(dciInfoElem->m_numSym) << " tbs "
                              << dciInfoElem->m_tbSize.at(stream) << " harqId "
                              << static_cast<uint32_t>(dciInfoElem->m_harqProcess));
        }

        /* BIG ASSUMPTION: We assume that K0 is always 0 */

        auto it = m_harqIdToK1Map.find(dciInfoElem->m_harqProcess);
        if (it != m_harqIdToK1Map.end())
        {
            m_harqIdToK1Map.erase(m_harqIdToK1Map.find(dciInfoElem->m_harqProcess));
        }

        m_harqIdToK1Map.insert(std::make_pair(dciInfoElem->m_harqProcess, dciMsg->GetK1Delay()));

        m_phyUeRxedDlDciTrace(m_currentSlot,
                              GetCellId(),
                              m_rnti,
                              GetBwpId(),
                              dciInfoElem->m_harqProcess,
                              dciMsg->GetK1Delay());

        InsertAllocation(dciInfoElem);

        m_phySapUser->ReceiveControlMessage(msg);

        if (m_enableUplinkPowerControl)
        {
            m_powerControl->ReportTpcPusch(dciInfoElem->m_tpc);
            m_powerControl->ReportTpcPucch(dciInfoElem->m_tpc);
        }
    }
    else if (msg->GetMessageType() == NrControlMessage::UL_DCI)
    {
        auto dciMsg = DynamicCast<NrUlDciMessage>(msg);
        auto dciInfoElem = dciMsg->GetDciInfoElement();

        m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), msg);

        if (dciInfoElem->m_rnti != 0 && dciInfoElem->m_rnti != m_rnti)
        {
            return; // DCI not for me
        }

        SfnSf ulSfnSf = m_currentSlot;
        uint32_t k2Delay = dciMsg->GetKDelay();
        ulSfnSf.Add(k2Delay);

        if (dciInfoElem->m_type == DciInfoElementTdma::DATA)
        {
            ProcessDataDci(ulSfnSf, dciInfoElem);
            m_phySapUser->ReceiveControlMessage(msg);
        }
        else if (dciInfoElem->m_type == DciInfoElementTdma::SRS)
        {
            ProcessSrsDci(ulSfnSf, dciInfoElem);
            // Do not pass the DCI to MAC
        }
    }
    else if (msg->GetMessageType() == NrControlMessage::MIB)
    {
        NS_LOG_INFO("received MIB");
        Ptr<NrMibMessage> msg2 = DynamicCast<NrMibMessage>(msg);
        m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), msg);
        m_ueCphySapUser->RecvMasterInformationBlock(GetCellId(), msg2->GetMib());
    }
    else if (msg->GetMessageType() == NrControlMessage::SIB1)
    {
        Ptr<NrSib1Message> msg2 = DynamicCast<NrSib1Message>(msg);
        m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), msg);
        m_ueCphySapUser->RecvSystemInformationBlockType1(GetCellId(), msg2->GetSib1());
        NrPhySapProvider::PrachConfig prachOccasion = GetPrachOccasionsFromIndex(msg2->GetSib1().servingCellConfigCommon.uplinkConfigCommon.rach_ConfigCommon.rachConfigGeneric.prachConfigurationIndex);
        m_phySapUser->RecvSib1(*msg2,prachOccasion);
    }
    else if (msg->GetMessageType() == NrControlMessage::RAR)
    {
        Ptr<NrRarMessage> rarMsg = DynamicCast<NrRarMessage>(msg);

        Simulator::Schedule((GetSlotPeriod() * (GetL1L2CtrlLatency() / 2)),
                            &NrUePhy::DoReceiveRar,
                            this,
                            rarMsg);
    }

    else if (msg->GetMessageType() == NrControlMessage::PAGING)
    {
        Ptr<NrPagingMessage> pagMsg = DynamicCast<NrPagingMessage>(msg);
        Simulator::Schedule((GetSlotPeriod() * (GetL1L2CtrlLatency() / 2)),
                            &NrUePhy::DoReceivePaging,
                            this,
                            pagMsg);
    }
   
    else
    {
        NS_LOG_INFO("Message type not recognized " << msg->GetMessageType());
        m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), msg);
        m_phySapUser->ReceiveControlMessage(msg);
    }
}

void
NrUePhy::TryToPerformLbt()
{
    NS_LOG_FUNCTION(this);
    uint8_t ulCtrlSymStart = 0;
    uint8_t ulCtrlNumSym = 0;

    for (const auto& alloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
    {
        if (alloc.m_dci->m_type == DciInfoElementTdma::CTRL &&
            alloc.m_dci->m_format == DciInfoElementTdma::UL)
        {
            ulCtrlSymStart = alloc.m_dci->m_symStart;
            ulCtrlNumSym = alloc.m_dci->m_numSym;
            break;
        }
    }

    if (ulCtrlNumSym != 0)
    {
        // We have an UL CTRL symbol scheduled and we have to transmit CTRLs..
        // .. so we check that we have at least 25 us between the latest DCI,
        // or we have to schedule an LBT event.

        Time limit = m_lastSlotStart + GetSlotPeriod() -
                     ((GetSymbolsPerSlot() - ulCtrlSymStart) * GetSymbolPeriod()) -
                     m_lbtThresholdForCtrl;

        for (const auto& alloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
        {
            int64_t symbolPeriod = GetSymbolPeriod().GetMicroSeconds();
            int64_t dciEndsAt = m_lastSlotStart.GetMicroSeconds() +
                                ((alloc.m_dci->m_numSym + alloc.m_dci->m_symStart) * symbolPeriod);

            if (alloc.m_dci->m_type != DciInfoElementTdma::DATA)
            {
                continue;
            }

            if (limit.GetMicroSeconds() < dciEndsAt)
            {
                NS_LOG_INFO("This data DCI ends at "
                            << MicroSeconds(dciEndsAt)
                            << " which is inside the LBT shared COT (the limit is " << limit
                            << "). No need for LBT");
                m_lbtEvent.Cancel(); // Forget any LBT we previously set, because of the new
                                     // DCI information
                m_channelStatus = GRANTED;
            }
            else
            {
                NS_LOG_INFO("This data DCI starts at "
                            << +alloc.m_dci->m_symStart << " for " << +alloc.m_dci->m_numSym
                            << " ends at " << MicroSeconds(dciEndsAt)
                            << " which is outside the LBT shared COT (the limit is " << limit
                            << ").");
            }
        }
        if (m_channelStatus != GRANTED)
        {
            Time sched = m_lastSlotStart - Simulator::Now() + (GetSymbolPeriod() * ulCtrlSymStart) -
                         MicroSeconds(25);
            NS_LOG_INFO("Scheduling an LBT for sending the UL CTRL at "
                        << Simulator::Now() + sched);
            m_lbtEvent.Cancel();
            m_lbtEvent = Simulator::Schedule(sched, &NrUePhy::RequestAccess, this);
        }
        else
        {
            NS_LOG_INFO("Not scheduling LBT: the UE has a channel status that is GRANTED");
        }
    }
    else
    {
        NS_LOG_INFO("Not scheduling LBT; the UE has no UL CTRL symbols available");
    }
}

void
NrUePhy::RequestAccess()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Request access at " << Simulator::Now() << " because we have to transmit UL CTRL");
    m_cam->RequestAccess(); // This will put the m_channelStatus to granted when
                            // the channel will be granted.
}

void
NrUePhy::DoReceiveRar(Ptr<NrRarMessage> rarMsg)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Received RAR in slot " << m_currentSlot);
    m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), rarMsg);

    for (auto it = rarMsg->RarListBegin(); it != rarMsg->RarListEnd(); ++it)
    {
        if (it->rapId == m_raPreambleId)
        {
            m_phySapUser->ReceiveControlMessage(rarMsg);
        }
    }
}

void
NrUePhy::DoReceivePaging(Ptr<NrPagingMessage> pagMsg)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Received Paging in slot " << m_currentSlot);
    m_phyRxedCtrlMsgsTrace(m_currentSlot, GetCellId(), m_rnti, GetBwpId(), pagMsg);


    m_phySapUser->ReceiveControlMessage(pagMsg);

}

void
NrUePhy::PushCtrlAllocations(const SfnSf currentSfnSf)
{
    NS_LOG_FUNCTION(this);

    // The UE does not know anything from the GNB yet, so listen on the default
    // bandwidth.
    std::vector<uint8_t> rbgBitmask(GetRbNum(), 1);

    // The UE still doesn't know the TDD pattern, so just add a DL CTRL
    if (m_tddPattern.size() == 0)
    {
        NS_LOG_INFO("TDD Pattern unknown, insert DL CTRL at the beginning of the slot");
        VarTtiAllocInfo dlCtrlSlot(std::make_shared<DciInfoElementTdma>(0,
                                                                        m_dlCtrlSyms,
                                                                        DciInfoElementTdma::DL,
                                                                        DciInfoElementTdma::CTRL,
                                                                        rbgBitmask));
        m_currSlotAllocInfo.m_varTtiAllocInfo.push_front(dlCtrlSlot);
        return;
    }

    uint64_t currentSlotN = currentSfnSf.Normalize() % m_tddPattern.size();

    if (m_tddPattern[currentSlotN] < LteNrTddSlotType::UL)
    {
        NS_LOG_INFO("The current TDD pattern indicates that we are in a "
                    << m_tddPattern[currentSlotN]
                    << " slot, so insert DL CTRL at the beginning of the slot");
        VarTtiAllocInfo dlCtrlSlot(std::make_shared<DciInfoElementTdma>(0,
                                                                        m_dlCtrlSyms,
                                                                        DciInfoElementTdma::DL,
                                                                        DciInfoElementTdma::CTRL,
                                                                        rbgBitmask));
        m_currSlotAllocInfo.m_varTtiAllocInfo.push_front(dlCtrlSlot);
    }
    if (m_tddPattern[currentSlotN] > LteNrTddSlotType::DL)
    {
        NS_LOG_INFO("The current TDD pattern indicates that we are in a "
                    << m_tddPattern[currentSlotN]
                    << " slot, so insert UL CTRL at the end of the slot");
        VarTtiAllocInfo ulCtrlSlot(
            std::make_shared<DciInfoElementTdma>(GetSymbolsPerSlot() - m_ulCtrlSyms,
                                                 m_ulCtrlSyms,
                                                 DciInfoElementTdma::UL,
                                                 DciInfoElementTdma::CTRL,
                                                 rbgBitmask));
        m_currSlotAllocInfo.m_varTtiAllocInfo.push_back(ulCtrlSlot);
    }
}

void
NrUePhy::StartSlot(const SfnSf& s)
{
    NS_LOG_FUNCTION(this);
    m_currentSlot = s;
    m_lastSlotStart = Simulator::Now();

    // Call MAC before doing anything in PHY
    m_phySapUser->SlotIndication(m_currentSlot); // trigger mac
    
    // update the current slot object, and insert DL/UL CTRL allocations depending on the TDD
    // pattern
    //if (SlotAllocInfoExists(m_currentSlot) &&  (m_activeBwpDL || m_activeBwpUL ))
    if (SlotAllocInfoExists(m_currentSlot) )
    {
        m_currSlotAllocInfo = RetrieveSlotAllocInfo(m_currentSlot);
    }
    else
    {
        NS_ASSERT(!SlotAllocInfoExists(m_currentSlot));
        m_currSlotAllocInfo = SlotAllocInfo(m_currentSlot);
    }

    PushCtrlAllocations(m_currentSlot);

    NS_ASSERT(m_currSlotAllocInfo.m_sfnSf == m_currentSlot);

    NS_LOG_INFO("UE " << m_rnti << " start slot " << m_currSlotAllocInfo.m_sfnSf
                    << " composed by the following allocations, total "
                    << m_currSlotAllocInfo.m_varTtiAllocInfo.size());
    if(m_activeBwpDL||m_activeBwpUL)
    {
        for (const auto& alloc : m_currSlotAllocInfo.m_varTtiAllocInfo)
        {
            std::string direction;
            std::string type;
            if (alloc.m_dci->m_type == DciInfoElementTdma::CTRL)
            {
                type = "CTRL";
            }
            else if (alloc.m_dci->m_type == DciInfoElementTdma::SRS)
            {
                type = "SRS";
            }
            else
            {
                type = "DATA";
            }

            if (alloc.m_dci->m_format == DciInfoElementTdma::UL)
            {
                direction = "UL";
            }
            else
            {
                direction = "DL";
            }
            NS_LOG_INFO("Allocation from sym "
                        << static_cast<uint32_t>(alloc.m_dci->m_symStart) << " to sym "
                        << static_cast<uint32_t>(alloc.m_dci->m_numSym + alloc.m_dci->m_symStart)
                        << " direction " << direction << " type " << type);
        }

        TryToPerformLbt();
    
        VarTtiAllocInfo allocation = m_currSlotAllocInfo.m_varTtiAllocInfo.front();
        m_currSlotAllocInfo.m_varTtiAllocInfo.pop_front();

        auto ctrlMsgs = PopCurrentSlotCtrlMsgs();
        if (m_netDevice)
        {
            DynamicCast<NrUeNetDevice>(m_netDevice)->RouteOutgoingCtrlMsgs(ctrlMsgs, GetBwpId());
        }
        else
        {
            // No netDevice (that could happen in tests) so just redirect them to us
            for (const auto& msg : ctrlMsgs)
            {
                EncodeCtrlMsg(msg);
            }
        }

        auto nextVarTtiStart = GetSymbolPeriod() * allocation.m_dci->m_symStart;
        Simulator::Schedule(nextVarTtiStart, &NrUePhy::StartVarTti, this, allocation.m_dci, allocation.m_isMsg3);

        
    }
    else
    {
        //Bwp not active. Schedule start of next Slot
        m_currentSlot.Add(1);
        Simulator::Schedule(GetSlotPeriod(),
                        &NrUePhy::StartSlot,
                        this,
                        m_currentSlot);
    }
}

Time
NrUePhy::DlCtrl(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;

    NS_LOG_DEBUG("UE" << m_rnti
                      << " RXing DL CTRL frame for"
                         " symbols "
                      << +dci->m_symStart << "-" << +(dci->m_symStart + dci->m_numSym - 1)
                      << "\t start " << Simulator::Now() << " end "
                      << (Simulator::Now() + varTtiDuration));

    m_tryToPerformLbt = true;

    return varTtiDuration;
}

Time
NrUePhy::UlSrs(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    std::vector<int> channelRbs;
    for (uint32_t i = 0; i < GetRbNum(); i++)
    {
        channelRbs.push_back(static_cast<int>(i));
    }
    // SRS is currently the only tranmsision in the uplink that is sent over all streams
    SetSubChannelsForTransmission(channelRbs, dci->m_numSym, m_spectrumPhys.size());

    std::list<Ptr<NrControlMessage>> srsMsg;
    Ptr<NrSrsMessage> srs = Create<NrSrsMessage>();
    srs->SetSourceBwp(GetBwpId());
    srsMsg.emplace_back(srs);
    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;

    // SRS will be transmitted over all streams/streams
    for (std::size_t streamIndex = 0; streamIndex < m_spectrumPhys.size(); streamIndex++)
    {
        m_phyTxedCtrlMsgsTrace(m_currentSlot,
                               GetCellId(),
                               dci->m_rnti,
                               GetBwpId(),
                               *srsMsg.begin());
        m_spectrumPhys.at(streamIndex)
            ->StartTxUlControlFrames(srsMsg, varTtiDuration - NanoSeconds(1.0));
    }

    NS_LOG_DEBUG("UE" << m_rnti << " TXing UL SRS frame for symbols " << +dci->m_symStart << "-"
                      << +(dci->m_symStart + dci->m_numSym - 1) << "\t start " << Simulator::Now()
                      << " end " << (Simulator::Now() + varTtiDuration - NanoSeconds(1.0)));

    ChannelAccessDenied(); // Reset the channel status
    return varTtiDuration;
}

Time
NrUePhy::UlCtrl(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;

    if (m_ctrlMsgs.size() == 0)
    {
        NS_LOG_INFO("UE" << m_rnti << " reserved space for UL CTRL frame for symbols "
                         << +dci->m_symStart << "-" << +(dci->m_symStart + dci->m_numSym - 1)
                         << "\t start " << Simulator::Now() << " end "
                         << (Simulator::Now() + varTtiDuration - NanoSeconds(1.0))
                         << " but no data to transmit");
        m_cam->Cancel();
        return varTtiDuration;
    }
    else if (m_channelStatus != GRANTED)
    {
        NS_LOG_INFO("UE" << m_rnti << " has to transmit CTRL but channel not granted");
        m_cam->Cancel();
        return varTtiDuration;
    }

    for (const auto& msg : m_ctrlMsgs)
    {
        m_phyTxedCtrlMsgsTrace(m_currentSlot, GetCellId(), dci->m_rnti, GetBwpId(), msg);

        if (msg->GetMessageType() == NrControlMessage::DL_HARQ)
        {
            Ptr<NrDlHarqFeedbackMessage> harqMsg = DynamicCast<NrDlHarqFeedbackMessage>(msg);
            uint8_t harqId = harqMsg->GetDlHarqFeedback().m_harqProcessId;

            auto it = m_harqIdToK1Map.find(harqId);
            if (it != m_harqIdToK1Map.end())
            {
                m_phyUeTxedHarqFeedbackTrace(m_currentSlot,
                                             GetCellId(),
                                             m_rnti,
                                             GetBwpId(),
                                             static_cast<uint32_t>(harqId),
                                             it->second);
            }
        }
    }

    std::vector<int> channelRbs;
    for (uint32_t i = 0; i < GetRbNum(); i++)
    {
        channelRbs.push_back(static_cast<int>(i));
    }

    if (m_enableUplinkPowerControl)
    {
        m_txPower = m_powerControl->GetPucchTxPower(channelRbs.size());
    }
    // Currently uplink CTRLis transmitted only over 1 stream
    SetSubChannelsForTransmission(channelRbs, dci->m_numSym, 1);

    NS_LOG_DEBUG("UE" << m_rnti << " TXing UL CTRL frame for symbols " << +dci->m_symStart << "-"
                      << +(dci->m_symStart + dci->m_numSym - 1) << "\t start " << Simulator::Now()
                      << " end " << (Simulator::Now() + varTtiDuration - NanoSeconds(1.0)));

    SendCtrlChannels(varTtiDuration - NanoSeconds(1.0));

    ChannelAccessDenied(); // Reset the channel status
    return varTtiDuration;
}

Time
NrUePhy::DlData(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);

    m_receptionEnabled = true;
    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;

    m_activeDlDataStreams = 0;
    m_activeDlDataStreamsPerHarqId.clear();
    //NS_ASSERT(dci->m_rnti == m_rnti);
    if(dci->m_rnti != m_rnti)
    {
        //rnti is no longer used. Most likely a timeout occured and the connecttion is released. The scheduler was to slow to assignt data for the ue.
        return varTtiDuration;
    }
    NS_ASSERT(m_dlHarqInfo.size() == 0);

    for (std::size_t streamIndex = 0; streamIndex < dci->m_tbSize.size(); streamIndex++)
    {
        if (dci->m_tbSize.at(streamIndex) > 0)
        {
            m_activeDlDataStreams++;
            // we need to know for each HARQ ID how many active streams there are, see function
            // NotifyDlHarqFeedback
            if (m_activeDlDataStreamsPerHarqId.find(dci->m_harqProcess) !=
                m_activeDlDataStreamsPerHarqId.end())
            {
                m_activeDlDataStreamsPerHarqId[dci->m_harqProcess] = 1;
            }
            else
            {
                m_activeDlDataStreamsPerHarqId[dci->m_harqProcess]++;
            }
            // Here we need to call the AddExpectedTb of a NrSpectrumPhy
            // responsible to receive the expected TB of the stream we
            // are iterating over
            m_spectrumPhys.at(streamIndex)
                ->AddExpectedTb(dci->m_rnti,
                                dci->m_ndi.at(streamIndex),
                                dci->m_tbSize.at(streamIndex),
                                dci->m_mcs.at(streamIndex),
                                FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask),
                                dci->m_harqProcess,
                                dci->m_rv.at(streamIndex),
                                true,
                                dci->m_symStart,
                                dci->m_numSym,
                                m_currentSlot);

            m_reportDlTbSize(m_netDevice->GetObject<NrUeNetDevice>()->GetImsi(),
                             dci->m_tbSize.at(streamIndex));
            NS_LOG_DEBUG("UE" << m_rnti << " stream " << streamIndex
                              << " RXing DL DATA frame for"
                                 " symbols "
                              << +dci->m_symStart << "-" << +(dci->m_symStart + dci->m_numSym - 1)
                              << " num of rbg assigned: "
                              << FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask).size()
                              << "\t start " << Simulator::Now() << " end "
                              << (Simulator::Now() + varTtiDuration));
        }
    }

    return varTtiDuration;
}

Time
NrUePhy::UlData(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);
    if (m_enableUplinkPowerControl)
    {
        m_txPower = m_powerControl->GetPuschTxPower(
            (FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask)).size());
    }
    // Currently uplink DATA is transmitted over only 1 stream
    SetSubChannelsForTransmission(FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask),
                                  dci->m_numSym,
                                  1);
    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;
    if(dci->m_rnti != m_rnti)
    {
        //rnti is no longer used. Most likely a timeout occured and the connecttion is released. The scheduler was to slow to assignt data for the ue.
        return varTtiDuration;
    }
    std::list<Ptr<NrControlMessage>> ctrlMsg;
    // MIMO is not supported for UL yet.
    // Therefore, there will be only
    // one stream with stream Id 0.
    uint8_t streamId = 0;
    Ptr<PacketBurst> pktBurst = GetPacketBurst(m_currentSlot, dci->m_symStart, streamId);
    if (pktBurst && pktBurst->GetNPackets() > 0)
    {
        std::list<Ptr<Packet>> pkts = pktBurst->GetPackets();
        LteRadioBearerTag bearerTag;
        if (!pkts.front()->PeekPacketTag(bearerTag))
        {
            NS_FATAL_ERROR("No radio bearer tag");
        }
    }
    else
    {
        // put an error, as something is wrong. The UE should not be scheduled
        // if there is no data for him...
        NS_FATAL_ERROR("The UE " << dci->m_rnti << " has been scheduled without data");
    }
    m_reportUlTbSize(m_netDevice->GetObject<NrUeNetDevice>()->GetImsi(), dci->m_tbSize.at(0));

    NS_LOG_DEBUG("UE" << m_rnti << " TXing UL DATA frame for"
                      << " symbols " << +dci->m_symStart << "-"
                      << +(dci->m_symStart + dci->m_numSym - 1) << "\t start " << Simulator::Now()
                      << " end " << (Simulator::Now() + varTtiDuration));

    Simulator::Schedule(NanoSeconds(1.0),
                        &NrUePhy::SendDataChannels,
                        this,
                        pktBurst,
                        ctrlMsg,
                        varTtiDuration - NanoSeconds(2.0));
    return varTtiDuration;
}

Time
NrUePhy::TransmitMsg3(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);
    if (m_enableUplinkPowerControl)
    {
        m_txPower = m_powerControl->GetPuschTxPower(
            (FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask)).size());
    }
    // Currently uplink DATA is transmitted over only 1 stream
    SetSubChannelsForTransmission(FromRBGBitmaskToRBAssignment(dci->m_rbgBitmask),
                                  dci->m_numSym,
                                  1);
    Time varTtiDuration = GetSymbolPeriod() * dci->m_numSym;
    std::list<Ptr<NrControlMessage>> ctrlMsg;
    // MIMO is not supported for UL yet.
    // Therefore, there will be only
    // one stream with stream Id 0.
    uint8_t streamId = 0;

    //Adds Packetburst from RRC
    //m_ueCphySapUser->SendRrcConnectionRequest(); 

    Ptr<PacketBurst> pktBurst = GetPacketBurst(m_currentSlot, dci->m_symStart, streamId);
    if (pktBurst && pktBurst->GetNPackets() > 0)
    {
        std::list<Ptr<Packet>> pkts = pktBurst->GetPackets();
        LteRadioBearerTag bearerTag;
        if (!pkts.front()->PeekPacketTag(bearerTag))
        {
            NS_FATAL_ERROR("No radio bearer tag");
        }
    }
    else
    {
        // put an error, as something is wrong. The UE should not be scheduled
        // if there is no data for him...
        NS_FATAL_ERROR("The UE " << dci->m_rnti << " has been scheduled without data");
    }
    m_reportUlTbSize(m_netDevice->GetObject<NrUeNetDevice>()->GetImsi(), dci->m_tbSize.at(0));

    NS_LOG_DEBUG("UE" << m_rnti << " TXing UL DATA frame for"
                      << " symbols " << +dci->m_symStart << "-"
                      << +(dci->m_symStart + dci->m_numSym - 1) << "\t start " << Simulator::Now()
                      << " end " << (Simulator::Now() + varTtiDuration));

    Simulator::Schedule(NanoSeconds(1.0),
                        &NrUePhy::SendDataChannels,
                        this,
                        pktBurst,
                        ctrlMsg,
                        varTtiDuration - NanoSeconds(2.0));
    return varTtiDuration;
}


void
NrUePhy::StartVarTti(const std::shared_ptr<DciInfoElementTdma>& dci, const bool isMsg3)
{
    NS_LOG_FUNCTION(this);
    Time varTtiDuration;

    for (const auto& it : dci->m_tbSize)
    {
        m_currTbs = it;
    }

    m_receptionEnabled = false;

    if(isMsg3)
    {
        m_netDevice->m_energyModel->DoNotifyStateChange(NrEnergyModel::PowerState::RRC_SENDING_PUSCH,m_imsi);
        varTtiDuration = TransmitMsg3(dci);
    }

    else if (dci->m_type == DciInfoElementTdma::CTRL && dci->m_format == DciInfoElementTdma::DL)
    {
        varTtiDuration = DlCtrl(dci);
    }
    else if (dci->m_type == DciInfoElementTdma::CTRL && dci->m_format == DciInfoElementTdma::UL)
    {
        varTtiDuration = UlCtrl(dci);
    }
    else if (dci->m_type == DciInfoElementTdma::SRS && dci->m_format == DciInfoElementTdma::UL)
    {
        varTtiDuration = UlSrs(dci);
    }
    else if (dci->m_type == DciInfoElementTdma::DATA && dci->m_format == DciInfoElementTdma::DL)
    {
        m_netDevice->m_energyModel->DoNotifyStateChange(NrEnergyModel::PowerState::RRC_RECEIVING_PDSCH,m_imsi);
        varTtiDuration = DlData(dci);
    }
    else if (dci->m_type == DciInfoElementTdma::DATA && dci->m_format == DciInfoElementTdma::UL)
    {
        m_netDevice->m_energyModel->DoNotifyStateChange(NrEnergyModel::PowerState::RRC_SENDING_PUSCH,m_imsi);
        varTtiDuration = UlData(dci);
    }

    Simulator::Schedule(varTtiDuration, &NrUePhy::EndVarTti, this, dci);
}

void
NrUePhy::EndVarTti(const std::shared_ptr<DciInfoElementTdma>& dci)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("DCI started at symbol "
                << static_cast<uint32_t>(dci->m_symStart) << " which lasted for "
                << static_cast<uint32_t>(dci->m_numSym) << " symbols finished");
    if (dci->m_type == DciInfoElementTdma::DATA)
    {
        m_netDevice->m_energyModel->DoNotifyStateChange(NrEnergyModel::PowerState::RRC_Connected,m_imsi);
    }
    if (m_tryToPerformLbt)
    {
        TryToPerformLbt();
        m_tryToPerformLbt = false;
    }

    if (m_currSlotAllocInfo.m_varTtiAllocInfo.size() == 0)
    {
        // end of slot
        m_currentSlot.Add(1);

        Simulator::Schedule(m_lastSlotStart + GetSlotPeriod() - Simulator::Now(),
                            &NrUePhy::StartSlot,
                            this,
                            m_currentSlot);
    }
    else
    {
        VarTtiAllocInfo allocation = m_currSlotAllocInfo.m_varTtiAllocInfo.front();
        m_currSlotAllocInfo.m_varTtiAllocInfo.pop_front();
         
        Time nextVarTtiStart = GetSymbolPeriod() * allocation.m_dci->m_symStart;
        Simulator::Schedule(nextVarTtiStart + m_lastSlotStart - Simulator::Now(),
                            &NrUePhy::StartVarTti,
                            this,
                            allocation.m_dci,allocation.m_isMsg3);
                         
    }

    m_receptionEnabled = false;
}

void
NrUePhy::PhyDataPacketReceived(const Ptr<Packet>& p)
{
    Simulator::ScheduleWithContext(m_netDevice->GetNode()->GetId(),
                                   GetTbDecodeLatency(),
                                   &NrUePhySapUser::ReceivePhyPdu,
                                   m_phySapUser,
                                   p);
    // m_phySapUser->ReceivePhyPdu (p);
}

void
NrUePhy::SendDataChannels(const Ptr<PacketBurst>& pb,
                          const std::list<Ptr<NrControlMessage>>& ctrlMsg,
                          const Time& duration)
{
    if (pb->GetNPackets() > 0)
    {
        LteRadioBearerTag tag;
        if (!pb->GetPackets().front()->PeekPacketTag(tag))
        {
            NS_FATAL_ERROR("No radio bearer tag");
        }
    }

    // Uplink data is sent only through a single stream, the first is assumed
    m_spectrumPhys.at(0)->StartTxDataFrames(pb, ctrlMsg, duration);
}

void
NrUePhy::SendCtrlChannels(Time duration)
{
    // Uplink CTRL is sent only through a single stream, the first is assumed
    m_spectrumPhys.at(0)->StartTxUlControlFrames(m_ctrlMsgs, duration);
    m_ctrlMsgs.clear();
}

Ptr<NrDlCqiMessage>
NrUePhy::CreateDlCqiFeedbackMessage(const DlCqiInfo& dlcqi)
{
    NS_LOG_FUNCTION(this);
    // Create DL CQI CTRL message
    Ptr<NrDlCqiMessage> msg = Create<NrDlCqiMessage>();
    msg->SetSourceBwp(GetBwpId());
    msg->SetDlCqi(dlcqi);
    return msg;
}

void
NrUePhy::GenerateDlCqiReport(const SpectrumValue& sinr, uint8_t streamId)
{
    NS_LOG_FUNCTION(this);
    // Not totally sure what this is about. We have to check.
    if (m_ulConfigured && (m_rnti > 0) && m_receptionEnabled)
    {
        m_dlDataSinrTrace(GetCellId(), m_rnti, ComputeAvgSinr(sinr), GetBwpId(), streamId);

        // TODO
        // Not sure what this IF is about, seems that it can be removed,
        // if not, then we have to support wbCqiLast time per stream
        // if (Simulator::Now () > m_wbCqiLast)
        if (m_prevDlWbCqi.empty()) // No DL CQI reported yet, initialize the vector
        {
            // Remember, scheduler uses MCS 0 for CQI 0.
            // See, NrMacSchedulerCQIManagement::DlWBCQIReported
            m_prevDlWbCqi = std::vector<uint8_t>(m_spectrumPhys.size(), 0);
            m_reportedRi2 =
                false; // already initialized to false in the header, added here for readability
        }

        uint8_t mcs; // it is initialized by AMC in the following call
        uint8_t wbCqi = m_amc->CreateCqiFeedbackWbTdma(sinr, mcs);

        std::vector<double> avrgSinr = std::vector<double>(m_spectrumPhys.size(), UINT32_MAX);

        NS_ASSERT(streamId < m_prevDlWbCqi.size());
        m_prevDlWbCqi[streamId] = wbCqi;
        double avrgSinrdB = 10 * log10(ComputeAvgSinr(sinr));
        avrgSinr[streamId] = avrgSinrdB;
        NS_LOG_DEBUG("Stream " << +streamId << " WB CQI " << +wbCqi << " avrg MCS " << +mcs
                               << " avrg SINR (dB) " << avrgSinrdB);
        m_dlCqiFeedbackCounter++;

        // if we received SINR from all the active streams,
        // we can proceed to trigger the corresponding callback
        if (m_dlCqiFeedbackCounter == m_activeDlDataStreams)
        {
            DlCqiInfo dlcqi;
            dlcqi.m_rnti = m_rnti;
            dlcqi.m_cqiType = DlCqiInfo::WB;
            if (m_spectrumPhys.size() == 1)
            {
                dlcqi.m_ri = 1;
            }
            else
            {
                dlcqi.m_ri = SelectRi(avrgSinr);
                NS_LOG_DEBUG("At " << Simulator::Now().As(Time::S) << " UE PHY reporting RI = "
                                   << static_cast<uint16_t>(dlcqi.m_ri));
            }

            // In MIMO, once the UE starts reporting RI = 2, both the CQI
            // must be reported even though one is measured, the other for
            // which we couldn't measure we will report a previously
            // computed CQI or if not computed at all then CQI 0. This choice is
            // made to keep the scheduler informed about the channel state in MIMO
            // when only one of the stream's TB is retransmitted. Also, remember,
            // if UE reports RI = 2 and one of the stream's CQI is 0, scheduler will
            // use MCS 0 to compute its TB size.
            dlcqi.m_wbCqi = m_prevDlWbCqi; // set DL CQI feedbacks

            NS_ASSERT_MSG(dlcqi.m_ri <= dlcqi.m_wbCqi.size(),
                          "Mismatch between the RI and the number of CQIs in a CQI report");

            Ptr<NrDlCqiMessage> msg = CreateDlCqiFeedbackMessage(dlcqi);
            if (msg)
            {
                DoSendControlMessage(msg);
            }
            // reset the key variables
            m_dlCqiFeedbackCounter = 0;
        }
    }
}

void
NrUePhy::EnqueueDlHarqFeedback(const DlHarqInfo& m)
{
    NS_LOG_FUNCTION(this);
    // get the feedback from NrSpectrumPhy and send it through ideal PUCCH to gNB
    Ptr<NrDlHarqFeedbackMessage> msg = Create<NrDlHarqFeedbackMessage>();
    msg->SetSourceBwp(GetBwpId());
    msg->SetDlHarqFeedback(m);

    auto k1It = m_harqIdToK1Map.find(m.m_harqProcessId);

    NS_LOG_DEBUG("ReceiveLteDlHarqFeedback"
                 << " Harq Process " << static_cast<uint32_t>(k1It->first)
                 << " K1: " << k1It->second << " Frame " << m_currentSlot);

    Time event = m_lastSlotStart + (GetSlotPeriod() * k1It->second);
    if (event <= Simulator::Now())
    {
        Simulator::ScheduleNow(&NrUePhy::DoSendControlMessageNow, this, msg);
    }
    else
    {
        Simulator::Schedule(event - Simulator::Now(), &NrUePhy::DoSendControlMessageNow, this, msg);
    }
}

void
NrUePhy::SetPhyDlHarqFeedbackCallback(const NrPhyDlHarqFeedbackCallback& c)
{
    NS_LOG_FUNCTION(this);
    m_phyDlHarqFeedbackCallback = c;
}

void
NrUePhy::NotifyDlHarqFeedback(uint8_t streamId,
                              DlHarqInfo::HarqStatus harqFeedback,
                              uint8_t harqProcessId,
                              uint8_t rv)
{
    // if we still did not report for this process ID for any stream
    if (m_dlHarqInfo.find(harqProcessId) == m_dlHarqInfo.end())
    {
        DlHarqInfo dlHarqInfo;
        dlHarqInfo.m_rnti = m_rnti;
        dlHarqInfo.m_bwpIndex = GetBwpId();
        // initialize the feedbacks from all streams with NONE
        dlHarqInfo.m_harqStatus =
            std::vector<enum DlHarqInfo::HarqStatus>(m_spectrumPhys.size(),
                                                     DlHarqInfo::HarqStatus::NONE);
        // above initialization logic also applies to m_numRetx vector
        dlHarqInfo.m_numRetx = std::vector<uint8_t>(m_spectrumPhys.size(), UINT8_MAX);
        dlHarqInfo.m_harqProcessId = harqProcessId;
        // insert this element
        m_dlHarqInfo[harqProcessId] = dlHarqInfo;
    }

    auto& harq = m_dlHarqInfo[harqProcessId];
    NS_ASSERT(harq.m_harqProcessId == harqProcessId);
    NS_ASSERT(streamId < harq.m_harqStatus.size());
    // we make sure that for this stream the HARQ feedback was not reported before
    NS_ASSERT(harq.m_harqStatus.at(streamId) == DlHarqInfo::HarqStatus::NONE);
    harq.m_harqStatus[streamId] = harqFeedback;
    harq.m_numRetx[streamId] = rv;

    uint8_t feedbackCounter = 0;
    for (const auto& i : harq.m_harqStatus)
    {
        if (i != DlHarqInfo::HarqStatus::NONE)
        {
            feedbackCounter++;
        }
    }

    NS_ASSERT(m_activeDlDataStreamsPerHarqId.find(harqProcessId) !=
              m_activeDlDataStreamsPerHarqId.end());
    // if we received the feedback from all the active streams, we
    // can proceed to trigger the corresponding callback
    if (feedbackCounter == m_activeDlDataStreamsPerHarqId[harqProcessId])
    {
        m_phyDlHarqFeedbackCallback(harq);
        m_dlHarqInfo.erase(m_dlHarqInfo.find(
            harqProcessId)); // remove this HARQ feedback from the list because it is just reported
    }
}

void
NrUePhy::SetCam(const Ptr<NrChAccessManager>& cam)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(cam != nullptr);
    m_cam = cam;
    m_cam->SetAccessGrantedCallback(
        std::bind(&NrUePhy::ChannelAccessGranted, this, std::placeholders::_1));
    m_cam->SetAccessDeniedCallback(std::bind(&NrUePhy::ChannelAccessDenied, this));
}

const SfnSf&
NrUePhy::GetCurrentSfnSf() const
{
    return m_currentSlot;
}

uint16_t
NrUePhy::GetRnti()
{
    return m_rnti;
}

void
NrUePhy::DoReset()
{
    NS_LOG_FUNCTION(this);
    DoSetRnti(0);
    m_currTbs=0;
    if(GetBwpId() == 0)
    {
        m_activeBwpDL=true;
        m_activeBwpUL =true;
    }
    else{
        m_activeBwpDL = false;
        m_activeBwpUL = false;
    }
    m_prevDlWbCqi.clear();
}

void
NrUePhy::DoStartCellSearch(uint16_t dlEarfcn)
{
    NS_LOG_FUNCTION(this << dlEarfcn);
    DoSetInitialBandwidth();
}

void
NrUePhy::DoSynchronizeWithGnb(uint16_t cellId, uint16_t dlEarfcn)
{
    NS_LOG_FUNCTION(this << cellId << dlEarfcn);
    DoSynchronizeWithGnb(cellId);
}

void
NrUePhy::DoSetPa(double pa)
{
    NS_LOG_FUNCTION(this << pa);
}

void
NrUePhy::DoSetRsrpFilterCoefficient(uint8_t rsrpFilterCoefficient)
{
    NS_LOG_FUNCTION(this << +rsrpFilterCoefficient);
}

void
NrUePhy::DoSynchronizeWithGnb(uint16_t cellId)
{
    NS_LOG_FUNCTION(this << cellId);
    DoSetCellId(cellId);
    DoSetInitialBandwidth();
}

BeamConfId
NrUePhy::GetBeamConfId([[maybe_unused]] uint16_t rnti) const
{
    NS_LOG_FUNCTION(this);
    // That's a bad specification: the UE PHY doesn't know anything about its beam conf id.
    NS_FATAL_ERROR("ERROR");
}

void
NrUePhy::ScheduleStartEventLoop(uint32_t nodeId, uint16_t frame, uint8_t subframe, uint16_t slot)
{
    NS_LOG_FUNCTION(this);
    Simulator::ScheduleWithContext(nodeId,
                                   MilliSeconds(0),
                                   &NrUePhy::StartEventLoop,
                                   this,
                                   frame,
                                   subframe,
                                   slot);
}

void
NrUePhy::ReportRsReceivedPower(const SpectrumValue& rsReceivedPower,
                               [[maybe_unused]] uint8_t streamIndex)
{
    NS_LOG_FUNCTION(this << rsReceivedPower);

    // TODO use streamIndex

    m_rsrp = 10 * log10(Integral(rsReceivedPower)) + 30;
    NS_LOG_DEBUG("RSRP value updated: " << m_rsrp << " dBm");

    if (m_enableUplinkPowerControl)
    {
        m_powerControl->SetLoggingInfo(GetCellId(), m_rnti);
        m_powerControl->SetRsrp(m_rsrp);
    }
}

void
NrUePhy::ReceivePss(uint16_t cellId, const Ptr<SpectrumValue>& p)
{
    NS_LOG_FUNCTION(this);

    double sum = 0.0;
    uint16_t nRB = 0;

    uint32_t subcarrierSpacing;
    subcarrierSpacing = 15000 * static_cast<uint32_t>(std::pow(2, GetNumerology()));

    Values::const_iterator itPi;
    for (itPi = p->ConstValuesBegin(); itPi != p->ConstValuesEnd(); itPi++)
    {
        // convert PSD [W/Hz] to linear power [W] for the single RE
        double powerTxW = (*itPi) * subcarrierSpacing;
        sum += powerTxW;
        nRB++;
    }

    // measure instantaneous RSRP now (in dBm)
    double rsrp = 10 * log10(1000 * (sum / static_cast<double>(nRB)));

    NS_LOG_DEBUG("RSRP value updated: " << rsrp << " dBm"
                                        << " for Cell Id: " << cellId << " RNTI: " << m_rnti);

    // store RSRP measurements
    std::map<uint16_t, UeMeasurementsElement>::iterator itMeasMap =
        m_ueMeasurementsMap.find(cellId);
    if (itMeasMap == m_ueMeasurementsMap.end())
    {
        // insert new entry
        UeMeasurementsElement newEl;
        newEl.rsrpSum = rsrp;
        newEl.rsrpNum = 1;
        newEl.rsrqSum = 0;
        newEl.rsrqNum = 0;

        NS_LOG_DEBUG("New RSRP entry for Cell Id: " << cellId << " RNTI: " << m_rnti
                                                    << " RSRP: " << newEl.rsrpSum << " dBm"
                                                    << " number of entries: " << +newEl.rsrpNum);

        m_ueMeasurementsMap.insert(std::pair<uint16_t, UeMeasurementsElement>(cellId, newEl));
    }
    else
    {
        (*itMeasMap).second.rsrpSum += rsrp;
        (*itMeasMap).second.rsrpNum++;

        NS_LOG_DEBUG("Update RSRP entry for Cell Id: "
                     << cellId << " RNTI: " << m_rnti
                     << " RSRP Sum: " << (*itMeasMap).second.rsrpSum << " dBm"
                     << " number of entries: " << +((*itMeasMap).second.rsrpNum));
    }
}

void
NrUePhy::ReportUeMeasurements()
{
    NS_LOG_FUNCTION(this);

    // LteUeCphySapUser::UeMeasurementsParameters ret;

    std::map<uint16_t, UeMeasurementsElement>::iterator it;
    for (it = m_ueMeasurementsMap.begin(); it != m_ueMeasurementsMap.end(); it++)
    {
        double avg_rsrp;
        // double avg_rsrq = 0;
        if ((*it).second.rsrpNum != 0)
        {
            avg_rsrp = (*it).second.rsrpSum / static_cast<double>((*it).second.rsrpNum);
        }
        else
        {
            NS_LOG_WARN(" RSRP nSamples is zero!");
            avg_rsrp = 0;
        }

        NS_LOG_DEBUG(" Report UE Measurements for CellId "
                     << (*it).first << " Reporting UE " << m_rnti << " Av. RSRP " << avg_rsrp
                     << " (nSamples " << +((*it).second.rsrpNum) << ")"
                     << " BwpID " << GetBwpId());

        m_reportRsrpTrace(GetCellId(), m_imsi, m_rnti, avg_rsrp, GetBwpId());

        /*LteUeCphySapUser::UeMeasurementsElement newEl;
        newEl.m_cellId = (*it).first;
        newEl.m_rsrp = avg_rsrp;
        newEl.m_rsrq = avg_rsrq;  //LEAVE IT 0 FOR THE MOMENT
        ret.m_ueMeasurementsList.push_back (newEl);
        ret.m_componentCarrierId = GetBwpId ();*/
    }

    // report to RRC
    // m_ueCphySapUser->ReportUeMeasurements (ret);

    m_ueMeasurementsMap.clear();
    Simulator::Schedule(m_ueMeasurementsFilterPeriod, &NrUePhy::ReportUeMeasurements, this);
}

void
NrUePhy::ReportDlCtrlSinr(const SpectrumValue& sinr, uint8_t streamId)
{
    NS_LOG_FUNCTION(this);
    uint32_t rbUsed = 0;
    double sinrSum = 0.0;

    for (uint32_t i = 0; i < sinr.GetValuesN(); i++)
    {
        double currentSinr = sinr.ValuesAt(i);
        if (currentSinr != 0)
        {
            rbUsed++;
            sinrSum += currentSinr;
        }
    }

    NS_ASSERT(rbUsed);
    m_dlCtrlSinrTrace(GetCellId(), m_rnti, sinrSum / rbUsed, GetBwpId(), streamId);
}

uint8_t
NrUePhy::ComputeCqi(const SpectrumValue& sinr)
{
    NS_LOG_FUNCTION(this);
    uint8_t mcs; // it is initialized by AMC in the following call
    uint8_t wbCqi = m_amc->CreateCqiFeedbackWbTdma(sinr, mcs);
    return wbCqi;
}

void
NrUePhy::StartEventLoop(uint16_t frame, uint8_t subframe, uint16_t slot)
{
    NS_LOG_FUNCTION(this);

    if (GetChannelBandwidth() == 0)
    {
        NS_LOG_INFO("Initial bandwidth not set, configuring the default one for Cell ID:"
                    << GetCellId() << ", RNTI" << GetRnti() << ", BWP ID:" << GetBwpId());
        DoSetInitialBandwidth();
    }

    NS_LOG_DEBUG("PHY starting. Configuration: "
                 << std::endl
                 << "\t TxPower: " << m_txPower << " dB" << std::endl
                 << "\t NoiseFigure: " << m_noiseFigure << std::endl
                 << "\t TbDecodeLatency: " << GetTbDecodeLatency().GetMicroSeconds() << " us "
                 << std::endl
                 << "\t Numerology: " << GetNumerology() << std::endl
                 << "\t SymbolsPerSlot: " << GetSymbolsPerSlot() << std::endl
                 << "\t Pattern: " << NrPhy::GetPattern(m_tddPattern) << std::endl
                 << "Attached to physical channel: " << std::endl
                 << "\t Channel bandwidth: " << GetChannelBandwidth() << " Hz" << std::endl
                 << "\t Channel central freq: " << GetCentralFrequency() << " Hz" << std::endl
                 << "\t Num. RB: " << GetRbNum());
    SfnSf startSlot(frame, subframe, slot, GetNumerology());
    StartSlot(startSlot);
}

void
NrUePhy::DoSetInitialBandwidth()
{
    NS_LOG_FUNCTION(this);
    // configure initial bandwidth to 6 RBs
    double initialBandwidthHz =
        6 * GetSubcarrierSpacing() * NrSpectrumValueHelper::SUBCARRIERS_PER_RB;
    // divided by 100*1000 because the parameter should be in 100KHz
    uint16_t initialBandwidthIn100KHz = ceil(initialBandwidthHz / (100 * 1000));
    // account for overhead that will be reduced when determining real BW
    uint16_t initialBandwidthWithOverhead = initialBandwidthIn100KHz / (1 - GetRbOverhead());

    NS_ABORT_MSG_IF(initialBandwidthWithOverhead == 0,
                    " Initial bandwidth could not be set. Parameters provided are: "
                    "\n dlBandwidthInRBNum = "
                        << 6 << "\n m_subcarrierSpacing = " << GetSubcarrierSpacing()
                        << "\n NrSpectrumValueHelper::SUBCARRIERS_PER_RB  = "
                        << (unsigned)NrSpectrumValueHelper::SUBCARRIERS_PER_RB
                        << "\n m_rbOh = " << GetRbOverhead());

    DoSetDlBandwidth(initialBandwidthWithOverhead);
}

uint16_t
NrUePhy::DoGetCellId()
{
    return GetCellId();
}

uint32_t
NrUePhy::DoGetDlEarfcn()
{
    // TBD See how to get rid of this function in future
    // Added for the compatibility with 810 MR to LTE.
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("DoGetDlEarfcn function is called. This function should be removed in future once "
                "NR has its own RRC.");
    return 0;
}

void
NrUePhy::DoSetDlBandwidth(uint16_t dlBandwidth)
{
    NS_LOG_FUNCTION(this << +dlBandwidth);

    SetChannelBandwidth(dlBandwidth);

    NS_LOG_DEBUG("PHY reconfiguring. Result: "
                 << std::endl
                 << "\t TxPower: " << m_txPower << " dB" << std::endl
                 << "\t NoiseFigure: " << m_noiseFigure << std::endl
                 << "\t TbDecodeLatency: " << GetTbDecodeLatency().GetMicroSeconds() << " us "
                 << std::endl
                 << "\t Numerology: " << GetNumerology() << std::endl
                 << "\t SymbolsPerSlot: " << GetSymbolsPerSlot() << std::endl
                 << "\t Pattern: " << NrPhy::GetPattern(m_tddPattern) << std::endl
                 << "Attached to physical channel: " << std::endl
                 << "\t Channel bandwidth: " << GetChannelBandwidth() << " Hz" << std::endl
                 << "\t Channel central freq: " << GetCentralFrequency() << " Hz" << std::endl
                 << "\t Num. RB: " << GetRbNum());
}

void
NrUePhy::DoConfigureUplink(uint16_t ulEarfcn, uint8_t ulBandwidth)
{
    NS_LOG_FUNCTION(this << ulEarfcn << +ulBandwidth);
    // Ignore this; should be equal to dlBandwidth
    m_ulConfigured = true;
}

void
NrUePhy::DoConfigureReferenceSignalPower(int8_t referenceSignalPower)
{
    NS_LOG_FUNCTION(this << referenceSignalPower);
    m_powerControl->ConfigureReferenceSignalPower(referenceSignalPower);
}

void
NrUePhy::DoSetRnti(uint16_t rnti)
{
    NS_LOG_FUNCTION(this << rnti);
    m_rnti = rnti;
}

void
NrUePhy::DoSetTransmissionMode(uint8_t txMode)
{
    NS_LOG_FUNCTION(this << +txMode);
}

void
NrUePhy::DoSetSrsConfigurationIndex(uint16_t srcCi)
{
    NS_LOG_FUNCTION(this << srcCi);
}

void
NrUePhy::SetPhySapUser(NrUePhySapUser* ptr)
{
    m_phySapUser = ptr;
}

void
NrUePhy::DoResetPhyAfterRlf()
{
    NS_LOG_FUNCTION(this);
    DoSetRnti(0);
    m_currTbs=0;
    if(GetBwpId() == 0)
    {
        m_activeBwpDL=true;
        m_activeBwpUL =true;
    }
    else{
        m_activeBwpDL = false;
        m_activeBwpUL = false;
    }
    m_prevDlWbCqi.clear();
    //NS_FATAL_ERROR("NrUePhy does not have RLF functionality yet");
}

void
NrUePhy::DoResetRlfParams()
{
    NS_LOG_FUNCTION(this);
    //NS_FATAL_ERROR("NrUePhy does not have RLF functionality yet");
}

void
NrUePhy::DoStartInSnycDetection()
{
    NS_LOG_FUNCTION(this);
    NS_FATAL_ERROR("NrUePhy does not have RLF functionality yet");
}

void
NrUePhy::DoSetImsi(uint64_t imsi)
{
    NS_LOG_FUNCTION(this);
    m_imsi = imsi;
}

void
NrUePhy::DoActivateBwp(LteNrTddSlotType type)
{
    NS_LOG_FUNCTION(this);
    if(type == LteNrTddSlotType::UL)
    {
        m_activeBwpUL = true;
    }
    else{
        m_activeBwpDL = true;
    }
}

void
NrUePhy::DoDeactivateBwp(LteNrTddSlotType type)
{
    NS_LOG_FUNCTION(this);
    if(type == LteNrTddSlotType::UL)
    {
        m_activeBwpUL = false;
    }
    else{
        m_activeBwpDL = false;
    }
}

uint64_t
NrUePhy::GetImsi()
{
    return m_imsi;
}


void
NrUePhy::SetFixedRankIndicator(uint8_t ri)
{
    NS_LOG_FUNCTION(this);
    m_fixedRi = ri;
}

uint8_t
NrUePhy::GetFixedRankIndicator() const
{
    return m_fixedRi;
}

void
NrUePhy::UseFixedRankIndicator(bool useFixedRi)
{
    NS_LOG_FUNCTION(this);
    m_useFixedRi = useFixedRi;
}

void
NrUePhy::SetRiSinrThreshold1(double sinrThreshold)
{
    NS_LOG_FUNCTION(this);
    m_riSinrThreshold1 = sinrThreshold;
}

double
NrUePhy::GetRiSinrThreshold1() const
{
    return m_riSinrThreshold1;
}

void
NrUePhy::SetRiSinrThreshold2(double sinrThreshold)
{
    NS_LOG_FUNCTION(this);
    m_riSinrThreshold2 = sinrThreshold;
}

double
NrUePhy::GetRiSinrThreshold2() const
{
    return m_riSinrThreshold2;
}

uint8_t
NrUePhy::SelectRi(const std::vector<double>& avrgSinr)
{
    NS_LOG_FUNCTION(this);
    uint8_t ri = 0;
    if (m_useFixedRi)
    {
        return m_fixedRi;
    }

    if (!m_reportedRi2)
    {
        // UE supports two stream but it has not yet reported RI equal to 2.
        // Let's check the average SINR of the first stream. If it is
        // above m_riSinrThreshold1 then we report RI equal to 2; otherwise, RI
        // equal to 1.
        if (avrgSinr[0] > m_riSinrThreshold1)
        {
            ri = 2;
            m_reportedRi2 = true;
        }
        else
        {
            ri = 1;
        }
    }
    else
    {
        std::vector<uint8_t> indexValidSinr;
        for (std::size_t i = 0; i < avrgSinr.size(); i++)
        {
            if (avrgSinr[i] != UINT32_MAX)
            {
                indexValidSinr.push_back(i);
            }
        }

        NS_ABORT_MSG_IF(indexValidSinr.size() == 0, "Unable to find valid average SINR");

        if (indexValidSinr.size() == avrgSinr.size())
        {
            // UE is able to measure both the streams
            // UE supports two stream and it has already reported RI equal to 2.
            // Meaning, that this UE has already received the data on stream 2
            // and has measured its average SINR. Let's check the average SINR
            // of both the streams. If the average SINR of both the streams is
            // above m_riSinrThreshold2 then we report RI equal to 2; otherwise, RI
            // equal to 1.
            if (avrgSinr[0] > m_riSinrThreshold2 && avrgSinr[1] > m_riSinrThreshold2)
            {
                ri = 2;
            }
            else
            {
                ri = 1;
            }
        }
        else
        {
            // There is at least one stream that UE is unable to measure.
            // If the average SINR of the measured stream is above
            // m_riSinrThreshold1, report RI equal to 2; otherwise, RI equal to 1.
            // This else was implemented to handle the situations when a UE
            // switches from 2 streams to 1, and unable to measure one of
            // the streams. In that case, following code would help us
            // not to get stuck with one stream till the end of simulation.
            if (avrgSinr[indexValidSinr.at(0)] > m_riSinrThreshold1)
            {
                ri = 2;
            }
            else
            {
                ri = 1;
            }
        }
    }

    NS_ASSERT_MSG(ri != 0, "UE is trying to report invalid RI value of 0");
    return ri;
}



bool
NrUePhy::GetActiveBwpStatus () const
{
  return m_activeBwpUL || m_activeBwpDL;
}

void
NrUePhy::SetActiveBwpStatus (bool state)
{
  m_activeBwpDL = state;
  m_activeBwpUL = state;

}

NrPhySapProvider::PrachConfig
NrUePhy::GetPrachOccasionsFromIndex(uint8_t prachIndex)
{
    NrPhySapProvider::PrachConfig prachConfig = NrPhy::GetPrachConfig(prachIndex);
    return prachConfig;
}


}


