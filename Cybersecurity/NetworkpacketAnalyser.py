from scapy.layers.inet import IP
from scapy.layers.l2 import Ether
from scapy.utils import RawPcapReader

import ipaddress


def read_file() -> RawPcapReader:
    # OBS! Kom ihåg att filen heter 4SICS-GeekLounge-151021-minimized.pcap
    return RawPcapReader("4SICS-GeekLounge-151021-minimized.pcap")

def search_for_ips(pcap_reader: RawPcapReader) -> list:
    suspektaNet = ipaddress.ip_network("192.168.2.0/24")
    suspektaIP = []
    
    for paketData, paketMetaData in pcap_reader:
        try:
            etherPaket = Ether(paketData)
        except Exception:
            continue
        
        if IP in etherPaket:
            ipLager = etherPaket[IP]
            source = ipLager.src
            destination = ipLager.dst
            
            try:
                if ipaddress.ip_address(source) in suspektaNet:
                    if source not in suspektaIP:
                        suspektaIP.append(source)
            except Exception:
                pass
            
            try:
                if ipaddress.ip_address(destination) in suspektaNet:
                    if destination not in suspektaIP:
                        suspektaIP.append(destination)
            except Exception:
                pass
    return suspektaIP
