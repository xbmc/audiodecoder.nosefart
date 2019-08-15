/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <algorithm>
#include <string>

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>

extern "C" {
#include "types.h"
#include "log.h"
#include "version.h"
#include "machine/nsf.h"
}

struct NSFContext
{
  nsf_t* module = nullptr;
  uint8_t* buffer = nullptr;
  uint8_t* head;
  size_t len;
  size_t pos;
  size_t track;
};

static nsf_t* LoadNSF(const std::string& toLoad)
{
  nsf_init();
  log_init();
  kodi::vfs::CFile file;
  if (!file.OpenFile(toLoad, 0))
    return nullptr;

  int len = file.GetLength();
  char *data = new char[len];
  if (!data)
  {
    file.Close();
    return nullptr;
  }
  file.Read(data, len);
  file.Close();

  // Now load the module
  nsf_t* result = nsf_load(nullptr,data,len);
  delete[] data;

  return result;
}

class ATTRIBUTE_HIDDEN CNSFCodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CNSFCodec(KODI_HANDLE instance) :
    CInstanceAudioDecoder(instance) {}

  virtual ~CNSFCodec()
  {
    if (ctx.module)
      nsf_free(&ctx.module);
    delete[] ctx.buffer;
  }

  virtual bool Init(const std::string& filename, unsigned int filecache,
                    int& channels, int& samplerate,
                    int& bitspersample, int64_t& totaltime,
                    int& bitrate, AEDataFormat& format,
                    std::vector<AEChannel>& channellist) override
  {
    int track=0;
    std::string toLoad(filename);
    if (toLoad.find(".nsfstream") != std::string::npos)
    {
      size_t iStart=toLoad.rfind('-') + 1;
      track = atoi(toLoad.substr(iStart, toLoad.size()-iStart-10).c_str());
      //  The directory we are in, is the file
      //  that contains the bitstream to play,
      //  so extract it
      size_t slash = toLoad.rfind('\\');
      if (slash == std::string::npos)
        slash = toLoad.rfind('/');
      toLoad = toLoad.substr(0, slash);
    }

    ctx.module = LoadNSF(toLoad);

    if (!ctx.module)
    {
      return false;
    }

    nsf_playtrack(ctx.module, track, 48000, 16, false);
    for (int i = 0; i < 6; i++)
      nsf_setchan(ctx.module,i,true);

    ctx.head = ctx.buffer = new uint8_t[2*48000/ctx.module->playback_rate];
    if (!ctx.buffer)
    {
      nsf_free(&ctx.module);
      return false;
    }
    ctx.len = ctx.pos = 0;
    ctx.track = track;

    channels = 1;
    samplerate = 48000;
    bitspersample = 16;
    totaltime = 4*60*1000;
    format = AE_FMT_S16NE;
    bitrate = 0;
    channellist = { AE_CH_FC };
    return true;
  }

  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if (!buffer)
      return 1;

    actualsize = 0;
    while (size)
    {
      if (!ctx.len)
      {
        nsf_frame(ctx.module);
        ctx.module->process(ctx.buffer, 48000/ctx.module->playback_rate);
        ctx.len = 2*48000/ctx.module->playback_rate;
        ctx.head = ctx.buffer;
      }
      size_t tocopy = std::min(ctx.len, size_t(size));
      memcpy(buffer, ctx.head, tocopy);
      ctx.head += tocopy;
      ctx.len -= tocopy;
      ctx.pos += tocopy;
      actualsize += tocopy;
      buffer += tocopy;
      size -= tocopy;
    }

    return size != 0;
  }

  virtual int64_t Seek(int64_t time) override
  {
    if (ctx.pos > time/1000*48000*2)
    {
      ctx.pos = 0;
      ctx.len = 0;
    }
    while (ctx.pos+2*48000/ctx.module->playback_rate < time/1000*48000*2)
    {
      nsf_frame(ctx.module);
      ctx.pos += 2*48000/ctx.module->playback_rate;
    }

    ctx.module->process(ctx.buffer, 2*48000/ctx.module->playback_rate);
    ctx.len = 2*48000/ctx.module->playback_rate-(time/1000*48000*2-ctx.pos);
    ctx.head = ctx.buffer+2*48000/ctx.module->playback_rate-ctx.len;
    ctx.pos += ctx.head-ctx.buffer;

    return time;
  }

  virtual bool ReadTag(const std::string& file, std::string& title,
                       std::string& artist, int& length) override
  {
    nsf_t*  module = LoadNSF(file);
    if (module)
    {
      title = (const char*)module->song_name;
      artist = (const char*)module->artist_name;
      length = 4*60;
      nsf_free(&module);

      return true;
    }

    return false;
  }

  virtual int TrackCount(const std::string& fileName) override
  {
    nsf_t* module = LoadNSF(fileName);
    int result=0;
    if (module)
    {
      result = module->num_songs;
      nsf_free(&module);
    }

    return result;
  }

private:
  NSFContext ctx;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CNSFCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon()
  {
  }
};


ADDONCREATOR(CMyAddon);
