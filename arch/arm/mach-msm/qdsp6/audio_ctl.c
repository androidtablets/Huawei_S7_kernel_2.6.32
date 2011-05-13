/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 HTC Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/msm_audio.h>

#include <mach/msm_qdsp6_audio.h>
#include <mach/debug_mm.h>

#include <linux/gpio.h>

#define BUFSZ (0)

static DEFINE_MUTEX(voice_lock);
static int voice_started;

static struct audio_client *voc_tx_clnt;
static struct audio_client *voc_rx_clnt;

static int q6_voice_start(void)
{
	int rc = 0;

	mutex_lock(&voice_lock);

	if (voice_started) {
		pr_err("[%s:%s] busy\n", __MM_FILE__, __func__);
		rc = -EBUSY;
		goto done;
	}

	voc_tx_clnt = q6voice_open(AUDIO_FLAG_WRITE);
	if (!voc_tx_clnt) {
		pr_err("[%s:%s] open voice tx failed.\n", __MM_FILE__,
				__func__);
		rc = -ENOMEM;
		goto done;
	}

	voc_rx_clnt = q6voice_open(AUDIO_FLAG_READ);
	if (!voc_rx_clnt) {
		pr_err("[%s:%s] open voice rx failed.\n", __MM_FILE__,
				__func__);
		q6voice_close(voc_tx_clnt);
		rc = -ENOMEM;
	}

	voice_started = 1;
done:
	mutex_unlock(&voice_lock);
	return rc;
}

static int q6_voice_stop(void)
{
	mutex_lock(&voice_lock);
	if (voice_started) {
		q6voice_close(voc_tx_clnt);
		q6voice_close(voc_rx_clnt);
		voice_started = 0;
	}
	mutex_unlock(&voice_lock);
	return 0;
}

static int q6_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int q6_ioctl(struct inode *inode, struct file *file,
		    unsigned int cmd, unsigned long arg)
{
	int rc;
	uint32_t n;
	uint32_t id[2];
	uint32_t mute_status;

	switch (cmd) {
	case AUDIO_SWITCH_DEVICE:
		rc = copy_from_user(&id, (void *)arg, sizeof(id));
		if (!rc)
			rc = q6audio_do_routing(id[0], id[1]);
		break;
	case AUDIO_SET_VOLUME:
		rc = copy_from_user(&n, (void *)arg, sizeof(n));
		if (!rc)
			rc = q6audio_set_rx_volume(n);
		break;
	case AUDIO_SET_MUTE:
		rc = copy_from_user(&n, (void *)arg, sizeof(n));
		if (!rc) {
			if (voice_started) {
				if (n == 1)
					mute_status = STREAM_MUTE;
				else
					mute_status = STREAM_UNMUTE;
			} else {
				if (n == 1)
					mute_status = DEVICE_MUTE;
				else
					mute_status = DEVICE_UNMUTE;
			}

			rc = q6audio_set_tx_mute(mute_status);
		}
		break;
	case AUDIO_UPDATE_ACDB:
		rc = copy_from_user(&id, (void *)arg, sizeof(id));
		if (!rc)
			rc = q6audio_update_acdb(id[0], 0);
		break;
	case AUDIO_START_VOICE:
		rc = q6_voice_start();
		break;
	case AUDIO_STOP_VOICE:
		rc = q6_voice_stop();
		break;
	case AUDIO_REINIT_ACDB:
		rc = 0;
		break;
	default:
		rc = -EINVAL;
	}

	return rc;
}


static int q6_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations q6_dev_fops = {
	.owner		= THIS_MODULE,
	.open		= q6_open,
	.ioctl		= q6_ioctl,
	.release	= q6_release,
};

struct miscdevice q6_control_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_audio_ctl",
	.fops	= &q6_dev_fops,
};

static unsigned int gSpeakerEnFlag = 0;
#define BSP_GPIO_SPK_PWN 19
static ssize_t speaker_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
 {
     return sprintf(buf, "%d", gSpeakerEnFlag);
 }

static ssize_t speaker_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
    int iret = 0;
    
    printk(KERN_ERR "speaker_attr_store\n");
    
    if(NULL != buf) {   
        iret = sscanf(buf,"%d", &gSpeakerEnFlag);
    }

    if(-1 == iret) {
        printk(KERN_ERR "sscanf gSpeakerEnFlag error\n");
        return 0;
    } else {
        gpio_set_value(BSP_GPIO_SPK_PWN, gSpeakerEnFlag);
        return count;
    }
}

/* Define the device attributes */
 static struct kobj_attribute speaker_attribute =
         __ATTR(switch, 0777, speaker_attr_show, speaker_attr_store);

 static struct attribute* speaker_attributes[] =
 {
         &speaker_attribute.attr,
         NULL
 };

 static struct attribute_group speaker_defattr_group =
 {
         .attrs = speaker_attributes,
 };


static int __init q6_audio_ctl_init(void) 
{
    struct kobject *kobj = NULL;
    
    if (misc_register(&q6_control_device) < 0)
        return -1;
    
    kobj = kobject_create_and_add("speaker_en", NULL);
    if (kobj == NULL) 
    {
        return -1;
    }
    if (sysfs_create_group(kobj, &speaker_defattr_group)) 
    {
        kobject_put(kobj);
        return -1;
    }

    return 0;
}

device_initcall(q6_audio_ctl_init);
