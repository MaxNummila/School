from argon2 import PasswordHasher

"""
Skapar PasswordHasher objektet. 
Krav: Argon2id, det minimala minnesstorlek (eng. minimal memory size) = 46 MiB,
det minimala antalet iterationer (eng. minimal number of iterations) = 1,
parallelitetsgraden (eng. degree of parallelism) = 1,
längden på ett slumpmässigt salt (length of random salt) = 16,
längden på hashen = 16
"""
def define_pwd_hasher() -> PasswordHasher:
    minnesstorlek = 46 * 1024
    return PasswordHasher(
        time_cost=1, memory_cost=minnesstorlek, parallelism=1, salt_len=16, hash_len=16
    )
"""
Denna funktion skapar hashen. OBS! Funktionen ska INTE anropa define_pwd_hasher() då det görs i testet
som utvärderar koden.
"""
def hash_pwd(ph: PasswordHasher, pwd: str) -> str:
    return ph.hash(pwd)
