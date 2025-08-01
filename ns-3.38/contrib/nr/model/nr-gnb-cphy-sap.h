/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>,
 *         Marco Miozzo <mmiozzo@cttc.es>
 */

#ifndef NR_GNB_CPHY_SAP_H
#define NR_GNB_CPHY_SAP_H

#include <ns3/nr-rrc-sap.h>
#include <ns3/ptr.h>

#include <stdint.h>

namespace ns3
{

class NrGnbNetDevice;

/**
 * Service Access Point (SAP) offered by the UE PHY to the UE RRC for control purposes
 *
 * This is the PHY SAP Provider, i.e., the part of the SAP that contains
 * the PHY methods called by the MAC
 */
class NrGnbCphySapProvider
{
  public:
    /**
     * Destructor
     */
    virtual ~NrGnbCphySapProvider();

    /**
     * Set cell ID
     *
     * \param cellId the Cell Identifier
     */
    virtual void SetCellId(uint16_t cellId) = 0;

    /**
     * Set bandwidth
     *
     * \param ulBandwidth the UL bandwidth in PRBs
     * \param dlBandwidth the DL bandwidth in PRBs
     */
    virtual void SetBandwidth(uint16_t ulBandwidth, uint16_t dlBandwidth) = 0;

    /**
     * Set EARFCN
     *
     * \param ulEarfcn the UL EARFCN
     * \param dlEarfcn the DL EARFCN
     */
    virtual void SetEarfcn(uint32_t ulEarfcn, uint32_t dlEarfcn) = 0;

    /**
     * Add a new UE to the cell
     *
     * \param rnti the UE id relative to this cell
     */
    virtual void AddUe(uint16_t rnti) = 0;

    /**
     * Remove an UE from the cell
     *
     * \param rnti the UE id relative to this cell
     */
    virtual void RemoveUe(uint16_t rnti) = 0;

    /**
     * Set the UE transmission power offset P_A
     *
     * \param rnti the UE id relative to this cell
     * \param pa transmission power offset
     */
    virtual void SetPa(uint16_t rnti, double pa) = 0;

    /**
     * Set transmission mode
     *
     * \param rnti the RNTI of the user
     * \param txMode the transmissionMode of the user
     */
    virtual void SetTransmissionMode(uint16_t rnti, uint8_t txMode) = 0;

    /**
     * Set SRS configuration index
     *
     * \param rnti the RNTI of the user
     * \param srsCi the SRS Configuration Index of the user
     */
    virtual void SetSrsConfigurationIndex(uint16_t rnti, uint16_t srsCi) = 0;

    /**
     * Set master information block
     *
     * \param mib the Master Information Block to be sent on the BCH
     */
    virtual void SetMasterInformationBlock(LteRrcSap::MasterInformationBlock mib) = 0;

    /**
     * Set system information block type 1
     *
     * \param sib1 the System Information Block Type 1 to be sent on the BCH
     */
    virtual void SetSystemInformationBlockType1(LteRrcSap::SystemInformationBlockType1 sib1) = 0;

    /**
     * Get reference signal power
     *
     * \return Reference Signal Power for SIB2
     */
    virtual int8_t GetReferenceSignalPower() = 0;

};

/**
 * Service Access Point (SAP) offered by the UE PHY to the UE RRC for control purposes
 *
 * This is the CPHY SAP User, i.e., the part of the SAP that contains the RRC
 * methods called by the PHY
 */
class NrGnbCphySapUser
{
  public:
    /**
     * Destructor
     */
    virtual ~NrGnbCphySapUser();
};

/**
 * Template for the implementation of the NrGnbCphySapProvider as a member
 * of an owner class of type C to which all methods are forwarded
 */
template <class C>
class MemberNrGnbCphySapProvider : public NrGnbCphySapProvider
{
  public:
    /**
     * Constructor
     *
     * \param owner the owner class
     */
    MemberNrGnbCphySapProvider(C* owner);

    // Delete default constructor to avoid misuse
    MemberNrGnbCphySapProvider() = delete;

    // inherited from NrGnbCphySapProvider
    void SetCellId(uint16_t cellId) override;
    void SetBandwidth(uint16_t ulBandwidth, uint16_t dlBandwidth) override;
    void SetEarfcn(uint32_t ulEarfcn, uint32_t dlEarfcn) override;
    void AddUe(uint16_t rnti) override;
    void RemoveUe(uint16_t rnti) override;
    void SetPa(uint16_t rnti, double pa) override;
    void SetTransmissionMode(uint16_t rnti, uint8_t txMode) override;
    void SetSrsConfigurationIndex(uint16_t rnti, uint16_t srsCi) override;
    void SetMasterInformationBlock(LteRrcSap::MasterInformationBlock mib) override;
    void SetSystemInformationBlockType1(LteRrcSap::SystemInformationBlockType1 sib1) override;
    int8_t GetReferenceSignalPower() override;

  private:
    C* m_owner; ///< the owner class
};

template <class C>
MemberNrGnbCphySapProvider<C>::MemberNrGnbCphySapProvider(C* owner)
    : m_owner(owner)
{
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetCellId(uint16_t cellId)
{
    m_owner->DoSetCellId(cellId);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetBandwidth(uint16_t ulBandwidth, uint16_t dlBandwidth)
{
    m_owner->DoSetBandwidth(ulBandwidth, dlBandwidth);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetEarfcn(uint32_t ulEarfcn, uint32_t dlEarfcn)
{
    m_owner->DoSetEarfcn(ulEarfcn, dlEarfcn);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::AddUe(uint16_t rnti)
{
    m_owner->DoAddUe(rnti);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::RemoveUe(uint16_t rnti)
{
    m_owner->DoRemoveUe(rnti);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetPa(uint16_t rnti, double pa)
{
    m_owner->DoSetPa(rnti, pa);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetTransmissionMode(uint16_t rnti, uint8_t txMode)
{
    m_owner->DoSetTransmissionMode(rnti, txMode);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetSrsConfigurationIndex(uint16_t rnti, uint16_t srsCi)
{
    m_owner->DoSetSrsConfigurationIndex(rnti, srsCi);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetMasterInformationBlock(LteRrcSap::MasterInformationBlock mib)
{
    m_owner->DoSetMasterInformationBlock(mib);
}

template <class C>
void
MemberNrGnbCphySapProvider<C>::SetSystemInformationBlockType1(
    LteRrcSap::SystemInformationBlockType1 sib1)
{
    m_owner->DoSetSystemInformationBlockType1(sib1);
}

template <class C>
int8_t
MemberNrGnbCphySapProvider<C>::GetReferenceSignalPower()
{
    return m_owner->DoGetReferenceSignalPower();
}

/**
 * Template for the implementation of the NrGnbCphySapUser as a member
 * of an owner class of type C to which all methods are forwarded
 */
template <class C>
class MemberNrGnbCphySapUser : public NrGnbCphySapUser
{
  public:
    /**
     * Constructor
     *
     * \param owner the owner class
     */
    MemberNrGnbCphySapUser(C* owner);

    // Delete default constructor to avoid misuse
    MemberNrGnbCphySapUser() = delete;

    // methods inherited from NrGnbCphySapUser go here

  private:
    C* m_owner; ///< the owner class
};

template <class C>
MemberNrGnbCphySapUser<C>::MemberNrGnbCphySapUser(C* owner)
    : m_owner(owner)
{
}

} // namespace ns3

#endif // NR_GNB_CPHY_SAP_H
