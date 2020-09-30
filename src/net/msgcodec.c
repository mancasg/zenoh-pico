/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdio.h>
#include "zenoh/private/logging.h"
#include "zenoh/net/private/msgcodec.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
void _zn_payload_encode(z_iobuf_t *buf, const _zn_payload_t *pld)
{
    _Z_DEBUG("Encoding _PAYLOAD\n");

    // Encode the body
    z_zint_t len = pld->iobuf.w_pos - pld->iobuf.r_pos;
    z_zint_encode(buf, len);
    z_iobuf_write_slice(buf, pld->iobuf.buf, pld->iobuf.r_pos, pld->iobuf.w_pos);
}

void _zn_payload_decode_na(z_iobuf_t *buf, _zn_payload_result_t *r)
{
    _Z_DEBUG("Decoding _PAYLOAD\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    z_zint_t len = r_zint.value.zint;

    uint8_t *bs = z_iobuf_read_n(buf, len);
    r->value.payload.iobuf = z_iobuf_wrap_wo(bs, len, 0, len);
}

_zn_payload_result_t _zn_payload_decode(z_iobuf_t *buf)
{
    _zn_payload_result_t r;
    _zn_payload_decode_na(buf, &r);
    return r;
}

/*------------------ Timestamp Field ------------------*/
void _zn_timestamp_encode(z_iobuf_t *buf, const z_timestamp_t *ts)
{
    _Z_DEBUG("Encoding _TIMESTAMP\n");

    // Encode the body
    z_zint_encode(buf, ts->time);
    z_uint8_array_encode(buf, &ts->id);
}

void _zn_timestamp_decode_na(z_iobuf_t *buf, _zn_timestamp_result_t *r)
{
    _Z_DEBUG("Decoding _TIMESTAMP\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.timestamp.time = r_zint.value.zint;

    z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.timestamp.id = r_arr.value.uint8_array;
}

_zn_timestamp_result_t _zn_timestamp_decode(z_iobuf_t *buf)
{
    _zn_timestamp_result_t r;
    _zn_timestamp_decode_na(buf, &r);
    return r;
}

/*------------------ SubMode Field ------------------*/
void zn_sub_mode_encode(z_iobuf_t *buf, const zn_sub_mode_t *fld)
{
    _Z_DEBUG("Encoding _SUB_MODE\n");

    // Encode the header
    z_iobuf_write(buf, fld->header);

    // Encode the body
    if _ZN_HAS_FLAG (fld->header, _ZN_FLAG_Z_P)
        zn_temporal_property_encode(buf, &fld->period);
}

void zn_sub_mode_decode_na(z_iobuf_t *buf, zn_sub_mode_result_t *r)
{
    _Z_DEBUG("Decoding _SUB_MODE\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    r->value.sub_mode.header = z_iobuf_read(buf);

    // Decode the body
    if _ZN_HAS_FLAG (r->value.sub_mode.header, _ZN_FLAG_Z_P)
    {
        zn_temporal_property_result_t r_tp = zn_temporal_property_decode(buf);
        ASSURE_P_RESULT(r_tp, r, ZN_SUB_MODE_PARSE_ERROR)
        r->value.sub_mode.period = r_tp.value.temporal_property;
    }
}

zn_sub_mode_result_t zn_sub_mode_decode(z_iobuf_t *buf)
{
    zn_sub_mode_result_t r;
    zn_sub_mode_decode_na(buf, &r);
    return r;
}

/*------------------ ResKey Field ------------------*/
void zn_res_key_encode(z_iobuf_t *buf, uint8_t header, const zn_res_key_t *fld)
{
    _Z_DEBUG("Encoding _RES_KEY\n");

    // Encode the body
    z_zint_encode(buf, fld->rid);
    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_K))
        z_string_encode(buf, fld->rname);
}

void zn_res_key_decode_na(z_iobuf_t *buf, uint8_t header, zn_res_key_result_t *r)
{
    _Z_DEBUG("Decoding _RES_KEY\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.res_key.rid = r_zint.value.zint;

    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_K))
    {
        z_string_result_t r_str = z_string_decode(buf);
        ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
        r->value.res_key.rname = r_str.value.string;
    }
    else
    {
        r->value.res_key.rname = NULL;
    }
}

zn_res_key_result_t zn_res_key_decode(z_iobuf_t *buf, uint8_t header)
{
    zn_res_key_result_t r;
    zn_res_key_decode_na(buf, header, &r);
    return r;
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachement Decorator ------------------*/
void _zn_attachment_encode(z_iobuf_t *buf, const _zn_attachment_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ATTACHMENT\n");

    // Encode the header
    z_iobuf_write(buf, msg->header);

    // Encode the body
    _zn_payload_encode(buf, &msg->payload);
}

void _zn_attachment_decode_na(z_iobuf_t *buf, uint8_t header, _zn_attachment_p_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ATTACHMENT\n");
    r->tag = Z_OK_TAG;

    // Store the header
    r->value.attachment->header = header;

    // Decode the body
    _zn_payload_result_t r_pld = _zn_payload_decode(buf);
    ASSURE_P_RESULT(r_pld, r, Z_ZINT_PARSE_ERROR)
    r->value.attachment->payload = r_pld.value.payload;
}

_zn_attachment_p_result_t _zn_attachment_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_attachment_p_result_t r;
    _zn_attachment_p_result_init(&r);
    _zn_attachment_decode_na(buf, header, &r);
    return r;
}

/*------------------ ReplyContext Decorator ------------------*/
void _zn_reply_context_encode(z_iobuf_t *buf, const _zn_reply_context_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_REPLY_CONTEXT\n");

    // Encode the header
    z_iobuf_write(buf, msg->header);

    // Encode the body
    z_zint_encode(buf, msg->qid);
    z_zint_encode(buf, msg->source_kind);
    if (!_ZN_HAS_FLAG(msg->header, _ZN_FLAG_Z_F))
        z_uint8_array_encode(buf, &msg->replier_id);
}

void _zn_reply_context_decode_na(z_iobuf_t *buf, uint8_t header, _zn_reply_context_p_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_REPLY_CONTEXT\n");
    r->tag = Z_OK_TAG;

    // Store the header
    r->value.reply_context->header = header;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context->qid = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context->source_kind = r_zint.value.zint;

    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_F))
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.reply_context->replier_id = r_arr.value.uint8_array;
    }
}

_zn_reply_context_p_result_t _zn_reply_context_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_reply_context_p_result_t r;
    _zn_reply_context_p_result_init(&r);
    _zn_reply_context_decode_na(buf, header, &r);
    return r;
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
void _zn_res_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_RESOURCE\n");

    // Encode the body
    z_zint_encode(buf, dcl->rid);
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_res_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_RESOURCE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.res_decl.rid = r_zint.value.zint;

    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.res_decl.key = r_res.value.res_key;
}

_zn_res_decl_result_t _zn_res_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_res_decl_result_t r;
    _zn_res_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Publisher Declaration ------------------*/
void _zn_pub_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_PUBLISHER\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_pub_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.pub_decl.key = r_res.value.res_key;
}

_zn_pub_decl_result_t _zn_pub_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_pub_decl_result_t r;
    _zn_pub_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Subscriber Declaration ------------------*/
void _zn_sub_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_SUBSCRIBER\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
        zn_sub_mode_encode(buf, &dcl->sub_mode);
}

void _zn_sub_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_SUBSCRIBER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.sub_decl.key = r_res.value.res_key;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
    {
        zn_sub_mode_result_t r_smd = zn_sub_mode_decode(buf);
        ASSURE_P_RESULT(r_smd, r, ZN_SUB_MODE_PARSE_ERROR)
        r->value.sub_decl.sub_mode = r_smd.value.sub_mode;
    }
    else
    {
        // Default subscription mode is PUSH
        zn_sub_mode_t sm;
        sm.header = _ZN_SUBMODE_PUSH;
        r->value.sub_decl.sub_mode = sm;
    }
}

_zn_sub_decl_result_t _zn_sub_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_sub_decl_result_t r;
    _zn_sub_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Queryable Declaration ------------------*/
void _zn_qle_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_QUERYABLE\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_qle_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_QUERYABLE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.qle_decl.key = r_res.value.res_key;
}

_zn_qle_decl_result_t _zn_qle_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_qle_decl_result_t r;
    _zn_qle_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Forget Resource Declaration ------------------*/
void _zn_forget_res_decl_encode(z_iobuf_t *buf, const _zn_forget_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_RESOURCE\n");

    // Encode the body
    z_zint_encode(buf, dcl->rid);
}

void _zn_forget_res_decl_decode_na(z_iobuf_t *buf, _zn_forget_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_RESOURCE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_res_decl.rid = r_zint.value.zint;
}

_zn_forget_res_decl_result_t _zn_forget_res_decl_decode(z_iobuf_t *buf)
{
    _zn_forget_res_decl_result_t r;
    _zn_forget_res_decl_decode_na(buf, &r);
    return r;
}

/*------------------ Forget Publisher Declaration ------------------*/
void _zn_forget_pub_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_forget_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_forget_pub_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_forget_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_pub_decl.key = r_res.value.res_key;
}

_zn_forget_pub_decl_result_t _zn_forget_pub_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_forget_pub_decl_result_t r;
    _zn_forget_pub_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Forget Subscriber Declaration ------------------*/
void _zn_forget_sub_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_forget_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_forget_sub_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_forget_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_sub_decl.key = r_res.value.res_key;
}

_zn_forget_sub_decl_result_t _zn_forget_sub_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_forget_sub_decl_result_t r;
    _zn_forget_sub_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Forget Queryable Declaration ------------------*/
void _zn_forget_qle_decl_encode(z_iobuf_t *buf, uint8_t header, const _zn_forget_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_QUERYABLE\n");

    // Encode the body
    zn_res_key_encode(buf, header, &dcl->key);
}

void _zn_forget_qle_decl_decode_na(z_iobuf_t *buf, uint8_t header, _zn_forget_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_QUERYABLE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_res = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_qle_decl.key = r_res.value.res_key;
}

_zn_forget_qle_decl_result_t _zn_forget_qle_decl_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_forget_qle_decl_result_t r;
    _zn_forget_qle_decl_decode_na(buf, header, &r);
    return r;
}

/*------------------ Declaration Field ------------------*/
void _zn_declaration_encode(z_iobuf_t *buf, _zn_declaration_t *dcl)
{
    // Encode the header
    z_iobuf_write(buf, dcl->header);
    // Encode the body
    uint8_t did = _ZN_MID(dcl->header);
    switch (did)
    {
    case _ZN_DECL_RESOURCE:
        _zn_res_decl_encode(buf, dcl->header, &dcl->body.res);
        break;
    case _ZN_DECL_PUBLISHER:
        _zn_pub_decl_encode(buf, dcl->header, &dcl->body.pub);
        break;
    case _ZN_DECL_SUBSCRIBER:
        _zn_sub_decl_encode(buf, dcl->header, &dcl->body.sub);
        break;
    case _ZN_DECL_QUERYABLE:
        _zn_qle_decl_encode(buf, dcl->header, &dcl->body.qle);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        _zn_forget_res_decl_encode(buf, &dcl->body.forget_res);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        _zn_forget_pub_decl_encode(buf, dcl->header, &dcl->body.forget_pub);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        _zn_forget_sub_decl_encode(buf, dcl->header, &dcl->body.forget_sub);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        _zn_forget_qle_decl_encode(buf, dcl->header, &dcl->body.forget_qle);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode declaration with unknown ID(%d)\n", did);
        break;
    }
}

void _zn_declaration_decode_na(z_iobuf_t *buf, _zn_declaration_result_t *r)
{
    r->tag = Z_OK_TAG;

    // Decode the header
    r->value.declaration.header = z_iobuf_read(buf);
    uint8_t mid = _ZN_MID(r->value.declaration.header);

    // Decode the body
    _zn_res_decl_result_t r_rdcl;
    _zn_pub_decl_result_t r_pdcl;
    _zn_sub_decl_result_t r_sdcl;
    _zn_qle_decl_result_t r_qdcl;
    _zn_forget_res_decl_result_t r_frdcl;
    _zn_forget_pub_decl_result_t r_fpdcl;
    _zn_forget_sub_decl_result_t r_fsdcl;
    _zn_forget_qle_decl_result_t r_fqdcl;

    switch (mid)
    {
    case _ZN_DECL_RESOURCE:
        r_rdcl = _zn_res_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_rdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.res = r_rdcl.value.res_decl;
        break;
    case _ZN_DECL_PUBLISHER:
        r_pdcl = _zn_pub_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_pdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.pub = r_pdcl.value.pub_decl;
        break;
    case _ZN_DECL_SUBSCRIBER:
        r_sdcl = _zn_sub_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_sdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.sub = r_sdcl.value.sub_decl;
        break;
    case _ZN_DECL_QUERYABLE:
        r_qdcl = _zn_qle_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_qdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.qle = r_qdcl.value.qle_decl;
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        r_frdcl = _zn_forget_res_decl_decode(buf);
        ASSURE_P_RESULT(r_frdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_res = r_frdcl.value.forget_res_decl;
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        r_fpdcl = _zn_forget_pub_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_fpdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_pub = r_fpdcl.value.forget_pub_decl;
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        r_fsdcl = _zn_forget_sub_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_fsdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_sub = r_fsdcl.value.forget_sub_decl;
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        r_fqdcl = _zn_forget_qle_decl_decode(buf, r->value.declaration.header);
        ASSURE_P_RESULT(r_fqdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_qle = r_fqdcl.value.forget_qle_decl;
        break;
    default:
        _Z_ERROR("WARNING: Trying to decode declaration with unknown ID(%d)\n", mid);
        break;
    }
}

_zn_declaration_result_t _zn_declaration_decode(z_iobuf_t *buf)
{
    _zn_declaration_result_t r;
    _zn_declaration_decode_na(buf, &r);
    return r;
}

/*------------------ Declaration Message ------------------*/
void _zn_declare_encode(z_iobuf_t *buf, const _zn_declare_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DECLARE\n");

    // Encode the body
    z_zint_t len = msg->declarations.length;
    z_zint_encode(buf, len);
    for (z_zint_t i = 0; i < len; ++i)
        _zn_declaration_encode(buf, &msg->declarations.elem[i]);
}

void _zn_declare_decode_na(z_iobuf_t *buf, _zn_declare_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DECLARE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_dlen = z_zint_decode(buf);
    ASSURE_P_RESULT(r_dlen, r, Z_ZINT_PARSE_ERROR)
    size_t len = r_dlen.value.zint;
    r->value.declare.declarations.length = len;
    r->value.declare.declarations.elem = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * len);

    _zn_declaration_result_t *r_decl = (_zn_declaration_result_t *)malloc(sizeof(_zn_declaration_result_t));
    for (size_t i = 0; i < len; ++i)
    {
        _zn_declaration_decode_na(buf, r_decl);
        if (r_decl->tag == Z_OK_TAG)
        {
            r->value.declare.declarations.elem[i] = r_decl->value.declaration;
        }
        else
        {
            r->value.declare.declarations.length = 0;
            free(r->value.declare.declarations.elem);
            free(r_decl);
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_ZENOH_MESSAGE_PARSE_ERROR;
            return;
        }
    }
    free(r_decl);
}

_zn_declare_result_t _zn_declare_decode(z_iobuf_t *buf)
{
    _zn_declare_result_t r;
    _zn_declare_decode_na(buf, &r);
    return r;
}

/*------------------ Data Info Field ------------------*/
void zn_data_info_encode(z_iobuf_t *buf, const zn_data_info_t *fld)
{
    _Z_DEBUG("Encoding ZN_DATA_INFO\n");

    // Encode the flags
    z_zint_encode(buf, fld->flags);

    // Encode the body
    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_SRC_ID)
        z_uint8_array_encode(buf, &fld->source_id);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_SRC_SN)
        z_zint_encode(buf, fld->source_sn);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_RTR_ID)
        z_uint8_array_encode(buf, &fld->first_router_id);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_RTR_SN)
        z_zint_encode(buf, fld->first_router_sn);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_TSTAMP)
        _zn_timestamp_encode(buf, &fld->tstamp);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_KIND)
        z_zint_encode(buf, fld->kind);

    if _ZN_HAS_FLAG (fld->flags, ZN_DATA_INFO_ENC)
        z_zint_encode(buf, fld->encoding);
}

void zn_data_info_decode_na(z_iobuf_t *buf, zn_data_info_result_t *r)
{
    _Z_DEBUG("Decoding ZN_DATA_INFO\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    z_zint_result_t r_flags = z_zint_decode(buf);
    ASSURE_P_RESULT(r_flags, r, Z_ZINT_PARSE_ERROR)
    r->value.data_info.flags = r_flags.value.zint;

    // Decode the body
    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_SRC_ID)
    {
        z_uint8_array_result_t r_sid = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_sid, r, Z_ARRAY_PARSE_ERROR)
        r->value.data_info.source_id = r_sid.value.uint8_array;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_SRC_SN)
    {
        z_zint_result_t r_ssn = z_zint_decode(buf);
        ASSURE_P_RESULT(r_ssn, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.source_sn = r_ssn.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_RTR_ID)
    {
        z_uint8_array_result_t r_rid = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_rid, r, Z_ARRAY_PARSE_ERROR)
        r->value.data_info.first_router_id = r_rid.value.uint8_array;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_RTR_SN)
    {
        z_zint_result_t r_rsn = z_zint_decode(buf);
        ASSURE_P_RESULT(r_rsn, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.first_router_sn = r_rsn.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_TSTAMP)
    {
        _zn_timestamp_result_t r_tsp = _zn_timestamp_decode(buf);
        ASSURE_P_RESULT(r_tsp, r, ZN_TIMESTAMP_PARSE_ERROR)
        r->value.data_info.tstamp = r_tsp.value.timestamp;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_KIND)
    {
        z_zint_result_t r_knd = z_zint_decode(buf);
        ASSURE_P_RESULT(r_knd, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.kind = r_knd.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, ZN_DATA_INFO_ENC)
    {
        z_zint_result_t r_enc = z_zint_decode(buf);
        ASSURE_P_RESULT(r_enc, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.encoding = r_enc.value.zint;
    }
}

zn_data_info_result_t zn_data_info_decode(z_iobuf_t *buf)
{
    zn_data_info_result_t r;
    zn_data_info_decode_na(buf, &r);
    return r;
}

/*------------------ Data Message ------------------*/
void _zn_data_encode(z_iobuf_t *buf, uint8_t header, const _zn_data_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DATA\n");

    // Encode the body
    zn_res_key_encode(buf, header, &msg->key);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_I)
        zn_data_info_encode(buf, &msg->info);

    _zn_payload_encode(buf, &msg->payload);
}

void _zn_data_decode_na(z_iobuf_t *buf, uint8_t header, _zn_data_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DATA\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_key = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.data.key = r_key.value.res_key;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_I)
    {
        zn_data_info_result_t r_dti = zn_data_info_decode(buf);
        ASSURE_P_RESULT(r_dti, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
        r->value.data.info = r_dti.value.data_info;
    }

    _zn_payload_result_t r_pld = _zn_payload_decode(buf);
    ASSURE_P_RESULT(r_pld, r, Z_ZINT_PARSE_ERROR)
    r->value.data.payload = r_pld.value.payload;
}

_zn_data_result_t _zn_data_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_data_result_t r;
    _zn_data_decode_na(buf, header, &r);
    return r;
}

/*------------------ Pull Message ------------------*/
void _zn_pull_encode(z_iobuf_t *buf, uint8_t header, const _zn_pull_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PULL\n");

    // Encode the body
    zn_res_key_encode(buf, header, &msg->key);

    z_zint_encode(buf, msg->pull_id);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_N)
        z_zint_encode(buf, msg->max_samples);
}

void _zn_pull_decode_na(z_iobuf_t *buf, uint8_t header, _zn_pull_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PULL\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    zn_res_key_result_t r_key = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.pull.key = r_key.value.res_key;

    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.pull.pull_id = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_N)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.pull.max_samples = r_zint.value.zint;
    }
}

_zn_pull_result_t _zn_pull_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_pull_result_t r;
    _zn_pull_decode_na(buf, header, &r);
    return r;
}

/*------------------ Query Message ------------------*/
void _zn_query_encode(z_iobuf_t *buf, uint8_t header, const _zn_query_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_QUERY\n");

    // Encode the body
    zn_res_key_encode(buf, header, &msg->key);

    z_string_encode(buf, msg->predicate);

    z_zint_encode(buf, msg->qid);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_T)
        z_zint_encode(buf, msg->target);

    z_zint_encode(buf, msg->consolidation);
}

void _zn_query_decode_na(z_iobuf_t *buf, uint8_t header, _zn_query_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_QUERY\n");
    r->tag = Z_OK_TAG;

    zn_res_key_result_t r_key = zn_res_key_decode(buf, header);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.query.key = r_key.value.res_key;

    z_string_result_t r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.query.predicate = r_str.value.string;

    z_zint_result_t r_qid = z_zint_decode(buf);
    ASSURE_P_RESULT(r_qid, r, Z_ZINT_PARSE_ERROR)
    r->value.query.qid = r_qid.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_T)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.query.target = r_zint.value.zint;
    }

    z_zint_result_t r_con = z_zint_decode(buf);
    ASSURE_P_RESULT(r_con, r, Z_ZINT_PARSE_ERROR)
    r->value.query.consolidation = r_con.value.zint;
}

_zn_query_result_t _zn_query_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_query_result_t r;
    _zn_query_decode_na(buf, header, &r);
    return r;
}

/*------------------ Zenoh Message ------------------*/
void zn_zenoh_message_encode(z_iobuf_t *buf, const zn_zenoh_message_t *msg)
{
    // Encode the decorators if present
    if (msg->attachment)
        _zn_attachment_encode(buf, msg->attachment);
    if (msg->reply_context)
        _zn_reply_context_encode(buf, msg->reply_context);

    // Encode the header
    z_iobuf_write(buf, msg->header);

    // Encode the body
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_DECLARE:
        _zn_declare_encode(buf, &msg->body.declare);
        break;
    case _ZN_MID_DATA:
        _zn_data_encode(buf, msg->header, &msg->body.data);
        break;
    case _ZN_MID_PULL:
        _zn_pull_encode(buf, msg->header, &msg->body.pull);
        break;
    case _ZN_MID_QUERY:
        _zn_query_encode(buf, msg->header, &msg->body.query);
        break;
    case _ZN_MID_UNIT:
        // Do nothing. Unit messages have no body
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", _ZN_MID(msg->header));
        break;
    }
}

#define _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT 0x1
#define _ZN_SESSION_MESSAGE_DECORATOR_REPLY_CONTEXT 0x2
#define _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, f)                        \
    if (!_ZN_HAS_FLAG(f, _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT))    \
        r->value.zenoh_message->attachment = NULL;                     \
    if (!_ZN_HAS_FLAG(f, _ZN_SESSION_MESSAGE_DECORATOR_REPLY_CONTEXT)) \
        r->value.zenoh_message->reply_context = NULL;

void zn_zenoh_message_decode_na(z_iobuf_t *buf, zn_zenoh_message_p_result_t *r)
{
    r->tag = Z_OK_TAG;

    _zn_attachment_p_result_t r_at;
    _zn_reply_context_p_result_t r_rc;
    _zn_declare_result_t r_de;
    _zn_data_result_t r_da;
    _zn_query_result_t r_qu;
    _zn_pull_result_t r_pu;

    uint8_t has_decorator = 0;
    do
    {
        r->value.zenoh_message->header = z_iobuf_read(buf);

        switch (_ZN_MID(r->value.zenoh_message->header))
        {
        case _ZN_MID_ATTACHMENT:
            _ZN_SET_FLAG(has_decorator, _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT);

            r_at = _zn_attachment_decode(buf, r->value.zenoh_message->header);
            ASSURE_P_RESULT(r_at, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->attachment = r_at.value.attachment;
            break;
        case _ZN_MID_REPLY_CONTEXT:
            _ZN_SET_FLAG(has_decorator, _ZN_SESSION_MESSAGE_DECORATOR_REPLY_CONTEXT);

            r_rc = _zn_reply_context_decode(buf, r->value.zenoh_message->header);
            ASSURE_P_RESULT(r_rc, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->reply_context = r_rc.value.reply_context;
            break;
        case _ZN_MID_DECLARE:
            _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, has_decorator)

            r_de = _zn_declare_decode(buf);
            ASSURE_P_RESULT(r_de, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.declare = r_de.value.declare;
            return;
        case _ZN_MID_DATA:
            _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, has_decorator)

            r_da = _zn_data_decode(buf, r->value.zenoh_message->header);
            ASSURE_P_RESULT(r_da, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.data = r_da.value.data;
            return;
        case _ZN_MID_QUERY:
            _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, has_decorator)

            r_qu = _zn_query_decode(buf, r->value.zenoh_message->header);
            ASSURE_P_RESULT(r_qu, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.query = r_qu.value.query;
            return;
        case _ZN_MID_PULL:
            _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, has_decorator)

            r_pu = _zn_pull_decode(buf, r->value.zenoh_message->header);
            ASSURE_P_RESULT(r_pu, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.pull = r_pu.value.pull;
            return;
        case _ZN_MID_UNIT:
            _ZN_INIT_ZENOH_MESSAGE_DECORATORS(r, has_decorator)

            return;
        default:
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_ZENOH_MESSAGE_PARSE_ERROR;
            _Z_ERROR("WARNING: Trying to decode zenoh message with unknown ID(%d)\n", _ZN_MID(r->value.zenoh_message->header));
            return;
        }
    } while (1);
}

zn_zenoh_message_p_result_t zn_zenoh_message_decode(z_iobuf_t *buf)
{
    zn_zenoh_message_p_result_t r;
    zn_zenoh_message_p_result_init(&r);
    zn_zenoh_message_decode_na(buf, &r);
    return r;
}

/*=============================*/
/*       Session Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
void zn_scout_encode(z_iobuf_t *buf, uint8_t header, const zn_scout_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SCOUT\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
        z_zint_encode(buf, msg->what);
}

void zn_scout_decode_na(z_iobuf_t *buf, uint8_t header, zn_scout_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SCOUT\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.scout.what = r_zint.value.zint;
    }
}

zn_scout_result_t zn_scout_decode(z_iobuf_t *buf, uint8_t header)
{
    zn_scout_result_t r;
    zn_scout_decode_na(buf, header, &r);
    return r;
}

/*------------------ Hello Message ------------------*/
void zn_hello_encode(z_iobuf_t *buf, uint8_t header, const zn_hello_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_HELLO\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        z_uint8_array_encode(buf, &msg->pid);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
        z_zint_encode(buf, msg->whatami);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        z_string_array_encode(buf, &msg->locators);
}

void zn_hello_decode_na(z_iobuf_t *buf, uint8_t header, zn_hello_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_HELLO\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.hello.pid = r_arr.value.uint8_array;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.hello.whatami = r_zint.value.zint;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        z_string_array_result_t r_locs = z_string_array_decode(buf);
        ASSURE_P_RESULT(r_locs, r, Z_ARRAY_PARSE_ERROR)
        r->value.hello.locators = r_locs.value.string_array;
    }
}

zn_hello_result_t zn_hello_decode(z_iobuf_t *buf, uint8_t header)
{
    zn_hello_result_t r;
    zn_hello_decode_na(buf, header, &r);
    return r;
}

/*------------------ Open Message ------------------*/
void _zn_open_encode(z_iobuf_t *buf, uint8_t header, const _zn_open_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_OPEN\n");

    // Encode the body
    z_iobuf_write(buf, msg->version);

    z_zint_encode(buf, msg->whatami);

    z_uint8_array_encode(buf, &msg->opid);

    z_zint_encode(buf, msg->lease);

    z_zint_encode(buf, msg->initial_sn);

    // Encode and write the options if any
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_O)
    {
        z_iobuf_write(buf, msg->options);

        if _ZN_HAS_FLAG (msg->options, _ZN_FLAG_S_S)
            z_zint_encode(buf, msg->sn_resolution);

        if _ZN_HAS_FLAG (msg->options, _ZN_FLAG_S_L)
            z_string_array_encode(buf, &msg->locators);
    }
}

void _zn_open_decode_na(z_iobuf_t *buf, uint8_t header, _zn_open_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_OPEN\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    r->value.open.version = z_iobuf_read(buf);

    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.whatami = r_zint.value.zint;

    z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.open.opid = r_arr.value.uint8_array;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.lease = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.initial_sn = r_zint.value.zint;

    // Decode the options if any
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_O)
    {
        r->value.open.options = z_iobuf_read(buf);

        if _ZN_HAS_FLAG (r->value.open.options, _ZN_FLAG_S_S)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.open.sn_resolution = r_zint.value.zint;
        }

        if _ZN_HAS_FLAG (r->value.open.options, _ZN_FLAG_S_L)
        {
            z_string_array_result_t r_locs = z_string_array_decode(buf);
            ASSURE_P_RESULT(r_locs, r, Z_ARRAY_PARSE_ERROR)
            r->value.open.locators = r_locs.value.string_array;
        }
    }
    else
    {
        r->value.open.options = 0;
    }
}

_zn_open_result_t _zn_open_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_open_result_t r;
    _zn_open_decode_na(buf, header, &r);
    return r;
}

/*------------------ Accept Message ------------------*/
void _zn_accept_encode(z_iobuf_t *buf, uint8_t header, const _zn_accept_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACCEPT\n");

    // Encode the body
    z_zint_encode(buf, msg->whatami);

    z_uint8_array_encode(buf, &msg->opid);

    z_uint8_array_encode(buf, &msg->apid);

    z_zint_encode(buf, msg->initial_sn);

    // Encode and write the options
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_O)
    {
        z_iobuf_write(buf, msg->options);

        if _ZN_HAS_FLAG (msg->options, _ZN_FLAG_S_S)
            z_zint_encode(buf, msg->sn_resolution);

        if _ZN_HAS_FLAG (msg->options, _ZN_FLAG_S_D)
            z_zint_encode(buf, msg->lease);

        if _ZN_HAS_FLAG (msg->options, _ZN_FLAG_S_L)
            z_string_array_encode(buf, &msg->locators);
    }
}

void _zn_accept_decode_na(z_iobuf_t *buf, uint8_t header, _zn_accept_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACCEPT\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_wami = z_zint_decode(buf);
    ASSURE_P_RESULT(r_wami, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.whatami = r_wami.value.zint;

    z_uint8_array_result_t r_opid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_opid, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.opid = r_opid.value.uint8_array;

    z_uint8_array_result_t r_apid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_apid, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.apid = r_apid.value.uint8_array;

    z_zint_result_t r_insn = z_zint_decode(buf);
    ASSURE_P_RESULT(r_insn, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.initial_sn = r_insn.value.zint;

    // Decode the options
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_O)
    {
        r->value.accept.options = z_iobuf_read(buf);

        if _ZN_HAS_FLAG (r->value.accept.options, _ZN_FLAG_S_S)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.accept.sn_resolution = r_zint.value.zint;
        }

        if _ZN_HAS_FLAG (r->value.accept.options, _ZN_FLAG_S_D)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.accept.lease = r_zint.value.zint;
        }

        if _ZN_HAS_FLAG (r->value.accept.options, _ZN_FLAG_S_L)
        {
            z_string_array_result_t r_locs = z_string_array_decode(buf);
            ASSURE_P_RESULT(r_locs, r, Z_ARRAY_PARSE_ERROR)
            r->value.accept.locators = r_locs.value.string_array;
        }
    }
    else
    {
        r->value.accept.options = 0;
    }
}

_zn_accept_result_t _zn_accept_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_accept_result_t r;
    _zn_accept_decode_na(buf, header, &r);
    return r;
}

/*------------------ Close Message ------------------*/
void _zn_close_encode(z_iobuf_t *buf, uint8_t header, const _zn_close_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_CLOSE\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        z_uint8_array_encode(buf, &msg->pid);

    z_iobuf_write(buf, msg->reason);
}

void _zn_close_decode_na(z_iobuf_t *buf, uint8_t header, _zn_close_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_CLOSE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.close.pid = r_arr.value.uint8_array;
    }

    r->value.close.reason = z_iobuf_read(buf);
}

_zn_close_result_t _zn_close_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_close_result_t r;
    _zn_close_decode_na(buf, header, &r);
    return r;
}

/*------------------ Sync Message ------------------*/
void _zn_sync_encode(z_iobuf_t *buf, uint8_t header, const _zn_sync_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SYNC\n");

    // Encode the body
    z_zint_encode(buf, msg->sn);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_C)
        z_zint_encode(buf, msg->count);
}

void _zn_sync_decode_na(z_iobuf_t *buf, uint8_t header, _zn_sync_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SYNC\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.sync.sn = r_zint.value.zint;

    if (_ZN_HAS_FLAG(header, _ZN_FLAG_S_R) && _ZN_HAS_FLAG(header, _ZN_FLAG_S_C))
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.sync.count = r_zint.value.zint;
    }
}

_zn_sync_result_t _zn_sync_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_sync_result_t r;
    _zn_sync_decode_na(buf, header, &r);
    return r;
}

/*------------------ AckNack Message ------------------*/
void _zn_ack_nack_encode(z_iobuf_t *buf, uint8_t header, const _zn_ack_nack_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACK_NACK\n");

    // Encode the body
    z_zint_encode(buf, msg->sn);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_M)
        z_zint_encode(buf, msg->mask);
}

void _zn_ack_nack_decode_na(z_iobuf_t *buf, uint8_t header, _zn_ack_nack_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACK_NACK\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.ack_nack.sn = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_M)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.ack_nack.mask = r_zint.value.zint;
    }
}

_zn_ack_nack_result_t _zn_ack_nack_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_ack_nack_result_t r;
    _zn_ack_nack_decode_na(buf, header, &r);
    return r;
}

/*------------------ Keep Alive Message ------------------*/
void _zn_keep_alive_encode(z_iobuf_t *buf, uint8_t header, const _zn_keep_alive_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_KEEP_ALIVE\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        z_uint8_array_encode(buf, &msg->pid);
}

void _zn_keep_alive_decode_na(z_iobuf_t *buf, uint8_t header, _zn_keep_alive_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_KEEP_ALIVE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.keep_alive.pid = r_arr.value.uint8_array;
    }
}

_zn_keep_alive_result_t _zn_keep_alive_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_keep_alive_result_t r;
    _zn_keep_alive_decode_na(buf, header, &r);
    return r;
}

/*------------------ PingPong Messages ------------------*/
void _zn_ping_pong_encode(z_iobuf_t *buf, const _zn_ping_pong_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PING_PONG\n");

    // Encode the body
    z_zint_encode(buf, msg->hash);
}

void _zn_ping_pong_decode_na(z_iobuf_t *buf, _zn_ping_pong_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PING_PONG\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.ping_pong.hash = r_zint.value.zint;
}

_zn_ping_pong_result_t _zn_ping_pong_decode(z_iobuf_t *buf)
{
    _zn_ping_pong_result_t r;
    _zn_ping_pong_decode_na(buf, &r);
    return r;
}

/*------------------ Frame Message ------------------*/
void _zn_frame_encode(z_iobuf_t *buf, uint8_t header, const _zn_frame_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_FRAME\n");

    // Encode the body
    z_zint_encode(buf, msg->sn);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        _zn_payload_encode(buf, &msg->payload.fragment);
    }
    else
    {
        unsigned int len = z_vec_length(&msg->payload.messages);
        for (unsigned int i = 0; i < len; ++i)
            zn_zenoh_message_encode(buf, z_vec_get(&msg->payload.messages, i));
    }
}

void _zn_frame_decode_na(z_iobuf_t *buf, uint8_t header, _zn_frame_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_FRAME\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.frame.sn = r_zint.value.zint;

    // Decode the payload
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        _zn_payload_result_t r_pld = _zn_payload_decode(buf);
        ASSURE_P_RESULT(r_pld, r, ZN_PAYLOAD_PARSE_ERROR);
        r->value.frame.payload.fragment = r_pld.value.payload;
    }
    else
    {
        r->value.frame.payload.messages = z_vec_make(_ZENOH_C_FRAME_MESSAGES_VEC_SIZE);
        while (z_iobuf_readable(buf))
        {
            zn_zenoh_message_p_result_t r_zm = zn_zenoh_message_decode(buf);
            if (r_zm.tag == Z_OK_TAG)
                z_vec_append(&r->value.frame.payload.messages, r_zm.value.zenoh_message);
            else
                return;
        }
    }
}

_zn_frame_result_t _zn_frame_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_frame_result_t r;
    _zn_frame_decode_na(buf, header, &r);
    return r;
}

/*------------------ Session Message ------------------*/
void zn_session_message_encode(z_iobuf_t *buf, const zn_session_message_t *msg)
{
    // Encode the decorators if present
    if (msg->attachment)
        _zn_attachment_encode(buf, msg->attachment);

    // Encode the header
    z_iobuf_write(buf, msg->header);

    // Encode the body
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_SCOUT:
        zn_scout_encode(buf, msg->header, &msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        zn_hello_encode(buf, msg->header, &msg->body.hello);
        break;
    case _ZN_MID_OPEN:
        _zn_open_encode(buf, msg->header, &msg->body.open);
        break;
    case _ZN_MID_ACCEPT:
        _zn_accept_encode(buf, msg->header, &msg->body.accept);
        break;
    case _ZN_MID_CLOSE:
        _zn_close_encode(buf, msg->header, &msg->body.close);
        break;
    case _ZN_MID_SYNC:
        _zn_sync_encode(buf, msg->header, &msg->body.sync);
        break;
    case _ZN_MID_ACK_NACK:
        _zn_ack_nack_encode(buf, msg->header, &msg->body.ack_nack);
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_keep_alive_encode(buf, msg->header, &msg->body.keep_alive);
        break;
    case _ZN_MID_PING_PONG:
        _zn_ping_pong_encode(buf, &msg->body.ping_pong);
        break;
    case _ZN_MID_FRAME:
        _zn_frame_encode(buf, msg->header, &msg->body.frame);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode session message with unknown ID(%d)\n", _ZN_MID(msg->header));
        return;
    }
}

#define _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT 0x1
#define _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, f)                   \
    if (!_ZN_HAS_FLAG(f, _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT)) \
        r->value.session_message->attachment = NULL;

void zn_session_message_decode_na(z_iobuf_t *buf, zn_session_message_p_result_t *r)
{
    r->tag = Z_OK_TAG;

    _zn_attachment_p_result_t r_at;
    zn_scout_result_t r_sc;
    zn_hello_result_t r_he;
    _zn_open_result_t r_op;
    _zn_accept_result_t r_ac;
    _zn_close_result_t r_cl;
    _zn_sync_result_t r_sy;
    _zn_ack_nack_result_t r_an;
    _zn_keep_alive_result_t r_ka;
    _zn_ping_pong_result_t r_pp;
    _zn_frame_result_t r_fr;

    uint8_t has_decorator = 0;
    do
    {
        // Decode the header
        r->value.session_message->header = z_iobuf_read(buf);

        // Decode the body
        switch (_ZN_MID(r->value.session_message->header))
        {
        case _ZN_MID_ATTACHMENT:
            _ZN_SET_FLAG(has_decorator, _ZN_SESSION_MESSAGE_DECORATOR_ATTACHMENT);

            r_at = _zn_attachment_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_at, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->attachment = r_at.value.attachment;
            break;
        case _ZN_MID_SCOUT:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_sc = zn_scout_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_sc, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.scout = r_sc.value.scout;
            return;
        case _ZN_MID_HELLO:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_he = zn_hello_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_he, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.hello = r_he.value.hello;
            return;
        case _ZN_MID_OPEN:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_op = _zn_open_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_op, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.open = r_op.value.open;
            return;
        case _ZN_MID_ACCEPT:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_ac = _zn_accept_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_ac, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.accept = r_ac.value.accept;
            return;
        case _ZN_MID_CLOSE:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_cl = _zn_close_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_cl, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.close = r_cl.value.close;
            return;
        case _ZN_MID_SYNC:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_sy = _zn_sync_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_sy, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.sync = r_sy.value.sync;
            return;
        case _ZN_MID_ACK_NACK:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_an = _zn_ack_nack_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_an, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.ack_nack = r_an.value.ack_nack;
            return;
        case _ZN_MID_KEEP_ALIVE:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_ka = _zn_keep_alive_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_ka, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.keep_alive = r_ka.value.keep_alive;
            return;
        case _ZN_MID_PING_PONG:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_pp = _zn_ping_pong_decode(buf);
            ASSURE_P_RESULT(r_pp, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.ping_pong = r_pp.value.ping_pong;
            return;
        case _ZN_MID_FRAME:
            _ZN_INIT_SESSION_MESSAGE_DECORATORS(r, has_decorator)

            r_fr = _zn_frame_decode(buf, r->value.session_message->header);
            ASSURE_P_RESULT(r_fr, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.frame = r_fr.value.frame;
            return;
        default:
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_SESSION_MESSAGE_PARSE_ERROR;
            _Z_ERROR("WARNING: Trying to decode session message with unknown ID(%d)\n", _ZN_MID(r->value.session_message->header));
            return;
        }
    } while (1);
}

zn_session_message_p_result_t zn_session_message_decode(z_iobuf_t *buf)
{
    zn_session_message_p_result_t r;
    zn_session_message_p_result_init(&r);
    zn_session_message_decode_na(buf, &r);
    return r;
}
