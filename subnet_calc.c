#include <stdio.h>
#include <string.h>
#include <math.h>

/* split up any ip and calculate the netmask, first,last addresses for all or any nth network in a super block*/
/* two special cases 0.0.0.0/0 and any.any.any.any/32 */

// pack this
struct Networks {
    char * network_address;
    char * gateway_address;
    char * first_address;
    char * last_address;
    char * broadcast_address;
};

// pack this
struct Subnet {
    unsigned int subnet_octet;                 // bit boundary base2 subnet octet
    int number_of_class_c_networks;            // total number of class C address in super block
    unsigned int octet_boundary[8];            // collection of bit boundaries that are indexed into
    unsigned int incrementor[8];
    signed int network_bits;                   // network bits of a subnet
    unsigned int host_bits;                    // host bits of a subnet
    float usable_hosts;                        // useable hosts per subnet
    float number_of_networks;                  // total number of available networks for the super block
    char netmask[20];                          // complete netmask for the ip
};

// pack this
struct Ip {
    unsigned int octet_1;                      // the first octet of an ip address
    unsigned int octet_2;                      // the second octet of an ip address
    unsigned int octet_3;                      // the third octet of an ip address
    unsigned int octet_4;                      // the fourth octet of an ip address
    unsigned int mask;                         // the cidr notation for an ip address
    struct Subnet subnet;                      // the network meta data for an ip address
    struct Networks network;                   // all networks available to a super block
};

struct Ip split_ip_by_octet(char ip_passed_as_string[]);
void initialise_ip_metadata(struct Ip *);
void determine_subnet_mask(struct Ip *);
void generate_network_list(struct Ip *);

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "pass ip to program n.n.n.n/n\n");
        return -1;
    }

    struct Ip ip = split_ip_by_octet(argv[1]);

    struct Ip * ipref = &ip;
    initialise_ip_metadata(ipref);
    generate_network_list(ipref);

    printf("net mask is %s\n", ip.subnet.netmask);
    printf("total number of /%d networks: %d\n", ip.mask, (int) ip.subnet.number_of_networks);
    printf("useable ip address per network: %d\n", (int) ip.subnet.usable_hosts);

    // if > /24 show fraction of available class C address
    if (ip.subnet.host_bits < 8) {
        printf("total number of class C networks: %.2f\n", (float) (1.0 / (float) (ip.subnet.network_bits+1)));
    } else  {
        printf("total number of class C networks: %d\n", ip.subnet.number_of_class_c_networks);
    }

    return 0;
}

struct Ip split_ip_by_octet(char ip_string[]) {

    struct Ip ip;
    char * value_before_delimeter = strtok(ip_string, "."); // N. 

    if (value_before_delimeter != NULL) {
        sscanf(value_before_delimeter, "%d", &ip.octet_1);
        value_before_delimeter = strtok(NULL, ".");        // n.N.

        sscanf(value_before_delimeter, "%d", &ip.octet_2);
        value_before_delimeter = strtok(NULL, ".");        // n.n.N.n/n

        sscanf(value_before_delimeter, "%d", &ip.octet_3);
        value_before_delimeter = strtok(NULL, "/");        // n.n.n.N/n

        sscanf(value_before_delimeter, "%d", &ip.octet_4);
        value_before_delimeter = strtok(NULL, "");         // n.n.n.n/N

        sscanf(value_before_delimeter, "%d", &ip.mask);
    }
    return ip; 
}

void initialise_ip_metadata(struct Ip *ip) {

    int bit_boundary_initialiser[] = {128,192,224,240,248,252,254,255};
    memcpy(ip->subnet.octet_boundary, bit_boundary_initialiser, sizeof bit_boundary_initialiser);
    int increment_by_initialiser[] = {128,64,32,16,8,4,2,1};
    memcpy(ip->subnet.incrementor, increment_by_initialiser, sizeof increment_by_initialiser);

    ip->subnet.host_bits = (32 - ip->mask);
    ip->subnet.network_bits = (ip->mask % 8);

    // -1 because octet_boundary is zero based and /17 will have 1 network_bits 
    ip->subnet.subnet_octet = ip->subnet.octet_boundary[ip->subnet.network_bits-1];
    ip->subnet.usable_hosts = pow(2.0, (float) ip->subnet.host_bits)-2;
    ip->subnet.number_of_networks = pow(2.0, (float) ip->subnet.network_bits);
    
    // set defaults, zeroth network counts as a network so +1
    if (ip->subnet.network_bits == 0) {
        ip->subnet.subnet_octet = 255;
    }

    determine_subnet_mask(ip);

}

void determine_subnet_mask(struct Ip * ip) {

    if (ip->mask > 0 && ip->mask <= 16) {
        // handle /16 natural mask defaults
        if (ip->mask == 16) {
            ip->subnet.usable_hosts = pow(2.0, (float) 16)-2;
        }
        ip->subnet.number_of_class_c_networks = (ip->subnet.usable_hosts / 256) + 1;
        sprintf(ip->subnet.netmask, "255.%d.0.0", ip->subnet.subnet_octet);
        return;
    }
    if (ip->mask > 16 && ip->mask <= 24) {
        // handle /24 natural mask defaults
        if (ip->mask == 24) {
            ip->subnet.usable_hosts = pow(2.0, (float) 8)-2;
        }
        ip->subnet.number_of_class_c_networks = (ip->subnet.usable_hosts / 256) + 1;
        sprintf(ip->subnet.netmask, "255.255.%d.0", ip->subnet.subnet_octet);
        return;
    }
    if (ip->mask > 24 && ip->mask <= 32) {
        sprintf(ip->subnet.netmask, "255.255.255.%d", ip->subnet.subnet_octet);
        return;
    }
}

void generate_network_list(struct Ip * ip) {
    // print all networks if last octet is .0
    for (int network=0; network < ip->subnet.number_of_networks; network++) {
        if (ip->octet_4 == 0) {
            int increment_by = ip->subnet.incrementor[ip->subnet.network_bits-1] * network;
            int bcast = increment_by + ip->subnet.incrementor[ip->subnet.network_bits-1] - 1;
            printf("net %d.%d.%d.%d gateway %d.%d.%d.%d first %d.%d.%d.%d last %d.%d.%d.%d bcast %d.%d.%d.%d\n", 
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by,
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by+1,
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by+2,
                ip->octet_1, ip->octet_2, ip->octet_3, bcast-1,
                ip->octet_1, ip->octet_2, ip->octet_3, bcast);
        } else {
            // print range networks of networks i.e. if further subnetting an already subnetted range
            int increment_by = ip->subnet.incrementor[ip->subnet.network_bits-1] * network;
            int bcast = increment_by + ip->subnet.incrementor[ip->subnet.network_bits-1] - 1;

            /* 
                ./subnet  192.168.1.0/26
                net 192.168.1.0 
                net 192.168.1.64
                net 192.168.1.128
                net 192.168.1.192

                LIMITATION
                ./subnet  192.168.1.64/28
                from .0-255.0 there are 16 networks in a /28
                here we're asking for networks 192.168.1.64/26 -> /28
                there's 4 in that block however the struct doesn't that
                if you only need a slice of that may network=0 above could be more dynamic?
                below i've hardcoded 4 for testing since the struct thinks there's 16

                FIX TODO
                create a start point based on the last octet 
                build out network struct with every block
                if start point 0 read all the memory in a struct
                if start != 0 then find which network the address belongs to 
                use that network as start point and read until new end point for that block


            */
            //int end_range = (ip->octet_4 + (ip->subnet.incrementor[ip->subnet.network_bits-1] * (int) (ip->subnet.number_of_networks)) -1);
            int end_range = (ip->octet_4 + (ip->subnet.incrementor[ip->subnet.network_bits-1] * 4) -1);

            if (increment_by >= ip->octet_4 && increment_by < end_range) {
            printf("net %d.%d.%d.%d gateway %d.%d.%d.%d first %d.%d.%d.%d last %d.%d.%d.%d bcast %d.%d.%d.%d\n", 
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by,
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by+1,
                ip->octet_1, ip->octet_2, ip->octet_3, increment_by+2,
                ip->octet_1, ip->octet_2, ip->octet_3, bcast-1,
                ip->octet_1, ip->octet_2, ip->octet_3, bcast);
            }
        }
    }

}