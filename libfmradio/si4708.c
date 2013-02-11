/*
 * Copyright (C) 2012 Grigori Goronzy <greg@kinoho.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include "utils/Log.h"
#include "hardware/fmradio.h"

#define LOG_TAG "libfmradio_si4708"
#define MAX_SCAN_STATIONS 205

/* RDS group type */
#define RDS_GROUP_TYPE_MASK 0xf000
#define RDS_GROUP_TYPE_SHIFT 12
/* RDS group version (0 = a, 1 = b) */
#define RDS_GROUP_VER_MASK 0x0800
#define RDS_GROUP_VER_SHIFT 11
#define RDS_GROUP_VER_A 0
#define RDS_GROUP_VER_B 1
/* RDS program type */
#define RDS_PTY_MASK 0x03e0
#define RDS_PTY_SHIFT 5
/* RDS program service name segment index */
#define RDS_PSNI_MASK 0x0003
/* RDS radio text segment index */
#define RDS_RTI_MASK 0x000f
/* RDS radio text a/b switch */
#define RDS_RTAB_MASK 0x0010
#define RDS_RTAB_SHIFT 4

/* band */
#define BAND_87500_108000_kHz   0
#define BAND_76000_108000_kHz   1
#define BAND_76000_90000_kHz    2

/* channel spacing */
#define CHAN_SPACING_200_kHz   0        /* US */
#define CHAN_SPACING_100_kHz   1        /* Europe, Japan */
#define CHAN_SPACING_50_kHz    2

/* de-emphasis */
#define FM_DE_TC_50    1
#define FM_DE_TC_75    0

/* seeking */
#define      SEEKUP             1
#define      SEEKDOWN           0

/* magic no */
#define Si4708_IOC_MAGIC  'k'

/* commands */
#define Si4708_IOC_INIT2NORMAL                      _IO(Si4708_IOC_MAGIC, 1)
#define Si4708_IOC_NORMAL2STANDBY                   _IO(Si4708_IOC_MAGIC, 2)
#define Si4708_IOC_STANDBY2NORMAL                   _IO(Si4708_IOC_MAGIC, 3)
#define Si4708_IOC_BAND_SET                         _IOW(Si4708_IOC_MAGIC, 12, int)
#define Si4708_IOC_CHAN_SPACING_SET                 _IOW(Si4708_IOC_MAGIC, 14, int)
#define Si4708_IOC_CHAN_SELECT                      _IOW(Si4708_IOC_MAGIC, 4, int)
#define Si4708_IOC_CHAN_SEEK                        _IOW(Si4708_IOC_MAGIC, 5, int[2])
#define Si4708_IOC_CHAN_GET                         _IOR(Si4708_IOC_MAGIC, 17, int)
#define Si4708_IOC_CUR_RSSI_GET                     _IOR(Si4708_IOC_MAGIC, 12, rssi_snr_t)
#define Si4708_IOC_VOLUME_GET                       _IOR(Si4708_IOC_MAGIC, 7, int)
#define Si4708_IOC_VOLUME_SET                       _IOW(Si4708_IOC_MAGIC, 8, int)
#define Si4708_IOC_MUTE                             _IOW(Si4708_IOC_MAGIC, 9, int)
#define Si4708_IOC_SET_RDS                          _IOW(Si4708_IOC_MAGIC, 23, int)
#define Si4708_IOC_GET_RDS                          _IOR(Si4708_IOC_MAGIC, 25, int[4])
#define Si4708_IOC_DE_SET                           _IOW(Si4708_IOC_MAGIC, 32,int)
#define Si4708_IOC_SET_AUDIOTRACK                   _IOW(Si4708_IOC_MAGIC, 16, int)

/* state */

struct si4708_session {
    int fd;
    bool radioInitialised;
    bool radioEnabled;
    int defaultFreq;
    int lastFreq;
    int lastVolume;
    const struct fmradio_vendor_callbacks_t *cb;

    /* RDS */
    pthread_t rds_thread;
    bool rds_running;
    bool rds_reset;
    int  rds_last_rtab;
    struct fmradio_rds_bundle_t rds_stage;
    struct fmradio_rds_bundle_t rds_submit;
};

/* helpers */

static int radioOn(struct si4708_session *priv)
{
    int ret;

    ALOGV("%s: enabling radio", __func__);

    if (priv->radioEnabled) {
        return 0;
    }

    if (!priv->radioInitialised) {
      ret = ioctl(priv->fd, Si4708_IOC_INIT2NORMAL);
      priv->radioInitialised = true;
    } else {
      ret = ioctl(priv->fd, Si4708_IOC_STANDBY2NORMAL);
    }

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_INIT2NORMAL failed %d", __func__, ret);
        return -1;
    }

    priv->radioEnabled = true;

    ALOGD("FMRadio on");
    return 0;
}

static int radioOff(struct si4708_session *priv)
{
    int ret;

    ALOGD("%s: disabling radio radioEnabled=%i", __func__, priv->radioEnabled);

    ret = ioctl(priv->fd, Si4708_IOC_NORMAL2STANDBY);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_NORMAL2STANDBY failed %d", __func__, ret);
        return -1;
    }

    priv->radioEnabled = false;
    ALOGD("FMRadio off");

    return 0;
}

static int setFreq(struct si4708_session *priv, int freq)
{
    int ret;

    ALOGI("setFreq freq=%d", freq);

    freq = freq / 10;
    ret = ioctl(priv->fd, Si4708_IOC_CHAN_SELECT, &freq);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_CHAN_SELECT failed %d", __func__, ret);
        return -1;
    }

    priv->lastFreq = freq * 10;
    priv->rds_reset = true;
    return 0;
}

static int setFreqSpacing(struct si4708_session *priv, int spacing)
{
    int nativeSpacing, ret;

    ALOGI("setFreqSpacing spacing=%d", spacing);

    switch (spacing) {
        case 200:
            nativeSpacing = CHAN_SPACING_200_kHz;
            break;
        case 100:
            nativeSpacing = CHAN_SPACING_100_kHz;
            break;
        case 50:
            nativeSpacing = CHAN_SPACING_50_kHz;
            break;
        default:
            ALOGE("%s : ERROR invalid Freqency spacing %d", __func__, spacing);
            return -1;
    }

    ALOGV("%s: spacing is %d", __func__, nativeSpacing);

    ret = ioctl(priv->fd, Si4708_IOC_CHAN_SPACING_SET, &nativeSpacing);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_CHAN_SPACING_SET failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

static int setMute(struct si4708_session *priv, int mute)
{
    int ret;

    ALOGI("%s: setting mute %d", __func__, mute);

    ret = ioctl(priv->fd, Si4708_IOC_MUTE, &mute);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_MUTE failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

static int setDeemphasis(struct si4708_session *priv, int de)
{
    int ret;

    ALOGI("%s: setting TC %d", __func__, de);

    ret = ioctl(priv->fd, Si4708_IOC_DE_SET, &de);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_DE_SET failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

static int setBand(struct si4708_session *priv, int low, int high)
{
    int ret;
    int spacing, de, band;

    ALOGI("%s", __func__);

    if (low == 76000 && high == 90000) /* Japan */
        band = BAND_76000_90000_kHz;
    else if ((low == 87500 || low == 87900) /* EU, USA */
            && (high == 107900 || high == 108000))
        band = BAND_87500_108000_kHz;
    else
        band = BAND_76000_108000_kHz;

    ALOGI("%s: Setting band %d", __func__, band);

    ret = ioctl(priv->fd, Si4708_IOC_BAND_SET, &band);

    /* Everyone except the US uses 50us de-emphasis */
    setDeemphasis(priv, (low == 87900) ? FM_DE_TC_75 : FM_DE_TC_50);

    if (ret != 0) {
        ALOGE("%s: IOCTL Si4708_IOC_BAND_SET failed %d", __func__, ret);
        return FMRADIO_IO_ERROR;
    }

    return FMRADIO_OK;
}

/* STE FM API implementation */

static int
si4708_rx_start(void **session_data,
                const struct fmradio_vendor_callbacks_t *callbacks,
                int low_freq, int high_freq, int default_freq, int grid)
{
    int res = 0;
    struct si4708_session *priv;
    *session_data = priv = calloc(sizeof(struct si4708_session), 1);

    ALOGI("rx_start low_freq=%d high_freq=%d default_freq=%d grid=%d",
            low_freq, high_freq, default_freq, grid);

    priv->cb = callbacks;
    priv->defaultFreq = default_freq;
    priv->fd = open("/dev/si4708", O_RDWR);

    res |= radioOn(priv);
    res |= setBand(priv, low_freq, high_freq);
    res |= setFreqSpacing(priv, grid);
    res |= setFreq(priv, default_freq);
    res |= setMute(priv, 0);

    return res;
}

static int
si4708_pause(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    ALOGI("pause");

    return setMute(priv, 1);
}

static int
si4708_resume(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    ALOGI("resume");

    return setMute(priv, 0);
}

static int
si4708_set_frequency(void **session_data, int frequency)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    ALOGI("set_frequency frequency=%d", frequency);

    return setFreq(priv, frequency);
}

static int
si4708_get_frequency(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;

    ALOGI("get_frequency frequency=%d", priv->lastFreq);

    return priv->lastFreq;
}

static int
si4708_scan(void **session_data, enum fmradio_seek_direction_t dir)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int val[2];
    int retval;

    ALOGI("scan %d", dir);

    if (dir == FMRADIO_SEEK_DOWN) {
        val[0] = SEEKDOWN;
        retval = ioctl(priv->fd, Si4708_IOC_CHAN_SEEK, val);
    } else {
        val[0] = SEEKUP;
        retval = ioctl(priv->fd, Si4708_IOC_CHAN_SEEK, val);
    }

    if (retval != 0) {
        ALOGE("Search failed");
        return priv->lastFreq;
    }

    ALOGI("%s: freq=%d", __func__, val[1]);
    priv->lastFreq = val[1] * 10;
    priv->rds_reset = true;
    return priv->lastFreq;
}

static int
si4708_set_force_mono(void **session_data, int force_mono)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int ret;

    ALOGI("%s: setting force mono %d", __func__, force_mono);

    if (force_mono == 0) {
        int stereo = 0;
        ret = ioctl(priv->fd, Si4708_IOC_SET_AUDIOTRACK, &stereo);

        if (ret != 0) {
            ALOGE("%s: IOCTL Si4708_IOC_STEREO_SET failed %d", __func__, ret);
            return FMRADIO_IO_ERROR;
        }
    } else {
        int mono = 1;
        ret = ioctl(priv->fd, Si4708_IOC_SET_AUDIOTRACK, &mono);

        if (ret != 0) {
            ALOGE("%s: IOCTL Si4708_IOC_MONO_SET failed %d", __func__, ret);
            return FMRADIO_IO_ERROR;
        }
    }

    return FMRADIO_OK;
}

static int
si4708_full_scan(void **session_data, int **found_freqs,
                 int **signal_strengths)
{
#if 0
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int i;
    int lastFreq = priv->lastFreq;

    *found_freqs      = calloc(MAX_SCAN_STATIONS, sizeof(int));
    *signal_strengths = calloc(MAX_SCAN_STATIONS, sizeof(int));

    setMute(priv, 1);
    setFreq(priv, priv->defaultFreq);

    for (i = 0; i < MAX_SCAN_STATIONS; i++) {
        int found = si4708_scan(session_data, FMRADIO_SEEK_UP);
        if (found <= 0 || found <= lastFreq)
            break;
        (*found_freqs)[i] = found;
        (*signal_strengths)[i] = 75;
        lastFreq = found;
    }

    setFreq(priv, priv->defaultFreq);
    setMute(priv, 0);

    return i-1;
#endif
    return FMRADIO_UNSUPPORTED_OPERATION;
}

static int
si4708_stop_scan(void **session_data)
{
    return FMRADIO_OK;
}

static int
si4708_rds_supported(void **session_data)
{
    return true;
}

static bool
si4708_check_rt(const char *str)
{
    int len = strlen(str);
    int i;

    for (i = len - 1; i > 0; i--) {
        /* 0x0d or 0x0a signals the end of the message */
        if (str[i] == 0x0d || str[i] == 0x0a)
            return true;
        /* if there are at least 3 trailing spaces, assume message is ended */
        if (i <= (len - 3))
            return true;
        /* some stations have trailing spaces, so skip those */
        if (str[i] <= ' ')
            continue;
        return false;
    }

    return false;
}

static void
si4708_process_rds_data(struct si4708_session *priv, int rds_data[4])
{
    int type = (rds_data[1] & RDS_GROUP_TYPE_MASK) >> RDS_GROUP_TYPE_SHIFT;
    int version = (rds_data[1] & RDS_GROUP_VER_MASK) >> RDS_GROUP_VER_SHIFT;

    //ALOGI("got RDS: %02x %02x %02x %02x", rds_data[0], rds_data[1],
    //        rds_data[2], rds_data[3]);

    /* group type 0a/0b for PI, PTY and PSN */
    if (type == 0) {
        int psni = rds_data[1] & RDS_PSNI_MASK;

        /* submit PI and PTY */
        priv->rds_submit.pty = (rds_data[1] & RDS_PTY_MASK) >> RDS_PTY_SHIFT;
        priv->rds_submit.pi  = rds_data[0];

        /* stage two characters of text data */
        priv->rds_stage.psn[psni*2]   = (rds_data[3] & 0x7f00) >> 8;
        priv->rds_stage.psn[psni*2+1] = rds_data[3] & 0x007f;
        priv->rds_stage.psn[RDS_PSN_MAX_LENGTH] = 0;

        /* check if psn is complete */
        if (strlen(priv->rds_stage.psn) == RDS_PSN_MAX_LENGTH) {
            memcpy(priv->rds_submit.psn, priv->rds_stage.psn,
                    RDS_PSN_MAX_LENGTH);
            memset(priv->rds_stage.psn, 0, RDS_PSN_MAX_LENGTH);
            priv->cb->on_rds_data_found(&priv->rds_submit, priv->lastFreq);
        }
        return;
    }

    /* group type 2a for RT */
    if (type == 2 && version == RDS_GROUP_VER_A) {
        int rti = (rds_data[1] & RDS_RTI_MASK);
        int rtab = (rds_data[1] & RDS_RTAB_MASK) >> RDS_RTAB_SHIFT;

        /* a/b switch toggled, this means a new message started */
        if (rtab != priv->rds_last_rtab) {
            memset(priv->rds_stage.rt, 0, RDS_RT_MAX_LENGTH);
            priv->rds_last_rtab = rtab;
        }

        /* write 4 characters of text data */
        priv->rds_stage.rt[rti*4]   = (rds_data[2] & 0x7f00) >> 8;
        priv->rds_stage.rt[rti*4+1] = rds_data[2] & 0x007f;
        priv->rds_stage.rt[rti*4+2] = (rds_data[3] & 0x7f00) >> 8;
        priv->rds_stage.rt[rti*4+3] = rds_data[3] & 0x007f;
        priv->rds_stage.rt[RDS_RT_MAX_LENGTH] = 0;

        /* check if text is complete (with some heuristics) */
        if ((rti == 15 && strlen(priv->rds_stage.rt) > 60) ||
                si4708_check_rt(priv->rds_stage.rt)) {
            memcpy(priv->rds_submit.rt, priv->rds_stage.rt, RDS_RT_MAX_LENGTH);
            memset(priv->rds_stage.rt, 0, RDS_RT_MAX_LENGTH);
            priv->cb->on_rds_data_found(&priv->rds_submit, priv->lastFreq);
        }
        return;
    }

    //ALOGI("rds unknown group type %d%c", type, version ? 'b' : 'a');
}

static void *
si4708_rds_thread_loop(void *arg)
{
    struct si4708_session *priv = (struct si4708_session *)arg;
    int res;
    int rds_data[4];
    struct timespec delay = { 0, 40000000 };

    while (priv->rds_running) {
        nanosleep(&delay, NULL);
        res = ioctl(priv->fd, Si4708_IOC_GET_RDS, &rds_data);

        /* no RDS data ready, retry later */
        if (res < 0 && errno == EAGAIN)
            continue;

        /* something went wrong */
        if (res < 0)
            break;

        /* rds reset was requested (after frequency change) */
        if (priv->rds_reset) {
            priv->rds_reset = false;
            memset(&priv->rds_stage, 0, sizeof(struct fmradio_rds_bundle_t));
            memset(&priv->rds_submit, 0, sizeof(struct fmradio_rds_bundle_t));
        }

        if (priv->rds_running)
            si4708_process_rds_data(priv, rds_data);
    }

    ALOGI("%s: exiting", __func__);

    return NULL;
}

static int
si4708_set_rds_reception(void **session_data, int use_rds)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int res;

    if (use_rds && priv->rds_thread == 0) {
        int toggle = 1;
        pthread_attr_t attr;

        res = ioctl(priv->fd, Si4708_IOC_SET_RDS, &toggle);
        if (res < 0) {
            ALOGI("Si4708_IOC_SET_RDS error: %d", res);
            return FMRADIO_IO_ERROR;
        }

        priv->rds_running = true;
        priv->rds_reset   = true;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        res = pthread_create(&priv->rds_thread, &attr, si4708_rds_thread_loop,
                (void *)priv);
        if (res < 0) {
            int toggle = 0;
            res = ioctl(priv->fd, Si4708_IOC_SET_RDS, &toggle);
            pthread_attr_destroy(&attr);
            priv->rds_thread = 0;
            return FMRADIO_IO_ERROR;
        }

        pthread_attr_destroy(&attr);
        ALOGI("enabled RDS");
        return FMRADIO_OK;
    } else if (!use_rds && priv->rds_thread) {
        int toggle = 0;

        priv->rds_running = false;
        res = pthread_join(priv->rds_thread, NULL);
        priv->rds_thread = 0;

        if (res < 0)
            return FMRADIO_IO_ERROR;

        res = ioctl(priv->fd, Si4708_IOC_SET_RDS, &toggle);
        if (res < 0) {
            ALOGI("Si4708_IOC_SET_RDS error: %d", res);
            return FMRADIO_IO_ERROR;
        }

        ALOGI("disabled RDS");
        return FMRADIO_OK;
    }

    return FMRADIO_OK;
}

static int
si4708_reset(void **session_data)
{
    struct si4708_session *priv = (struct si4708_session *)*session_data;
    int ret = 0;

    ALOGI("reset");

    if (priv) {
        ret |= si4708_set_rds_reception(session_data, 0);
        ret |= radioOff(priv);
        close(priv->fd);
        free(priv);
        *session_data = 0;
    }

    return ret;
}

int register_fmradio_functions(unsigned int *sig,
                               struct fmradio_vendor_methods_t *funcs)
{
    memset(funcs, 0, sizeof(struct fmradio_vendor_methods_t));

    funcs->rx_start = si4708_rx_start;
    funcs->pause = si4708_pause;
    funcs->resume = si4708_resume;
    funcs->reset = si4708_reset;
    funcs->set_frequency = si4708_set_frequency;
    funcs->get_frequency = si4708_get_frequency;
    funcs->scan = si4708_scan;
    funcs->stop_scan = si4708_stop_scan;
    funcs->set_force_mono = si4708_set_force_mono;
    funcs->full_scan = si4708_full_scan;
    funcs->is_rds_data_supported = si4708_rds_supported;
    funcs->set_rds_reception = si4708_set_rds_reception;

    *sig = FMRADIO_SIGNATURE;
    return FMRADIO_OK;
}

