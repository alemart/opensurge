#ifdef _WIN32

#include <sdkddkver.h>
#undef WINVER
#undef _WIN32_WINNT
#undef NTDDI_VERSION
#define WINVER _WIN32_WINNT_WIN8
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#define NTDDI_VERSION NTDDI_WIN8

#include <allegro5/allegro_windows.h>
#include <processthreadsapi.h>
#include <versionhelpers.h>

static void opt_out_of_power_throttling()
{
    /*

    We'll turn off execution speed throttling (EcoQoS) for the whole process
    See also: https://learn.microsoft.com/en-us/windows/win32/procthread/quality-of-service

    SetProcessInformation() requires Windows 8 or newer (_WIN32_WINNT >= 0x602)
    https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation

    */

    if(!IsWindows8OrGreater())
        return;

    PROCESS_POWER_THROTTLING_STATE power_throttling = { 0 };
    power_throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    
    power_throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    power_throttling.StateMask = 0;

    SetProcessInformation(
        GetCurrentProcess(),
        ProcessPowerThrottling,
        &power_throttling,
        sizeof(power_throttling)
    );
}

void optimize_power_settings()
{
    opt_out_of_power_throttling();
}

#else

void optimize_power_settings()
{
    /* nothing to do */
}

#endif