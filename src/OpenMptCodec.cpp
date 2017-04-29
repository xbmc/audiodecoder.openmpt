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

#include "libXBMC_addon.h"

extern "C" {
#include <libopenmpt/libopenmpt.h>
#include "kodi_audiodec_dll.h"
#include "AEChannelData.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

//! \brief Create addon
//! \details Called on load. Addon should fully initalize or return error status
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

//! \brief Stop addon
//! \details This dll must cease all runtime activities
void ADDON_Stop()
{
}

//! \brief Destroy addon
//! \details Do everything before unload of this add-on
void ADDON_Destroy()
{
  XBMC=NULL;
}

//! \brief Get status of addon
//! \details Returns the current Status of this audio decoder
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//! \brief Set the value of a given setting
//! \details Set a specific Setting value (called from XBMC)
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

static size_t psf_file_fread( void * handle, void* dst, size_t size)
{
  return XBMC->ReadFile(handle, dst, size);
}

static int psf_file_fseek( void * handle, int64_t offset, int whence )
{
  return XBMC->SeekFile(handle, offset, whence) > -1?0:-1;
}

static int64_t psf_file_ftell( void * handle )
{
  return XBMC->GetFilePosition(handle);
}

#define SET_IF(ptr, value) \
{ \
  if ((ptr)) \
   *(ptr) = (value); \
}

struct MPTContext
{
  openmpt_module* module;
  void* file;
};

//! \brief Initialize an audio decoder for a given file
void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  if (!strFile)
    return NULL;

  MPTContext* result = new MPTContext;
  if (!result)
    return NULL;

  result->file = XBMC->OpenFile(strFile,0);
  if (!result->file)
  {
    delete result;
    return NULL;
  }

  static openmpt_stream_callbacks callbacks = { psf_file_fread, psf_file_fseek, psf_file_ftell };

  result->module = openmpt_module_create(callbacks, result->file, openmpt_log_func_silent, NULL, NULL);
  if (!result->module)
  {
    XBMC->CloseFile(result->file);
    delete result;
    return NULL;
  }

  SET_IF(channels, 2)
  SET_IF(samplerate, 48000)
  SET_IF(bitspersample, 32)
  SET_IF(totaltime,(int64_t)(openmpt_module_get_duration_seconds(result->module)*1000));
  SET_IF(format, AE_FMT_FLOAT)
  static enum AEChannel map[3] = {
    AE_CH_FL, AE_CH_FR, AE_CH_NULL
  };
  SET_IF(channelinfo, map)
  SET_IF(bitrate, openmpt_module_get_num_channels(result->module));

  return result;
}

//! \brief Return decoded data
int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  if (!context)
    return 1;

  MPTContext* ctx = (MPTContext*)context;

  if ((*actualsize = openmpt_module_read_interleaved_float_stereo(ctx->module, 48000, size/8, (float*)pBuffer)*8) == size)
    return 0;
  
  return 1;
}

//! \brief Seek to a given time
int64_t Seek(void* context, int64_t time)
{
  if (!context)
    return -1;

  MPTContext* ctx = (MPTContext*)context;

  return openmpt_module_set_position_seconds(ctx->module, time/1000.0)*1000.0;
}

//! \brief Deinitialize decoder
bool DeInit(void* context)
{
  if (!context)
    return true;

  MPTContext* ctx = (MPTContext*)context;
  openmpt_module_destroy(ctx->module);
  XBMC->CloseFile(ctx->file);

  delete ctx;

  return true;
}

//! \brief Returns any tag values for file
bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  return false;
}

//! \brief Returns track count for a given file
int TrackCount(const char* strFile)
{
  return 1;
}
}
