#ifndef _M4_P4_
#define _M4_P4_

#include <core.p4>
#include <tna.p4>

const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<8> IPV4PROTOCOL_UDP = 17;

#define RECIRCULATE_IN_PORT 0xD
#define RECIRCULATE_OUT_PORT 0xD

#define FLOW_ID_BITS 32
#define LATENCY_BITS 16

#define L1_BUCKET_INDEX_BITS 9
#define L1_BUCKETS (1 << L1_BUCKET_INDEX_BITS)
#define L1_MX_BITS LATENCY_BITS
#define L1_COUNTER_OFFSET_BITS 2
#define L1_COUNTER_INDEX_BITS (L1_BUCKET_INDEX_BITS + L1_COUNTER_OFFSET_BITS)
#define L1_COUNTERS (L1_BUCKETS * (1 << L1_COUNTER_OFFSET_BITS))
#define L1_COUNTER_BITS 8

#define DD_SEGMENT_INDEX_BITS 4
#define DD_SEGMENTS (1 << DD_SEGMENT_INDEX_BITS)

#define L2_BUCKET_INDEX_BITS 8
#define L2_BUCKETS (1 << L2_BUCKET_INDEX_BITS)
#define L2_COUNTER_INDEX_BITS (L2_BUCKET_INDEX_BITS + DD_SEGMENT_INDEX_BITS)
#define L2_COUNTERS (L2_BUCKETS * DD_SEGMENTS)
#define L2_COUNTER_BITS 8

#define L3_BUCKET_INDEX_BITS 6
#define L3_BUCKETS (1 << L3_BUCKET_INDEX_BITS)
#define L3_COUNTER_INDEX_BITS (L3_BUCKET_INDEX_BITS + DD_SEGMENT_INDEX_BITS)
#define L3_COUNTERS (L3_BUCKETS * DD_SEGMENTS)
#define L3_COUNTER_BITS 16

#define L4_BUCKET_INDEX_BITS 4
#define L4_BUCKETS (1 << L4_BUCKET_INDEX_BITS)
#define L4_COUNTER_INDEX_BITS (L4_BUCKET_INDEX_BITS + DD_SEGMENT_INDEX_BITS)
#define L4_COUNTERS (L4_BUCKETS * DD_SEGMENTS)
#define L4_COUNTER_BITS 32

header ethernet_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> total_len;
    bit<16> checksum;
}

header recirculate_h {
    bit<2> insert_level;
    @padding bit<6> _pad0;
}

header my_protocol_h {
    bit<FLOW_ID_BITS> flow_id;
    bit<LATENCY_BITS> latency;
}

header egress_data_h {
    bool is_recirculated;
    bool overflowed_l1;
    @padding bit<6> _pad0;

    bit<L4_COUNTER_INDEX_BITS> counter_index_l4_1;
    bit<L3_COUNTER_INDEX_BITS> counter_index_l3_1;
    @padding bit<6> _pad4;
    bit<L2_COUNTER_INDEX_BITS> counter_index_l2_1;
    @padding bit<4> _pad1;
    
    bit<L4_COUNTER_INDEX_BITS> counter_index_l4_2;
    bit<L3_COUNTER_INDEX_BITS> counter_index_l3_2;
    @padding bit<6> _pad5;
    bit<L2_COUNTER_INDEX_BITS> counter_index_l2_2;
    @padding bit<4> _pad2;

    bit<L4_COUNTER_INDEX_BITS> counter_index_l4_3;
    bit<L3_COUNTER_INDEX_BITS> counter_index_l3_3;
    @padding bit<6> _pad6;
    bit<L2_COUNTER_INDEX_BITS> counter_index_l2_3;
    @padding bit<4> _pad3;
}

struct my_ingress_header_t {
    egress_data_h egress_data;
    recirculate_h recirculate;
    ethernet_h ethernet;
    ipv4_h ipv4;
    udp_h udp;
    my_protocol_h my_protocol;
}

struct my_ingress_metadata_t {
    bit<32> hash_1;
    bit<32> hash_2;
    bit<32> hash_3;

    bit<DD_SEGMENT_INDEX_BITS> dd_segment;

    bit<L1_COUNTER_INDEX_BITS> counter_index_l1_1;
    bit<L1_COUNTER_INDEX_BITS> counter_index_l1_2;
    bit<L1_COUNTER_INDEX_BITS> counter_index_l1_3;

    bool overflowed_l1_1;
    bool overflowed_l1_2;
    bool overflowed_l1_3;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------------------- INGRESS -----------------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_header_t           hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md) 
{
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition select(ig_intr_md.ingress_port) {
            RECIRCULATE_IN_PORT: parse_recirculate;
            default: meta_init;
        }
    }

    state parse_recirculate {
        pkt.extract(hdr.recirculate);
        transition meta_init;
    }

    state meta_init {
        
        meta.hash_1 = 0;
        meta.hash_2 = 0;
        meta.hash_3 = 0;

        meta.dd_segment = 0;

        meta.counter_index_l1_1 = 0;
        meta.counter_index_l1_2 = 0;
        meta.counter_index_l1_3 = 0;
        
        meta.overflowed_l1_1 = false;
        meta.overflowed_l1_2 = false;
        meta.overflowed_l1_3 = false;

        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPV4PROTOCOL_UDP: parse_udp;
            default: accept;
        }
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        transition parse_my_protocol;
    }

    state parse_my_protocol {
        pkt.extract(hdr.my_protocol);
        transition accept;
    }
}

control Ingress(
    /* User */
    inout my_ingress_header_t                        hdr,
    inout my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
    
    /////////////////////////////////// hash ///////////////////////////////////
    
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;

    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32a) hash_1;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32b) hash_2;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32c) hash_3;

    action apply_hash_1() {
        meta.hash_1 = hash_1.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_1 {
        actions = {
            apply_hash_1;
        }
        size = 1;
        const default_action = apply_hash_1;
    }

    action apply_hash_2() {
        meta.hash_2 = hash_2.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_2 {
        actions = {
            apply_hash_2;
        }
        size = 1;
        const default_action = apply_hash_2;
    }

    action apply_hash_3() {
        meta.hash_3 = hash_3.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_3 {
        actions = {
            apply_hash_3;
        }
        size = 1;
        const default_action = apply_hash_3;
    }

    /////////////////////////////////// DDSketch seg ///////////////////////////////////

    action set_seg(bit<DD_SEGMENT_INDEX_BITS> segment) {
        meta.dd_segment = segment;
    }

    // @stage(0)
    table table_dd_seg {
        key = {
            hdr.my_protocol.latency : lpm;
        }
        actions = {
            set_seg;
        }
        size = 8;
    }

    /////////////////////////////////// L1 counter ///////////////////////////////////

    /* -------------------------------- L1 1 -------------------------------- */
    Register<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>>(L1_COUNTERS) register_counter_l1_1;
    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_1) action_counter_l1_1 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_counter_l1_1() {
        action_counter_l1_1.execute(meta.counter_index_l1_1);
    }

    // @stage(2)
    table table_counter_l1_1 {
        actions = {
            apply_counter_l1_1;
        }
        size = 1;
        const default_action = apply_counter_l1_1;
    }
    
    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_1) action_lookup_l1_1 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };
    
    action apply_lookup_l1_1() {
        meta.overflowed_l1_1 = action_lookup_l1_1.execute(meta.counter_index_l1_1);
    }

    // @stage(2)
    table table_lookup_l1_1 {
        actions = {
            apply_lookup_l1_1;
        }
        size = 1;
        const default_action = apply_lookup_l1_1;
    }

    /* -------------------------------- L1 2 -------------------------------- */
    Register<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>>(L1_COUNTERS) register_counter_l1_2;
    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_2) action_counter_l1_2 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_counter_l1_2() {
        action_counter_l1_2.execute(meta.counter_index_l1_2);
    }

    // @stage(2)
    table table_counter_l1_2 {
        actions = {
            apply_counter_l1_2;
        }
        size = 1;
        const default_action = apply_counter_l1_2;
    }

    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_2) action_lookup_l1_2 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };
    
    action apply_lookup_l1_2() {
        meta.overflowed_l1_2 = action_lookup_l1_2.execute(meta.counter_index_l1_2);
    }

    // @stage(2)
    table table_lookup_l1_2 {
        actions = {
            apply_lookup_l1_2;
        }
        size = 1;
        const default_action = apply_lookup_l1_2;
    }

    /* -------------------------------- L1 3 -------------------------------- */
    Register<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>>(L1_COUNTERS) register_counter_l1_3;
    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_3) action_counter_l1_3 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_counter_l1_3() {
        action_counter_l1_3.execute(meta.counter_index_l1_3);
    }

    // @stage(2)
    table table_counter_l1_3 {
        actions = {
            apply_counter_l1_3;
        }
        size = 1;
        const default_action = apply_counter_l1_3;
    }

    RegisterAction<bit<L1_COUNTER_BITS>, bit<L1_COUNTER_INDEX_BITS>, bool>(register_counter_l1_3) action_lookup_l1_3 = {
        void apply(inout bit<L1_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };
    
    action apply_lookup_l1_3() {
        meta.overflowed_l1_3 = action_lookup_l1_3.execute(meta.counter_index_l1_3);
    }

    // @stage(2)
    table table_lookup_l1_3 {
        actions = {
            apply_lookup_l1_3;
        }
        size = 1;
        const default_action = apply_lookup_l1_3;
    }

    /////////////////////////////////// L1 mx ///////////////////////////////////
    
    /* -------------------------------- L1 1 -------------------------------- */
    Register<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>>(L1_BUCKETS) register_mx_l1_1;
    RegisterAction<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>, bit<L1_MX_BITS>>(register_mx_l1_1) action_mx_l1_1 = {
        void apply(inout bit<L1_MX_BITS> reg_data, out bit<L1_MX_BITS> max_latency) {
            reg_data = max(reg_data, hdr.my_protocol.latency);
            max_latency = reg_data;
        }
    };

    action apply_mx_l1_1() {
        action_mx_l1_1.execute((bit<L1_BUCKET_INDEX_BITS>)(meta.counter_index_l1_1 >> L1_COUNTER_OFFSET_BITS));
    }

    // @stage(3)
    table table_mx_l1_1 {
        actions = {
            apply_mx_l1_1;
        }
        size = 1;
        const default_action = apply_mx_l1_1;
    }

    /* -------------------------------- L1 2 -------------------------------- */
    Register<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>>(L1_BUCKETS) register_mx_l1_2;
    RegisterAction<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>, bit<L1_MX_BITS>>(register_mx_l1_2) action_mx_l1_2 = {
        void apply(inout bit<L1_MX_BITS> reg_data, out bit<L1_MX_BITS> max_latency) {
            reg_data = max(reg_data, hdr.my_protocol.latency);
            max_latency = reg_data;
        }
    };

    action apply_mx_l1_2() {
        action_mx_l1_2.execute((bit<L1_BUCKET_INDEX_BITS>)(meta.counter_index_l1_2 >> L1_COUNTER_OFFSET_BITS));
    }

    // @stage(3)
    table table_mx_l1_2 {
        actions = {
            apply_mx_l1_2;
        }
        size = 1;
        const default_action = apply_mx_l1_2;
    }

    /* -------------------------------- L1 3 -------------------------------- */
    Register<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>>(L1_BUCKETS) register_mx_l1_3;
    RegisterAction<bit<L1_MX_BITS>, bit<L1_BUCKET_INDEX_BITS>, bit<L1_MX_BITS>>(register_mx_l1_3) action_mx_l1_3 = {
        void apply(inout bit<L1_MX_BITS> reg_data, out bit<L1_MX_BITS> max_latency) {
            reg_data = max(reg_data, hdr.my_protocol.latency);
            max_latency = reg_data;
        }
    };

    action apply_mx_l1_3() {
        action_mx_l1_3.execute((bit<L1_BUCKET_INDEX_BITS>)(meta.counter_index_l1_3 >> L1_COUNTER_OFFSET_BITS));
    }

    // @stage(3)
    table table_mx_l1_3 {
        actions = {
            apply_mx_l1_3;
        }
        size = 1;
        const default_action = apply_mx_l1_3;
    }
    
    /////////////////////////////////// RECIRCULATE ///////////////////////////////////

    action recirculate_packet() {
        ig_tm_md.ucast_egress_port = RECIRCULATE_OUT_PORT;
    }

    table table_recirculate {
        actions = {
            recirculate_packet;
        }
        size = 1;
        const default_action = recirculate_packet;
    }

    /////////////////////////////////// APPLY ///////////////////////////////////

    apply {
        hdr.egress_data.setValid();

        table_hash_1.apply();
        table_hash_2.apply();
        table_hash_3.apply();

        meta.counter_index_l1_1 = (bit<L1_COUNTER_INDEX_BITS>)meta.hash_1;
        meta.counter_index_l1_2 = (bit<L1_COUNTER_INDEX_BITS>)meta.hash_1;
        meta.counter_index_l1_3 = (bit<L1_COUNTER_INDEX_BITS>)meta.hash_1;
        
        table_dd_seg.apply();
        
        /* -------------------------------- L2 index -------------------------------- */
        meta.hash_1 = meta.hash_1 >> (L1_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS);
        meta.hash_2 = meta.hash_2 >> (L1_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS);
        meta.hash_3 = meta.hash_3 >> (L1_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS);

        hdr.egress_data.counter_index_l2_1 = (bit<L2_COUNTER_INDEX_BITS>)meta.hash_1;
        hdr.egress_data.counter_index_l2_2 = (bit<L2_COUNTER_INDEX_BITS>)meta.hash_2;
        hdr.egress_data.counter_index_l2_3 = (bit<L2_COUNTER_INDEX_BITS>)meta.hash_3;

        hdr.egress_data.counter_index_l2_1 = (hdr.egress_data.counter_index_l2_1 & ~(bit<L2_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L2_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l2_2 = (hdr.egress_data.counter_index_l2_2 & ~(bit<L2_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L2_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l2_3 = (hdr.egress_data.counter_index_l2_3 & ~(bit<L2_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L2_COUNTER_INDEX_BITS>)meta.dd_segment;


        /* -------------------------------- L3 index -------------------------------- */
        meta.hash_1 = meta.hash_1 >> L2_COUNTER_INDEX_BITS;
        meta.hash_2 = meta.hash_2 >> L2_COUNTER_INDEX_BITS;
        meta.hash_3 = meta.hash_3 >> L2_COUNTER_INDEX_BITS;

        hdr.egress_data.counter_index_l3_1 = (bit<L3_COUNTER_INDEX_BITS>)meta.hash_1;
        hdr.egress_data.counter_index_l3_2 = (bit<L3_COUNTER_INDEX_BITS>)meta.hash_2;
        hdr.egress_data.counter_index_l3_3 = (bit<L3_COUNTER_INDEX_BITS>)meta.hash_3;

        hdr.egress_data.counter_index_l3_1 = 
            (hdr.egress_data.counter_index_l3_1 & ~(bit<L3_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
            | (bit<L3_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l3_2 = 
            (hdr.egress_data.counter_index_l3_2 & ~(bit<L3_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
            | (bit<L3_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l3_3 = 
            (hdr.egress_data.counter_index_l3_3 & ~(bit<L3_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
            | (bit<L3_COUNTER_INDEX_BITS>)meta.dd_segment;

        if (ig_intr_md.ingress_port == RECIRCULATE_IN_PORT) { // recirculated
            hdr.egress_data.is_recirculated = true;

            if (hdr.recirculate.insert_level == 0) {
                table_counter_l1_1.apply();
                table_counter_l1_2.apply();
                table_counter_l1_3.apply();

                table_mx_l1_1.apply();
                table_mx_l1_2.apply();
                table_mx_l1_3.apply();
            }
        }
        else {
            hdr.egress_data.is_recirculated = false;

            hdr.recirculate.setValid();
            
            table_lookup_l1_1.apply();
            table_lookup_l1_2.apply();
            table_lookup_l1_3.apply();
            hdr.recirculate.insert_level = 0;

            if (meta.overflowed_l1_1 || meta.overflowed_l1_2 || meta.overflowed_l1_3)
                hdr.egress_data.overflowed_l1 = true;
            
            table_recirculate.apply();
        }
    }
}

control IngressDeparser(packet_out                   pkt,
    /* User */
    inout my_ingress_header_t                        hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    apply {
        pkt.emit(hdr);   
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------------------- EGRESS -----------------------------------------------------------------------------------*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct my_egress_headers_t {
    egress_data_h egress_data;
    recirculate_h recirculate;
    ethernet_h ethernet;
    ipv4_h ipv4;
    udp_h udp;
    my_protocol_h my_protocol;
}

struct my_egress_metadata_t {
    bit<32> hash_1;
    bit<32> hash_2;
    bit<32> hash_3;

    bit<DD_SEGMENT_INDEX_BITS> dd_segment;

    bool overflowed_l2_1;
    bool overflowed_l2_2;
    bool overflowed_l2_3;
        
    bool overflowed_l3_1;
    bool overflowed_l3_2;
    bool overflowed_l3_3;
}

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    state start {
        pkt.extract(eg_intr_md);
        transition meta_init;
    }

    state meta_init {
        meta.hash_1 = 0;
        meta.hash_2 = 0;
        meta.hash_3 = 0;

        meta.dd_segment = 0;

        meta.overflowed_l2_1 = false;
        meta.overflowed_l2_2 = false;
        meta.overflowed_l2_3 = false;

        meta.overflowed_l3_1 = false;
        meta.overflowed_l3_2 = false;
        meta.overflowed_l3_3 = false;
        
        transition parse_egress_data;
    }

    state parse_egress_data {
        pkt.extract(hdr.egress_data);
        transition parse_recirculate;
    }

    state parse_recirculate {
        pkt.extract(hdr.recirculate);
        transition parse_ethernet;
    }
    
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPV4PROTOCOL_UDP: parse_udp;
            default: accept;
        }
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        transition parse_my_protocol;
    }

    state parse_my_protocol {
        pkt.extract(hdr.my_protocol);
        transition accept;
    }
}

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */    
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{    
    /////////////////////////////////// hash ///////////////////////////////////
    
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;

    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32a) hash_1;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32b) hash_2;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM, crc32c) hash_3;

    action apply_hash_1() {
        meta.hash_1 = hash_1.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_1 {
        actions = {
            apply_hash_1;
        }
        size = 1;
        const default_action = apply_hash_1;
    }

    action apply_hash_2() {
        meta.hash_2 = hash_2.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_2 {
        actions = {
            apply_hash_2;
        }
        size = 1;
        const default_action = apply_hash_2;
    }

    action apply_hash_3() {
        meta.hash_3 = hash_3.get(hdr.my_protocol.flow_id);
    }

    // @stage(0)
    table table_hash_3 {
        actions = {
            apply_hash_3;
        }
        size = 1;
        const default_action = apply_hash_3;
    }

    /////////////////////////////////// DDSketch seg ///////////////////////////////////

    action set_seg(bit<DD_SEGMENT_INDEX_BITS> segment) {
        meta.dd_segment = segment;
    }

    // @stage(0)
    table table_dd_seg {
        key = {
            hdr.my_protocol.latency : lpm;
        }
        actions = {
            set_seg;
        }
        size = 8;
    }

    /////////////////////////////////// L2 DD ///////////////////////////////////
    /* -------------------------------- L2 1 -------------------------------- */
    Register<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>>(L2_COUNTERS) register_dd_l2_1;
    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_1) action_dd_l2_1 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l2_1() {
        action_dd_l2_1.execute(hdr.egress_data.counter_index_l2_1);
    }

    // @stage(4)
    table table_dd_l2_1 {
        actions = {
            apply_dd_l2_1;
        }
        size = 1;
        const default_action = apply_dd_l2_1;
    }

    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_1) lookup_dd2_l2_1 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l2_1() {
        meta.overflowed_l2_1 = lookup_dd2_l2_1.execute(hdr.egress_data.counter_index_l2_1);
    }

    // @stage(4)
    table table_lookup_l2_1 {
        actions = {
            apply_lookup_l2_1;
        }
        size = 1;
        const default_action = apply_lookup_l2_1;
    }
    
    /* -------------------------------- L2 2 -------------------------------- */
    Register<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>>(L2_COUNTERS) register_dd_l2_2;
    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_2) action_dd_l2_2 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l2_2() {
        action_dd_l2_2.execute(hdr.egress_data.counter_index_l2_2);
    }

    // @stage(4)
    table table_dd_l2_2 {
        actions = {
            apply_dd_l2_2;
        }
        size = 1;
        const default_action = apply_dd_l2_2;
    }

    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_2) lookup_dd2_l2_2 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l2_2() {
        meta.overflowed_l2_2 = lookup_dd2_l2_2.execute(hdr.egress_data.counter_index_l2_2);
    }

    // @stage(4)
    table table_lookup_l2_2 {
        actions = {
            apply_lookup_l2_2;
        }
        size = 1;
        const default_action = apply_lookup_l2_2;
    }

    /* -------------------------------- L2 3 -------------------------------- */
    Register<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>>(L2_COUNTERS) register_dd_l2_3;
    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_3) action_dd_l2_3 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l2_3() {
        action_dd_l2_3.execute(hdr.egress_data.counter_index_l2_3);
    }

    // @stage(4)
    table table_dd_l2_3 {
        actions = {
            apply_dd_l2_3;
        }
        size = 1;
        const default_action = apply_dd_l2_3;
    }

    RegisterAction<bit<L2_COUNTER_BITS>, bit<L2_COUNTER_INDEX_BITS>, bool>(register_dd_l2_3) lookup_dd2_l2_3 = {
        void apply(inout bit<L2_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l2_3() {
        meta.overflowed_l2_3 = lookup_dd2_l2_3.execute(hdr.egress_data.counter_index_l2_3);
    }

    // @stage(4)
    table table_lookup_l2_3 {
        actions = {
            apply_lookup_l2_3;
        }
        size = 1;
        const default_action = apply_lookup_l2_3;
    }

    /////////////////////////////////// L3 DD ///////////////////////////////////
    /* -------------------------------- L3 1 -------------------------------- */
    Register<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>>(L3_COUNTERS) register_dd_l3_1;
    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_1) action_dd_l3_1 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l3_1() {
        action_dd_l3_1.execute(hdr.egress_data.counter_index_l3_1);
    }

    // @stage(5)
    table table_dd_l3_1 {
        actions = {
            apply_dd_l3_1;
        }
        size = 1;
        const default_action = apply_dd_l3_1;
    }

    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_1) lookup_dd2_l3_1 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l3_1() {
        meta.overflowed_l3_1 = lookup_dd2_l3_1.execute(hdr.egress_data.counter_index_l3_1);
    }

    // @stage(5)
    table table_lookup_l3_1 {
        actions = {
            apply_lookup_l3_1;
        }
        size = 1;
        const default_action = apply_lookup_l3_1;
    }
    
    /* -------------------------------- L3 2 -------------------------------- */
    Register<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>>(L3_COUNTERS) register_dd_l3_2;
    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_2) action_dd_l3_2 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l3_2() {
        action_dd_l3_2.execute(hdr.egress_data.counter_index_l3_2);
    }

    // @stage(5)
    table table_dd_l3_2 {
        actions = {
            apply_dd_l3_2;
        }
        size = 1;
        const default_action = apply_dd_l3_2;
    }

    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_2) lookup_dd2_l3_2 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l3_2() {
        meta.overflowed_l3_2 = lookup_dd2_l3_2.execute(hdr.egress_data.counter_index_l3_2);
    }

    // @stage(5)
    table table_lookup_l3_2 {
        actions = {
            apply_lookup_l3_2;
        }
        size = 1;
        const default_action = apply_lookup_l3_2;
    }

    /* -------------------------------- L3 3 -------------------------------- */
    Register<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>>(L3_COUNTERS) register_dd_l3_3;
    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_3) action_dd_l3_3 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l3_3() {
        action_dd_l3_3.execute(hdr.egress_data.counter_index_l3_3);
    }

    // @stage(5)
    table table_dd_l3_3 {
        actions = {
            apply_dd_l3_3;
        }
        size = 1;
        const default_action = apply_dd_l3_3;
    }

    RegisterAction<bit<L3_COUNTER_BITS>, bit<L3_COUNTER_INDEX_BITS>, bool>(register_dd_l3_3) lookup_dd2_l3_3 = {
        void apply(inout bit<L3_COUNTER_BITS> reg_data, out bool overflowed) {
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_lookup_l3_3() {
        meta.overflowed_l3_3 = lookup_dd2_l3_3.execute(hdr.egress_data.counter_index_l3_3);
    }

    // @stage(5)
    table table_lookup_l3_3 {
        actions = {
            apply_lookup_l3_3;
        }
        size = 1;
        const default_action = apply_lookup_l3_3;
    }

    /////////////////////////////////// L4 DD ///////////////////////////////////
    /* -------------------------------- L4 1 -------------------------------- */
    Register<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>>(L4_COUNTERS) register_dd_l4_1;
    RegisterAction<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>, bool>(register_dd_l4_1) action_dd_l4_1 = {
        void apply(inout bit<L4_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l4_1() {
        action_dd_l4_1.execute(hdr.egress_data.counter_index_l4_1);
    }

    // @stage(6)
    table table_dd_l4_1 {
        actions = {
            apply_dd_l4_1;
        }
        size = 1;
        const default_action = apply_dd_l4_1;
    }

    /* -------------------------------- L4 2 -------------------------------- */
    Register<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>>(L4_COUNTERS) register_dd_l4_2;
    RegisterAction<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>, bool>(register_dd_l4_2) action_dd_l4_2 = {
        void apply(inout bit<L4_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l4_2() {
        action_dd_l4_2.execute(hdr.egress_data.counter_index_l4_2);
    }

    // @stage(6)
    table table_dd_l4_2 {
        actions = {
            apply_dd_l4_2;
        }
        size = 1;
        const default_action = apply_dd_l4_2;
    }

    /* -------------------------------- L4 3 -------------------------------- */
    Register<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>>(L4_COUNTERS) register_dd_l4_3;
    RegisterAction<bit<L4_COUNTER_BITS>, bit<L4_COUNTER_INDEX_BITS>, bool>(register_dd_l4_3) action_dd_l4_3 = {
        void apply(inout bit<L4_COUNTER_BITS> reg_data, out bool overflowed) {
            reg_data = reg_data |+| 1;
            overflowed = reg_data + 1 == 0;
        }
    };

    action apply_dd_l4_3() {
        action_dd_l4_3.execute(hdr.egress_data.counter_index_l4_3);
    }

    // @stage(6)
    table table_dd_l4_3 {
        actions = {
            apply_dd_l4_3;
        }
        size = 1;
        const default_action = apply_dd_l4_3;
    }

    apply { 
        
        table_hash_1.apply();
        table_hash_2.apply();
        table_hash_3.apply();

        table_dd_seg.apply();

        /* -------------------------------- L4 index -------------------------------- */
        meta.hash_1 = meta.hash_1 >> 
            L1_COUNTER_INDEX_BITS + L2_COUNTER_INDEX_BITS + L3_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS;
        meta.hash_2 = meta.hash_2 >> 
            L1_COUNTER_INDEX_BITS + L2_COUNTER_INDEX_BITS + L3_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS;
        meta.hash_3 = meta.hash_3 >> 
            L1_COUNTER_INDEX_BITS + L2_COUNTER_INDEX_BITS + L3_COUNTER_INDEX_BITS - DD_SEGMENT_INDEX_BITS;

        hdr.egress_data.counter_index_l4_1 = (bit<L4_COUNTER_INDEX_BITS>)meta.hash_1;
        hdr.egress_data.counter_index_l4_2 = (bit<L4_COUNTER_INDEX_BITS>)meta.hash_2;
        hdr.egress_data.counter_index_l4_3 = (bit<L4_COUNTER_INDEX_BITS>)meta.hash_3;

        hdr.egress_data.counter_index_l4_1 = (hdr.egress_data.counter_index_l4_1 & ~(bit<L4_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L4_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l4_2 = (hdr.egress_data.counter_index_l4_2 & ~(bit<L4_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L4_COUNTER_INDEX_BITS>)meta.dd_segment;
        hdr.egress_data.counter_index_l4_3 = (hdr.egress_data.counter_index_l4_3 & ~(bit<L4_COUNTER_INDEX_BITS>)(DD_SEGMENTS - 1)) 
                                  | (bit<L4_COUNTER_INDEX_BITS>)meta.dd_segment;

        if (hdr.egress_data.is_recirculated) { // recirculated
            if (hdr.recirculate.insert_level == 1) {
                table_dd_l2_1.apply();
                table_dd_l2_2.apply();
                table_dd_l2_3.apply();
            }
            if (hdr.recirculate.insert_level == 2) {
                table_dd_l3_1.apply();
                table_dd_l3_2.apply();
                table_dd_l3_3.apply();
            }
            if (hdr.recirculate.insert_level == 3) {
                table_dd_l4_1.apply();
                table_dd_l4_2.apply();
                table_dd_l4_3.apply();
            }
            hdr.recirculate.setInvalid();
        }
        else {
            if (hdr.egress_data.overflowed_l1) {
                table_lookup_l2_1.apply();
                table_lookup_l2_2.apply();
                table_lookup_l2_3.apply();

                hdr.recirculate.insert_level = 1;

                if (meta.overflowed_l2_1 || meta.overflowed_l2_2 || meta.overflowed_l2_3) {
                    table_lookup_l3_1.apply();
                    table_lookup_l3_2.apply();
                    table_lookup_l3_3.apply();

                    hdr.recirculate.insert_level = 2;

                    if (meta.overflowed_l3_1 || meta.overflowed_l3_2 || meta.overflowed_l3_3)
                        hdr.recirculate.insert_level = 3;
                }
            }
        }
        hdr.egress_data.setInvalid();
    }
}

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}

Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;

#endif