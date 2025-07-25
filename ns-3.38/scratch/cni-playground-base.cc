/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ideal-beamforming-algorithm.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/udp-client-5hine.h"
#include <filesystem>
#include <math.h>
#include "ns3/nr-energy-model.h"

#define M_PI 3.14159265358979323846
using namespace ns3;

void progressPrint()
{
    uint16_t sim_time = 35;
    auto end = std::chrono::system_clock::now();
    static std::time_t last_time = std::chrono::system_clock::to_time_t(end);
    std::time_t current_time = std::chrono::system_clock::to_time_t(end);
    uint32_t time_for_1s = (current_time - last_time)*10;
    double remaining_sim_time = sim_time - Simulator::Now().GetMilliSeconds()/1000.0;
    uint32_t remaining_real_time = std::round(remaining_sim_time * time_for_1s / 60);
    last_time = current_time;

    std::cout <<"Current simulation time: " << Simulator::Now().GetMilliSeconds()/1000.0 << "s at timestamp: " << current_time << " - For 1s simTime " << time_for_1s << "s are used - Runtime left: " << remaining_real_time <<"min\n";
    Simulator::Schedule(Seconds(0.1), &progressPrint);
}

int
main(int argc, char* argv[])
{

    //fixed values
    int16_t gNbOffsetX = 0; //-50
    int16_t gNbOffsetY = 0; //3
    uint16_t numerology = 1; // numerology for contiguous case (see https://arxiv.org/pdf/1911.05534.pdf Table 1)
    //uint32_t followUpTime = 1000;
    uint8_t numBands = 1;
    double centralFrequencyBand = 3750e6;

    double enddeviceHeight = 2.5;


    //data traffic
    
    //uint mimoScalingFactor = 2; // UL does not use MIMO. Packet sizes are reduced to consider MIMO effects
   

    //used chips 
    int16_t transmitPower = 23;

    //-------------------------------------------------------------------------------------------------------------------------------------------------
    //simulation parameters 
    uint32_t seed = 1;
    
    bool enddeviceDownlinkstream = true;
    uint32_t packetSize = 5000;
    double transmissionIntervall =5 ; // in seconds

    uint16_t enddevicesNumPergNb = 1;
    double bandwidthBand = 20e6;
    uint16_t m_dataInactivityTimer = 60000;
    uint8_t redCapConfig = 1; // 1: minimal config; 2: maximum config; 3: eRedCap
 

    // non-contiguous case
    double centralFrequencyCc0 = 3750e6;
    double bandwidthCc0 = bandwidthBand;
    uint16_t numerologyCc0Bwp0 = 1;
    uint16_t numerologyCc0Bwp1 = 1;

    //std::string pattern ="F|F|F|F|F|F|F|F|F|F|";
    std::string pattern ="DL|DL|S|UL|UL|UL|UL|UL|UL|UL|"; //start with 2 DL slots to ensure the BCH to be in DL slots (doesnt interfere with UL scheduling)
    double totalTxPower = 27; // 250mW
    bool cellScan = false;
    double beamSearchAngleStep = 10.0;

    bool udpFullBuffer = false;

    bool m_useSdt = false;

    bool m_useErrorModel = false;

    bool useFixedMcs = true;
    std::string simTag = "default";
    std::string outputDir = "./";

    double simTime = 36;  // seconds 
    
    CommandLine cmd(__FILE__);

    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("enddevicesNumPergNb", "The number of UE per gNb in multiple-ue topology", enddevicesNumPergNb);
    cmd.AddValue("numBands",
                 "Number of operation bands. More than one implies non-contiguous CC",
                 numBands);
    cmd.AddValue("centralFrequencyBand",
                 "The system frequency to be used in band 1",
                 centralFrequencyBand);
    cmd.AddValue("bandwidthBand", "The system bandwidth to be used in band 1", bandwidthBand);
    cmd.AddValue("numerology", "Numerlogy to be used in contiguous case", numerology);
    cmd.AddValue("centralFrequencyCc0",
                 "The system frequency to be used in CC 0",
                 centralFrequencyCc0);
    cmd.AddValue("bandwidthBand", "The system bandwidth to be used in CC 0", bandwidthCc0);

    cmd.AddValue("numerologyCc0Bwp0", "Numerlogy to be used in CC 0, BWP 0", numerologyCc0Bwp0);
    cmd.AddValue("numerologyCc0Bwp1", "Numerlogy to be used in CC 0, BWP 1", numerologyCc0Bwp1);
    cmd.AddValue("tddPattern",
                 "LTE TDD pattern to use (e.g. --tddPattern=DL|S|UL|UL|UL|DL|S|UL|UL|UL|)",
                 pattern);
    cmd.AddValue("totalTxPower",
                 "total tx power that will be proportionally assigned to"
                 " bandwidth parts depending on each BWP bandwidth ",
                 totalTxPower);
    cmd.AddValue("cellScan",
                 "Use beam search method to determine beamforming vector,"
                 "true to use cell scanning method",
                 cellScan);
    cmd.AddValue("beamSearchAngleStep",
                 "Beam search angle step for beam search method",
                 beamSearchAngleStep);
    cmd.AddValue("udpFullBuffer",
                 "Whether to set the full buffer traffic; if this parameter is "
                 "set then the udpInterval parameter will be neglected.",
                 udpFullBuffer);
    cmd.AddValue("sdt", "Whether SDT should be used or not.",m_useSdt);
    cmd.AddValue("seed", "Seed for RNG", seed);
    cmd.AddValue("simTag",
                 "tag to be appended to output filenames to distinguish simulation campaigns",
                 simTag);
    cmd.AddValue("outputDir", "directory where to store simulation results", outputDir);
    cmd.AddValue("DataErrorModelEnabled","Usage of the Error Model",m_useErrorModel);
    cmd.AddValue("intervall","used intervall for transmitting data", transmissionIntervall);
    cmd.AddValue("packetSize","used packetSize for datastream", packetSize);
    cmd.AddValue("inactivityTimer", "Timer to release the connection after it´s expiration",m_dataInactivityTimer);
    cmd.AddValue("transmitpower", "Ul tandmitower of user equipments",transmitPower);

    cmd.Parse(argc, argv);
    
    uint8_t mimo_layer = 1;
    uint8_t McsToBeUsed =19;
    bool use5MHz = false;

    switch (redCapConfig)
    {
    //minimal configuration
    case 1:
        mimo_layer=1;
        McsToBeUsed = 19;
        use5MHz = false;
        break;
    //maximum configuration
    case 2:
        mimo_layer=2;
        McsToBeUsed=27;
        use5MHz = false;
        break;
    //5MHz configuration
    case 3:
        mimo_layer=1;
        McsToBeUsed = 19;
        use5MHz = true;
        transmitPower -=6;
        break;
    default:
        NS_ABORT_IF(true);
        break;
    }
    
    NrChip chip_RedCap = RG255C(centralFrequencyBand,transmitPower);
    NrChip chip_NR = RM520N(centralFrequencyBand,transmitPower);

    uint initTime = 100 + enddevicesNumPergNb * 10;
    uint8_t numBwp_nonOverlapping = bandwidthBand/20e6;
    
    uint8_t numBwp_Overlapping = 2;
    bandwidthCc0 = bandwidthBand;
    std::string redCapBwp = "";
    std::string embbBwp= "";
    for(uint i = 0; i<numBwp_nonOverlapping; ++i)
    {
        redCapBwp.append(std::to_string(i));
    }
    for(uint i = numBwp_nonOverlapping; i<numBwp_nonOverlapping+numBwp_Overlapping; ++i)
    {
        embbBwp.append(std::to_string(i));
    }

    NS_ASSERT_MSG(redCapBwp!="","No Bwp assigned for RedCap");
    NS_ASSERT_MSG(embbBwp!="","No Bwp assigned for eMBB");
    Config::SetDefault("ns3::NrGnbRrc::BwpForRedCap", StringValue(redCapBwp));
    Config::SetDefault("ns3::NrGnbRrc::BwpForEmBB", StringValue(embbBwp));
    Config::SetDefault("ns3::UeManagerNr::dataInactivityTimer",UintegerValue(m_dataInactivityTimer));

    Config::SetDefault("ns3::NrGnbPhy::RbOverhead", DoubleValue(0.08)); //adjusted to 0.08 to align allocated rb with table Table 5.3.2-1 from 3GPP 38.104. Now 51 RB are set for 20MHz BW
 
    std::string ResultDir = "Results/Cni-Szenario/"+simTag+"/";
    std::string usedRedCapConfig;

    switch (redCapConfig)
    {
    case 1:
        usedRedCapConfig = "minimal";
        break;
    case 2:
        usedRedCapConfig = "maximum";
        break;
    case 3:
        usedRedCapConfig = "5MHz_minimum";
        break;
    
    default:
        break;
    }

       
    Config::SetDefault("ns3::NrNetDevice::outputDir", StringValue(ResultDir+"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+"/Seed_"+std::to_string(seed)+"/"));
 
    try {
        std::filesystem::create_directory(ResultDir);
        std::filesystem::create_directory(ResultDir +"Delay/");
        std::filesystem::create_directory(ResultDir +"Delay/"+std::to_string(packetSize));
        std::filesystem::create_directory(ResultDir +"Delay/"+std::to_string(packetSize)+"/"+usedRedCapConfig);
        std::filesystem::create_directory(ResultDir +"Delay/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower));
        std::filesystem::create_directory(ResultDir +"Ressourcen/");
        std::filesystem::create_directory(ResultDir +"Ressourcen/"+std::to_string(packetSize));
        std::filesystem::create_directory(ResultDir +"Ressourcen/"+std::to_string(packetSize)+"/"+usedRedCapConfig);
        std::filesystem::create_directory(ResultDir +"Ressourcen/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower));
        std::filesystem::create_directory(ResultDir +"PDCCH/");
        std::filesystem::create_directory(ResultDir +"PDCCH/"+std::to_string(packetSize));
        std::filesystem::create_directory(ResultDir +"Ausgaben/");
        std::filesystem::create_directory(ResultDir +"Ausgaben/"+std::to_string(packetSize));
        std::filesystem::create_directory(ResultDir +"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig);
        std::filesystem::create_directory(ResultDir +"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower));
        std::filesystem::create_directory(ResultDir +"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+"/Seed_"+std::to_string(seed));
        
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to create directory: " << ex.what() << std::endl;
    }

    RngSeedManager::SetSeed (seed);
    Ptr<UniformRandomVariable> RaUeUniformVariable = CreateObject<UniformRandomVariable> ();

    //set attribute for gnb
    Config::SetDefault("ns3::NrGnbRrc::SdtUsage", BooleanValue(m_useSdt));
    Config::SetDefault("ns3::NrSpectrumPhy::DataErrorModelEnabled", BooleanValue(m_useErrorModel));

    Config::SetDefault("ns3::NrGnbRrc::PrachConfigurationIndex", UintegerValue(199)); //see https://www.sharetechnote.com/html/5G/5G_RACH.html or 38.211 v15.3.0-Table 6.3.3.2-3 for details to the configuration. Note only short configurations are possible


    double udpAppStartTime = 400; // milliseconds
    double udpAppStopTime = simTime * 1000 + initTime; 

    //simTime = simTime + 1; // add 1 seconds for the last transmissions to finish

    NS_ABORT_IF(numBands < 1);

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    Config::SetDefault("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue(999999999));
    Config::SetDefault ("ns3::NrGnbRrc::EpsBearerToRlcMapping", EnumValue(NrGnbRrc::RLC_AM_ALWAYS));
    Config::SetDefault("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(200)));
    Config::SetDefault("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(80))); //maybe has no impact on simulations
    Config::SetDefault("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(0))); // needed to be able to send small packets with low interarrival times

    Config::SetDefault("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(5)));
    
    //Config::SetDefault("ns3::LteRlcAm::TxOpportunityForRetxAlwaysBigEnough", BooleanValue(true));


    //Create the scenario
    BandwidthPartInfo::Scenario scen;
    BandwidthPartInfo::Scenario scen2;
    scen = BandwidthPartInfo::RMa_nLoS; //Adjusted for CNI-lossModel
    scen2 = BandwidthPartInfo::RMa_LoS; //Adjusted for Freespace model

    // create base stations and mobile terminals
    NodeContainer gNbNodes;
    NodeContainer embbUeNodes;
    NodeContainer redCapUeNodes;
    MobilityHelper mobility;

    gNbNodes.Create(1); // For now just 1 gNb

    redCapUeNodes.Create( enddevicesNumPergNb ); 


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //----- Setting positions in enddevice field -----------------------------------------------------------------//
    Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator>();
    Ptr<ListPositionAllocator> staPositionAlloc = CreateObject<ListPositionAllocator>();
    
    apPositionAlloc->Add(Vector(gNbOffsetX, gNbOffsetY, 10));
    
    std::ofstream outFile_position;
        std::string fileposition = ResultDir+"Ausgaben/"+std::to_string(packetSize) + "/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+"/position_"+std::to_string(seed)+".txt" ;
    	outFile_position.open(fileposition.c_str(), std::ofstream::out | std::ofstream::trunc);
    	if (!outFile_position.is_open())
    	{
        		std::cerr << "Can't open file " << fileposition<< std::endl;
        		return 1;
    	}

    outFile_position.setf(std::ios_base::fixed);

    uint32_t j = 0;

    while(j< enddevicesNumPergNb)
    {
        staPositionAlloc->Add(Vector(10, 10+j, enddeviceHeight));
        j++;
    }
                   
    outFile_position.close();
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(apPositionAlloc);
    mobility.Install(gNbNodes);
    mobility.SetPositionAllocator(staPositionAlloc);
    mobility.Install (embbUeNodes);
    mobility.Install (redCapUeNodes);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //----- Setting 5G NR network ---------------------------------------------------------------------------------//

    // setup the nr simulation
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();

    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);

    // Disable SRS
    nrHelper->SetSchedulerAttribute("SrsSymbols", UintegerValue(0));
    nrHelper->SetSchedulerAttribute("EnableSrsInFSlots", BooleanValue(false));
    nrHelper->SetSchedulerAttribute("EnableSrsInUlSlots", BooleanValue(false));

    nrHelper->SetSchedulerAttribute("FixedMcsDl", BooleanValue(useFixedMcs));
    nrHelper->SetSchedulerAttribute("FixedMcsUl", BooleanValue(useFixedMcs));
    if(useFixedMcs)
    {
        nrHelper->SetSchedulerAttribute("StartingMcsDl", UintegerValue(McsToBeUsed));
        nrHelper->SetSchedulerAttribute("StartingMcsUl", UintegerValue(McsToBeUsed));
    }
    

    //set UE transmit power
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(transmitPower));

    // enable or disable HARQ retransmissions
    nrHelper->SetSchedulerAttribute ("EnableHarqReTx", BooleanValue (false)); //TODO_ enable and schedule harq over manager
    //Config::SetDefault ("ns3::NrHelper::HarqEnabled", BooleanValue (false)); //disabling this doesnt change anything and disabling it in the nr-helper lets the program crash

    nrHelper->SetSchedulerTypeId (NrMacSchedulerOfdmaRR::GetTypeId ());

    nrHelper->SetSchedulerAttribute ("NumNonOverlappingBwp", UintegerValue(numBwp_nonOverlapping)); //TODO_ enable and schedule harq over manager

    std::string errorModel = "ns3::NrEesmIrT2";  // Do not change this table when RedCap devices are used
    Config::SetDefault("ns3::NrAmc::ErrorModelType", TypeIdValue(TypeId::LookupByName("ns3::NrEesmIrT2"))); // Do not change this table when RedCap devices are used

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

    /*
     * Setup the configuration of the spectrum. There is a contiguous and a non-contiguous
     * example:
     * 1) One operation band is deployed with 4 contiguous component carriers
     *    (CC)s, which are automatically generated by the ccBwpManager
     * 2) One operation bands non-contiguous case. CCs and BWPs are manually created
     */

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;

    OperationBandInfo band;

     // For the case of manual configuration of CCs and BWPs
    std::unique_ptr<ComponentCarrierInfo> cc0(new ComponentCarrierInfo());


    std::vector< std::unique_ptr<BandwidthPartInfo>>bwp_vec;
    for(uint i = 0; i< numBwp_nonOverlapping+numBwp_Overlapping; ++i)
    {
        bwp_vec.emplace_back(std::unique_ptr<BandwidthPartInfo>(new BandwidthPartInfo()));
    }

    /*
        * 100MHz band divides into 5 BWP with 20MHz and one BWP with 100MHz
        * The configured spectrum division is:
        * ----------------------------- Band ----------------------------------
        * ----------------------------- CC0------------------------------------
        * ----BWP0-----|----BWP1-----|----BWP2-----|----BWP3-----|----BWP4-----|
        * ------------------------------BWP5-----------------------------------|
        */ 
    uint8_t coresetSymbols = 2; //3
    //For now coresetSymbols should equal for every bandwitdh part. 

    band.m_centralFrequency = centralFrequencyBand;
    band.m_channelBandwidth = bandwidthBand;
    band.m_lowerFrequency = band.m_centralFrequency - band.m_channelBandwidth / 2;
    band.m_higherFrequency = band.m_centralFrequency + band.m_channelBandwidth / 2;
    uint8_t bwpCount = 0;

    // Component Carrier 0
    cc0->m_ccId = 0;
    cc0->m_centralFrequency = centralFrequencyBand;
    cc0->m_channelBandwidth = bandwidthBand;
    cc0->m_lowerFrequency = cc0->m_centralFrequency - cc0->m_channelBandwidth / 2;
    cc0->m_higherFrequency = cc0->m_centralFrequency + cc0->m_channelBandwidth / 2;

    
    for(uint bwpID = 0; bwpID < numBwp_nonOverlapping; ++bwpID)
    {
        bwp_vec.at(bwpID)->m_bwpId = bwpCount;
        bwp_vec.at(bwpID)->m_scenario = scen;
        bwp_vec.at(bwpID)->m_centralFrequency = cc0->m_lowerFrequency + 10e6+ bwpID * 20e6;
        bwp_vec.at(bwpID)->m_channelBandwidth = 20e6;
        bwp_vec.at(bwpID)->m_lowerFrequency = bwp_vec.at(bwpID)->m_centralFrequency - bwp_vec.at(bwpID)->m_channelBandwidth / 2;
        bwp_vec.at(bwpID)->m_higherFrequency = bwp_vec.at(bwpID)->m_centralFrequency + bwp_vec.at(bwpID)->m_channelBandwidth / 2;
        bwp_vec.at(bwpID)->m_coresetSymbols = coresetSymbols;

        cc0->AddBwp(std::move(bwp_vec.at(bwpID)));
        ++bwpCount;
    }

    for(uint bwpID = numBwp_nonOverlapping; bwpID < numBwp_nonOverlapping+numBwp_Overlapping; ++bwpID)
    {
        bwp_vec.at(bwpID)->m_bwpId = bwpCount;
        if(bwpID == uint(numBwp_nonOverlapping+numBwp_Overlapping-2))
        {
             bwp_vec.at(bwpID)->m_scenario = scen2;
        }
        else{
             bwp_vec.at(bwpID)->m_scenario = scen;
        }
        bwp_vec.at(bwpID)->m_centralFrequency = cc0->m_lowerFrequency + cc0->m_channelBandwidth/2;
        bwp_vec.at(bwpID)->m_channelBandwidth = bandwidthBand;
        bwp_vec.at(bwpID)->m_lowerFrequency = bwp_vec.at(bwpID)->m_centralFrequency - bwp_vec.at(bwpID)->m_channelBandwidth / 2;
        bwp_vec.at(bwpID)->m_higherFrequency = bwp_vec.at(bwpID)->m_centralFrequency + bwp_vec.at(bwpID)->m_channelBandwidth / 2;
        bwp_vec.at(bwpID)->m_coresetSymbols = coresetSymbols;

        cc0->AddBwp(std::move(bwp_vec.at(bwpID)));
        ++bwpCount;
    }
    // Add CC to the corresponding operation band.
    band.AddCc(std::move(cc0));

    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));
    nrHelper->SetSchedulerTypeId(TypeId::LookupByName("ns3::NrMacSchedulerOfdmaRR"));
    // Beamforming method
    if (cellScan)
    {
        idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                             TypeIdValue(CellScanBeamforming::GetTypeId()));
        idealBeamformingHelper->SetBeamformingAlgorithmAttribute("BeamSearchAngleStep",
                                                                 DoubleValue(beamSearchAngleStep));
    }
    else
    {
        idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                             TypeIdValue(DirectPathBeamforming::GetTypeId()));
    }

    nrHelper->InitializeOperationBand(&band);
    allBwps = CcBwpCreator::GetAllBwps({band});

    double x = pow(10, totalTxPower / 10); // convert from dBm to mW

    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2)); 
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4)); // 1 and 1 makes no difference for UL? //4
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    nrHelper->SetUeRedCapAntennaAttribute ("NumRows", UintegerValue (mimo_layer));
    nrHelper->SetUeRedCapAntennaAttribute ("NumColumns", UintegerValue (mimo_layer));
    nrHelper->SetUeRedCapAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));



    //implementeation of Ri not compatible with fixedMCS. Set to 2 but doesnt do anything due to an necessary workaround
    nrHelper->SetUePhyAttribute("UseFixedRi", BooleanValue(true));
    nrHelper->SetUePhyAttribute("FixedRankIndicator", UintegerValue(2));


    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4)); 
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8)); 
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    //Initialize Ressource Manager
    std::string logDir = ResultDir+"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+"/Seed_"+std::to_string(seed)+"/";

    NrMacSchedulerRessourceManager schedmanager =  NrMacSchedulerRessourceManager(pattern, 1,simTime,initTime/1000,logDir,bwpCount,use5MHz);
    NrMacSchedulerRessourceManager* schedmanagerPtr = &schedmanager;

    // Install and get the pointers to the NetDevices
    bool SdtUsable = false;
    NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice(gNbNodes, allBwps,mimo_layer,schedmanagerPtr);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (embbUeNodes, allBwps,SdtUsable, chip_NR);
    NetDeviceContainer redCapUeNetDev = nrHelper->InstallRedCapUeDevice (redCapUeNodes, allBwps, SdtUsable, chip_RedCap,mimo_layer);
    ueNetDev.Add(redCapUeNetDev);
    
    double polarizationFirstSubArray = (0.0 * M_PI) / 180.0;  // converting to radians
    double polarizationSecondSubArray = (90.0 * M_PI) / 180.0; // converting to radians
    ObjectVectorValue gnbSpectrumPhys;
    Ptr<NrSpectrumPhy> nrSpectrumPhy;
    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)->GetAttribute("NrSpectrumPhyList", gnbSpectrumPhys);
    nrSpectrumPhy = gnbSpectrumPhys.Get(0)->GetObject<NrSpectrumPhy>();
    nrSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>()->SetAttribute(
        "PolSlantAngle",
        DoubleValue(polarizationFirstSubArray));
    if (gnbSpectrumPhys.GetN() == 2)
    {
        nrSpectrumPhy = gnbSpectrumPhys.Get(1)->GetObject<NrSpectrumPhy>();
        nrSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>()->SetAttribute(
            "PolSlantAngle",
            DoubleValue(polarizationSecondSubArray));
    }


    // for UE
    ObjectVectorValue ueSpectrumPhys;
    nrHelper->GetUePhy(ueNetDev.Get(0), 0)->GetAttribute("NrSpectrumPhyList", ueSpectrumPhys);
    nrSpectrumPhy = ueSpectrumPhys.Get(0)->GetObject<NrSpectrumPhy>();
    nrSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>()->SetAttribute(
        "PolSlantAngle",
        DoubleValue(polarizationFirstSubArray));
    if (ueSpectrumPhys.GetN() == 2)
    {
        nrSpectrumPhy = ueSpectrumPhys.Get(1)->GetObject<NrSpectrumPhy>();
        nrSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>()->SetAttribute(
            "PolSlantAngle",
            DoubleValue(polarizationSecondSubArray));
    }



    int64_t randomStream = 2;
    randomStream += nrHelper->AssignStreams(enbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);


    // Manually set the attribute of the netdevice (enbNetDev.Get (0)) and bandwidth part (0),
    // (1), ...
    for(uint i = 0; i< bwpCount; ++i)
    {
        if(i < uint(numBwp_nonOverlapping))
        {
            nrHelper->GetGnbPhy(enbNetDev.Get(0), i)
            ->SetAttribute("Numerology", UintegerValue(numerology));
            nrHelper->GetGnbPhy(enbNetDev.Get(0), i)
                ->SetAttribute("TxPower", DoubleValue(10 * log10(x/(bwpCount-2))));
            nrHelper->GetGnbPhy(enbNetDev.Get(0), i)->SetAttribute("Pattern", StringValue(pattern));
        }
        else{
             nrHelper->GetGnbPhy(enbNetDev.Get(0), i)
                ->SetAttribute("Numerology", UintegerValue(numerology));
            nrHelper->GetGnbPhy(enbNetDev.Get(0), i)
                ->SetAttribute("TxPower", DoubleValue(10 * log10(x)));
            nrHelper->GetGnbPhy(enbNetDev.Get(0), i)->SetAttribute("Pattern", StringValue(pattern));

        }
    }

    for (auto it = enbNetDev.Begin(); it != enbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ueNetDev.Begin(); it != ueNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //----- Setting traffic models --------------------------------------------------------------------------------//

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(redCapUeNodes);
    internet.Install(embbUeNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

     // Set the default gateway for the UEs
    for (uint32_t j = 0; j < redCapUeNodes.GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(redCapUeNodes.Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }
     for (uint32_t j = 0; j < embbUeNodes.GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(embbUeNodes.Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // attach UEs to the closest eNB before creating the dedicated flows
    nrHelper->AttachToClosestEnb(ueNetDev, enbNetDev);

    // install UDP applications
    uint16_t dlPort = 1;
    uint16_t ulPort = 1;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    uint64_t imsi;
    std::ofstream outFile_ports;
    std::string filename = ResultDir+"Ausgaben/"+std::to_string(packetSize) +"/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+ "/_ports_Seed"+std::to_string(seed)+".txt" ;
    outFile_ports.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile_ports.is_open())
    {
        std::cerr << "Can't open file " << filename << std::endl;
        return 1;
    }

    outFile_ports.setf(std::ios_base::fixed);


    uint accessbreak =20;

    // Install UDP applications for the enddevices
    if(enddevicesNumPergNb > 0){
        for (uint32_t u = 0; u < enddevicesNumPergNb; ++u)
        {
            uint16_t port;
            int access = RaUeUniformVariable->GetValue (8000, 12000); // 
            enum EpsBearer::Qci q;
            if(enddeviceDownlinkstream)
            {
                port = dlPort;
                UdpClientHelper udpDlClient(ueIpIface.GetAddress(u), dlPort);
                udpDlClient.SetAttribute("PacketSize", UintegerValue(packetSize));
                udpDlClient.SetAttribute("Interval", TimeValue(Seconds(transmissionIntervall)));
                udpDlClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
                udpDlClient.SetAttribute("StopApp", UintegerValue(udpAppStopTime));
                clientApps.Add(udpDlClient.Install(remoteHost));
                PacketSinkHelper psh(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), port));
                    serverApps.Add(psh.Install(redCapUeNodes.Get(u)));
            }
            else{
                port = ulPort;
                UdpClientHelper udpUlClient(remoteHostAddr, ulPort);
                udpUlClient.SetAttribute("PacketSize", UintegerValue(packetSize));
                udpUlClient.SetAttribute("Interval", TimeValue(Seconds(transmissionIntervall)));
                udpUlClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
                udpUlClient.SetAttribute("StopApp", UintegerValue(udpAppStopTime));
                clientApps.Add(udpUlClient.Install(redCapUeNodes.Get(u)));
                PacketSinkHelper psh(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), port));
                serverApps.Add(psh.Install(remoteHost));
            }

            serverApps.Get(serverApps.GetN()-1)->SetStartTime (MilliSeconds (access));
            clientApps.Get(serverApps.GetN()-1)->SetStartTime (MilliSeconds (access));

            imsi = nrHelper->GetUePhy(ueNetDev.Get(u),0)->GetImsi();
            outFile_ports << "enddevice,datastream," << port << "," << access << "," << imsi << "\n";

            Ptr<EpcTft> tft = Create<EpcTft>();
            EpcTft::PacketFilter pfD;
            pfD.localPortStart = port;
            pfD.localPortEnd = port;
            ++port;
            if(enddeviceDownlinkstream){
                ++dlPort;
            }
            else{
                ++ulPort;
            }
            tft->Add(pfD);

            q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
            EpsBearer bearerD(q);
            nrHelper->ActivateDedicatedEpsBearer(ueNetDev.Get(u), bearerD, tft);



            //add initialization ul-transmission add the start of the simulation
            PacketSinkHelper ulInitPacketSinkHelper(
                "ns3::UdpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            serverApps.Add(ulInitPacketSinkHelper.Install(remoteHost));
            UdpClientHelper ulInitClient(remoteHostAddr, ulPort);
            ulInitClient.SetAttribute("PacketSize", UintegerValue(20));
            ulInitClient.SetAttribute("Interval", TimeValue(Seconds(1)));
            ulInitClient.SetAttribute("MaxPackets", UintegerValue(1));
            access = 10+ (u * accessbreak);
            ulInitClient.SetAttribute("StopApp", UintegerValue(access));
            clientApps.Add(ulInitClient.Install(redCapUeNodes.Get(u)));
            serverApps.Get(serverApps.GetN()-1)->SetStartTime (MilliSeconds (access));
            clientApps.Get(serverApps.GetN()-1)->SetStartTime (MilliSeconds (access));
            Ptr<EpcTft> tftInit = Create<EpcTft>();
            EpcTft::PacketFilter ulpfInit;
            ulpfInit.remotePortStart = ulPort;
            ulpfInit.remotePortEnd = ulPort;
            ++ulPort;
            tftInit->Add(ulpfInit);
            q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
            EpsBearer bearerZ(q);
            nrHelper->ActivateDedicatedEpsBearer(ueNetDev.Get(u), bearerZ, tftInit);


        }
    }
       
    outFile_ports.close();

    serverApps.Stop(Seconds(simTime+initTime/1000));
    clientApps.Stop(Seconds(simTime+initTime/1000));

    // enable the traces provided by the nr module
     //nrHelper->EnableTraces();
     //nrHelper->GetPhyRxTrace()->SetResultsFolder(logDir);

    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(redCapUeNodes);
    endpointNodes.Add(embbUeNodes);
    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
    Simulator::Schedule(Seconds(0.1), &progressPrint);  
    Simulator::Stop(Seconds(simTime+initTime/1000));
    Simulator::Run();

    /*
     * To check what was installed in the memory, i.e., BWPs of eNb Device, and its configuration.
     * Example is: Node 1 -> Device 0 -> BandwidthPartMap -> {0,1} BWPs -> NrGnbPhy ->
    NrPhyMacCommong-> Numerology, Bandwidth, ... GtkConfigStore config; config.ConfigureAttributes
    ();
    */


   //log spectral ressources
    //schedmanager.logRessources(enddevicesNumPergNb,seed);

    schedmanager.collectFinalResUsage();

    RessourceUsageStats resStats;
    //resStats = schedmanager.calculateCapacityUsage(initTime,followUpTime); 
    resStats = schedmanager.getCapacityUsage(); 

    UeRessourceUsage
    //ueStats = schedmanager.calculateUeSpecificCapacityUsage(initTime,followUpTime,enddevicesNumPergNb+sumOtherDevices+1); 
    ueStats = schedmanager.getUeSpecificCapacityUsage(); 



    double capacityUsage_DL = double(resStats.usedRessources_DL)/(resStats.usedRessources_DL+resStats.freeRessources_DL);
    double capacityUsage_UL = double(resStats.usedRessources_UL)/(resStats.usedRessources_UL+resStats.freeRessources_UL);

    std::cout << "Capacity usage DL: "<<capacityUsage_DL << std::endl;
    std::cout << "Capacity usage UL: "<<capacityUsage_UL << std::endl;

    // Print per-flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    //filename = outputDir + "/" + simTag + ".txt";
    filename = ResultDir+"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig  +"/"+std::to_string(transmitPower)+"/outFile_Seed"+std::to_string(seed)+"_"+".txt" ;
    outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Can't open file " << filename << std::endl;
        return 1;
    }
    outFile.setf(std::ios_base::fixed);
    uint64_t totalTxPackets = 0;
    uint64_t totalTxBytes = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalRxBytes = 0;

    outFile << "Ressource stats" << "\n";
    outFile << "ControlRessources: "<<resStats.ControlRessources << "\n";
    outFile << "freeRessources_DL: "<<resStats.freeRessources_DL << "\n";
    outFile << "freeRessources_UL: "<<resStats.freeRessources_UL << "\n";
    outFile << "PdcchRessources: "<<resStats.PdcchRessources << "\n";
    outFile << "PdcchUsed: "<<resStats.PdcchUsed << "\n";
    outFile << "PrachRessources: "<<resStats.PrachRessources << "\n";
    outFile << "PucchRessources: "<<resStats.PucchRessources << "\n";
    outFile << "usedRessources_DL:"<<resStats.usedRessources_DL << "\n";
    outFile << "usedRessources_UL:"<<resStats.usedRessources_UL << "\n";

    outFile<< "Capacity usage DL: " << capacityUsage_DL << "\n";
    outFile<< "Capacity usage UL: " << capacityUsage_UL << "\n";

    outFile<< "Device specific ressource usage:"<<"\n";
    for(uint i=1;i<=enddevicesNumPergNb;++i)
    {
        outFile<<"Ressource usage for device "<<i<<": "<<ueStats.ueResMap[i]<<" ressource blocks (enddevice)"<<"\n";
    }


    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str("UDP");
        }
        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                << t.destinationAddress << ":" << t.destinationPort << ") proto "
                << protoStream.str() << "\n";
        totalTxPackets = totalTxPackets + i->second.txPackets;
        totalTxBytes = totalTxBytes + i->second.txBytes;
        outFile << "  Tx Packets: " << i->second.txPackets << "\n";
        outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        outFile << "  TxOffered:  "
                << i->second.txBytes * 8.0 / (simTime - udpAppStartTime/1000.0) / 1000 / 1000 << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        if (i->second.rxPackets > 0)
        {
            // Measure the duration of the flow from receiver's perspective
            double rxDuration = i->second.timeLastRxPacket.GetSeconds () -
             i->second.timeFirstTxPacket.GetSeconds ();
            //double rxDuration = (simTime - udpAppStartTime/1000.0);

            averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            outFile << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000
                    << " Mbps\n";
            outFile << "  Mean delay:  "
                    << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << " ms\n";
            // outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << "
            // Mbps \n";
            outFile << "  Mean jitter:  "
                    << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << " ms\n";
        }
        else
        {
            outFile << "  Throughput:  0 Mbps\n";
            outFile << "  Mean delay:  0 ms\n";
            outFile << "  Mean jitter: 0 ms\n";
        }
        totalRxPackets = totalRxPackets + i->second.rxPackets;
        totalRxBytes = totalRxBytes + i->second.rxBytes;
        outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
        if(i->second.txPackets == 0){
            outFile << "  PDR: 100%" << "%\n";
        }
        else{
            outFile << "  PDR: " << (i->second.rxPackets*1.0/(i->second.txPackets*1.0))*100 << "%\n";
        }
    }

    outFile << "\n\n  Flow time: " << (simTime - udpAppStartTime/1000.0) << "\n";
    outFile << "  Mean flow throughput: " << averageFlowThroughput / stats.size() << "\n";
    outFile << "  Mean flow delay: " << averageFlowDelay / stats.size() << "\n";
    outFile << "  Total Tx Packets: " << totalTxPackets << "\n";
    outFile << "  Total Rx Packets: " << totalRxPackets << "\n";
    outFile << "  Total Tx Bytes: " << totalTxBytes << "\n";
    outFile << "  Total Rx Bytes: " << totalRxBytes << "\n";
    if (totalTxPackets > 0){
        outFile << "  Total PDR: " << totalRxPackets*100.0 / totalTxPackets << "%\n";
    }
    else{
        outFile << "  Total PDR: " << -1 << "%\n";
    }

    outFile.close();

    std::ifstream f(filename.c_str());

    if (f.is_open())
    {
        std::cout << f.rdbuf();
    }

    //Create a files for Data processing
    std::ofstream dataFile;
    std::string delayFileName = ResultDir+"Delay/"+std::to_string(packetSize) +"/"+usedRedCapConfig+"/"+std::to_string(transmitPower)+"/Seed"+std::to_string(seed)+ ".txt";
    dataFile.open(delayFileName.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!dataFile.is_open())
    {
        std::cerr << "Can't open file " << delayFileName << std::endl;
        return 1;
    }

    dataFile.setf(std::ios_base::fixed);
    
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
      for(uint32_t indexDelay = 0; indexDelay <i->second.allDelays.size(); ++indexDelay)
      {
        if(indexDelay>0)
        {
          dataFile<<":";
        }
        dataFile<<i->second.allDelays.at(indexDelay);

      }
      dataFile<<"\n";
    }

    
    dataFile.close();


    std::string resUsage =  ResultDir+"Ressourcen/"+std::to_string(packetSize)+"/"+usedRedCapConfig +"/"+std::to_string(transmitPower)+ "/Seed"+std::to_string(seed)+".txt";
     dataFile.open(resUsage .c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!dataFile.is_open())
    {
        std::cerr << "Can't open file " << resUsage << std::endl;
        return 1;
    }

    dataFile.setf(std::ios_base::fixed);
    dataFile << "freeRessources_DL:freeRessources_UL:usedRessources_DL:usedRessources_UL:PrachRessources:PdcchRessources:PdcchUsed:PucchRessources:ControlRessources"<<"\n";
    
    dataFile<<resStats.freeRessources_DL<<":"<<resStats.freeRessources_UL<<":"<<resStats.usedRessources_DL<<":"<<resStats.usedRessources_UL<<":"<<
        resStats.PrachRessources<<":"<<resStats.PdcchRessources<<":"<<resStats.PdcchUsed<<":"<<resStats.PucchRessources<<":"<<resStats.ControlRessources<<"\n";

    
    dataFile.close();


    std::string flowOutput = ResultDir+"Ausgaben/"+std::to_string(packetSize)+"/"+usedRedCapConfig+"/"+std::to_string(transmitPower) + "/flowOutput" + "_Seed"+std::to_string(seed)+".txt";
    dataFile.open(flowOutput .c_str(), std::ofstream::out | std::ofstream::trunc);
    dataFile.setf(std::ios_base::fixed);
    dataFile << "Tx Packets" <<":"<< "Rx Packets"<<":"<<"Mean delay"<<":"<<"Mean jitter"<<":"<<"Tx Bytes"<<":"<<"Rx Bytes"<<"\n";
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
        i != stats.end();
        ++i)
    {
    dataFile<<i->second.txPackets<<":"<<i->second.rxPackets<<":"<<1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets<<
        ":"<<1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets<<":"<<i->second.txBytes<<":"<<i->second.rxBytes<<"\n";

    dataFile<<"\n";
    }

    Simulator::Destroy();
    return 0;
}
