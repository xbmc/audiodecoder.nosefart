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

#include "libXBMC_addon.h"

extern "C" {
#include "types.h"
#include "log.h"
#include "version.h"
#include "machine/nsf.h"
#include "kodi_audiodec_dll.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct NSFContext
{
  nsf_t* module;
  uint8_t* buffer;
  uint8_t* head;
  size_t len;
  size_t pos;
  size_t track;
};

#define SET_IF(ptr, value) \
{ \
  if ((ptr)) \
   *(ptr) = (value); \
}

static nsf_t* LoadNSF(const std::string& toLoad)
{
  nsf_init();
  log_init();
  void* file = XBMC->OpenFile(toLoad.c_str(),0);
  if (!file)
    return NULL;

  int len = XBMC->GetFileLength(file);
  char *data = new char[len];
  if (!data)
  {
    XBMC->CloseFile(file);
    return NULL;
  }
  XBMC->ReadFile(file, data, len);
  XBMC->CloseFile(file);

  // Now load the module
  nsf_t* result = nsf_load(NULL,data,len);
  delete[] data;

  return result;
}

void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  if (!strFile)
    return NULL;

  int track=0;
  std::string toLoad(strFile);
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

  NSFContext* result = new NSFContext;
  if (!result)
    return NULL;

  result->module = LoadNSF(toLoad);

  if (!result->module)
  {
    delete result;
    return NULL;
  }

  nsf_playtrack(result->module, track, 48000, 16, false);
  for (int i = 0; i < 6; i++)
    nsf_setchan(result->module,i,true);

  result->head = result->buffer = new uint8_t[2*48000/result->module->playback_rate];
  if (!result->buffer)
  {
    delete result;
    return NULL;
  }
  result->len = result->pos = 0;
  result->track = track;

  SET_IF(channels, 1)
  SET_IF(samplerate, 48000)
  SET_IF(bitspersample, 16)
  SET_IF(totaltime, 4*60*1000)
  SET_IF(format, AE_FMT_S16NE)
  SET_IF(bitrate, 0)
  static enum AEChannel map[2] = {
    AE_CH_FC, AE_CH_NULL
  };
  SET_IF(channelinfo, map)


  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  if (!context || !pBuffer || !actualsize)
    return 1;

  NSFContext* ctx = (NSFContext*)context;

  *actualsize = 0;
  while (size)
  {
    if (!ctx->len)
    {
      nsf_frame(ctx->module);
      ctx->module->process(ctx->buffer, 48000/ctx->module->playback_rate);
      ctx->len = 2*48000/ctx->module->playback_rate;
      ctx->head = ctx->buffer;
    }
    size_t tocopy = std::min(ctx->len, size_t(size));
    memcpy(pBuffer, ctx->head, tocopy);
    ctx->head += tocopy;
    ctx->len -= tocopy;
    ctx->pos += tocopy;
    *actualsize += tocopy;
    pBuffer += tocopy;
    size -= tocopy;
  }

  return size != 0;
}

int64_t Seek(void* context, int64_t time)
{
  if (!context)
    return 1;

  NSFContext* ctx = (NSFContext*)context;
  if (ctx->pos > time/1000*48000*2)
  {
    ctx->pos = 0;
    ctx->len = 0;
  }
  while (ctx->pos+2*48000/ctx->module->playback_rate < time/1000*48000*2)
  {
    nsf_frame(ctx->module);
    ctx->pos += 2*48000/ctx->module->playback_rate;
  }

  ctx->module->process(ctx->buffer, 2*48000/ctx->module->playback_rate);
  ctx->len = 2*48000/ctx->module->playback_rate-(time/1000*48000*2-ctx->pos);
  ctx->head = ctx->buffer+2*48000/ctx->module->playback_rate-ctx->len;
  ctx->pos += ctx->head-ctx->buffer;

  return time;
}

bool DeInit(void* context)
{
  if (!context)
    return 1;

  NSFContext* ctx = (NSFContext*)context;
  nsf_free(&ctx->module);
  delete ctx->buffer;
  delete ctx;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  if (!strFile || !title || !artist || !length)
    return false;

  nsf_t*  module = LoadNSF(strFile);
  if (module)
  {
    strcpy(title, (const char*)module->song_name);
    strcpy(artist, (const char*)module->artist_name);
    *length = 4*60;
    nsf_free(&module);

    return true;
  }

  return false;
}

int TrackCount(const char* strFile)
{
  nsf_t* module = LoadNSF(strFile);
  int result=0;
  if (module)
  {
    result = module->num_songs;
    nsf_free(&module);
  }

  return result;
}
}
