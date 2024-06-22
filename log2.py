import re
import json
import cv2
import serial
import time
import os
from datetime import datetime
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding as sym_padding
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import firebase_admin
from firebase_admin import credentials, db, storage
# Initialize the webcam


cred = credentials.Certificate('esp32-tesat-firebase-adminsdk-6e73u-6c8210c35a.json')
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://esp32-tesat-default-rtdb.asia-southeast1.firebasedatabase.app/',
    'storageBucket': 'esp32-tesat.appspot.com'
})


cap = cv2.VideoCapture(0)
encrypted_logs = []

# Load the public key
def load_public_key(file_path):
    with open(file_path, "rb") as key_file:
        public_key = serialization.load_pem_public_key(
            key_file.read(),
            backend=default_backend()
        )
    return public_key

# Encrypt the log entry using the public key
def encrypt_log_entry(log_entry, public_key):
    encrypted_log = public_key.encrypt(
        log_entry.encode('utf-8'),
        padding.OAEP(
            mgf=padding.MGF1(algorithm=hashes.SHA256()),
            algorithm=hashes.SHA256(),
            label=None
        )
    )
    return encrypted_log

# Save the encrypted logs to a file
def save_encrypted_logs():
    filename = "encrypted_logs.bin"
    with open(filename, "wb") as log_file:
        log_file.write(json.dumps(encrypted_logs).encode('utf-8'))
    print(f"Encrypted logs saved: {filename}")

# Encrypt the image using AES
def encrypt_image(image_path, password):
    # Generate a random salt
    salt = os.urandom(16)
    # Derive the key from the password
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend()
    )
    key = kdf.derive(password.encode())
    
    # Read the image data
    with open(image_path, 'rb') as f:
        image_data = f.read()
    
    # Generate a random IV
    iv = os.urandom(16)
    # Create the cipher object and encrypt the data
    cipher = Cipher(algorithms.AES(key), modes.CFB(iv), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted_image = encryptor.update(image_data) + encryptor.finalize()
    
    # Save the encrypted image
    encrypted_image_path = "save_image/"+ image_path + ".enc"
    with open(encrypted_image_path, 'wb') as f:
        f.write(salt + iv + encrypted_image)
    
    print(f"Encrypted image saved: {encrypted_image_path}")
    return encrypted_image_path

def log_message(message, screenshot_filename):
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    log_entry = {
        "timestamp": timestamp,
        "message": message,
        "screenshot": screenshot_filename
    }
    log_entry_json = json.dumps(log_entry)
    print(log_entry_json)  # Print to console

    public_key = load_public_key("public_key.pem")
    encrypted_log = encrypt_log_entry(log_entry_json, public_key)
    encrypted_logs.append(encrypted_log.hex())
    
    screenshot_url = upload_screenshot_to_storage(screenshot_filename)
    push_to_firebase(encrypted_log.hex(), screenshot_url)

def upload_screenshot_to_storage(screenshot_filename):
    bucket = storage.bucket()
    blob = bucket.blob(f"screenshots/{screenshot_filename}")
    blob.upload_from_filename(screenshot_filename)
    blob.make_public()
    print(f"Screenshot URL: {blob.public_url}")
    return blob.public_url

def push_to_firebase(encrypted_log_hex, screenshot_url):
    ref = db.reference('logs')  # Reference to the logs node
    new_log_ref = ref.push()  # Create a new child node
    new_log_ref.set({
        'encrypted_log': encrypted_log_hex,
        'screenshot_url': screenshot_url
    })
    print("Encrypted log and screenshot URL pushed to Firebase")


# Capture the screenshot and encrypt it
def capture_screenshot(password):
    ret, frame = cap.read()
    if ret:
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        screenshot_filename = f"screenshot_{timestamp}.png"
        cv2.imwrite(screenshot_filename, frame)
        print(f"Screenshot saved: {screenshot_filename}")
        encrypted_screenshot_filename = encrypt_image(screenshot_filename, password)
        os.remove(screenshot_filename)  # Remove the original unencrypted file
        return encrypted_screenshot_filename
    return None

# Parse the message and handle it
def parse_message(message, password):
    match = re.match(r"(\d) : (.+)", message)
    if match:
        number = int(match.group(1))
        content = match.group(2)
        if number == 0:
            content = handle_create_new_code(content)
        elif number == 1:
            handle_unlock_successfully(content)
        elif number == 2:
            handle_unlock_failed(content)
        elif number == 3:
            handle_door_locked(content)
        elif number == 4:
            handle_door_unlocked(content)
        elif number == 5:
            handle_door_rang(content)
        else:
            print("Unknown message type.")
        
        screenshot_filename = capture_screenshot(password)
        if screenshot_filename:
            log_message(content, screenshot_filename)
        else:
            log_message(content, "save_image/no_img.jpg")

# Placeholder functions for handling specific message types
def handle_create_new_code(content):
    pattern = r"Create new Code (\d+)"
    match = re.match(pattern, content)
    
    if match:
        new_code = match.group(1)
    else:
        return content 
    public_key = load_public_key("public_key.pem")
    encrypted_code = encrypt_log_entry(new_code, public_key)
    with open("currentcode.bin",'wb') as f:
        f.write(encrypted_code)
    censored_message = content.replace(new_code, '****')
    return censored_message
    print(f"Handling create new code: {content}")

def handle_unlock_successfully(content):
    print(f"Handling unlock successfully: {content}")

def handle_unlock_failed(content):
    print(f"Handling unlock failed: {content}")

def handle_door_locked(content):
    print(f"Handling door locked: {content}")

def handle_door_unlocked(content):
    print(f"Handling door unlocked: {content}")

def handle_door_rang(content):
    print(f"Handling door rang: {content}")    

# Set up serial connection
ser = serial.Serial('COM6', 9600, timeout=1)  # Adjust 'COM6' to your serial port

try:
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            parse_message(line, "USTH")  # Replace with your password
        time.sleep(1)
except KeyboardInterrupt:
    print("Exiting program")
finally:
    ser.close()
    cap.release()
    cv2.destroyAllWindows()
    save_encrypted_logs()
