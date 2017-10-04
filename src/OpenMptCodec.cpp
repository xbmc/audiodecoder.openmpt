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

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>

#include <libopenmpt/libopenmpt.h>

static size_t vfs_file_fread( void * handle, void* dst, size_t size)
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->Read(dst, size);
}

static int vfs_file_fseek( void * handle, int64_t offset, int whence )
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->Seek(offset, whence) > -1 ? 0 : -1;
}

static int64_t vfs_file_ftell( void * handle )
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->GetPosition();
}

struct MPTContext
{
  openmpt_module* module = nullptr;
  kodi::vfs::CFile file;
};

class CMPTCodec : public kodi::addon::CInstanceAudioDecoder,
                  public kodi::addon::CAddonBase
{
public:
  CMPTCodec(KODI_HANDLE instance) :
    CInstanceAudioDecoder(instance) {}

  virtual ~CMPTCodec()
  {
    if (ctx.module)
      openmpt_module_destroy(ctx.module);
  }

  virtual bool Init(const std::string& filename, unsigned int filecache,
                    int& channels, int& samplerate,
                    int& bitspersample, int64_t& totaltime,
                    int& bitrate, AEDataFormat& format,
                    std::vector<AEChannel>& channellist) override
  {
    if (!ctx.file.OpenFile(filename,READ_CACHED))
      return false;

    static openmpt_stream_callbacks callbacks = { vfs_file_fread, vfs_file_fseek, vfs_file_ftell };

    ctx.module = openmpt_module_create2(callbacks, &ctx.file, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!ctx.module)
      return false;

    channels = 2;
    samplerate = 48000;
    bitspersample = 32;
    totaltime = openmpt_module_get_duration_seconds(ctx.module)*1000;
    format = AE_FMT_FLOAT;
    channellist = { AE_CH_FL, AE_CH_FR };
    bitrate = openmpt_module_get_num_channels(ctx.module);

    return true;
  }

  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if ((actualsize = openmpt_module_read_interleaved_float_stereo(ctx.module, 48000, size/8, (float*)buffer)*8) == size)
      return 0;

    return 1;
  }

  virtual int64_t Seek(int64_t time) override
  {
    return openmpt_module_set_position_seconds(ctx.module, time/1000.0)*1000.0;
  }

private:
  MPTContext ctx;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CMPTCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon()
  {
  }
};


ADDONCREATOR(CMyAddon);
