
#include "config.h"

#include <stdio.h>
#include <time.h>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <memory>

#include "globalregistry.h"
#include "packetchain.h"
#include "timetracker.h"
#include "phy_ti_cc_2531.h"
#include "kis_httpd_registry.h"
#include "devicetracker.h"
#include "dlttracker.h"
#include "manuf.h"

Kis_TICC2531_Phy::Kis_TICC2531_Phy(global_registry *in_globalreg, int in_phyid) :
    kis_phy_handler(in_globalreg, in_phyid) {

    //globalreg = in_globalreg;

    set_phy_name("TICC2531");

    packetchain = 
        Globalreg::fetch_mandatory_global_as<packet_chain>();
    entrytracker = 
        Globalreg::fetch_mandatory_global_as<entry_tracker>();
    devicetracker =
        Globalreg::fetch_mandatory_global_as<device_tracker>();

    ticc2531_device_entry_id =
        entrytracker->register_field("ticc2531.device",
                tracker_element_factory<ticc2531_tracked_device>(),
                "TI CC2531 device");

    pack_comp_common = packetchain->register_packet_component("COMMON");
	pack_comp_linkframe = packetchain->register_packet_component("LINKFRAME");

    // Extract the dynamic DLT
    auto dltt = 
        Globalreg::fetch_mandatory_global_as<dlt_tracker>("DLTTRACKER");
    dlt = dltt->register_linktype("TICC2531");

    /*
    auto httpregistry = 
        Globalreg::FetchMandatoryGlobalAs<Kis_Httpd_Registry>("WEBREGISTRY");
        */

    // Make the manuf string
    //mj_manuf_amazon = Globalreg::globalreg->manufdb->MakeManuf("Amazon");
    //mj_manuf_logitech = Globalreg::globalreg->manufdb->MakeManuf("Logitech");
    //mj_manuf_microsoft = Globalreg::globalreg->manufdb->MakeManuf("Microsoft");
    mj_manuf_ti = Globalreg::globalreg->manufdb->MakeManuf("Texas Instruments");

    packetchain->register_handler(&DissectorTICC2531, this, CHAINPOS_LLCDISSECT, -100);
    packetchain->register_handler(&CommonClassifierTICC2531, this, CHAINPOS_CLASSIFIER, -100);
}

Kis_TICC2531_Phy::~Kis_TICC2531_Phy() {
    packetchain->remove_handler(&CommonClassifierTICC2531, CHAINPOS_CLASSIFIER);
}

int Kis_TICC2531_Phy::DissectorTICC2531(CHAINCALL_PARMS) {
    auto mphy = static_cast<Kis_TICC2531_Phy *>(auxdata);

    auto packdata = in_pack->fetch<kis_datachunk>(mphy->pack_comp_linkframe);

    if (packdata == NULL)
        return 0;

    // Is it a packet we care about?
    if (packdata->dlt != mphy->dlt)
        return 0;

    // Do we have enough data for an OUI?
    if (packdata->length < 6)
        return 0;

    // Did something already classify this?
    auto common = in_pack->fetch<kis_common_info>(mphy->pack_comp_common);

    if (common != NULL)
        return 0;

    common = new kis_common_info;

    common->basic_crypt_set = crypt_none;
    common->type = packet_basic_data;
    common->source = mac_addr(packdata->data, 6);

    in_pack->insert(mphy->pack_comp_common, common);

    return 1;
}

int Kis_TICC2531_Phy::CommonClassifierTICC2531(CHAINCALL_PARMS) {
    auto mphy = static_cast<Kis_TICC2531_Phy *>(auxdata);

    auto packdata = in_pack->fetch<kis_datachunk>(mphy->pack_comp_linkframe);

    if (packdata == nullptr)
        return 0;

    // Is it a packet we care about?
    if (packdata->dlt != mphy->dlt)
        return 0;
/*
    unsigned char payload[128];memset(payload,0x00,128);
    int pkt_len = packdata[1];
    if(pkt_len != (len-3)) {
    printf("packet length mismatch\n");
        return false;
    }
    //get the paylaod
    int p_ctr=0;
    for(int i=8;i<(len-2);i++) {
        payload[p_ctr] = packdata[i];p_ctr++;
    }
    int payload_len = packdata[7] - 0x02;
    if(p_ctr != payload_len) {
            printf("payload size mismatch\n");
            return false;
    }

    unsigned char fcs1 = packdata[len-2];
    unsigned char fcs2 = packdata[len-1];
//rssi is the signed value at fcs1
    int rssi = (fcs1 + (int)pow(2,7)) % (int)pow(2,8) - (int)pow(2,7) - 73;
    unsigned char crc_ok = fcs2 & (1 << 7);
    unsigned char corr = fcs2 & 0x7f;
    if(crc_ok > 0) {
    unsigned char plen = packdata[7];
    unsigned short frame_control = (packdata[9] << 8) + packdata[8];
    unsigned char seq_num = packdata[10];
        //beacon packet
//          unsigned short frame_control = (packdata[9] << 8) + packdata[8];//0x8000
//        unsigned char seq_num = packdata[10];
if(frame_control == 0x8000)
{
    unsigned short source_pan = (packdata[12] << 8) + packdata[11];
    unsigned short source_address = (packdata[14] << 8) + packdata[13];
    unsigned short superrf_spec = (packdata[16] << 8) + packdata[15];
    unsigned short gts_fields = (packdata[18] << 8) + packdata[17];
    printf("\n");
    printf("    BEACON FRAME\n");
    printf("    frame_control:%04X\n",frame_control);
    printf("    seq_num:%02X\n",seq_num);
    printf("    source_pan:%04X\n",source_pan);
    printf("    source_address:%04X\n",source_address);
    printf("    superrf_spec:%04X\n",superrf_spec);
    printf("    gts_fields:%04X\n",gts_fields);
    printf("\n");
}
        //command packet
//          unsigned short frame_control = (packdata[9] << 8) + packdata[8];//0x8023
//          unsigned char seq_num = packdata[10];
else if(frame_control == 0x8023)
{
    unsigned short source_pan = (packdata[12] << 8) + packdata[11];
    unsigned short source_address = (packdata[14] << 8) + packdata[13];
    unsigned short cmd_frame_id = packdata[15];
    printf("\n");
    printf("    COMMAND FRAME\n");
    printf("    frame_control:%04X\n",frame_control);
    printf("    seq_num:%02X\n",seq_num);
    printf("    source_pan:%04X\n",source_pan);
    printf("    source_address:%04X\n",source_address);
    printf("    cmd_frame_id:%02X\n",cmd_frame_id);
    printf("\n");
}
        //data packet
//         unsigned char plen = packdata[7];
//         unsigned short frame_control = (packdata[9] << 8) + packdata[8];//0x8841
//         unsigned char seq_num = packdata[10];
else if(frame_control == 0x8841)
{
    unsigned short dest_pan = (packdata[12] << 8) + packdata[11];
    unsigned short dest_address = (packdata[14] << 8) + packdata[13];
    unsigned short source_address = (packdata[16] << 8) + packdata[15];
    //unsigned short mac_payload = (packdata[18] << 8) + packdata[17];//I think this can be variable.... so use plen
    unsigned short mac_payload_length = plen - 9 - 2;//length of stuff from plen to the payload minus the fcs
    printf("\n");
    printf("    DATA PACKET\n");
    printf("    plen:%02X\n",plen);
    printf("    frame_control:%04X\n",frame_control);
    printf("    seq_num:%02X\n",seq_num);
    printf("    dest_pan:%04X\n",dest_pan);
    printf("    dest_address:%04X\n",dest_address);
    printf("    source_address:%04X\n",source_address);
    //printf("    mac_payload:%04X\n",mac_payload);
    printf("    payload:");
    for(int i=0;i < mac_payload_length;i++)
    printf("%02X",packdata[17+i]);
    printf("\n");
}
*/

/**
    // Did we classify this?
    auto common = in_pack->fetch<kis_common_info>(mphy->pack_comp_common);

    if (common == NULL)
        return 0;

    // Update with all the options in case we can add signal and frequency
    // in the future
    auto device = 
        mphy->devicetracker->update_common_device(common,
                common->source, mphy, in_pack,
                (UCD_UPDATE_SIGNAL | UCD_UPDATE_FREQUENCIES |
                 UCD_UPDATE_PACKETS | UCD_UPDATE_LOCATION |
                 UCD_UPDATE_SEENBY | UCD_UPDATE_ENCRYPTION),
                "TI");
    auto ticc2531 =
        device->get_sub_as<ticc2531_tracked_device>(mphy->ticc2531_device_entry_id);

    if (ticc2531 == NULL) {
        _MSG_INFO("Detected new TI CC 2531 device {}",
                common->source.mac_to_string());
        ticc2531 = std::make_shared<ticc2531_tracked_device>(mphy->ticc2531_device_entry_id);
        device->insert(ticc2531);
    }
/**/
    return 1;
}

void Kis_TICC2531_Phy::load_phy_storage(shared_tracker_element in_storage,
        shared_tracker_element in_device) {
    if (in_storage == nullptr || in_device == nullptr)
        return;

    auto storage = std::static_pointer_cast<tracker_element_map>(in_storage);

    auto ticc2531devi = storage->find(ticc2531_device_entry_id);

    if (ticc2531devi != storage->end()) {
        auto ticc2531dev =
            std::make_shared<ticc2531_tracked_device>(ticc2531_device_entry_id,
                    std::static_pointer_cast<tracker_element_map>(ticc2531devi->second));
        std::static_pointer_cast<tracker_element_map>(in_device)->insert(ticc2531dev);
    }
}
