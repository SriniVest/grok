/*
*    Copyright (C) 2016 Grok Image Compression Inc.
*
*    This source code is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This source code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*    This source code incorporates work covered by the following copyright and
*    permission notice:
*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008, 2011-2012, Centre National d'Etudes Spatiales (CNES), FR
 * Copyright (c) 2012, CS Systemes d'Information, France
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opj_includes.h"

/** @defgroup T2 T2 - Implementation of a tier-2 coding */
/*@{*/

/** @name Local static functions */
/*@{*/

static void opj_t2_putcommacode(opj_bio_t *bio, int32_t n);

static uint32_t opj_t2_getcommacode(opj_bio_t *bio);
/**
Variable length code for signalling delta Zil (truncation point)
@param bio  Bit Input/Output component
@param n    delta Zil
*/
static void opj_t2_putnumpasses(opj_bio_t *bio, uint32_t n);
static uint32_t opj_t2_getnumpasses(opj_bio_t *bio);

/**
Encode a packet of a tile to a destination buffer
@param tileno Number of the tile encoded
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param dest Destination buffer
@param p_data_written   FIXME DOC
@param len Length of the destination buffer
@param cstr_info Codestream information structure
@return
*/
static bool opj_t2_encode_packet(   uint32_t tileno,
                                    opj_tcd_tile_t *tile,
                                    opj_tcp_t *tcp,
                                    opj_pi_iterator_t *pi,
                                    uint8_t *dest,
                                    uint32_t * p_data_written,
                                    uint32_t len,
                                    opj_codestream_info_t *cstr_info);

/**
Encode a packet of a tile to a destination buffer
@param tileno Number of the tile encoded
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param dest Destination buffer
@param p_data_written   FIXME DOC
@param len Length of the destination buffer
@param cstr_info Codestream information structure
@return
*/
static bool opj_t2_encode_packet_thresh(opj_tcd_tile_t *tile,
                                        opj_tcp_t *tcp,
                                        opj_pi_iterator_t *pi,
                                        uint32_t * p_data_written,
                                        uint32_t len);



/**
Decode a packet of a tile from a source buffer
@param t2 T2 handle
@param tile Tile for which to write the packets
@param tcp Tile coding parameters
@param pi Packet identity
@param src Source buffer
@param data_read   FIXME DOC
@param max_length  FIXME DOC
@param pack_info Packet information

@return  FIXME DOC
*/
static bool opj_t2_decode_packet(   opj_t2_t* t2,
                                    opj_tcd_tile_t *tile,
                                    opj_tcp_t *tcp,
                                    opj_pi_iterator_t *pi,
                                    opj_seg_buf_t* src_buf,
                                    uint32_t * data_read,
                                    opj_packet_info_t *pack_info,
                                    opj_event_mgr_t *p_manager);

static bool opj_t2_skip_packet( opj_t2_t* p_t2,
                                opj_tcd_tile_t *p_tile,
                                opj_tcp_t *p_tcp,
                                opj_pi_iterator_t *p_pi,
                                opj_seg_buf_t* src_buf,
                                uint32_t * p_data_read,
                                opj_packet_info_t *p_pack_info,
                                opj_event_mgr_t *p_manager);

static bool opj_t2_read_packet_header(  opj_t2_t* p_t2,
                                        opj_tcd_tile_t *p_tile,
                                        opj_tcp_t *p_tcp,
                                        opj_pi_iterator_t *p_pi,
                                        bool * p_is_data_present,
                                        opj_seg_buf_t* src_buf,
                                        uint32_t * p_data_read,
                                        opj_packet_info_t *p_pack_info,
                                        opj_event_mgr_t *p_manager);

static bool opj_t2_read_packet_data(opj_t2_t* p_t2,
                                    opj_tcd_tile_t *p_tile,
                                    opj_pi_iterator_t *p_pi,
                                    opj_seg_buf_t* src_buf,
                                    uint32_t * p_data_read,
                                    opj_packet_info_t *pack_info,
                                    opj_event_mgr_t *p_manager);

static bool opj_t2_skip_packet_data(opj_t2_t* p_t2,
                                    opj_tcd_tile_t *p_tile,
                                    opj_pi_iterator_t *p_pi,
                                    uint32_t * p_data_read,
                                    uint32_t p_max_length,
                                    opj_packet_info_t *pack_info,
                                    opj_event_mgr_t *p_manager);

/**
@param cblk
@param index
@param cblksty
@param first
*/
static bool opj_t2_init_seg(    opj_tcd_cblk_dec_t* cblk,
                                uint32_t index,
                                uint32_t cblksty,
                                uint32_t first);

/*@}*/

/*@}*/

/* ----------------------------------------------------------------------- */

/* #define RESTART 0x04 */
static void opj_t2_putcommacode(opj_bio_t *bio, int32_t n)
{
    while (--n >= 0) {
        opj_bio_write(bio, 1, 1);
    }
    opj_bio_write(bio, 0, 1);
}

static uint32_t opj_t2_getcommacode(opj_bio_t *bio)
{
    uint32_t n = 0;
    while (opj_bio_read(bio, 1)) {
        ++n;
    }
    return n;
}

static void opj_t2_putnumpasses(opj_bio_t *bio, uint32_t n)
{
    if (n == 1) {
        opj_bio_write(bio, 0, 1);
    } else if (n == 2) {
        opj_bio_write(bio, 2, 2);
    } else if (n <= 5) {
        opj_bio_write(bio, 0xc | (n - 3), 4);
    } else if (n <= 36) {
        opj_bio_write(bio, 0x1e0 | (n - 6), 9);
    } else if (n <= 164) {
        opj_bio_write(bio, 0xff80 | (n - 37), 16);
    }
}

static uint32_t opj_t2_getnumpasses(opj_bio_t *bio)
{
    uint32_t n;
    if (!opj_bio_read(bio, 1))
        return 1;
    if (!opj_bio_read(bio, 1))
        return 2;
    if ((n = opj_bio_read(bio, 2)) != 3)
        return (3 + n);
    if ((n = opj_bio_read(bio, 5)) != 31)
        return (6 + n);
    return (37 + opj_bio_read(bio, 7));
}

/* ----------------------------------------------------------------------- */

bool opj_t2_encode_packets( opj_t2_t* p_t2,
                            uint32_t p_tile_no,
                            opj_tcd_tile_t *p_tile,
                            uint32_t p_maxlayers,
                            uint8_t *p_dest,
                            uint32_t * p_data_written,
                            uint32_t p_max_len,
                            opj_codestream_info_t *cstr_info,
                            uint32_t p_tp_num,
                            uint32_t p_tp_pos,
                            uint32_t p_pino)
{
    uint8_t *l_current_data = p_dest;
    uint32_t l_nb_bytes = 0;
    opj_pi_iterator_t *l_pi = 00;
    opj_pi_iterator_t *l_current_pi = 00;
    opj_image_t *l_image = p_t2->image;
    opj_cp_t *l_cp = p_t2->cp;
    opj_tcp_t *l_tcp = &l_cp->tcps[p_tile_no];
    uint32_t l_nb_pocs = l_tcp->numpocs + 1;

    l_pi = opj_pi_initialise_encode(l_image, l_cp, p_tile_no, FINAL_PASS);
    if (!l_pi) {
        return false;
    }

    * p_data_written = 0;

    opj_pi_create_encode(l_pi, l_cp,p_tile_no,p_pino,p_tp_num,p_tp_pos, FINAL_PASS);

    l_current_pi = &l_pi[p_pino];
    if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
        /* TODO ADE : add an error */
        opj_pi_destroy(l_pi, l_nb_pocs);
        return false;
    }
    while (opj_pi_next(l_current_pi)) {
        if (l_current_pi->layno < p_maxlayers) {
            l_nb_bytes=0;

            if (! opj_t2_encode_packet(p_tile_no,p_tile, l_tcp, l_current_pi, l_current_data, &l_nb_bytes, p_max_len, cstr_info)) {
                opj_pi_destroy(l_pi, l_nb_pocs);
                return false;
            }

            l_current_data += l_nb_bytes;
            p_max_len -= l_nb_bytes;

            * p_data_written += l_nb_bytes;

            /* INDEX >> */
            if(cstr_info) {
                if(cstr_info->index_write) {
                    opj_tile_info_t *info_TL = &cstr_info->tile[p_tile_no];
                    opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
                    if (!cstr_info->packno) {
                        info_PK->start_pos = info_TL->end_header + 1;
                    } else {
                        info_PK->start_pos = ((l_cp->m_specific_param.m_enc.m_tp_on | l_tcp->POC)&& info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
                    }
                    info_PK->end_pos = info_PK->start_pos + l_nb_bytes - 1;
                    info_PK->end_ph_pos += info_PK->start_pos - 1;  /* End of packet header which now only represents the distance
                                                                                                                                                                                                                                                to start of packet is incremented by value of start of packet*/
                }

                cstr_info->packno++;
            }
            /* << INDEX */
            ++p_tile->packno;
        }
    }

    opj_pi_destroy(l_pi, l_nb_pocs);

    return true;
}


bool opj_t2_encode_packets_thresh(opj_t2_t* p_t2,
                                  uint32_t p_tile_no,
                                  opj_tcd_tile_t *p_tile,
                                  uint32_t p_maxlayers,
                                  uint32_t * p_data_written,
                                  uint32_t p_max_len,
                                  uint32_t p_tp_pos)
{
    uint32_t l_nb_bytes = 0;
    uint32_t compno;
    uint32_t poc;
    opj_pi_iterator_t *l_pi = 00;
    opj_pi_iterator_t *l_current_pi = 00;
    opj_image_t *l_image = p_t2->image;
    opj_cp_t *l_cp = p_t2->cp;
    opj_tcp_t *l_tcp = l_cp->tcps + p_tile_no;
    uint32_t pocno = (l_cp->rsiz == OPJ_PROFILE_CINEMA_4K) ? 2 : 1;
    uint32_t l_max_comp = l_cp->m_specific_param.m_enc.m_max_comp_size > 0 ? l_image->numcomps : 1;
    uint32_t l_nb_pocs = l_tcp->numpocs + 1;

    l_pi = opj_pi_initialise_encode(l_image, l_cp, p_tile_no, THRESH_CALC);
    if (!l_pi) {
        return false;
    }
    *p_data_written = 0;
    l_current_pi = l_pi;

    for (compno = 0; compno < l_max_comp; ++compno) {
        uint32_t l_comp_len = 0;
        l_current_pi = l_pi;

        for (poc = 0; poc < pocno; ++poc) {
            uint32_t l_tp_num = compno;

            /* TODO MSD : check why this function cannot fail (cf. v1) */
            opj_pi_create_encode(l_pi, l_cp, p_tile_no, poc, l_tp_num, p_tp_pos, THRESH_CALC);

            if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
                /* TODO ADE : add an error */
                opj_pi_destroy(l_pi, l_nb_pocs);
                return false;
            }
            while (opj_pi_next(l_current_pi)) {
                if (l_current_pi->layno < p_maxlayers) {
                    l_nb_bytes = 0;

                    if (!opj_t2_encode_packet_thresh(p_tile, l_tcp, l_current_pi, &l_nb_bytes, p_max_len)) {
                        opj_pi_destroy(l_pi, l_nb_pocs);
                        return false;
                    }

                    l_comp_len += l_nb_bytes;
                    p_max_len -= l_nb_bytes;

                    *p_data_written += l_nb_bytes;
                }
            }

            if (l_cp->m_specific_param.m_enc.m_max_comp_size) {
                if (l_comp_len > l_cp->m_specific_param.m_enc.m_max_comp_size) {
                    opj_pi_destroy(l_pi, l_nb_pocs);
                    return false;
                }
            }

            ++l_current_pi;
        }
    }
    opj_pi_destroy(l_pi, l_nb_pocs);
    return true;
}

/* see issue 80 */
#if 0
#define JAS_FPRINTF fprintf
#else
/* issue 290 */
static void opj_null_jas_fprintf(FILE* file, const char * format, ...)
{
    (void)file;
    (void)format;
}
#define JAS_FPRINTF opj_null_jas_fprintf
#endif

bool opj_t2_decode_packets( opj_t2_t *p_t2,
                            uint32_t p_tile_no,
                            opj_tcd_tile_t *p_tile,
                            opj_seg_buf_t* src_buf,
                            uint32_t * p_data_read,
                            opj_event_mgr_t *p_manager)
{
    opj_pi_iterator_t *l_pi = 00;
    uint32_t pino;
    opj_image_t *l_image = p_t2->image;
    opj_cp_t *l_cp = p_t2->cp;
    opj_tcp_t *l_tcp = p_t2->cp->tcps + p_tile_no;
    uint32_t l_nb_bytes_read;
    uint32_t l_nb_pocs = l_tcp->numpocs + 1;
    opj_pi_iterator_t *l_current_pi = 00;
#ifdef TODO_MSD
    uint32_t curtp = 0;
    uint32_t tp_start_packno;
#endif
    opj_packet_info_t *l_pack_info = 00;
    opj_image_comp_t* l_img_comp = 00;

    /* create a packet iterator */
    l_pi = opj_pi_create_decode(l_image, l_cp, p_tile_no);
    if (!l_pi) {
        return false;
    }


    l_current_pi = l_pi;

    for     (pino = 0; pino <= l_tcp->numpocs; ++pino) {

        /* if the resolution needed is too low, one dim of the tilec could be equal to zero
         * and no packets are used to decode this resolution and
         * l_current_pi->resno is always >= p_tile->comps[l_current_pi->compno].minimum_num_resolutions
         * and no l_img_comp->resno_decoded are computed
         */
        bool* first_pass_failed = NULL;

        if (l_current_pi->poc.prg == OPJ_PROG_UNKNOWN) {
            /* TODO ADE : add an error */
            opj_pi_destroy(l_pi, l_nb_pocs);
            return false;
        }

        first_pass_failed = (bool*)opj_malloc(l_image->numcomps * sizeof(bool));
        if (!first_pass_failed) {
            opj_pi_destroy(l_pi,l_nb_pocs);
            return false;
        }
        memset(first_pass_failed, true, l_image->numcomps * sizeof(bool));

        while (opj_pi_next(l_current_pi)) {
            bool skip_precinct = false;
            opj_tcd_tilecomp_t* tilec = p_tile->comps + l_current_pi->compno;
            bool skip_layer_or_res = l_current_pi->layno >= l_tcp->num_layers_to_decode ||
                                     l_current_pi->resno >= tilec->minimum_num_resolutions;

            l_img_comp = l_image->comps + l_current_pi->compno;

            JAS_FPRINTF( stderr,
                         "packet offset=00000166 prg=%d cmptno=%02d rlvlno=%02d prcno=%03d lyrno=%02d\n\n",
                         l_current_pi->poc.prg1,
                         l_current_pi->compno,
                         l_current_pi->resno,
                         l_current_pi->precno,
                         l_current_pi->layno );



            if (!skip_layer_or_res) {
                opj_tcd_resolution_t* res = tilec->resolutions + l_current_pi->resno;
                uint32_t bandno;
                skip_precinct = true;
                for (bandno = 0; bandno < res->numbands && skip_precinct; ++bandno) {
                    opj_tcd_band_t* band = res->bands + bandno;
                    uint32_t num_precincts =
                        band->precincts_data_size / sizeof(opj_tcd_precinct_t);
                    uint32_t precno;
                    for (precno = 0; precno < num_precincts && skip_precinct; ++precno) {
                        opj_rect_t prec_rect;
                        opj_tcd_precinct_t* prec = band->precincts + precno;
                        opj_rect_init(&prec_rect,prec->x0, prec->y0, prec->x1, prec->y1);
                        if (opj_tile_buf_hit_test(tilec->buf,&prec_rect)) {
                            skip_precinct = false;
                            break;
                        }
                    }
                }
            }

            if (!skip_layer_or_res && !skip_precinct) {
                l_nb_bytes_read = 0;

                first_pass_failed[l_current_pi->compno] = false;

                if (! opj_t2_decode_packet(p_t2,
                                           p_tile,
                                           l_tcp,
                                           l_current_pi,
                                           src_buf,
                                           &l_nb_bytes_read,
                                           l_pack_info,
                                           p_manager)) {
                    opj_pi_destroy(l_pi,l_nb_pocs);
                    opj_free(first_pass_failed);
                    return false;
                }
            } else {
                l_nb_bytes_read = 0;
                if (! opj_t2_skip_packet(p_t2,
                                         p_tile,
                                         l_tcp,l_current_pi,
                                         src_buf,
                                         &l_nb_bytes_read,
                                         l_pack_info, p_manager)) {
                    opj_pi_destroy(l_pi,l_nb_pocs);
                    opj_free(first_pass_failed);
                    return false;
                }
            }

            if (!skip_layer_or_res)
                l_img_comp->resno_decoded = opj_uint_max(l_current_pi->resno, l_img_comp->resno_decoded);

            if (first_pass_failed[l_current_pi->compno]) {
                if (l_img_comp->resno_decoded == 0)
                    l_img_comp->resno_decoded =
                        p_tile->comps[l_current_pi->compno].minimum_num_resolutions - 1;
            }

            *p_data_read += l_nb_bytes_read;
        }
        ++l_current_pi;

        opj_free(first_pass_failed);
    }

    /* don't forget to release pi */
    opj_pi_destroy(l_pi,l_nb_pocs);
    return true;
}

/* ----------------------------------------------------------------------- */

/**
 * Creates a Tier 2 handle
 *
 * @param       p_image         Source or destination image
 * @param       p_cp            Image coding parameters.
 * @return              a new T2 handle if successful, NULL otherwise.
*/
opj_t2_t* opj_t2_create(opj_image_t *p_image, opj_cp_t *p_cp)
{
    /* create the t2 structure */
    opj_t2_t *l_t2 = (opj_t2_t*)opj_calloc(1,sizeof(opj_t2_t));
    if (!l_t2) {
        return NULL;
    }

    l_t2->image = p_image;
    l_t2->cp = p_cp;

    return l_t2;
}

void opj_t2_destroy(opj_t2_t *t2)
{
    if(t2) {
        opj_free(t2);
    }
}

static bool opj_t2_decode_packet(  opj_t2_t* p_t2,
                                   opj_tcd_tile_t *p_tile,
                                   opj_tcp_t *p_tcp,
                                   opj_pi_iterator_t *p_pi,
                                   opj_seg_buf_t* src_buf,
                                   uint32_t * p_data_read,
                                   opj_packet_info_t *p_pack_info,
                                   opj_event_mgr_t *p_manager)
{
    bool l_read_data;
    uint32_t l_nb_bytes_read = 0;
    uint32_t l_nb_total_bytes_read = 0;

    *p_data_read = 0;

    if (! opj_t2_read_packet_header(p_t2,p_tile,p_tcp,p_pi,&l_read_data,
                                    src_buf,
                                    &l_nb_bytes_read,
                                    p_pack_info, p_manager)) {
        return false;
    }

    l_nb_total_bytes_read += l_nb_bytes_read;

    /* we should read data for the packet */
    if (l_read_data) {
        l_nb_bytes_read = 0;

        if (! opj_t2_read_packet_data(p_t2,p_tile,p_pi,
                                      src_buf,&l_nb_bytes_read,
                                      p_pack_info, p_manager)) {
            return false;
        }
        l_nb_total_bytes_read += l_nb_bytes_read;
    }

    *p_data_read = l_nb_total_bytes_read;

    return true;
}

static bool opj_t2_encode_packet(  uint32_t tileno,
                                   opj_tcd_tile_t * tile,
                                   opj_tcp_t * tcp,
                                   opj_pi_iterator_t *pi,
                                   uint8_t *dest,
                                   uint32_t * p_data_written,
                                   uint32_t length,
                                   opj_codestream_info_t *cstr_info)
{
    uint32_t bandno, cblkno;
    uint8_t* c = dest;
    uint32_t l_nb_bytes;
    uint32_t compno = pi->compno;     /* component value */
    uint32_t resno  = pi->resno;      /* resolution level value */
    uint32_t precno = pi->precno;     /* precinct value */
    uint32_t layno  = pi->layno;      /* quality layer value */
    uint32_t l_nb_blocks;
    opj_tcd_band_t *band = 00;
    opj_tcd_cblk_enc_t* cblk = 00;
    opj_tcd_pass_t *pass = 00;

    opj_tcd_tilecomp_t *tilec = &tile->comps[compno];
    opj_tcd_resolution_t *res = &tilec->resolutions[resno];

    opj_bio_t *bio = 00;    /* BIO component */

    /* <SOP 0xff91> */
    if (tcp->csty & J2K_CP_CSTY_SOP) {
        c[0] = 255;
        c[1] = 145;
        c[2] = 0;
        c[3] = 4;
#if 0
        c[4] = (tile->packno % 65536) / 256;
        c[5] = (tile->packno % 65536) % 256;
#else
        c[4] = (tile->packno >> 8) & 0xff; /* packno is uint32_t */
        c[5] = tile->packno & 0xff;
#endif
        c += 6;
        length -= 6;
    }
    /* </SOP> */

    if (!layno) {
        band = res->bands;

        for(bandno = 0; bandno < res->numbands; ++bandno) {
            opj_tcd_precinct_t *prc = &band->precincts[precno];

            opj_tgt_reset(prc->incltree);
            opj_tgt_reset(prc->imsbtree);

            l_nb_blocks = prc->cw * prc->ch;
            for     (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
                cblk = &prc->cblks.enc[cblkno];

                cblk->numpasses = 0;
                opj_tgt_setvalue(prc->imsbtree, cblkno, band->numbps - (int32_t)cblk->numbps);
            }
            ++band;
        }
    }

    bio = opj_bio_create();
    if (!bio) {
        /* FIXME event manager error callback */
        return false;
    }
    opj_bio_init_enc(bio, c, length);
    opj_bio_write(bio, 1, 1);           /* Empty header bit */

    /* Writing Packet header */
    band = res->bands;
    for (bandno = 0; bandno < res->numbands; ++bandno)      {
        opj_tcd_precinct_t *prc = &band->precincts[precno];

        l_nb_blocks = prc->cw * prc->ch;
        cblk = prc->cblks.enc;

        for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
            opj_tcd_layer_t *layer = &cblk->layers[layno];

            if (!cblk->numpasses && layer->numpasses) {
                opj_tgt_setvalue(prc->incltree, cblkno, (int32_t)layno);
            }

            ++cblk;
        }

        cblk = prc->cblks.enc;
        for (cblkno = 0; cblkno < l_nb_blocks; cblkno++) {
            opj_tcd_layer_t *layer = &cblk->layers[layno];
            uint32_t increment = 0;
            uint32_t nump = 0;
            uint32_t len = 0, passno;
            uint32_t l_nb_passes;

            /* cblk inclusion bits */
            if (!cblk->numpasses) {
                opj_tgt_encode(bio, prc->incltree, cblkno, (int32_t)(layno + 1));
            } else {
                opj_bio_write(bio, layer->numpasses != 0, 1);
            }

            /* if cblk not included, go to the next cblk  */
            if (!layer->numpasses) {
                ++cblk;
                continue;
            }

            /* if first instance of cblk --> zero bit-planes information */
            if (!cblk->numpasses) {
                cblk->numlenbits = 3;
                opj_tgt_encode(bio, prc->imsbtree, cblkno, 999);
            }

            /* number of coding passes included */
            opj_t2_putnumpasses(bio, layer->numpasses);
            l_nb_passes = cblk->numpasses + layer->numpasses;
            pass = cblk->passes +  cblk->numpasses;

            /* computation of the increase of the length indicator and insertion in the header     */
            for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                ++nump;
                len += pass->len;

                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                    increment = (uint32_t)opj_int_max((int32_t)increment, opj_int_floorlog2((int32_t)len) + 1
                                                      - ((int32_t)cblk->numlenbits + opj_int_floorlog2((int32_t)nump)));
                    len = 0;
                    nump = 0;
                }

                ++pass;
            }
            opj_t2_putcommacode(bio, (int32_t)increment);

            /* computation of the new Length indicator */
            cblk->numlenbits += increment;

            pass = cblk->passes +  cblk->numpasses;
            /* insertion of the codeword segment length */
            for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                nump++;
                len += pass->len;

                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                    opj_bio_write(bio, (uint32_t)len, cblk->numlenbits + (uint32_t)opj_int_floorlog2((int32_t)nump));
                    len = 0;
                    nump = 0;
                }
                ++pass;
            }

            ++cblk;
        }

        ++band;
    }

    if (!opj_bio_flush(bio)) {
        opj_bio_destroy(bio);
        return false;               /* modified to eliminate longjmp !! */
    }

    l_nb_bytes = (uint32_t)opj_bio_numbytes(bio);
    c += l_nb_bytes;
    length -= l_nb_bytes;

    opj_bio_destroy(bio);

    /* <EPH 0xff92> */
    if (tcp->csty & J2K_CP_CSTY_EPH) {
        c[0] = 255;
        c[1] = 146;
        c += 2;
        length -= 2;
    }
    /* </EPH> */

    /* << INDEX */
    /* End of packet header position. Currently only represents the distance to start of packet
       Will be updated later by incrementing with packet start value*/
    if(cstr_info && cstr_info->index_write) {
        opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
        info_PK->end_ph_pos = (int32_t)(c - dest);
    }
    /* INDEX >> */

    /* Writing the packet body */
    band = res->bands;
    for (bandno = 0; bandno < res->numbands; bandno++) {
        opj_tcd_precinct_t *prc = &band->precincts[precno];

        l_nb_blocks = prc->cw * prc->ch;
        cblk = prc->cblks.enc;

        for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
            opj_tcd_layer_t *layer = &cblk->layers[layno];

            if (!layer->numpasses) {
                ++cblk;
                continue;
            }

            if (layer->len > length) {
                return false;
            }

            memcpy(c, layer->data, layer->len);
            cblk->numpasses += layer->numpasses;
            c += layer->len;
            length -= layer->len;

            /* << INDEX */
            if(cstr_info && cstr_info->index_write) {
                opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
                info_PK->disto += layer->disto;
                if (cstr_info->D_max < info_PK->disto) {
                    cstr_info->D_max = info_PK->disto;
                }
            }

            ++cblk;
            /* INDEX >> */
        }
        ++band;
    }

    assert( c >= dest );
    * p_data_written += (uint32_t)(c - dest);

    return true;
}


static bool opj_t2_encode_packet_thresh(opj_tcd_tile_t * tile,
                                        opj_tcp_t * tcp,
                                        opj_pi_iterator_t *pi,
                                        uint32_t * p_data_written,
                                        uint32_t length)
{
    uint32_t bandno, cblkno;
    uint32_t l_nb_bytes;
    uint32_t compno = pi->compno;     /* component value */
    uint32_t resno = pi->resno;      /* resolution level value */
    uint32_t precno = pi->precno;     /* precinct value */
    uint32_t layno = pi->layno;      /* quality layer value */
    uint32_t l_nb_blocks;
    opj_tcd_band_t *band = 00;
    opj_tcd_cblk_enc_t* cblk = 00;
    opj_tcd_pass_t *pass = 00;

    opj_tcd_tilecomp_t *tilec = tile->comps + compno;
    opj_tcd_resolution_t *res = tilec->resolutions + resno;

    opj_bio_t *bio = 00;    /* BIO component */
    uint32_t packet_bytes_written = 0;

    /* <SOP 0xff91> */
    if (tcp->csty & J2K_CP_CSTY_SOP) {
        length -= 6;
        packet_bytes_written += 6;
    }
    /* </SOP> */

    if (!layno) {
        band = res->bands;

        for (bandno = 0; bandno < res->numbands; ++bandno) {
            opj_tcd_precinct_t *prc = band->precincts + precno;

            opj_tgt_reset(prc->incltree);
            opj_tgt_reset(prc->imsbtree);

            l_nb_blocks = prc->cw * prc->ch;
            for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
                cblk = prc->cblks.enc+cblkno;

                cblk->numpasses = 0;
                opj_tgt_setvalue(prc->imsbtree, cblkno, band->numbps - (int32_t)cblk->numbps);
            }
            ++band;
        }
    }

    bio = opj_bio_create();
    if (!bio) {
        /* FIXME event manager error callback */
        return false;
    }
    opj_bio_init_enc(bio, 0, length);
    opj_bio_write(bio, 1, 1);           /* Empty header bit */
    bio->sim_out = true;

    /* Writing Packet header */
    band = res->bands;
    for (bandno = 0; bandno < res->numbands; ++bandno) {
        opj_tcd_precinct_t *prc = band->precincts + precno;

        l_nb_blocks = prc->cw * prc->ch;
        cblk = prc->cblks.enc;

        for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
            opj_tcd_layer_t *layer = cblk->layers + layno;

            if (!cblk->numpasses && layer->numpasses) {
                opj_tgt_setvalue(prc->incltree, cblkno, (int32_t)layno);
            }

            ++cblk;
        }

        cblk = prc->cblks.enc;
        for (cblkno = 0; cblkno < l_nb_blocks; cblkno++) {
            opj_tcd_layer_t *layer = cblk->layers + layno;
            uint32_t increment = 0;
            uint32_t nump = 0;
            uint32_t len = 0, passno;
            uint32_t l_nb_passes;

            /* cblk inclusion bits */
            if (!cblk->numpasses) {
                opj_tgt_encode(bio, prc->incltree, cblkno, (int32_t)(layno + 1));
            } else {
                opj_bio_write(bio, layer->numpasses != 0, 1);
            }

            /* if cblk not included, go to the next cblk  */
            if (!layer->numpasses) {
                ++cblk;
                continue;
            }

            /* if first instance of cblk --> zero bit-planes information */
            if (!cblk->numpasses) {
                cblk->numlenbits = 3;
                opj_tgt_encode(bio, prc->imsbtree, cblkno, 999);
            }

            /* number of coding passes included */
            opj_t2_putnumpasses(bio, layer->numpasses);
            l_nb_passes = cblk->numpasses + layer->numpasses;
            pass = cblk->passes + cblk->numpasses;

            /* computation of the increase of the length indicator and insertion in the header     */
            for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                ++nump;
                len += pass->len;

                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                    increment = (uint32_t)opj_int_max((int32_t)increment, opj_int_floorlog2((int32_t)len) + 1
                                                      - ((int32_t)cblk->numlenbits + opj_int_floorlog2((int32_t)nump)));
                    len = 0;
                    nump = 0;
                }

                ++pass;
            }
            opj_t2_putcommacode(bio, (int32_t)increment);

            /* computation of the new Length indicator */
            cblk->numlenbits += increment;

            pass = cblk->passes + cblk->numpasses;
            /* insertion of the codeword segment length */
            for (passno = cblk->numpasses; passno < l_nb_passes; ++passno) {
                nump++;
                len += pass->len;

                if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
                    opj_bio_write(bio, (uint32_t)len, cblk->numlenbits + (uint32_t)opj_int_floorlog2((int32_t)nump));
                    len = 0;
                    nump = 0;
                }
                ++pass;
            }

            ++cblk;
        }

        ++band;
    }

    if (!opj_bio_flush(bio)) {
        opj_bio_destroy(bio);
        return false;
    }

    l_nb_bytes = (uint32_t)opj_bio_numbytes(bio);
    packet_bytes_written += l_nb_bytes;
    length -= l_nb_bytes;

    opj_bio_destroy(bio);

    /* <EPH 0xff92> */
    if (tcp->csty & J2K_CP_CSTY_EPH) {
        length -= 2;
        packet_bytes_written += 2;
    }
    /* </EPH> */


    /* Writing the packet body */
    band = res->bands;
    for (bandno = 0; bandno < res->numbands; bandno++) {
        opj_tcd_precinct_t *prc = band->precincts + precno;

        l_nb_blocks = prc->cw * prc->ch;
        cblk = prc->cblks.enc;

        for (cblkno = 0; cblkno < l_nb_blocks; ++cblkno) {
            opj_tcd_layer_t *layer = cblk->layers + layno;

            if (!layer->numpasses) {
                ++cblk;
                continue;
            }

            if (layer->len > length) {
                return false;
            }

            cblk->numpasses += layer->numpasses;
            packet_bytes_written += layer->len;
            length -= layer->len;
            ++cblk;
        }
        ++band;
    }
    *p_data_written += packet_bytes_written;

    return true;
}
static bool opj_t2_skip_packet( opj_t2_t* p_t2,
                                opj_tcd_tile_t *p_tile,
                                opj_tcp_t *p_tcp,
                                opj_pi_iterator_t *p_pi,
                                opj_seg_buf_t* src_buf,
                                uint32_t * p_data_read,
                                opj_packet_info_t *p_pack_info,
                                opj_event_mgr_t *p_manager)
{
    bool l_read_data;
    uint32_t l_nb_bytes_read = 0;
    uint32_t l_nb_total_bytes_read = 0;
    uint32_t p_max_length = (uint32_t)opj_seg_buf_get_cur_seg_len(src_buf);

    *p_data_read = 0;

    if (! opj_t2_read_packet_header(p_t2,p_tile,p_tcp,p_pi,&l_read_data,
                                    src_buf,
                                    &l_nb_bytes_read,
                                    p_pack_info, p_manager)) {
        return false;
    }

    l_nb_total_bytes_read += l_nb_bytes_read;
    p_max_length -= l_nb_bytes_read;

    /* we should read data for the packet */
    if (l_read_data) {
        l_nb_bytes_read = 0;

        if (! opj_t2_skip_packet_data(p_t2,p_tile,p_pi,
                                      &l_nb_bytes_read,
                                      p_max_length,p_pack_info, p_manager)) {
            return false;
        }
        opj_seg_buf_incr_cur_seg_offset(src_buf, l_nb_bytes_read);
        l_nb_total_bytes_read += l_nb_bytes_read;
    }
    *p_data_read = l_nb_total_bytes_read;


    return true;
}


static bool opj_t2_read_packet_header( opj_t2_t* p_t2,
                                       opj_tcd_tile_t *p_tile,
                                       opj_tcp_t *p_tcp,
                                       opj_pi_iterator_t *p_pi,
                                       bool * p_is_data_present,
                                       opj_seg_buf_t* src_buf,
                                       uint32_t * p_data_read,
                                       opj_packet_info_t *p_pack_info,
                                       opj_event_mgr_t *p_manager)

{
    uint8_t *p_src_data = opj_seg_buf_get_global_ptr(src_buf);
    uint32_t p_max_length = (uint32_t)opj_seg_buf_get_cur_seg_len(src_buf);
    /* loop */
    uint32_t bandno, cblkno;
    uint32_t l_nb_code_blocks;
    uint32_t l_remaining_length;
    uint32_t l_header_length;
    uint32_t * l_modified_length_ptr = 00;
    uint8_t *l_current_data = p_src_data;
    opj_cp_t *l_cp = p_t2->cp;
    opj_bio_t *l_bio = 00;  /* BIO component */
    opj_tcd_band_t *l_band = 00;
    opj_tcd_cblk_dec_t* l_cblk = 00;
    opj_tcd_resolution_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

    uint8_t *l_header_data = 00;
    uint8_t **l_header_data_start = 00;

    uint32_t l_present;

    if (p_pi->layno == 0) {
        l_band = l_res->bands;

        /* reset tagtrees */
        for (bandno = 0; bandno < l_res->numbands; ++bandno) {
            if ( ! ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) ) {
				opj_tcd_precinct_t *l_prc = &l_band->precincts[p_pi->precno];
				if (!(p_pi->precno < (l_band->precincts_data_size / sizeof(opj_tcd_precinct_t)))) {
					opj_event_msg(p_manager, EVT_ERROR, "Invalid precinct\n");
					return false;
				}
                opj_tgt_reset(l_prc->incltree);
                opj_tgt_reset(l_prc->imsbtree);
                l_cblk = l_prc->cblks.dec;

                l_nb_code_blocks = l_prc->cw * l_prc->ch;
                for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
                    l_cblk->numsegs = 0;
                    l_cblk->real_num_segs = 0;
                    ++l_cblk;
                }
            }

            ++l_band;
        }
    }

    /* SOP markers */

    if (p_tcp->csty & J2K_CP_CSTY_SOP) {
        if (p_max_length < 6) {
            opj_event_msg(p_manager, EVT_WARNING, "Not enough space for expected SOP marker\n");
        } else if ((*l_current_data) != 0xff || (*(l_current_data + 1) != 0x91)) {
            opj_event_msg(p_manager, EVT_WARNING, "Expected SOP marker\n");
        } else {
            l_current_data += 6;
        }

        /** TODO : check the Nsop value */
    }

    /*
    When the marker PPT/PPM is used the packet header are store in PPT/PPM marker
    This part deal with this caracteristic
    step 1: Read packet header in the saved structure
    step 2: Return to codestream for decoding
    */

    l_bio = opj_bio_create();
    if (! l_bio) {
        return false;
    }

    if (l_cp->ppm == 1) { /* PPM */
        l_header_data_start = &l_cp->ppm_data;
        l_header_data = *l_header_data_start;
        l_modified_length_ptr = &(l_cp->ppm_len);

    } else if (p_tcp->ppt == 1) { /* PPT */
        l_header_data_start = &(p_tcp->ppt_data);
        l_header_data = *l_header_data_start;
        l_modified_length_ptr = &(p_tcp->ppt_len);
    } else { /* Normal Case */
        l_header_data_start = &(l_current_data);
        l_header_data = *l_header_data_start;
        l_remaining_length = (uint32_t)(p_src_data+p_max_length-l_header_data);
        l_modified_length_ptr = &(l_remaining_length);
    }

    opj_bio_init_dec(l_bio, l_header_data,*l_modified_length_ptr);

    l_present = opj_bio_read(l_bio, 1);
    JAS_FPRINTF(stderr, "present=%d \n", l_present );
    if (!l_present) {
		if (!opj_bio_inalign(l_bio))
			return false;
        l_header_data += opj_bio_numbytes(l_bio);
        opj_bio_destroy(l_bio);

        /* EPH markers */
        if (p_tcp->csty & J2K_CP_CSTY_EPH) {
            if ((*l_modified_length_ptr - (uint32_t)(l_header_data - *l_header_data_start)) < 2U) {
                opj_event_msg(p_manager, EVT_WARNING, "Not enough space for expected EPH marker\n");
            } else if ((*l_header_data) != 0xff || (*(l_header_data + 1) != 0x92)) {
                opj_event_msg(p_manager, EVT_WARNING, "Expected EPH marker\n");
            } else {
                l_header_data += 2;
            }
        }

        l_header_length = (uint32_t)(l_header_data - *l_header_data_start);
        *l_modified_length_ptr -= l_header_length;
        *l_header_data_start += l_header_length;

        /* << INDEX */
        /* End of packet header position. Currently only represents the distance to start of packet
           Will be updated later by incrementing with packet start value */
        if (p_pack_info) {
            p_pack_info->end_ph_pos = (int32_t)(l_current_data - p_src_data);
        }
        /* INDEX >> */

        * p_is_data_present = false;
        *p_data_read = (uint32_t)(l_current_data - p_src_data);
        opj_seg_buf_incr_cur_seg_offset(src_buf, *p_data_read);
        return true;
    }

    l_band = l_res->bands;
    for (bandno = 0; bandno < l_res->numbands; ++bandno) {
        opj_tcd_precinct_t *l_prc = &(l_band->precincts[p_pi->precno]);

        if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
            ++l_band;
            continue;
        }

        l_nb_code_blocks = l_prc->cw * l_prc->ch;
        l_cblk = l_prc->cblks.dec;
        for (cblkno = 0; cblkno < l_nb_code_blocks; cblkno++) {
            uint32_t l_included,l_increment, l_segno;
            int32_t n;

            /* if cblk not yet included before --> inclusion tagtree */
            if (!l_cblk->numsegs) {
                l_included = opj_tgt_decode(l_bio, l_prc->incltree, cblkno, (int32_t)(p_pi->layno + 1));
                /* else one bit */
            } else {
                l_included = opj_bio_read(l_bio, 1);
            }

            /* if cblk not included */
            if (!l_included) {
                l_cblk->numnewpasses = 0;
                ++l_cblk;
                JAS_FPRINTF(stderr, "included=%d \n", l_included);
                continue;
            }

            /* if cblk not yet included --> zero-bitplane tagtree */
            if (!l_cblk->numsegs) {
                uint32_t i = 0;

                while (!opj_tgt_decode(l_bio, l_prc->imsbtree, cblkno, (int32_t)i)) {
                    ++i;
                }

                l_cblk->numbps = (uint32_t)l_band->numbps + 1 - i;
                l_cblk->numlenbits = 3;
            }

            /* number of coding passes */
            l_cblk->numnewpasses = opj_t2_getnumpasses(l_bio);
            l_increment = opj_t2_getcommacode(l_bio);

            /* length indicator increment */
            l_cblk->numlenbits += l_increment;
            l_segno = 0;

            if (!l_cblk->numsegs) {
                if (! opj_t2_init_seg(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 1)) {
                    opj_bio_destroy(l_bio);
                    return false;
                }
            } else {
                l_segno = l_cblk->numsegs - 1;
                if (l_cblk->segs[l_segno].numpasses == l_cblk->segs[l_segno].maxpasses) {
                    ++l_segno;
                    if (! opj_t2_init_seg(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 0)) {
                        opj_bio_destroy(l_bio);
                        return false;
                    }
                }
            }
            n = (int32_t)l_cblk->numnewpasses;

            do {
                l_cblk->segs[l_segno].numnewpasses = (uint32_t)opj_int_min((int32_t)(l_cblk->segs[l_segno].maxpasses - l_cblk->segs[l_segno].numpasses), n);
                l_cblk->segs[l_segno].newlen = opj_bio_read(l_bio, l_cblk->numlenbits + opj_uint_floorlog2(l_cblk->segs[l_segno].numnewpasses));
                JAS_FPRINTF(stderr, "included=%d numnewpasses=%d increment=%d len=%d \n", l_included, l_cblk->segs[l_segno].numnewpasses, l_increment, l_cblk->segs[l_segno].newlen );

                n -= (int32_t)l_cblk->segs[l_segno].numnewpasses;
                if (n > 0) {
                    ++l_segno;

                    if (! opj_t2_init_seg(l_cblk, l_segno, p_tcp->tccps[p_pi->compno].cblksty, 0)) {
                        opj_bio_destroy(l_bio);
                        return false;
                    }
                }
            } while (n > 0);

            ++l_cblk;
        }

        ++l_band;
    }

    if (!opj_bio_inalign(l_bio)) {
        opj_bio_destroy(l_bio);
        return false;
    }

    l_header_data += opj_bio_numbytes(l_bio);
    opj_bio_destroy(l_bio);

    /* EPH markers */
    if (p_tcp->csty & J2K_CP_CSTY_EPH) {
        if ((*l_modified_length_ptr - (uint32_t)(l_header_data - *l_header_data_start)) < 2U) {
            opj_event_msg(p_manager, EVT_WARNING, "Not enough space for expected EPH marker\n");
        } else if ((*l_header_data) != 0xff || (*(l_header_data + 1) != 0x92)) {
            opj_event_msg(p_manager, EVT_WARNING, "Expected EPH marker\n");
        } else {
            l_header_data += 2;
        }
    }

    l_header_length = (uint32_t)(l_header_data - *l_header_data_start);
    JAS_FPRINTF( stderr, "hdrlen=%d \n", l_header_length );
    JAS_FPRINTF( stderr, "packet body\n");
    *l_modified_length_ptr -= l_header_length;
    *l_header_data_start += l_header_length;

    /* << INDEX */
    /* End of packet header position. Currently only represents the distance to start of packet
     Will be updated later by incrementing with packet start value */
    if (p_pack_info) {
        p_pack_info->end_ph_pos = (int32_t)(l_current_data - p_src_data);
    }
    /* INDEX >> */

    *p_is_data_present = true;
    *p_data_read = (uint32_t)(l_current_data - p_src_data);
    opj_seg_buf_incr_cur_seg_offset(src_buf, *p_data_read);

    return true;
}

static bool opj_t2_read_packet_data(   opj_t2_t* p_t2,
                                       opj_tcd_tile_t *p_tile,
                                       opj_pi_iterator_t *p_pi,
                                       opj_seg_buf_t* src_buf,
                                       uint32_t * p_data_read,
                                       opj_packet_info_t *pack_info,
                                       opj_event_mgr_t* p_manager)
{
    uint32_t bandno, cblkno;
    uint32_t l_nb_code_blocks;
    opj_tcd_band_t *l_band = 00;
    opj_tcd_cblk_dec_t* l_cblk = 00;
    opj_tcd_resolution_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

    OPJ_ARG_NOT_USED(p_t2);
    OPJ_ARG_NOT_USED(pack_info);

    l_band = l_res->bands;
    for (bandno = 0; bandno < l_res->numbands; ++bandno) {
        opj_tcd_precinct_t *l_prc = &l_band->precincts[p_pi->precno];

        if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
            ++l_band;
            continue;
        }

        l_nb_code_blocks = l_prc->cw * l_prc->ch;
        l_cblk = l_prc->cblks.dec;

        for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
            opj_tcd_seg_t *l_seg = 00;

            if (!l_cblk->numnewpasses) {
                /* nothing to do */
                ++l_cblk;
                continue;
            }

            if (!l_cblk->numsegs) {
                l_seg = l_cblk->segs;
                ++l_cblk->numsegs;
                l_cblk->data_current_size = 0;
            } else {
                l_seg = &l_cblk->segs[l_cblk->numsegs - 1];

                if (l_seg->numpasses == l_seg->maxpasses) {
                    ++l_seg;
                    ++l_cblk->numsegs;
                }
            }

            do {
                int64_t offset = opj_seg_buf_get_global_offset(src_buf);
                size_t len = src_buf->data_len;
                /* Check possible overflow (on l_current_data only, assumes input args already checked) then size */
                if ((((size_t)offset + (size_t)l_seg->newlen) >	(size_t)len)) {
                    opj_event_msg(p_manager, EVT_ERROR, "read: segment too long (%d) with max (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                  l_seg->newlen, len, cblkno, p_pi->precno, bandno, p_pi->resno, p_pi->compno);
                    return false;
                }

                /* Check possible overflow on size */
                if ((l_cblk->data_current_size + l_seg->newlen) < l_cblk->data_current_size) {
                    opj_event_msg(p_manager, EVT_ERROR, "read: segment too long (%d) with current size (%d > %d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                  l_seg->newlen, l_cblk->data_current_size, 0xFFFFFFFF - l_seg->newlen, cblkno, p_pi->precno, bandno, p_pi->resno, p_pi->compno);
                    return false;
                }
                if (l_seg->numpasses == 0) {
                    l_seg->dataindex = l_cblk->data_current_size;
                }

                opj_min_buf_vec_push_back(&l_cblk->seg_buffers, opj_seg_buf_get_global_ptr(src_buf), (uint16_t)l_seg->newlen);



                *(p_data_read) += l_seg->newlen;
                opj_seg_buf_incr_cur_seg_offset(src_buf, l_seg->newlen);
                l_seg->numpasses += l_seg->numnewpasses;
                l_cblk->numnewpasses -= l_seg->numnewpasses;

                l_seg->real_num_passes = l_seg->numpasses;
                l_cblk->data_current_size += l_seg->newlen;
                l_seg->len += l_seg->newlen;

                if (l_cblk->numnewpasses > 0) {
                    ++l_seg;
                    ++l_cblk->numsegs;
                }
            } while (l_cblk->numnewpasses > 0);

            l_cblk->real_num_segs = l_cblk->numsegs;
            ++l_cblk;
        } /* next code_block */

        ++l_band;
    }
    return true;
}

static bool opj_t2_skip_packet_data(   opj_t2_t* p_t2,
                                       opj_tcd_tile_t *p_tile,
                                       opj_pi_iterator_t *p_pi,
                                       uint32_t * p_data_read,
                                       uint32_t p_max_length,
                                       opj_packet_info_t *pack_info,
                                       opj_event_mgr_t *p_manager)
{
    uint32_t bandno, cblkno;
    uint32_t l_nb_code_blocks;
    opj_tcd_band_t *l_band = 00;
    opj_tcd_cblk_dec_t* l_cblk = 00;
    opj_tcd_resolution_t* l_res = &p_tile->comps[p_pi->compno].resolutions[p_pi->resno];

    OPJ_ARG_NOT_USED(p_t2);
    OPJ_ARG_NOT_USED(pack_info);

    *p_data_read = 0;
    l_band = l_res->bands;

    for (bandno = 0; bandno < l_res->numbands; ++bandno) {
        opj_tcd_precinct_t *l_prc = &l_band->precincts[p_pi->precno];

        if ((l_band->x1-l_band->x0 == 0)||(l_band->y1-l_band->y0 == 0)) {
            ++l_band;
            continue;
        }

        l_nb_code_blocks = l_prc->cw * l_prc->ch;
        l_cblk = l_prc->cblks.dec;

        for (cblkno = 0; cblkno < l_nb_code_blocks; ++cblkno) {
            opj_tcd_seg_t *l_seg = 00;

            if (!l_cblk->numnewpasses) {
                /* nothing to do */
                ++l_cblk;
                continue;
            }

            if (!l_cblk->numsegs) {
                l_seg = l_cblk->segs;
                ++l_cblk->numsegs;
                l_cblk->data_current_size = 0;
            } else {
                l_seg = &l_cblk->segs[l_cblk->numsegs - 1];

                if (l_seg->numpasses == l_seg->maxpasses) {
                    ++l_seg;
                    ++l_cblk->numsegs;
                }
            }

            do {
                /* Check possible overflow then size */
                if (((*p_data_read + l_seg->newlen) < (*p_data_read)) || ((*p_data_read + l_seg->newlen) > p_max_length)) {
                    opj_event_msg(p_manager, EVT_ERROR, "skip: segment too long (%d) with max (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
                                  l_seg->newlen, p_max_length, cblkno, p_pi->precno, bandno, p_pi->resno, p_pi->compno);
                    return false;
                }

                JAS_FPRINTF(stderr, "p_data_read (%d) newlen (%d) \n", *p_data_read, l_seg->newlen );
                *(p_data_read) += l_seg->newlen;

                l_seg->numpasses += l_seg->numnewpasses;
                l_cblk->numnewpasses -= l_seg->numnewpasses;
                if (l_cblk->numnewpasses > 0) {
                    ++l_seg;
                    ++l_cblk->numsegs;
                }
            } while (l_cblk->numnewpasses > 0);

            ++l_cblk;
        }

        ++l_band;
    }

    return true;
}


static bool opj_t2_init_seg(   opj_tcd_cblk_dec_t* cblk,
                               uint32_t index,
                               uint32_t cblksty,
                               uint32_t first)
{
    opj_tcd_seg_t* seg = 00;
    uint32_t l_nb_segs = index + 1;

    if (l_nb_segs > cblk->m_current_max_segs) {
        opj_tcd_seg_t* new_segs;
        cblk->m_current_max_segs += OPJ_J2K_DEFAULT_NB_SEGS;

        new_segs = (opj_tcd_seg_t*) opj_realloc(cblk->segs, cblk->m_current_max_segs * sizeof(opj_tcd_seg_t));
        if(! new_segs) {
            opj_free(cblk->segs);
            cblk->segs = NULL;
            cblk->m_current_max_segs = 0;
            /* opj_event_msg(p_manager, EVT_ERROR, "Not enough memory to initialize segment %d\n", l_nb_segs); */
            return false;
        }
        cblk->segs = new_segs;
    }

    seg = &cblk->segs[index];
    memset(seg,0,sizeof(opj_tcd_seg_t));

    if (cblksty & J2K_CCP_CBLKSTY_TERMALL) {
        seg->maxpasses = 1;
    } else if (cblksty & J2K_CCP_CBLKSTY_LAZY) {
        if (first) {
            seg->maxpasses = 10;
        } else {
            seg->maxpasses = (((seg - 1)->maxpasses == 1) || ((seg - 1)->maxpasses == 10)) ? 2 : 1;
        }
    } else {
        seg->maxpasses = 109;
    }

    return true;
}
