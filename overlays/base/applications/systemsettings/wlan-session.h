/*
 * PROJECT:     LiberNT Settings
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Own a WLAN session and preserve operation errors
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#include <windows.h>
#include <wlanapi.h>

class WlanSession
{
public:
    WlanSession() : m_Handle(NULL), m_Interfaces(NULL), m_Result(ERROR_SUCCESS)
    {
        DWORD version = 0;

        m_Result = WlanOpenHandle(2, NULL, &version, &m_Handle);
        if (m_Result != ERROR_SUCCESS)
            m_Handle = NULL;
        else
        {
            m_Result = WlanEnumInterfaces(m_Handle, NULL, &m_Interfaces);
            if (m_Result != ERROR_SUCCESS)
                m_Interfaces = NULL;
        }
    }

    ~WlanSession()
    {
        if (m_Interfaces)
            WlanFreeMemory(m_Interfaces);
        if (m_Handle)
            WlanCloseHandle(m_Handle, NULL);
    }

    bool Available() const { return m_Interfaces && m_Interfaces->dwNumberOfItems; }
    DWORD Result() const { return m_Result; }
    HANDLE Handle() const { return m_Handle; }
    const GUID *Interface() const { return Available() ? &m_Interfaces->InterfaceInfo[0].InterfaceGuid : NULL; }
    WLAN_INTERFACE_STATE State() const
    {
        return Available() ? m_Interfaces->InterfaceInfo[0].isState : wlan_interface_state_not_ready;
    }

    bool RadioOn()
    {
        PWLAN_RADIO_STATE state = NULL;
        DWORD size = 0;
        bool on = false;

        if (!Available())
            return false;
        m_Result = WlanQueryInterface(m_Handle, Interface(), wlan_intf_opcode_radio_state, NULL, &size,
                                      (PVOID *)&state, NULL);
        if (m_Result != ERROR_SUCCESS)
            return false;
        if (!state || size < sizeof(*state))
        {
            if (state) WlanFreeMemory(state);
            m_Result = ERROR_INVALID_DATA;
            return false;
        }
        for (DWORD i = 0; i < state->dwNumberOfPhys && i < WLAN_MAX_PHY_INDEX; i++)
        {
            if (state->PhyRadioState[i].dot11SoftwareRadioState == dot11_radio_state_on &&
                state->PhyRadioState[i].dot11HardwareRadioState == dot11_radio_state_on)
                on = true;
        }
        WlanFreeMemory(state);
        return on;
    }

    DWORD SetRadio(bool on)
    {
        WLAN_PHY_RADIO_STATE state;

        if (!Available())
            return m_Result != ERROR_SUCCESS ? m_Result : ERROR_NOT_FOUND;
        ZeroMemory(&state, sizeof(state));
        state.dot11SoftwareRadioState = on ? dot11_radio_state_on : dot11_radio_state_off;
        state.dot11HardwareRadioState = dot11_radio_state_on;
        m_Result = WlanSetInterface(m_Handle, Interface(), wlan_intf_opcode_radio_state, sizeof(state), &state, NULL);
        return m_Result;
    }

private:
    HANDLE m_Handle;
    PWLAN_INTERFACE_INFO_LIST m_Interfaces;
    DWORD m_Result;
};
