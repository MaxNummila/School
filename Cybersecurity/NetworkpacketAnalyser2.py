from scapy.layers.inet import TCP, IP, UDP
from scapy.layers.l2 import Ether
from scapy.utils import RawPcapReader


def read_file() -> RawPcapReader:
    # OBS! Kom ihåg att filen heter 4SICS-GeekLounge-151021-minimized.pcap
    return RawPcapReader("4SICS-GeekLounge-151021-minimized.pcap")


def search_for_ports(pcap_reader: RawPcapReader) -> list:
    suspektaPortar = {20, 21, 22, 23, 25, 53, 80, 137, 139, 443, 445, 1433, 1434, 3306, 3389, 8080, 8443}
    hittadePortar = []
    
    for paketData, _ in pcap_reader:
        try:
            etherPaket = Ether(paketData)
        except Exception:
            continue
        if IP in etherPaket:
            if TCP in etherPaket:
                dport = etherPaket[TCP].dport
                if dport in suspektaPortar and dport not in hittadePortar:
                    hittadePortar.append(dport)
            elif UDP in etherPaket:
                dport = etherPaket[UDP].dport
                if dport in suspektaPortar and dport not in hittadePortar:
                    hittadePortar.append(dport)
        
    return hittadePortar
