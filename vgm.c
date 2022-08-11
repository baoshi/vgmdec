#include <memory.h>
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
        // We only support NES VGM for now
        if (0 == header.nes_apu_clk > 0) break;
        vgm->nes_apu_clk = header.nes_apu_clk;
        vgm->rate = header.rate;
        if (0 == vgm->rate) vgm->rate = 60;
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
        if (vgm->apu) nesapu_destroy(vgm->apu);
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


bool vgm_prepare_playback(vgm_t *vgm, uint32_t srate)
{
    vgm->apu = nesapu_create(vgm->rate == 50 ? true : false, vgm->nes_apu_clk, srate);
    if (NULL == vgm->apu)
        return false;
    vgm->sample_rate = srate;
    vgm->data_pos = vgm->data_offset;
    vgm->samples_waiting = 0;
    return true;
}


// Execute VGM data, stop when wait command found and return number of samples (in 44100Hz) to generate
// return 0 with data finished
// return negative on error
static int32_t vgm_exec(vgm_t *vgm)
{
    // still have unretrieved samples, don't execute any more
    if (vgm->samples_waiting > 0) return ((int32_t)(vgm->samples_waiting));

    int32_t r = 0;
    bool stop = false;
    while (!stop)
    {
        uint8_t cmd, tt, aa, dd;
        uint16_t data16;
        uint32_t data32;
        file_reader_t *reader = vgm->reader;
        if (reader->read(reader, &cmd, vgm->data_pos, 1) != 1)
        {
            // can't read, treat as data finished
            stop = true;
            vgm->samples_waiting = 0;
        }
        else
        {
            ++vgm->data_pos;
            switch (cmd)
            {
            case 0x30:  // dd : Used for dual chip support
            case 0x31:  // dd : Set AY8910 stereo mask
                vgm->data_pos += 1;
                break;
            case 0x32:  // 0x32..0x3E dd : one operand, reserved for future use
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3A:
            case 0x3B:
            case 0x3C:
            case 0x3D:
            case 0x3E:
                vgm->data_pos += 1;
                break;
            case 0x3F:  // dd : Used for dual chip support
            case 0x40:  // 0x40..0x4E dd dd : two operands, reserved for future use
            case 0x41:  //                    Note: was one operand only til v1.60
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            case 0x4A:
            case 0x4B:
            case 0x4C:
            case 0x4D:
            case 0x4E:
                vgm->data_pos += 2;
                break;
            case 0x4F:  // dd : Game Gear PSG stereo, write dd to port 0x06
            case 0x50:  // dd : PSG (SN76489/SN76496) write value dd
                vgm->data_pos += 1;
                break;
            case 0x51:  // aa dd : YM2413, write value dd to register aa
                // no break
            case 0x52:  // aa dd : YM2612 port 0, write value dd to register aa
                // no break
            case 0x53:  // aa dd : YM2612 port 1, write value dd to register aa
                // no break
            case 0x54:  // aa dd : YM2151, write value dd to register aa
                // no break
            case 0x55:  // aa dd : YM2203, write value dd to register aa
                // no break
            case 0x56:  // aa dd : YM2608 port 0, write value dd to register aa
                // no break
            case 0x57:  // aa dd : YM2608 port 1, write value dd to register aa
                // no break
            case 0x58:  // aa dd : YM2610 port 0, write value dd to register aa
                // no break
            case 0x59:  // aa dd : YM2610 port 1, write value dd to register aa
                // no break
            case 0x5A:  // aa dd : YM3812, write value dd to register aa
                // no break
            case 0x5B:  // aa dd : YM3526, write value dd to register aa
                // no break
            case 0x5C:  // aa dd : Y8950, write value dd to register aa
                // no break
            case 0x5D:  // aa dd : YMZ280B, write value dd to register aa
                // no break
            case 0x5E:  // aa dd : YMF262 port 0, write value dd to register aa
                // no break
            case 0x5F:  // aa dd : YMF262 port 1, write value dd to register aa
                vgm->data_pos += 2;
                break;
            case 0x61:  // nn nn : Wait n samples, n can range from 0 to 65535 (approx 1.49s)
                if (reader->read(reader, (uint8_t *)&data16, vgm->data_pos, 2) != 2)
                {
                    r = -1;
                    stop = true;
                }
                else
                {
                    vgm->data_pos += 2;
                    vgm->samples_waiting = data16;
                    VGM_PRINTDBG("VGM: Wait %d samples\n", vgm->samples_waiting);
                    stop = true;
                }
                break;
            case 0x62:  // wait 735 samples (60th of a second)
                vgm->samples_waiting = 735;
                VGM_PRINTDBG("VGM: Wait 735 samples\n");
                stop = true;
                break;
            case 0x63:  // wait 882 samples (50th of a second)
                vgm->samples_waiting = 882;
                VGM_PRINTDBG("VGM: Wait 882 samples\n");
                stop = true;
                break;
            case 0x66:  // end of sound data
                if (vgm->loops > 0)
                {
                    vgm->data_pos = vgm->loop_offset;
                    VGM_PRINTDBG("VGM: Loop %d\n", vgm->loops);
                    --vgm->loops;
                }
                else
                {
                    VGM_PRINTDBG("VGM: End\n");
                    vgm->samples_waiting = 0;
                    stop = true;
                }
                break;
            case 0x67:  // data block, 0x67 0x66 tt ss ss ss ss (data)
                        // tt = data type
                        // ss ss ss ss = size of data
                        // (data) = data
                vgm->data_pos += 1;    // skip 0x66
                reader->read(reader, &tt, vgm->data_pos, 1);  // tt
                vgm->data_pos += 1;
                data32 = 0;
                if (reader->read(reader, (uint8_t *)&data32, vgm->data_pos, 4) != 4)
                {
                    r = -1;
                    stop = true;
                }
                else
                {
                    vgm->data_offset += 4 + data32;
                    VGM_PRINTDBG("VGM: Data block type=%02x, len=%d\n", tt, data32);
                }
                break;
            case 0x68:  // PCM RAM writes, 0x68 0x66 cc oo oo oo dd dd dd ss ss ss
                VGM_PRINTDBG("VGM: PCM writes\n");
                vgm->data_pos += 11;
                break;
            case 0x70:  // 0x7n:  wait n+1 samples
                vgm->samples_waiting = 1;
                VGM_PRINTDBG("VGM: Wait 1 sample\n");
                stop = true;
                break;
            case 0x71:
                vgm->samples_waiting = 2;
                VGM_PRINTDBG("VGM: Wait 2 samples\n");
                stop = true;
                break;
            case 0x72:
                vgm->samples_waiting = 3;
                VGM_PRINTDBG("VGM: Wait 3 samples\n");
                stop = true;
                break;
            case 0x73:
                vgm->samples_waiting = 4;
                VGM_PRINTDBG("VGM: Wait 4 samples\n");
                stop = true;
                break;
            case 0x74:
                vgm->samples_waiting = 5;
                VGM_PRINTDBG("VGM: Wait 5 samples\n");
                stop = true;
                break;
            case 0x75:
                vgm->samples_waiting = 6;
                VGM_PRINTDBG("VGM: Wait 6 samples\n");
                stop = true;
                break;
            case 0x76:
                vgm->samples_waiting = 7;
                VGM_PRINTDBG("VGM: Wait 7 samples\n");
                stop = true;
                break;
            case 0x77:
                vgm->samples_waiting = 8;
                VGM_PRINTDBG("VGM: Wait 8 samples\n");
                stop = true;
                break;
            case 0x78:
                vgm->samples_waiting = 9;
                VGM_PRINTDBG("VGM: Wait 9 samples\n");
                stop = true;
                break;
            case 0x79:
                vgm->samples_waiting = 10;
                VGM_PRINTDBG("VGM: Wait 10 samples\n");
                stop = true;
                break;
            case 0x7A:
                vgm->samples_waiting = 11;
                VGM_PRINTDBG("VGM: Wait 11 samples\n");
                stop = true;
                break;
            case 0x7B:
                vgm->samples_waiting = 12;
                VGM_PRINTDBG("VGM: Wait 12 samples\n");
                stop = true;
                break;
            case 0x7C:
                vgm->samples_waiting = 13;
                VGM_PRINTDBG("VGM: Wait 13 samples\n");
                stop = true;
                break;
            case 0x7D:
                vgm->samples_waiting = 14;
                VGM_PRINTDBG("VGM: Wait 14 samples\n");
                stop = true;
                break;
            case 0x7E:
                vgm->samples_waiting = 15;
                VGM_PRINTDBG("VGM: Wait 15 samples\n");
                stop = true;
                break;
            case 0x7F:
                vgm->samples_waiting = 16;
                VGM_PRINTDBG("VGM: Wait 16 samples\n");
                stop = true;
                break;
            case 0x80:  // 0x8n: YM2612 port 0 address 2A write from the data bank, then wait n samples
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8A:
            case 0x8B:
            case 0x8C:
            case 0x8D:
            case 0x8E:
            case 0x8F:
                break;
            case 0x90:  // 0x90-0x95: DAC Stream control write
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
                break;
            case 0xA0:  // aa dd : AY8910, write value dd to register aa
            case 0xA1:  // 0xA1-0xAF: aa dd : Used for dual chip supportw
            case 0xA2:
            case 0xA3:
            case 0xA4:
            case 0xA5:
            case 0xA6:
            case 0xA7:
            case 0xA8:
            case 0xA9:
            case 0xAA:
            case 0xAB:
            case 0xAC:
            case 0xAD:
            case 0xAE:
            case 0xAF:
            case 0xB0:  // aa dd : RF5C68, write value dd to register aa
            case 0xB1:  // aa dd : RF5C164, write value dd to register aa
            case 0xB2:  // ad dd : PWM, write value ddd to register a (d is MSB, dd is LSB)
            case 0xB3:  // aa dd : GameBoy DMG, write value dd to register aa
                vgm->data_pos += 2;
                break;
            case 0xB4:  // aa dd : NES APU, write value dd to register aa
                if (reader->read(reader, &aa, vgm->data_pos, 1) != 1)
                {
                    r = -1;
                    stop = true;
                    break;
                }
                ++vgm->data_pos;
                if (reader->read(reader, &dd, vgm->data_pos, 1) != 1)
                {
                    r = -1;
                    stop = true;
                    break;
                }
                ++vgm->data_pos;
                VGM_PRINTDBG("VGM: NES APU write reg[$%04X] = 0x%02X\n", aa + 0x4000, dd);
                break;
            case 0xB5:  // aa dd : MultiPCM, write value dd to register aa
            case 0xB6:  // aa dd : uPD7759, write value dd to register aa
            case 0xB7:  // aa dd : OKIM6258, write value dd to register aa
            case 0xB8:  // aa dd : OKIM6295, write value dd to register aa
            case 0xB9:  // aa dd : HuC6280, write value dd to register aa
            case 0xBA:  // aa dd : K053260, write value dd to register aa
            case 0xBB:  // aa dd : Pokey, write value dd to register aa
            case 0xBC:  // aa dd : WonderSwan, write value dd to register aa
            case 0xBD:  // aa dd : SAA1099, write value dd to register aa
            case 0xBE:  // aa dd : ES5506, write 8-bit value dd to register aa
            case 0xBF:  // aa dd : GA20, write value dd to register aa
                vgm->data_pos += 2;
                break;
            case 0xC0:  // bbaa dd : Sega PCM, write value dd to memory offset aabb
            case 0xC1:  // bbaa dd : RF5C68, write value dd to memory offset aabb
            case 0xC2:  // bbaa dd : RF5C164, write value dd to memory offset aabb
            case 0xC3:  // cc bbaa : MultiPCM, write set bank offset aabb to channel cc
            case 0xC4:  // mmll rr : QSound, write value mmll to register rr
                        // (mm - data MSB, ll - data LSB)
            case 0xC5:  // mmll dd : SCSP, write value dd to memory offset mmll
                        // (mm - offset MSB, ll - offset LSB)
            case 0xC6:  // mmll dd : WonderSwan, write value dd to memory offset mmll
                        // (mm - offset MSB, ll - offset LSB)
            case 0xC7:  // mmll dd : VSU, write value dd to register mmll
                        // (mm - MSB, ll - LSB)
            case 0xC8:  // mmll dd : X1-010, write value dd to memory offset mmll
                        // (mm - offset MSB, ll - offset LSB)
                vgm->data_pos += 3;
                break;
            case 0xC9:  // 0xC9..0xCF dd dd dd : three operands, reserved for future use
            case 0xCA:
            case 0xCB:
            case 0xCC:
            case 0xCD:
            case 0xCE:
            case 0xCF:
                vgm->data_offset += 3;
                break;
            case 0xD0:  // pp aa dd : YMF278B port pp, write value dd to register aa
            case 0xD1:  // pp aa dd : YMF271 port pp, write value dd to register aa
            case 0xD2:  // pp aa dd : SCC1 port pp, write value dd to register aa
            case 0xD3:  // pp aa dd : K054539 write value dd to register ppaa
            case 0xD4:  // pp aa dd : C140 write value dd to register ppaa
            case 0xD5:  // pp aa dd : ES5503 write value dd to register ppaa
            case 0xD6:  // aa ddee  : ES5506 write 16-bit value ddee to register aa 
                vgm->data_pos += 3;
                break;
            case 0xD7:  // 0xD7..0xDF dd dd dd : three operands, reserved for future use
            case 0xD8:
            case 0xD9:
            case 0xDA:
            case 0xDB:
            case 0xDC:
            case 0xDD:
            case 0xDE:
            case 0xDF:
                vgm->data_pos += 3;
                break;
            case 0xE0:  // dddddddd : seek to offset dddddddd (Intel byte order) in PCM data bank
            case 0xE1:  // aabb ddee: C352 write 16-bit value ddee to register aabb
                vgm->data_pos += 4;
                break;
            case 0xE2:  // 0xE2..0xFF dd dd dd dd : four operands, reserved for future use
            case 0xE3:
            case 0xE4:
            case 0xE5:
            case 0xE6:
            case 0xE7:
            case 0xE8:
            case 0xE9:
            case 0xEA:
            case 0xEB:
            case 0xEC:
            case 0xED:
            case 0xEE:
            case 0xEF:
            case 0xF0:
            case 0xF1:
            case 0xF2:
            case 0xF3:
            case 0xF4:
            case 0xF5:
            case 0xF6:
            case 0xF7:
            case 0xF8:
            case 0xF9:
            case 0xFA:
            case 0xFB:
            case 0xFC:
            case 0xFD:
            case 0xFE:
            case 0xFF:
                vgm->data_pos += 4;
                break;
            default:
                VGM_PRINTERR("VGM: Unknown command 0x%02X\n", cmd);
                stop = true;
                r = -1;
                break;         
            }  // end of switch-case
        } // end of read cmd
    } // end of while loop
    if (r < 0)
        return r;
    return (vgm->samples_waiting);
}


int vgm_get_sample(vgm_t *vgm, int16_t *buf, int size)
{
    int32_t samples = 0, r;
    while (size > 0)
    {   
        r = vgm_exec(vgm);
        if (r < 0)
        {
            VGM_PRINTERR("VGM: Exec error\n");
            break;
        }
        if (r == 0)
        {
            VGM_PRINTF("VGM: Finished\n");
            break;
        }
        if (size >= r)
        {
            VGM_PRINTDBG("VGM: To retrieve %d samples from APU\n", r);
            vgm->samples_waiting = 0;
            // fill r samples to buf
            samples += r;
            size -= r;
        }
        else
        {
            VGM_PRINTDBG("VGM: To retrieve %d samples from APU\n", size);
            vgm->samples_waiting -= size;
            // fill size samples to buf
            samples += size;
            size = 0;
        }
    }
    return samples;   
}