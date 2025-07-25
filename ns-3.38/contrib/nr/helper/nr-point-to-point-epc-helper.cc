/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include <ns3/epc-ue-nas.h>
#include <ns3/epc-x2.h>
#include <ns3/log.h>
#include <ns3/nr-gnb-rrc.h>
#include <ns3/net-device-container.h>
#include <ns3/node-container.h>
#include <ns3/nr-gnb-net-device.h>
#include <ns3/nr-point-to-point-epc-helper.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/object.h>
#include <ns3/point-to-point-helper.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrPointToPointEpcHelper");

NS_OBJECT_ENSURE_REGISTERED(NrPointToPointEpcHelper);

NrPointToPointEpcHelper::NrPointToPointEpcHelper()
    : PointToPointEpcHelper()
{
    NS_LOG_FUNCTION(this);
}

NrPointToPointEpcHelper::~NrPointToPointEpcHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId
NrPointToPointEpcHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrPointToPointEpcHelper")
                            .SetParent<PointToPointEpcHelper>()
                            .SetGroupName("nr")
                            .AddConstructor<NrPointToPointEpcHelper>();
    return tid;
}

void
NrPointToPointEpcHelper::DoAddX2Interface(const Ptr<EpcX2>& gnb1X2,
                                          const Ptr<NetDevice>& gnb1NetDev,
                                          const Ipv4Address& gnb1X2Address,
                                          const Ptr<EpcX2>& gnb2X2,
                                          const Ptr<NetDevice>& gnb2NetDev,
                                          const Ipv4Address& gnb2X2Address) const
{
    NS_LOG_FUNCTION(this);

    Ptr<NrGnbNetDevice> gnb1NetDevice = gnb1NetDev->GetObject<NrGnbNetDevice>();
    Ptr<NrGnbNetDevice> gnb2NetDevice = gnb2NetDev->GetObject<NrGnbNetDevice>();
    std::vector<uint16_t> gnb1CellIds = gnb1NetDevice->GetCellIds();
    std::vector<uint16_t> gnb2CellIds = gnb2NetDevice->GetCellIds();

    NS_ABORT_IF(gnb1NetDevice == nullptr);
    NS_ABORT_IF(gnb2NetDevice == nullptr);

    uint16_t gnb1CellId = gnb1CellIds.at(0);
    uint16_t gnb2CellId = gnb2CellIds.at(0);

    NS_LOG_LOGIC("NrGnbNetDevice #1 = " << gnb1NetDev << " - CellId = " << gnb1CellId);
    NS_LOG_LOGIC("NrGnbNetDevice #2 = " << gnb2NetDev << " - CellId = " << gnb2CellId);

    gnb1X2->AddX2Interface(gnb1CellId, gnb1X2Address, gnb2CellIds, gnb2X2Address);
    gnb2X2->AddX2Interface(gnb2CellId, gnb2X2Address, gnb1CellIds, gnb1X2Address);

    gnb1NetDevice->GetRrc()->AddX2Neighbour(gnb2CellId);
    gnb2NetDevice->GetRrc()->AddX2Neighbour(gnb1CellId);
}

void
NrPointToPointEpcHelper::DoActivateEpsBearerForUe(const Ptr<NetDevice>& ueDevice,
                                                  const Ptr<EpcTft>& tft,
                                                  const EpsBearer& bearer) const
{
    Ptr<NrUeNetDevice> ueNetDevice = ueDevice->GetObject<NrUeNetDevice>();
    if (ueNetDevice)
    {
        Simulator::ScheduleNow(&EpcUeNas::ActivateEpsBearer, ueNetDevice->GetNas(), bearer, tft);
    }
    else
    {
        NS_FATAL_ERROR("What kind of device are you trying to pass to the NR helper?");
    }
}

} // namespace ns3
