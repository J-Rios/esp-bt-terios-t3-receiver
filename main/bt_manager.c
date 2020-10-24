/*
 * Copyright (C) 2017 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

/*************************************************************************************************/

#include "bt_manager.h"
#include "config.h"
#include "gamepad_manager.h"

#include "btstack.h"
#include "btstack_config.h"
#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "hci_dump.h"

#include <inttypes.h>

/*************************************************************************************************/

#define ENABLE_LOG_DEBUG
#define L2CAP_CHANNEL_MTU 128

/*************************************************************************************************/

static bd_addr_t remote_addr;

static uint16_t           l2cap_hid_control_cid;
static uint16_t           l2cap_hid_interrupt_cid;
static btstack_packet_callback_registration_t hci_event_callback_registration;

/*************************************************************************************************/


/* @section Main application configuration
 *
 * @text In the application configuration, L2CAP is initialized 
 */

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static void hid_host_setup(void)
{
    // Initialize L2CAP 
    l2cap_init();

    // register L2CAP Services for reconnections
    l2cap_register_service(packet_handler, PSM_HID_INTERRUPT, 0xffff, gap_get_security_level());
    l2cap_register_service(packet_handler, PSM_HID_CONTROL, 0xffff, gap_get_security_level());

    // Allow sniff mode requests by HID device and support role switch
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);

    // try to become master on incoming connections
    hci_set_master_slave_policy(HCI_ROLE_MASTER);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Disable stdout buffering
    setbuf(stdout, NULL);
}

/* @section SDP parser callback 
 * 
 * @text The SDP parsers retrieves the BNEP PAN UUID as explained in  
 * Section [on SDP BNEP Query example](#sec:sdpbnepqueryExample}.
 */

static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t status;

    switch (hci_event_packet_get_type(packet))
    {
        case SDP_EVENT_QUERY_COMPLETE:
                status = l2cap_create_channel(packet_handler, remote_addr, PSM_HID_CONTROL, L2CAP_CHANNEL_MTU, &l2cap_hid_control_cid);
                printf("SDP_EVENT_QUERY_COMPLETE l2cap_create_channel 0x%02x\n", status);
                if (status){
                    printf("Connecting to HID Control failed: 0x%02x\n", status);
                }
            break;
    }
}

static void list_link_keys(void)
{
    bd_addr_t  addr;
    link_key_t link_key;
    link_key_type_t type;
    btstack_link_key_iterator_t it;

    int ok = gap_link_key_iterator_init(&it);
    if (!ok) {
        printf("Link key iterator not implemented\n");
        return;
    }
    uint8_t delete_keys = 0; //uni_platform_is_button_pressed();
    if (delete_keys)
        printf("Deleting stored link keys:\n");
    else
        printf("Stored link keys:\n");
    while (gap_link_key_iterator_get_next(&it, addr, link_key, &type)) {
        printf("%s - type %u, key: ", bd_addr_to_str(addr), (int)type);
        debug_hexdump(link_key, 16);
        if (delete_keys) {
        gap_drop_link_key_for_bd_addr(addr);
        }
    }
    printf(".\n");
    gap_link_key_iterator_done(&it);
}

static void on_l2cap_incoming_connection(uint16_t channel, uint8_t* packet, uint16_t size)
{
    uint16_t local_cid, remote_cid;
    uint16_t psm;
    hci_con_handle_t handle;

    UNUSED(size);

    psm = l2cap_event_incoming_connection_get_psm(packet);
    handle = l2cap_event_incoming_connection_get_handle(packet);
    remote_cid = l2cap_event_incoming_connection_get_remote_cid(packet);
    local_cid = l2cap_event_channel_opened_get_local_cid(packet);

    printf("L2CAP_EVENT_INCOMING_CONNECTION (psm=0x%04x, local_cid=0x%04x, "
    "remote_cid=0x%04x, handle=0x%04x, channel=0x%04x\n", psm, local_cid, remote_cid, handle, channel);
    switch (psm)
    {
        case PSM_HID_CONTROL:
            l2cap_hid_control_cid = channel;
            l2cap_accept_connection(channel);
            break;
        case PSM_HID_INTERRUPT:
            l2cap_hid_interrupt_cid = channel;
            l2cap_accept_connection(channel);
            break;
        default:
            printf("Unknown PSM = 0x%02x\n", psm);
    }
}

static void on_l2cap_channel_closed(uint16_t channel, uint8_t* packet, int16_t size)
{
    uint16_t local_cid;

    UNUSED(size);

    local_cid = l2cap_event_channel_closed_get_local_cid(packet);
    printf("L2CAP_EVENT_CHANNEL_CLOSED: 0x%04x (channel=0x%04x)\n", local_cid, channel);
}

static void on_l2cap_channel_opened(uint16_t channel, uint8_t* packet, uint16_t size)
{
    uint16_t psm;
    uint8_t status;
    uint16_t local_cid, remote_cid;
    uint16_t local_mtu, remote_mtu;
    hci_con_handle_t handle;
    bd_addr_t address;
    uint8_t incoming;

    UNUSED(size);

    printf("L2CAP_EVENT_CHANNEL_OPENED (channel=0x%04x)\n", channel);

    l2cap_event_channel_opened_get_address(packet, address);
    status = l2cap_event_channel_opened_get_status(packet);
    if (status) {
        printf("L2CAP Connection failed: 0x%02x.\n", status);
        // Practice showed that if any of these two status are received, it is
        // best to remove the link key. But this is based on empirical evidence,
        // not on theory.
        if (status == L2CAP_CONNECTION_RESPONSE_RESULT_RTX_TIMEOUT ||
            status == L2CAP_CONNECTION_BASEBAND_DISCONNECT)
        {
            printf("Removing previous link key for address=%s.\n",
            bd_addr_to_str(address));
            //uni_hid_device_remove_entry_with_channel(channel);
            // Just in case the key is outdated we remove it. If fixes some
            // l2cap_channel_opened issues. It proves that it works when the status
            // is 0x6a (L2CAP_CONNECTION_BASEBAND_DISCONNECT).
            gap_drop_link_key_for_bd_addr(address);
        }
        return;
    }
    psm = l2cap_event_channel_opened_get_psm(packet);
    local_cid = l2cap_event_channel_opened_get_local_cid(packet);
    remote_cid = l2cap_event_channel_opened_get_remote_cid(packet);
    handle = l2cap_event_channel_opened_get_handle(packet);
    incoming = l2cap_event_channel_opened_get_incoming(packet);
    local_mtu = l2cap_event_channel_opened_get_local_mtu(packet);
    remote_mtu = l2cap_event_channel_opened_get_remote_mtu(packet);

    printf(
    "PSM: 0x%04x, local CID=0x%04x, remote CID=0x%04x, handle=0x%04x, "
    "incoming=%d, local MTU=%d, remote MTU=%d\n",
    psm, local_cid, remote_cid, handle, incoming, local_mtu, remote_mtu);

    switch (psm)
    {
        case PSM_HID_CONTROL:
            l2cap_hid_control_cid = l2cap_event_channel_opened_get_local_cid(packet);
            if(!incoming)
            {
                printf("l2cap_create_channel PSM_HID_INTERRUPT");
                status = l2cap_create_channel(packet_handler, address, PSM_HID_INTERRUPT, L2CAP_CHANNEL_MTU, &l2cap_hid_interrupt_cid);
                if (status)
                {
                    printf("Connecting to HID Control failed: 0x%02x\n", status);
                    break;
                }
            }
            break;

        case PSM_HID_INTERRUPT:
            l2cap_hid_interrupt_cid = l2cap_event_channel_opened_get_local_cid(packet);
            break;
    }
}

/*
 * @section Packet Handler
 * 
 * @text The packet handler responds to various HCI Events.
 */

/* LISTING_START(packetHandler): Packet Handler */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    /* LISTING_PAUSE */
    uint8_t   event;
    bd_addr_t event_addr;
    uint8_t   status;

    /* LISTING_RESUME */
    switch (packet_type)
    {
		case HCI_EVENT_PACKET:
            event = hci_event_packet_get_type(packet);

            //printf("HCI_EVENT_PACKET: 0x%02x\n", hci_event_packet_get_type(packet));

            switch (event)
            {
                /* @text When BTSTACK_EVENT_STATE with state HCI_STATE_WORKING
                 * is received and the example is started in client mode, the remote SDP HID query is started.
                 */
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
                    {
                        printf("Start SDP HID query for remote HID Device.\n");
                        list_link_keys();
                        sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr, BLUETOOTH_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE_SERVICE);
                    }
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%" PRIu32 "'\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\n");
                    break;

                case HCI_EVENT_COMMAND_COMPLETE:
                {
                    uint16_t opcode = hci_event_command_complete_get_command_opcode(packet);
                    const uint8_t* param = hci_event_command_complete_get_return_parameters(packet);
                    
                    status = param[0];
                    printf("--> HCI_EVENT_COMMAND_COMPLETE: opcode = 0x%04x - status=%d\n", opcode, status);
                    break;
                }
                case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
                {
                    status = hci_event_authentication_complete_get_status(packet);
                    uint16_t handle =
                        hci_event_authentication_complete_get_connection_handle(packet);
                    printf(
                        "--> HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT: status=%d, "
                        "handle=0x%04x\n",
                        status, handle);
                    break;
                }

                case HCI_EVENT_ROLE_CHANGE:
                    printf("--> HCI_EVENT_ROLE_CHANGE\n");
                    break;

                case HCI_EVENT_CONNECTION_REQUEST:
                {
                    printf("--> HCI_EVENT_CONNECTION_REQUEST: link_type = %d <--\n", hci_event_connection_request_get_link_type(packet));
                    hci_event_connection_request_get_bd_addr(packet, event_addr);
                    hci_event_connection_request_get_class_of_device(packet);
                    break;
                }

                case HCI_EVENT_CONNECTION_COMPLETE: 
                {
                    printf("--> HCI_EVENT_CONNECTION_COMPLETE\n");
                    hci_event_connection_complete_get_bd_addr(packet, event_addr);
                    status = hci_event_connection_complete_get_status(packet);
                    if (status) {
                        printf("on_hci_connection_complete failed (0x%02x) for %s\n", status,
                            bd_addr_to_str(event_addr));
                        return;
                    }
                    break;
                }
                
                case L2CAP_EVENT_INCOMING_CONNECTION:
                    printf("--> L2CAP_EVENT_INCOMING_CONNECTION\n");
                    on_l2cap_incoming_connection(channel, packet, size);
                    break;

                case L2CAP_EVENT_CHANNEL_CLOSED:
                    on_l2cap_channel_closed(channel, packet, size);
                    break;

                case L2CAP_EVENT_CHANNEL_OPENED: 
                    on_l2cap_channel_opened(channel, packet, size);
                    break;

                default:
                    break;
            }
            break;
        case L2CAP_DATA_PACKET:
            debug("L2CAP_DATA_PACKET ");
            
            // for now, just dump incoming data
            if (channel == l2cap_hid_interrupt_cid)
            {
                debug_hexdump(packet, size);
                gamepad_handler(packet, size);
            }
            else if (channel == l2cap_hid_control_cid)
            {
                printf("HID Control: ");
                debug_hexdump(packet, size);
            }
            else
                break;

        default:
            //printf("default: 0x%02x\n", hci_event_packet_get_type(packet));
            break;
    }
}

/*************************************************************************************************/

void btstack_run(void)
{
    btstack_run_loop_execute();
}

/*************************************************************************************************/

int btstack_main(int argc, const char * argv[])
{
    (void)argc;
    (void)argv;

    // Configure BTstack
    btstack_init();

    // Setup for HID Host
    hid_host_setup();

    // Parse human readable Bluetooth address
    sscanf_bd_addr(remote_addr_string, remote_addr);

    // Turn on the device 
    hci_power_control(HCI_POWER_ON);

    return 0;
}
