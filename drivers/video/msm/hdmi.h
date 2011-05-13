#ifndef HDMI_H
#define HDMI_H

#include <mach/board.h>

//#define HDMI_DEBUG

#if HWVERID_HIGHER(S70, A)
#define HUAWEI_HW_SUPPORT_VERSION_C
#else
#define HUAWEI_HW_SUPPORT_VERSION_B
#endif

int hdmi_startup(void);
int hdmi_shutdown(void);
void hdmi_standby(void);
int hdmi_create_proc_file(void);
int hdmi_get_plug_status(void);
void hdmi_change_dss_drive_strength(void);
int hdmi_device_register(struct msm_panel_info *pinfo);

void hdmi_early_suspend(struct early_suspend *h);
void hdmi_later_resume(struct early_suspend *h);

#endif  // HDMI_H
