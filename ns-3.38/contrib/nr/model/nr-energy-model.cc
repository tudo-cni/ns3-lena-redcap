/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Communication Networks Institute at TU Dortmund University
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
 * Author: Mike Dabrowski based on prior work from Tim Gebauer 
 */


#include "nr-energy-model.h"
#include <fstream>
namespace ns3{

NS_OBJECT_ENSURE_REGISTERED (NrEnergyModel);

void NrChip::SetDownlinkPower(double power){
    m_downlinkPower = power;
}
void NrChip::SetUplinkPower(double power){
    m_uplinkPower = power;
}
void NrChip::SetEdrxPower(double power){
    m_edrxPower = power;
}
void NrChip::SetDrxPower(double power){
    m_drxPower = power;
}
void NrChip::SetPsmPower(double power){
    m_psmPower = power;
}
void NrChip::SetIdlePower(double power){
    m_idlePower = power;
}
double NrChip::GetDownlinkPower(){
    return m_downlinkPower;
}
double NrChip::GetUplinkPower(){
    return m_uplinkPower;
}
double NrChip::GetEdrxPower(){
    return m_edrxPower;
}
double NrChip::GetDrxPower(){
    return m_drxPower;
}
double NrChip::GetPsmPower(){
    return m_psmPower;
}
double NrChip::GetIdlePower(){
    return m_idlePower;
}
double NrChip::GetConnectedPower(){
    return m_connectedPower;
}


void NrChip::SetEnergyParameterFromCentralFrequency(double centralFrequency,double transmitPower)
{
     for( auto& energyParameter :energyParametersVec )
    {
        if(centralFrequency>= energyParameter.startFrequency && centralFrequency < energyParameter.endFrequency )
        {
            m_psmPower = energyParameter.m_psmPower;
            m_drxPower = energyParameter.m_drxPower;
            m_edrxPower = energyParameter.m_edrxPower;
            m_downlinkPower = energyParameter.m_downlinkPower;
            m_idlePower = energyParameter.m_idlePower;
            m_connectedPower = energyParameter.m_connectedPower;
            
            uint index = 0;
            while( transmitPower > energyParameter.m_uplinkPowerBorders.at(index))
            {
                index = index +1;
            }
            m_uplinkPower = energyParameter.m_uplinkPowerVec.at(index);
        }
    }
    std::cout<<m_uplinkPower<<std::endl;
}
NrEnergyModel::NrEnergyModel(NrChip module,uint32_t startTime):
m_module(module),m_lastState(PowerState::OFF), m_lastStateChange(Time(0))
{
   m_battery = CreateObject<LiIonEnergySource>();
   m_battery->SetInitialEnergy(37000.0);
   Simulator::Schedule(MilliSeconds(startTime),&NrEnergyModel::ActivateEnergyModel, this);
};

NrEnergyModel::~NrEnergyModel(){
    DoNotifyStateChange(PowerState::OFF);
    std::string logfile_path = m_outputDir+"Energystates.log";
    std::ofstream logfile;
    logfile.open(logfile_path, std::ios_base::app);
    for( auto  const& state : m_timeSpendInState)
    {
        logfile << Simulator::Now().GetNanoSeconds() <<" imsi; " <<m_imsi<<", Time in state " << PowerStateNames[uint(state.first)]<< ":" << (state.second/1e6)  <<"ms"<<"\n";
    }
    for( auto  const& state : m_energySpendInState)
    {
        logfile << Simulator::Now().GetNanoSeconds() <<" imsi; " <<m_imsi<< ", Energy in state " << PowerStateNames[uint(state.first)]<< ":" << state.second<<"\n";
    }
    logfile.close();
}

NrEnergyModel::PowerState 
NrEnergyModel::DoGetState(){
    return m_lastState;
}

void NrEnergyModel::ActivateEnergyModel(){
    m_isActive = true;
    m_lastStateChange= Simulator::Now();
    m_lastState = m_savedInactiveState;
}


void NrEnergyModel::DoNotifyStateChange(PowerState newState,uint64_t imsi){

    if(m_isActive)
    {
        Time stateTime = (Simulator::Now()-m_lastStateChange); 
        DoNotifyStateChange(newState);
        m_imsi = imsi;
    }
    else{
        if(newState == PowerState::RRC_IDLE)
        {
            if(m_module.m_useEdrx)
            {
                newState = PowerState::RRC_IDLE_EDRX;
            }
            else{
                newState = PowerState::RRC_IDLE_DRX;
            }

        }
        m_savedInactiveState = newState;
    }
}

void NrEnergyModel::DoNotifyStateChange(PowerState newState){
    Time stateTime = (Simulator::Now()-m_lastStateChange); 
    if(newState == PowerState::RRC_IDLE)
    {
        if(m_module.m_useEdrx)
        {
            newState = PowerState::RRC_IDLE_EDRX;
        }
        else{
            newState = PowerState::RRC_IDLE_DRX;
        }

    }
    double lostEnergy = 0; // Energy in [Ws] or [J]
    m_states.push_back(std::pair<PowerState,uint32_t>(newState,Simulator::Now().GetMilliSeconds()));
    switch (m_lastState)
    {
    case PowerState::RRC_Connected:
        lostEnergy = m_module.GetConnectedPower()*stateTime.GetMicroSeconds()/1e6; // Energy in [Ws] or [J]
        break;
    case PowerState::RRC_RECEIVING_PDSCH:
        lostEnergy = m_module.GetDownlinkPower()*stateTime.GetMicroSeconds()/1e6;
        break;
    case PowerState::RRC_SENDING_PRACH:
    case PowerState::RRC_SENDING_PUSCH:
        lostEnergy = m_module.GetUplinkPower()*stateTime.GetMicroSeconds()/1e6;
        break;
    case PowerState::RRC_IDLE_EDRX:
        lostEnergy = m_module.GetEdrxPower()*stateTime.GetMicroSeconds()/1e6;
        break;
    case PowerState::RRC_IDLE_DRX:
        lostEnergy = m_module.GetDrxPower()*stateTime.GetMicroSeconds()/1e6;
        break;
    case PowerState::RRC_IDLE:
        //TODO has to be changed?
        if(m_module.m_useEdrx)
        {
            lostEnergy = m_module.GetEdrxPower()*stateTime.GetMicroSeconds()/1e6;
        }
        else{
            lostEnergy = m_module.GetDrxPower()*stateTime.GetMicroSeconds()/1e6;
        }
        break;
    default:
        break;
    }
    m_lastStateChange = Simulator::Now();
    m_battery->DecreaseRemainingEnergy(lostEnergy);
    m_timeSpendInState[m_lastState] += stateTime.GetNanoSeconds();
    m_energySpendInState[m_lastState] += lostEnergy;
    m_lastState = newState;

}
double NrEnergyModel::GetEnergyRemaining(){
    // Update Power when reading
    DoNotifyStateChange(m_lastState);
    return m_battery->GetRemainingEnergy();
}
double NrEnergyModel::GetEnergyRemainingFraction(){
    // Update Power when reading
    DoNotifyStateChange(m_lastState);
    return m_battery->GetEnergyFraction();
}


}
