/**
* Copyright 2016 The Open Source Research Group,
*                University of Erlangen-Nürnberg
*
* Licensed under the GNU AFFERO GENERAL PUBLIC LICENSE, Version 3.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     https://www.gnu.org/licenses/gpl.html
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "config.h"
#include <epan/proto.h>

#include "doip-header.h"
#include "doip-helper.h"
#include "doip-payload-8001.h"

/* Source address */
static gint hf_sa = -1; 

/* Target address */
static gint hf_ta = -1; 

/* User data */
static gint hf_ud = -1; 

static gint ett_diagnostic_msg = -1;

static void
fill_tree(doip_header *, proto_tree *tree, tvbuff_t *tvb);

static const gchar *description = "Diagnostic message";
static const gchar *description_format = "Diagnostic message [Source addr: %#x, Dest addr: %#x]";


/* values which will be displayed for payload type 8001 in proto_tree */
void
register_proto_doip_payload_8001(gint proto_doip)
{
    static hf_register_info hf[] =
    {
        /* prepare info for the header field
         * based on ISO 13400-2:2012(E) table 26
        */
        {
            &hf_sa,
            {
                "Source address",
                "doip.sa",
                FT_UINT16,
                BASE_HEX,
                NULL,
                0x0,
                "Contains the logical address of the sender of a diagnostic message (e.g. the external test equipment address).",
                HFILL
            }
        },
        {
            &hf_ta,
            {
                "Target address",
                "doip.ta",
                FT_UINT16,
                BASE_HEX,
                NULL,
                0x0,
                "Contains the logical address of the receiver of a diagnostic message (e.g. a specific ECU on the vehicle's network).",
                HFILL
            }
        },
        {
            &hf_ud,
            {
                "User data",
                "doip.ud",
                FT_BYTES,  
                BASE_NONE,
                NULL,
                0x0,
                "Contains the actual diagnostic data (e.g. a ISO 14229-1 diagnostic request) which shall be routed to the destination (e.g. the ECM).",
                HFILL
            }
        }
    };


    static gint *ett[] = 
    {
        &ett_diagnostic_msg
    };

    /* one-time registration after Wireshark is started */
    proto_register_field_array(proto_doip, hf, array_length(hf));  
    proto_register_subtree_array(ett, array_length(ett));
}

/* After a doip row is selected in Wireshark */
void
dissect_payload_8001(doip_header *header, proto_item *pitem, packet_info *pinfo)       
{
    tvbuff_t *tvb;
    proto_tree *doip_tree;

    guint16 source_addr;
    guint16 dest_addr;

    /* set info column to description */
    if(get_guint16_from_message(header, &source_addr, 0)
        && get_guint16_from_message(header, &dest_addr, 2))
    {
        col_add_fstr(pinfo->cinfo, COL_INFO, description_format, source_addr, dest_addr);
    }
    else
    {
        col_set_str(pinfo->cinfo, COL_INFO, description);
    }

    tvb = retrieve_tvbuff(header);
    /* attach a new tree to proto_item pitem */
    doip_tree = proto_item_add_subtree(pitem, ett_diagnostic_msg);

    /* check for a valid tvbuff_t */
    if(doip_tree && tvb)
    {
        fill_tree(header, doip_tree, tvb);
    }
}

static void
fill_tree(doip_header *header, proto_tree *tree, tvbuff_t *tvb)
{
    /* Values taken from ISO 13400-2:2012(E) table 26
    *
    * Constants starting with prefix "REL_" indicate a relative
    * offset to a doip-messages payload.
    * In order to get the absolute offset starting from the very
    * first doip-header byte we have to calculate the
    * absolute position
    */
    const gint REL_SRC_ADDR_POS = 0;
    const gint SRC_ADDR_LEN = 2;

    const gint REL_TARGET_ADDR_POS = 2;
    const gint TARGET_ADDR_LEN = 2;

    const gint REL_DIAGNOSTIC_MSG_POS = 4;
    gint diagnostic_msg_len;

    gint payload_len;

    payload_len = header->payload.length;
    diagnostic_msg_len = payload_len - REL_DIAGNOSTIC_MSG_POS;

    insert_item_to_tree(tree, hf_sa, tvb, REL_SRC_ADDR_POS, SRC_ADDR_LEN, ENC_BIG_ENDIAN);
    insert_item_to_tree(tree, hf_ta, tvb, REL_TARGET_ADDR_POS, TARGET_ADDR_LEN, ENC_BIG_ENDIAN);
    /* if(diagnostic_msg_len > 0)*/
    if(diagnostic_msg_len)
    {
        insert_item_to_tree(tree, hf_ud, tvb, REL_DIAGNOSTIC_MSG_POS, diagnostic_msg_len, ENC_BIG_ENDIAN);
    }
}

