/*
 * Copyright (c) 2015 Danilo Abrignani
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
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 *
 */

#ifndef LTE_UE_CCM_RRC_SAP_H
#define LTE_UE_CCM_RRC_SAP_H

#include <ns3/eps-bearer.h>
#include <ns3/lte-mac-sap.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/lte-ue-cmac-sap.h>

#include <map>

namespace ns3
{
class LteUeCmacSapProvider;
class LteMacSapUser;
class LteMacSapProvider;

/**
 * \brief Service Access Point (SAP) offered by the UE component carrier manager
 *  to the UE RRC.
 *
 * This is the *Component Carrier Management SAP Provider*, i.e., the part of the SAP
 * that contains the component carrier manager methods called by the Ue RRC
 * instance.
 */
class LteUeCcmRrcSapProvider
{
    /// allow  LteMacSapUser class friend access
    friend class LteMacSapUser;
    friend class LteMacSapProvider;

  public:
    virtual ~LteUeCcmRrcSapProvider();

    /// LcsConfig structure
    struct LcsConfig
    {
        uint8_t componentCarrierId;                          ///< component carrier ID
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig; ///< logical channel config
        LteMacSapUser* msu;                                  ///< MSU
    };

    /**
     * add a new Logical Channel (LC)
     *
     * \param lcId is the Logical Channel Id
     * \param lcConfig is a single structure contains logical Channel Id, Logical Channel config and
     * Component Carrier Id
     * \param msu is the pointer to LteMacSapUser related to the Rlc instance
     * \return vector of LcsConfig contains the lc configuration for each Mac
     *                the size of the vector is equal to the number of component
     *                carrier enabled.
     *
     * The Logical Channel configurations for each component carrier depend on the
     * algorithm used to split the traffic between the component carriers themself.
     */
    virtual std::vector<LteUeCcmRrcSapProvider::LcsConfig> AddLc(
        uint8_t lcId,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu) = 0;

    /**
     * \brief Remove an existing Logical Channel for a Ue in the LteUeComponentCarrierManager
     * \param lcid the Logical Channel Id
     * \return vector of integer the componentCarrierId of the componentCarrier
     *                where the bearer is enabled
     */
    virtual std::vector<uint16_t> RemoveLc(uint8_t lcid) = 0;
    /**
     * \brief Reset LC maps
     *
     */
    virtual void Reset() = 0;
    /// Notify reconfiguration msg function
    virtual void NotifyConnectionReconfigurationMsg() = 0;

    /**
     * \brief Add the Signal Bearer for a specific Ue in LteUeComponenCarrierManager
     * \param lcid the Logical Channel Id
     * \param lcConfig this structure it is hard-coded in the LteEnbRrc
     * \param msu it is the MacSapUser of the Rlc instance
     * \return the LteMacSapUser of the ComponentCarrierManager
     *
     */
    virtual LteMacSapUser* ConfigureSignalBearer(
        uint8_t lcid,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu) = 0;
    virtual void StartBwpSwitching( uint8_t bwpID) =0;
    virtual void SetBwpInactivityTimer(Time bwpInactivity) =0;
    virtual void RefreshBwpInactivityTimer() =0;
    virtual LteMacSapProvider* GetLteMacSapProvider (uint8_t bwpID) =0;

}; // end of class LteUeCcmRrcSapProvider

/// MemberLteUeCcmRrcSapProvider class
template <class C>
class MemberLteUeCcmRrcSapProvider : public LteUeCcmRrcSapProvider
{
  public:
    /**
     * Constructor
     *
     * \param owner the owner class
     */
    MemberLteUeCcmRrcSapProvider(C* owner);

    // inherited from LteUeCcmRrcSapProvider
    std::vector<uint16_t> RemoveLc(uint8_t lcid) override;
    void Reset() override;
    std::vector<LteUeCcmRrcSapProvider::LcsConfig> AddLc(
        uint8_t lcId,
        LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
        LteMacSapUser* msu) override;
    void NotifyConnectionReconfigurationMsg() override;
    LteMacSapUser* ConfigureSignalBearer(uint8_t lcid,
                                         LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
                                         LteMacSapUser* msu) override;
    void StartBwpSwitching( uint8_t bwpID) override;
    void SetBwpInactivityTimer(Time bwpInactivity) override;
    void RefreshBwpInactivityTimer() override;
    LteMacSapProvider* GetLteMacSapProvider( uint8_t bwpID) override;

  private:
    C* m_owner; ///< the owner class
};

template <class C>
MemberLteUeCcmRrcSapProvider<C>::MemberLteUeCcmRrcSapProvider(C* owner)
    : m_owner(owner)
{
}

template <class C>
std::vector<uint16_t>
MemberLteUeCcmRrcSapProvider<C>::RemoveLc(uint8_t lcid)
{
    return m_owner->DoRemoveLc(lcid);
}

template <class C>
void
MemberLteUeCcmRrcSapProvider<C>::Reset()
{
    return m_owner->DoReset();
}

template <class C>
std::vector<LteUeCcmRrcSapProvider::LcsConfig>
MemberLteUeCcmRrcSapProvider<C>::AddLc(uint8_t lcId,
                                       LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
                                       LteMacSapUser* msu)
{
    return m_owner->DoAddLc(lcId, lcConfig, msu);
}

template <class C>
void
MemberLteUeCcmRrcSapProvider<C>::NotifyConnectionReconfigurationMsg()
{
    NS_FATAL_ERROR("Function should not be called because it is not implemented.");
    // m_owner->DoNotifyConnectionReconfigurationMsg ();
}

template <class C>
LteMacSapUser*
MemberLteUeCcmRrcSapProvider<C>::ConfigureSignalBearer(
    uint8_t lcid,
    LteUeCmacSapProvider::LogicalChannelConfig lcConfig,
    LteMacSapUser* msu)
{
    return m_owner->DoConfigureSignalBearer(lcid, lcConfig, msu);
}


template <class C>
void
MemberLteUeCcmRrcSapProvider<C>::StartBwpSwitching(uint8_t bwpID)
{
    return m_owner->DoStartBwpSwitching(bwpID);
}

template <class C>
void
MemberLteUeCcmRrcSapProvider<C>::SetBwpInactivityTimer(Time bwpInactivity)
{
    return m_owner->SetBwpInactivityTimer(bwpInactivity);
}

template <class C>
void
MemberLteUeCcmRrcSapProvider<C>::RefreshBwpInactivityTimer()
{
    return m_owner->DoRefreshBwpInactivityTimer();
}







template <class C>
LteMacSapProvider*
MemberLteUeCcmRrcSapProvider<C>::GetLteMacSapProvider(uint8_t bwpID)
{
    return m_owner->DoGetLteMacSapProvider(bwpID);
}


/**
 * \brief Service Access Point (SAP) offered by the UE RRC to the UE CCM.
 *
 * This is the *Component Carrier Management SAP User*, i.e., the part of the SAP
 * that contains the UE RRC methods called by the UE CCM instance.
 */
class LteUeCcmRrcSapUser
{
  public:
    virtual ~LteUeCcmRrcSapUser();

    /**
     * this function will be used after the RRC notify to ComponentCarrierManager
     * that a reconfiguration message with Secondary component carrier (SCc) arrived or not
     * the method it is called only if the SCc wasn't set up
     * \param componentCarrierList component carrier list
     */
    virtual void ComponentCarrierEnabling(std::vector<uint8_t> componentCarrierList) = 0;
    /**
     * \brief Set the number of component carriers
     *
     * \param noOfComponentCarriers The number of component carriers
     */
    virtual void SetNumberOfComponentCarriers(uint16_t noOfComponentCarriers) = 0;

    virtual void NotifySwitchedBwp(uint8_t bwpID) =0;

    virtual uint64_t GetImsi() =0;

}; // end of class LteUeCcmRrcSapUser

/// MemberLteUeCcmRrcSapUser class
template <class C>
class MemberLteUeCcmRrcSapUser : public LteUeCcmRrcSapUser
{
  public:
    /**
     * Constructor
     *
     * \param owner the owner class
     */
    MemberLteUeCcmRrcSapUser(C* owner);
    // inherited from LteUeCcmRrcSapUser
    void ComponentCarrierEnabling(std::vector<uint8_t> componentCarrierList) override;
    void SetNumberOfComponentCarriers(uint16_t noOfComponentCarriers) override;
    void NotifySwitchedBwp(uint8_t bwpID) override;
    uint64_t GetImsi() override;

  private:
    C* m_owner; ///< the owner class
};

template <class C>
MemberLteUeCcmRrcSapUser<C>::MemberLteUeCcmRrcSapUser(C* owner)
    : m_owner(owner)
{
}

template <class C>
void
MemberLteUeCcmRrcSapUser<C>::ComponentCarrierEnabling(std::vector<uint8_t> componentCarrierList)
{
    NS_FATAL_ERROR("Function should not be called because it is not implemented.");
    // m_owner->DoComponentCarrierEnabling (componentCarrierList);
}

template <class C>
void
MemberLteUeCcmRrcSapUser<C>::SetNumberOfComponentCarriers(uint16_t noOfComponentCarriers)
{
    m_owner->DoSetNumberOfComponentCarriers(noOfComponentCarriers);
}

template <class C>
void
MemberLteUeCcmRrcSapUser<C>::NotifySwitchedBwp(uint8_t bwpID)
{
    m_owner->DoNotifySwitchedBwp(bwpID);
}

template <class C>
uint64_t
MemberLteUeCcmRrcSapUser<C>::GetImsi()
{
    return m_owner->GetImsi();
}


} // end of namespace ns3

#endif /* LTE_UE_CCM_RRC_SAP_H */
