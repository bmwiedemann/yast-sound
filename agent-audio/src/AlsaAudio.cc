
#include <math.h>
#include "AlsaAudio.h"

#include <Y2.h>
#include <scr/SCRAgent.h>


#ifdef __sparc__
    #define __HAVE_ALSA 0
#endif
#ifdef __s390__
    #define __HAVE_ALSA 0
#endif

#ifndef __HAVE_ALSA
    #define __HAVE_ALSA 1
#endif

#if __HAVE_ALSA
#include <alsa/asoundlib.h>

#define INIT_MIXER	char card[32];	\
			sprintf(card, "hw:%d", card_id); \
			\
			int err; \
			snd_mixer_t *handle; \
			snd_mixer_selem_id_t *sid; \
			snd_mixer_elem_t *elem; \
			snd_mixer_selem_id_alloca(&sid); \
			\
			if ((err = snd_mixer_open(&handle, 0)) < 0) \
			{ \
				y2error("Mixer %s open error: %s", card, snd_strerror(err)); \
				return YCPNull(); \
			} \
			\
			if ((err = snd_mixer_attach(handle, card)) < 0) \
			{ \
				y2error("Mixer attach %s error: %s", card, snd_strerror(err)); \
				snd_mixer_close(handle); \
				return YCPNull(); \
			} \
			\
			if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) \
			{ \
				y2error("Mixer register error: %s", snd_strerror(err)); \
				snd_mixer_close(handle); \
				return YCPNull(); \
			} \
			\
			err = snd_mixer_load(handle); \
			if (err < 0) \
			{ \
				y2error("Mixer load error: %s %s", card, snd_strerror(err)); \
				snd_mixer_close(handle); \
				return YCPNull(); \
			}


YCPValue alsaGetVolume(int card_id, const string& channel)
{
    INIT_MIXER

    long from, to, value;
    long left;

    snd_mixer_selem_channel_id_t chn;


    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
	snd_mixer_selem_get_id(elem, sid);
	if (snd_mixer_selem_id_get_name(sid) == channel
	    && snd_mixer_selem_is_active(elem)
	    && snd_mixer_selem_has_playback_volume(elem))
	{
	    snd_mixer_selem_get_playback_volume_range(elem, &from, &to);
	    for (chn = (snd_mixer_selem_channel_id_t)0;
                    chn <= SND_MIXER_SCHN_LAST;
                    chn=(snd_mixer_selem_channel_id_t)((int)chn+(snd_mixer_selem_channel_id_t)1))
            {
		if (!snd_mixer_selem_has_playback_channel(elem, chn))
                      continue;
                snd_mixer_selem_get_playback_volume(elem, chn, &left);

		if (to - from == 0)
		{
		    return YCPInteger((long long) 0);
		}
		value = (long long)(100.0 * ((double)(left - from) / (double)(to - from)));
		snd_mixer_close(handle);
		return YCPInteger(value);
            }
	}
    }

    snd_mixer_close(handle);
    return YCPInteger((long long)0);
}

YCPValue alsaGetMute(int card_id, const string& channel)
{
    INIT_MIXER

    int left;

    snd_mixer_selem_channel_id_t chn;

    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
        snd_mixer_selem_get_id(elem, sid);
        if (snd_mixer_selem_id_get_name(sid) == channel
            && snd_mixer_selem_is_active(elem)
            && snd_mixer_selem_has_playback_switch(elem))
        {
            for (chn = (snd_mixer_selem_channel_id_t)0;
                    chn <= SND_MIXER_SCHN_LAST;
                    chn=(snd_mixer_selem_channel_id_t)((int)chn+(snd_mixer_selem_channel_id_t)1))
            {
	//	if (
                snd_mixer_selem_get_playback_switch(elem, chn, &left);

		snd_mixer_close(handle);
                return left ? YCPBoolean(false) : YCPBoolean(true);
            }
        }
    }

    snd_mixer_close(handle);
    return YCPBoolean(false);
}

YCPBoolean alsaSetVolume(int card_id, const string& channel, int value)
{
    INIT_MIXER

    long from, to, val;


    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
        snd_mixer_selem_get_id(elem, sid);
        if (snd_mixer_selem_id_get_name(sid) == channel
            && snd_mixer_selem_is_active(elem)
            && snd_mixer_selem_has_playback_volume(elem))
        {
            snd_mixer_selem_get_playback_volume_range(elem, &from, &to);

	    val = (long)( (double)(value * (to - from)) / 100.0 );

	    snd_mixer_selem_set_playback_volume_all(elem, val);

	    snd_mixer_close(handle);
	    return YCPBoolean(true);
        }
    }


    snd_mixer_close(handle);
    return YCPBoolean(false);
}

YCPBoolean alsaSetMute(int card_id, const string& channel, bool value)
{
    INIT_MIXER


    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
        snd_mixer_selem_get_id(elem, sid);
        if (snd_mixer_selem_id_get_name(sid) == channel
            && snd_mixer_selem_is_active(elem)
            && snd_mixer_selem_has_playback_switch(elem))
        {
	    snd_mixer_selem_set_playback_switch_all(elem, value ? 0 : 1);
	    snd_mixer_close(handle);
	    return YCPBoolean(true);
        }
    }

    snd_mixer_close(handle);
    return YCPBoolean(false);
}

YCPList alsaGetChannels(int card_id)
{
    YCPList outlist;

    INIT_MIXER // well, this doesn't look like a c++ code... i'm sorry for that... see definition above

    for (elem = snd_mixer_first_elem(handle); elem; elem = snd_mixer_elem_next(elem))
    {
	snd_mixer_selem_get_id(elem, sid);
        if (!snd_mixer_selem_is_active(elem))
        {
	    continue;
	}

        if (!snd_mixer_selem_has_playback_volume(elem))
	{
	    continue;
	}

	outlist->add(YCPString(snd_mixer_selem_id_get_name(sid)));

    }

    snd_mixer_close(handle);

    return outlist;
}

YCPList alsaGetCards()
{
    YCPList list;
    char str[4];
    char *dummy;
    for(int i=0; i < 7; i++)
    {
	if (!snd_card_get_name(i, &dummy))
	{
	    sprintf(str, "%d", i);
            list->add(YCPString(str));
	}
    }
    return list;
}

YCPValue alsaStore(int card)
{

    string cmd="/usr/sbin/alsactl store";
    if(card>=0)
    {
	// add card id
	cmd+=" ";
	char tmp[32];
	sprintf(tmp, "%d", card);
	cmd+=tmp;
    }
    cmd+=" > /dev/null 2>&1";
    y2debug("executing '%s'", cmd.c_str());
    if(system(cmd.c_str())!=-1)
    {
	return YCPBoolean(true);
    }
    return YCPBoolean(false);

}

YCPValue alsaRestore(int card)
{

    string cmd="/usr/sbin/alsactl restore";
    if(card>=0)
    {
        // add card id
        cmd+=" ";
	char tmp[32];
	sprintf(tmp, "%d", card);
	cmd+=tmp;
    }
    cmd+=" > /dev/null 2>&1";
    y2debug("executing '%s'", cmd.c_str());
    if(system(cmd.c_str()))
    {
        return YCPBoolean(true);
    }
    return YCPBoolean(false);

}

YCPValue alsaGetCardName(int card_id)
{
    char *cname;

    if (snd_card_get_name(card_id, &cname) != 0)
    {
	return YCPVoid();
    }

    return YCPString(cname);

}

#else // __HAVE_ALSA

YCPValue alsaSetVolume(int card, const string& channel, int value)
{
    return YCPVoid();
}

YCPValue alsaGetVolume(int card, const string& channel)
{
    return YCPVoid();
}

YCPValue alsaSetMute(int card, const string& channel, bool value)
{
    return YCPVoid();
}

YCPValue alsaGetMute(int card, const string& channel)
{
    return YCPVoid();
}

YCPValue alsaGetChannels(int card)
{
    return YCPVoid();
}

YCPValue alsaGetCards()
{
    return YCPVoid();
}

YCPValue alsaGetCardName(int card_id)
{
    return YCPVoid();
}

YCPValue alsaStore(int card)
{
    return YCPVoid();
}

YCPValue alsaRestore(int card)
{
    return YCPVoid();
}

#endif
