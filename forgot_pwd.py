from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives import hashes

def load_private_key(private_key_path):
    with open(private_key_path, 'rb') as key_file:
        private_key = serialization.load_pem_private_key(
            key_file.read(),
            password=None,
            backend=default_backend()
        )
    return private_key

def decrypt_new_code(private_key_path, encrypted_code_path):
    private_key = load_private_key(private_key_path)
    
    with open(encrypted_code_path, 'rb') as f:
        encrypted_code = f.read()
    
    decrypted_code = private_key.decrypt(
        encrypted_code,
        padding.OAEP(
            mgf=padding.MGF1(algorithm=hashes.SHA256()),
            algorithm=hashes.SHA256(),
            label=None
        )
    )
    
    return decrypted_code.decode('utf-8')

# Usage example
private_key_path = 'private_key.pem'
encrypted_code_path = 'currentcode.bin'

new_code = decrypt_new_code(private_key_path, encrypted_code_path)
print(f"The decrypted new code is: {new_code}")
