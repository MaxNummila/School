import re

def email_is_accepted(email: str) -> bool:
    if email.count("@") != 1:
        return False
    
    local, domain = email.split("@", 1)
    
    if not (1 <= len(local) <= 63):
        return False
        
    if len(domain) < 1:
        return False
    
    olagligaTecken = ["`", "'", '"']
    for char in olagligaTecken:
        if char in email:
            return False
    
    if "\x00" in email or "%00" in email:
        return False
    
    if not re.fullmatch(r"[A-Za-z0-9\.-]+", domain):
        return False
    
    if len(email) > 254:
        return False
        
    return True
