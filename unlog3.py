import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import json
import os
from datetime import datetime
import io

def load_private_key(private_key_path):
    with open(private_key_path, 'rb') as key_file:
        private_key = serialization.load_pem_private_key(
            key_file.read(),
            password=None,
            backend=default_backend()
        )
    return private_key

def decrypt_log_entry(encrypted_log, private_key):
    decrypted_log = private_key.decrypt(
        bytes.fromhex(encrypted_log),
        padding.OAEP(
            mgf=padding.MGF1(algorithm=hashes.SHA256()),
            algorithm=hashes.SHA256(),
            label=None
        )
    )
    return decrypted_log.decode('utf-8')

def read_and_decrypt_logs(encrypted_file_path, private_key_path):
    private_key = load_private_key(private_key_path)
    with open(encrypted_file_path, 'rb') as encrypted_file:
        encrypted_logs = json.loads(encrypted_file.read().decode('utf-8'))

    decrypted_logs = []
    for encrypted_log in encrypted_logs:
        decrypted_log = decrypt_log_entry(encrypted_log, private_key)
        decrypted_logs.append(json.loads(decrypted_log))

    return decrypted_logs

def decrypt_image(encrypted_image_path, password):
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
    with open(encrypted_image_path, 'rb') as f:
        data = f.read()

    
    salt = data[:16]
    iv = data[16:32]
    encrypted_image = data[32:]

    
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend()
    )
    key = kdf.derive(password.encode())

    
    cipher = Cipher(algorithms.AES(key), modes.CFB(iv), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted_image = decryptor.update(encrypted_image) + decryptor.finalize()

    return decrypted_image

def display_logs(decrypted_logs):
    root = tk.Tk()
    root.title("Decrypted Logs")

    
    canvas = tk.Canvas(root)
    scrollbar = ttk.Scrollbar(root, orient="vertical", command=canvas.yview)
    scrollable_frame = ttk.Frame(canvas)

    scrollable_frame.bind(
        "<Configure>",
        lambda e: canvas.configure(
            scrollregion=canvas.bbox("all")
        )
    )

    canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
    canvas.configure(yscrollcommand=scrollbar.set)

    for log in decrypted_logs:
        frame = ttk.Frame(scrollable_frame)
        frame.pack(padx=10, pady=10, fill='x')

        
        timestamp_label = ttk.Label(frame, text=f"Timestamp: {log['timestamp']}", font=('Helvetica', 12, 'bold'))
        timestamp_label.pack(anchor='w')

        message_label = ttk.Label(frame, text=f"Message: {log['message']}", font=('Helvetica', 12, 'bold'))
        message_label.pack(anchor='w')

        
        screenshot_data = decrypt_image(log['screenshot'], 'USTH')  
        screenshot_image = Image.open(io.BytesIO(screenshot_data))
        screenshot_image = screenshot_image.resize((screenshot_image.width // 2, screenshot_image.height // 2))  
        screenshot_photo = ImageTk.PhotoImage(screenshot_image)

        screenshot_label = ttk.Label(frame, image=screenshot_photo)
        screenshot_label.image = screenshot_photo  
        screenshot_label.pack()

        
        separator = ttk.Separator(scrollable_frame, orient='horizontal')
        separator.pack(fill='x', padx=10, pady=5)

    canvas.pack(side="left", fill="both", expand=True)
    scrollbar.pack(side="right", fill="y")

    root.mainloop()


private_key_path = 'private_key.pem'
encrypted_file_path = 'encrypted_logs.bin'

decrypted_logs = read_and_decrypt_logs(encrypted_file_path, private_key_path)
display_logs(decrypted_logs)
