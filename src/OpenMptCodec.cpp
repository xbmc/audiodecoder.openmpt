/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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

struct ATTRIBUTE_HIDDEN MPTContext
{
  openmpt_module* module = nullptr;
  kodi::vfs::CFile file;
};

class ATTRIBUTE_HIDDEN CMPTCodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CMPTCodec(KODI_HANDLE instance, const std::string& version) :
    CInstanceAudioDecoder(instance, version) {}

  virtual ~CMPTCodec()
  {
    if (ctx.module)
      openmpt_module_destroy(ctx.module);
  }

  bool Init(const std::string& filename, unsigned int filecache,
            int& channels, int& samplerate,
            int& bitspersample, int64_t& totaltime,
            int& bitrate, AudioEngineDataFormat& format,
            std::vector<AudioEngineChannel>& channellist) override
  {
    if (!ctx.file.OpenFile(filename, ADDON_READ_CACHED))
      return false;

    static openmpt_stream_callbacks callbacks = { vfs_file_fread, vfs_file_fseek, vfs_file_ftell };

    ctx.module = openmpt_module_create2(callbacks, &ctx.file, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!ctx.module)
      return false;

    const char* keys = openmpt_module_get_metadata_keys(ctx.module);

    fprintf(stderr, "keays = '%s'\n", keys);

    channels = 2;
    samplerate = 48000;
    bitspersample = 32;
    totaltime = openmpt_module_get_duration_seconds(ctx.module)*1000;
    format = AUDIOENGINE_FMT_FLOAT;
    channellist = { AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR };
    bitrate = openmpt_module_get_num_channels(ctx.module);

    return true;
  }

  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if ((actualsize = openmpt_module_read_interleaved_float_stereo(ctx.module, 48000, size/8, (float*)buffer)*8) == size)
      return 0;

    return 1;
  }

  int64_t Seek(int64_t time) override
  {
    return openmpt_module_set_position_seconds(ctx.module, time/1000.0)*1000.0;
  }

  bool ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag) override
  {
    if (!ctx.file.OpenFile(filename, ADDON_READ_CACHED))
      return false;

    static openmpt_stream_callbacks callbacks = { vfs_file_fread, vfs_file_fseek, vfs_file_ftell };

    ctx.module = openmpt_module_create2(callbacks, &ctx.file, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!ctx.module)
      return false;

    std::string keys = openmpt_module_get_metadata_keys(ctx.module);
    if (keys.find("artist") != std::string::npos)
      tag.SetArtist(openmpt_module_get_metadata(ctx.module, "artist"));
    if (keys.find("title") != std::string::npos)
      tag.SetTitle(openmpt_module_get_metadata(ctx.module, "title"));

    tag.SetDuration(openmpt_module_get_duration_seconds(ctx.module));

    return true;
  }

private:
  MPTContext ctx;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(int instanceType, const std::string& instanceID, KODI_HANDLE instance, const std::string& version, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CMPTCodec(instance, version);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};


ADDONCREATOR(CMyAddon);
