#include <string.h>
#include "platform.h"
#include "vgm.h"


static char * read_gd3_str(file_reader_t *reader, uint32_t *poffset, uint32_t eof, bool convert)
{
    uint16_t temp[VGM_GD3_STR_MAX_LEN + 1];
    int index = 0;
    uint16_t ch;
    while (*poffset < eof)
    {
        if (reader->read(reader, (uint8_t *)&ch, *poffset, 2) != 2)  break;
        *poffset += 2;
        if (0 == ch) break;
        if (index < VGM_GD3_STR_MAX_LEN)
        {
            temp[index] = ch;
            ++index;
        }
    }
    temp[index] = 0;
    if (convert && (index > 0))
    {
        char *out = VGM_MALLOC(index + 1);
        if (out)
        {
            for (int i = 0; i <= index; ++i)
            {
                out[i] = temp[i] & 0xff;
            }
            return out;
        }
    }
    return NULL;
}


static void read_vgm_gd3(vgm_t *vgm, uint32_t offset)
{
    // https://www.smspower.org/uploads/Music/gd3spec100.txt
    do
    {
        uint32_t temp, len, eof;
        // read GD3 signature, should be "Gd3 " 0x20336447
        if (vgm->reader->read(vgm->reader, (uint8_t*)&temp, offset, 4) != 4) break;
        if (temp != 0x20336447) break;
        offset += 4;
        // next 4 bytes is version, should be 0x00 0x01 0x00 0x00
        if (vgm->reader->read(vgm->reader, (uint8_t*)&temp, offset, 4) != 4) break;
        if (temp != 0x00000100) break;
        offset += 4;
        // next 4 bytes is length
        if (vgm->reader->read(vgm->reader, (uint8_t*)&len, offset, 4) != 4) break;
        if (len == 0) break;
        offset += 4;
        eof = offset + len; // note eof is last byte + 1
        vgm->track_name_en = read_gd3_str(vgm->reader, &offset, eof, true);
        read_gd3_str(vgm->reader, &offset, eof, false); // skip japanese track name
        vgm->game_name_en = read_gd3_str(vgm->reader, &offset, eof, true);
        read_gd3_str(vgm->reader, &offset, eof, false); // skip japanese game name
        vgm->sys_name_en = read_gd3_str(vgm->reader, &offset, eof, true);
        read_gd3_str(vgm->reader, &offset, eof, false); // skip japanese system name
        vgm->author_name_en = read_gd3_str(vgm->reader, &offset, eof, true);
        read_gd3_str(vgm->reader, &offset, eof, false); // skip japanese author name
        vgm->release_date = read_gd3_str(vgm->reader, &offset, eof, true);
        vgm->creator = read_gd3_str(vgm->reader, &offset, eof, true);
        vgm->notes = read_gd3_str(vgm->reader, &offset, eof, true);
    } while (0);
}


vgm_t * vgm_create(file_reader_t *reader)
{
    vgm_t *vgm = NULL;
    vgm_header_t header;
    bool success = false;
    do
    {
        vgm = (vgm_t *)VGM_MALLOC(sizeof(vgm_t));
        if (NULL == vgm) break;
        memset(vgm, 0, sizeof(vgm_t));
        vgm->reader = reader;
        if (reader->read(reader, (uint8_t *)&header, 0, sizeof(vgm_header_t)) != sizeof(vgm_header_t)) break;
        if (header.ident != 0x206d6756) break;
        if (header.eof_offset + 4 != reader->size(reader)) break;
        vgm->version = header.version;
        // For version 1.50 below, data starts at 0x40. Otherwise data starts from 0x34 + data_offset
        if (header.version >= 0x00000150 && header.data_offset != 0)
        {
            vgm->data_offset = header.data_offset + 0x34;
        }
        else
        {
            vgm->data_offset = 0x40;
        }
        // samples
        vgm->total_samples = header.total_samples;
        // any loop?
        if (header.loop_offset != 0 && header.loop_samples != 0)
        {
            vgm->loops = 1;
            vgm->loop_offset = header.loop_offset + 0x1c;
            vgm->loop_samples = header.loop_samples;
        }
        else
        {
            vgm->loops = 0;
        }
        // NES APU, will verify later
        vgm->nes_apu_clk = header.nes_apu_clk;
        // GD3
        if (header.gd3_offset != 0)
        {
            read_vgm_gd3(vgm, header.gd3_offset + 0x14);
        }
        // Print Info
        VGM_PRINTDBG("VGM version %X.%X\n", vgm->version >> 8, vgm->version & 0xff);
        VGM_PRINTDBG("Total samples: %d+%d (%.2fs+%.2fs)\n", vgm->total_samples, vgm->loop_samples, vgm->total_samples / 44100.0f, vgm->loop_samples / 44100.0f);
        VGM_PRINTDBG("Track Name:    %s\n", vgm->track_name_en);
        VGM_PRINTDBG("Game Name:     %s\n", vgm->game_name_en);
        VGM_PRINTDBG("Author:        %s\n", vgm->author_name_en);
        VGM_PRINTDBG("Release Date:  %s\n", vgm->release_date);
        VGM_PRINTDBG("Ripped by:     %s\n", vgm->creator);
        success = true;
    } while (0);
    if (!success)
    {
        if (vgm != NULL)
        {
            vgm_destroy(vgm);
            vgm = NULL;
        }
    }
    return vgm;
}


void vgm_destroy(vgm_t *vgm)
{
    if (vgm)
    {
        if (vgm->notes) VGM_FREE(vgm->notes);
        if (vgm->creator) VGM_FREE(vgm->creator);
        if (vgm->release_date) VGM_FREE(vgm->release_date);
        if (vgm->author_name_en) VGM_FREE(vgm->author_name_en);
        if (vgm->sys_name_en) VGM_FREE(vgm->sys_name_en);
        if (vgm->game_name_en) VGM_FREE(vgm->game_name_en);
        if (vgm->track_name_en) VGM_FREE(vgm->track_name_en);
        VGM_FREE(vgm);
    }

}
